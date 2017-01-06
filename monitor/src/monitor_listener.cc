#include "slash_string.h"

#include <string>
#include <vector>
#include <iostream>

#include <signal.h>
#include <sys/types.h>
#include <netdb.h>

#include <zookeeper.h>
#include <zk_adaptor.h>

#include "monitor_options.h"
#include "monitor_service_item.h"
#include "monitor_listener.h"
#include "monitor_load_balance.h"
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

  for (auto it1 = options_->service_father_to_ip.begin();
       it1 != options_->service_father_to_ip.end();
       ++it1) {
    std::string service_father = it1->first; // /pathto/service
    std::set<std::string> ip_ports = it1->second; // 127.0.0.1:1000
    std::vector<int> status(4, 0);
    for (auto it2 = ip_ports.begin(); it2 != ip_ports.end(); ++it2) {
      std::string path = service_father + "/" + (*it2);
      LoadService(path, service_father, *it2, status);
    }
  }
}

// Get all ip belong to my service father
void ServiceListener::GetAllIp() {
  for (auto it = options_->service_father_to_ip.begin();
       it != options_->service_father_to_ip.end();
       ++it) {
    std::string service_father = it->first;
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

int ServiceListener::LoadService(std::string path, std::string service_father, std::string ip_port, std::vector<int>& st) {
  char status = kStatusUnknow;
  int ret = kSuccess;
  if ((ret = monitor_zk_->zk_get_service_status(path, status))  != kSuccess) {
    LOG(LOG_ERROR, "get service status failed. service:%s", path.c_str());
    return ret;
  }
  if (status < -1 || status > 2) {
    status = -1;
  }
  ++(st[status + 1]);
  std::string ip;
  int port;
  slash::ParseIpPortString(ip_port, ip, port);

  ServiceItem item(ip, port, service_father, status);
  options_->service_map[path] = item;
  LOG(LOG_INFO, "load service succeed, service:%s, status:%d", path.c_str(), status);
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
  if (op == kAdd) {
    //If this ip_port has exist, no need to do anything
    if (options_->service_father_to_ip[service_father].find(ip_port) !=
        options_->service_father_to_ip[service_father].end())
      return;

    char status = kStatusUnknow;
    if (monitor_zk_->zk_get_service_status(ip_path, status) != kSuccess) return;

    ServiceItem item(ip, port, service_father, status);

    options_->service_map[ip_path] = item;
    options_->service_father_to_ip[service_father].insert(ip_port);
  } else if (op == kDelete) {
    options_->service_map.erase(ip_path);
    options_->service_father_to_ip[service_father].erase(ip_port);
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
