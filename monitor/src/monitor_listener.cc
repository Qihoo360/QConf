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

using namespace std;

ServiceListener::ServiceListener(LoadBalance *load_balance, MonitorOptions *options) :
  MonitorZk(options),
  load_balance_(load_balance) {
    // It makes sense. Make all locks occur in one function
    ModifyServiceFatherToIp(CLEAR, "");
  }

int ServiceListener::InitListener() {
  return InitEnv();
}

// Get all ip belong to my service father
void ServiceListener::GetAllIp() {
  auto service_father = load_balance_->GetMyServiceFather();
  for (auto it = service_father.begin(); it != service_father.end(); ++it) {
    struct String_vector children = {0};
    // Get all ip_port belong to this service_father
    if (zk_get_chdnodes(*it, children) != MONITOR_OK) {
      LOG(LOG_ERROR, "get IP:Port failed. service_father:%s", (*it).c_str());
      AddIpPort(*it, "");
    } else {
      // Add the service_father and ip_port to the map service_fatherToIp
      AddChildren(*it, children);
    }
    deallocate_String_vector(&children);
  }
}

void ServiceListener::LoadAllService() {
  // Here we need locks. Maybe we can remove it
  slash::MutexLock l(&service_father_to_ip_lock_);
  for (auto it1 = service_father_to_ip_.begin(); it1 != service_father_to_ip_.end(); ++it1) {
    string service_father = it1->first;
    unordered_set<string> ip_ports = it1->second;
    service_father_to_ip_lock_.Unlock();
    vector<int> status(4, 0);
    for (auto it2 = ip_ports.begin(); it2 != ip_ports.end(); ++it2) {
      string path = service_father + "/" + (*it2);
      LoadService(path, service_father, *it2, status);
    }
    service_father_to_ip_lock_.Lock();
  }
}

void ServiceListener::ProcessDeleteEvent(const string& path) {
  // It must be a service node. Because I do zoo_get only in service node
  // update service_fatherToIp
  ModifyServiceFatherToIp(DELETE, path);
}

void ServiceListener::ProcessChildEvent(const string& path) {
  // It must be a service father node. Because I do zoo_get_children only in service father node
  struct String_vector children = {0};
  if (zk_get_chdnodes(path, children) == MONITOR_OK) {
    LOG(LOG_INFO, "get children success");
    if (children.count <= GetIpNum(path)) {
      LOG(LOG_INFO, "actually It's a delete event");
    } else {
      LOG(LOG_INFO, "add new service");
      for (int i = 0; i < children.count; ++i) {
        string ip_port = string(children.data[i]);
        ModifyServiceFatherToIp(ADD, path + "/" + ip_port);
      }
    }
  }
  deallocate_String_vector(&children);
}

void ServiceListener::ProcessChangedEvent(const string& path) {
  int new_status = STATUS_UNKNOWN;
  string data;
  if (zk_get_node(path, data, 1) == MONITOR_OK) {
    new_status = atoi(data.c_str());
    options_->SetServiceMap(path, new_status);
  }
}

int ServiceListener::AddChildren(const string &service_father, struct String_vector &children) {
  if (children.count == 0) AddIpPort(service_father, "");
  for (int i = 0; i < children.count; ++i)
    AddIpPort(service_father, string(children.data[i]));
  return 0;
}

int ServiceListener::GetAddrByHost(const string &host, struct in_addr* addr) {
  int ret = MONITOR_ERR_OTHER;
  struct hostent *ht;
  if ((ht = gethostbyname(host.c_str())) !=  NULL) {
    *addr = *((struct in_addr *)ht->h_addr);
    ret = MONITOR_OK;
  }
  return ret;
}

int ServiceListener::LoadService(string path, string service_father, string ip_port, vector<int>& st) {
  char status = STATUS_UNKNOWN;
  int ret = MONITOR_OK;
  if ((ret = zk_get_service_status(path, status))  != MONITOR_OK) {
    LOG(LOG_ERROR, "get service status failed. service:%s", path.c_str());
    return ret;
  }
  if (status < -1 || status > 2) {
    status = -1;
  }
  ++(st[status + 1]);
  string ip;
  int port;
  slash::ParseIpPortString(ip_port, ip, port);
  struct in_addr addr;
  GetAddrByHost(ip, &addr);
  ServiceItem item(ip, &addr, port, service_father, status);
  options_->AddService(path, item);
  LOG(LOG_INFO, "load service succeed, service:%s, status:%d", path.c_str(), status);
  return MONITOR_OK;
}

int ServiceListener::GetIpNum(const string& service_father) {
  int ret = 0;
  slash::MutexLock l(&service_father_to_ip_lock_);
  if (service_father_to_ip_.find(service_father) != service_father_to_ip_.end())
    ret = service_father_to_ip_[service_father].size();
  return ret;
}

//path is the path of ip_port
void ServiceListener::ModifyServiceFatherToIp(const int &op, const string& path) {
  size_t pos = path.rfind('/');
  string service_father = path.substr(0, pos);
  string ip_port = path.substr(pos + 1);
  string ip;
  int port;
  slash::ParseIpPortString(ip_port, ip, port);
  /* size_t pos2 = ip_port.rfind(':');
  string ip = ip_port.substr(0, pos2);
  string port = ip_port.substr(pos2 + 1); */
  if (op == ADD) {
    //If this ip_port has exist, no need to do anything
    if (IsIpExist(service_father, ip_port)) return;

    char status = STATUS_UNKNOWN;
    if (zk_get_service_status(path, status) != MONITOR_OK) return;
    struct in_addr addr;
    GetAddrByHost(ip, &addr);
    ServiceItem item(ip, &addr, port, service_father, status);

    options_->AddService(path, item);
    AddIpPort(service_father, ip_port);
  } else if (op == DELETE) {
    DeleteIpPort(service_father, ip_port);
    options_->DeleteService(path);
  } else if (op == CLEAR) {
    slash::MutexLock l(&service_father_to_ip_lock_);
    service_father_to_ip_.clear();
  }
}
