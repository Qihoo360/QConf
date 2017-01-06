#ifndef LOADBALANCE_H
#define LOADBALANCE_H

#include "slash_mutex.h"

#include <string>
#include <vector>
#include <iostream>

#include <pthread.h>

#include <zookeeper.h>
#include <zk_adaptor.h>

#include "monitor_options.h"
#include "monitor_zk.h"

class LoadBalance {
 public:
  // Initial
  LoadBalance(MonitorOptions *options);
  ~LoadBalance();
  int Init();
  int DoBalance();

 private:
  // Implement of interface for callback handle
  struct BalanceZkHandle : public MonitorZk::ZkCallBackHandle {
    MonitorOptions *options_;
    MonitorZk *monitor_zk_;
    std::map<std::string, std::string> *md5_to_service_father_;

    void ProcessDeleteEvent(const std::string& path);
    void ProcessChildEvent(const std::string &path);
    void ProcessChangedEvent(const std::string &path);
  };

  char zk_lock_buf_[512] = {0};

  MonitorOptions *options_;
  MonitorZk *monitor_zk_;
  BalanceZkHandle cb_handle;

  // Use map but not unordered_map so it can be sorted autonatically
  // Key is md5 and value is service_father
  std::map<std::string, std::string> md5_to_service_father_;
  slash::Mutex md5_to_service_father_lock_;

  int RegisterMonitor(const std::string &path);
  int GetMd5ToServiceFather();
  int GetMonitors(size_t &monitors_size, size_t &rank);
};
#endif
