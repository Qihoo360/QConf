#include "slash_string.h"

#include "monitor_options.h"
#include "monitor_service_item.h"
#include "monitor_listener.h"
#include "monitor_log.h"
#include "monitor_const.h"

ServiceListener::ServiceListener(MonitorOptions *options)
      : options_(options) {
  monitor_zk_ = new MonitorZk(options_, &cb_handle);
}

ServiceListener::~ServiceListener() {}

int ServiceListener::Init() {
  cb_handle.options_ = options_;
  cb_handle.monitor_zk_ = monitor_zk_;
  return monitor_zk_->InitEnv();
}

void ServiceListener::LoadAllService() {
  options_->service_map.clear();
  GetAllIp();

  for (auto &item : options_->service_father_to_ip) {
    std::string service_father = item.first; // /pathto/service
    std::set<std::string> ip_ports = item.second; // 127.0.0.1:1000 ...
    for (auto &ip_port : ip_ports) {
      std::string ip_port_path = service_father + "/" + ip_port;
      LoadService(ip_port_path, service_father, ip_port);
    }
  }
}

// Get all ip belong to my service father
void ServiceListener::GetAllIp() {
  for (auto item : options_->service_father_to_ip) {
    std::string service_father = item.first;
    struct String_vector children = {0};
    // Get all ip_port belong to this service_father
    if (monitor_zk_->zk_get_chdnodes(service_father, children) != kSuccess) {
      LOG(LOG_ERROR, "get IP:Port failed. service_father:%s", service_father.c_str());
      options_->service_father_to_ip[service_father].insert("");
    } else {
      // Add the service_father and ip_port to the map service_fatherToIp
      AddChildren(service_father, children);
    }
    deallocate_String_vector(&children);
  }
}

int ServiceListener::AddChildren(const std::string &service_father,
                                 struct String_vector &children) {
  if (children.count == 0)
    options_->service_father_to_ip[service_father].insert("");
  for (int i = 0; i < children.count; ++i)
    options_->service_father_to_ip[service_father].insert(std::string(children.data[i]));
  return 0;
}

int ServiceListener::LoadService(std::string &ip_port_path,
                                 std::string service_father,
                                 std::string ip_port) {
  char status = kStatusUnknow;
  int ret = kSuccess;
  if ((ret = monitor_zk_->zk_get_service_status(ip_port_path, status))  != kSuccess) {
    LOG(LOG_ERROR, "get service status failed. service:%s", ip_port_path.c_str());
    return ret;
  }

  // Handle illegal status
  if (status < -1 || status > 2) {
    status = -1;
  }

  std::string ip;
  int port;
  slash::ParseIpPortString(ip_port, ip, port);
  ServiceItem item(ip, port, service_father, status);

  options_->service_map[ip_port_path] = item;
  LOG(LOG_INFO, "load service succeed, service:%s, status:%d", ip_port_path.c_str(), status);
  return kSuccess;
}

void ServiceListener::BalanceZkHandle::ModifyServiceFatherToIp(const int &op,
                                                               const std::string& ip_path) {
  size_t pos = ip_path.rfind('/');
  std::string service_father = ip_path.substr(0, pos);
  std::string ip_port = ip_path.substr(pos + 1);
  std::string ip;
  int port;
  slash::ParseIpPortString(ip_port, ip, port);

  auto &ipset = options_->service_father_to_ip[service_father];
  if (op == kAdd) {
    //If this ip_port has exist, no need to do anything
    if (ipset.find(ip_port) != ipset.end())
      return;

    char status = kStatusUnknow;
    if (monitor_zk_->zk_get_service_status(ip_path, status) != kSuccess) return;

    ServiceItem item(ip, port, service_father, status);

    options_->service_map[ip_path] = item;
    ipset.insert(ip_port);
  } else if (op == kDelete) {
    options_->service_map.erase(ip_path);
    ipset.erase(ip_port);
  }
}

void ServiceListener::BalanceZkHandle::ProcessDeleteEvent(const std::string& path) {
  // It must be a service node. Because I do zoo_get only in service node
  // update service_father_to_ip
  ModifyServiceFatherToIp(kDelete, path);
}

void ServiceListener::BalanceZkHandle::ProcessChildEvent(const std::string& path) {
  // It must be a service father node. Because I do zoo_get_children only in service father node
  struct String_vector children = {0};
  if (monitor_zk_->zk_get_chdnodes(path, children) == kSuccess) {
    if ((options_->service_father_to_ip.find(path) ==
         options_->service_father_to_ip.end()) ||
        children.count <= (int)options_->service_father_to_ip[path].size()) {
      LOG(LOG_INFO, "actually It's a delete event");
    } else {
      LOG(LOG_INFO, "add new service");
      for (int i = 0; i < children.count; ++i) {
        std::string ip_port = std::string(children.data[i]);
        ModifyServiceFatherToIp(kAdd, path + "/" + ip_port);
      }
    }
  }
  deallocate_String_vector(&children);
}

void ServiceListener::BalanceZkHandle::ProcessChangedEvent(const std::string& path) {
  int new_status = kStatusUnknow;
  std::string data;
  if (monitor_zk_->zk_get_node(path, data, 1) == kSuccess) {
    new_status = atoi(data.c_str());
    options_->service_map[path].status = new_status;
  }
}
