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

class ServiceListener {
 public:
  ServiceListener(MonitorOptions *options);
  ~ServiceListener();
  int Init();
  void LoadAllService();
  int ZkModify(const std::string &path, const std::string &value);

 private:
  // Implement of interface for callback handle
  struct BalanceZkHandle : public MonitorZk::ZkCallBackHandle {
    MonitorOptions *options_;
    MonitorZk *monitor_zk_;

    void ModifyServiceFatherToIp(const int &op, const std::string& path);
    void ProcessDeleteEvent(const std::string& path);
    void ProcessChildEvent(const std::string &path);
    void ProcessChangedEvent(const std::string &path);
  };

  MonitorOptions *options_;
  MonitorZk *monitor_zk_;
  BalanceZkHandle cb_handle;

  void GetAllIp();
  int AddChildren(const std::string &service_father, struct String_vector &children);
  int LoadService(std::string path, std::string service_father, std::string ip_port, vector<int>& );
};
#endif
