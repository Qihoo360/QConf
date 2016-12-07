#ifndef SERVICELISTENER_H
#define SERVICELISTENER_H
#include <zookeeper.h>
#include <zk_adaptor.h>
#include "slash_mutex.h"

#include <pthread.h>

#include <set>
#include <unordered_set>
#include <map>
#include <unordered_map>
#include <string>
#include <vector>
#include <iostream>

#include "monitor_options.h"
#include "monitor_service_item.h"
#include "monitor_load_balance.h"

using namespace std;

class ServiceListener : public MonitorZk {
 private:
  LoadBalance *load_balance_;
  /*
     core data
     key is serviceFather and value is a set of ipPort
     */
  unordered_map<string, unordered_set<string>> service_father_to_ip_;
  slash::Mutex service_father_to_ip_lock_;

  int AddChildren(const string &service_father, struct String_vector &children);
  int GetAddrByHost(const string &host, struct in_addr* addr);
  int LoadService(string path, string service_father, string ip_port, vector<int>& );
  int GetIpNum(const string& service_father);
  void ModifyServiceFatherToIp(const string &op, const string& path);
  bool IsIpExist(const string& service_father, const string& ip_port) {
    slash::MutexLock l(&service_father_to_ip_lock_);
    return (service_father_to_ip_[service_father].find(ip_port) == service_father_to_ip_[service_father].end());
  }
  bool IsServiceFatherExist(const string& service_father) {
    slash::MutexLock l(&service_father_to_ip_lock_);
    return (service_father_to_ip_.find(service_father) == service_father_to_ip_.end());
  }
  void AddIpPort(const string& service_father, const string& ip_port) {
    slash::MutexLock l(&service_father_to_ip_lock_);
    service_father_to_ip_[service_father].insert(ip_port);
  }
  void DeleteIpPort(const string& service_father, const string& ip_port) {
    slash::MutexLock l(&service_father_to_ip_lock_);
    service_father_to_ip_[service_father].erase(ip_port);
  }
 public:
  ServiceListener(LoadBalance *load_balance, MonitorOptions *options);
  int InitListener();
  void GetAllIp();
  void LoadAllService();
  void CleanServiceFatherToIp() { service_father_to_ip_.clear(); }
  unordered_map<string, unordered_set<string>> GetServiceFatherToIp() {
    slash::MutexLock l(&service_father_to_ip_lock_);
    return service_father_to_ip_;
  }

  void ProcessDeleteEvent(const string& path);
  void ProcessChildEvent(const string& path);
  void ProcessChangedEvent(const string& path);
};
#endif
