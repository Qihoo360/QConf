#ifndef LOADBALANCE_H
#define LOADBALANCE_H
#include "slash_mutex.h"

#include <unordered_set>
#include <unordered_map>
#include <string>
#include <vector>
#include <iostream>

#include <pthread.h>

#include <zookeeper.h>
#include <zk_adaptor.h>

#include "monitor_options.h"
#include "monitor_zk.h"

using namespace std;

// Implement Zk interface
class LoadBalance : public MonitorZk {
 private:
  char zk_lock_buf_[512] = {0};
  bool need_rebalance_;
  slash::Mutex md5_to_service_father_lock_;

  //use map but not unordered_map so it can be sorted autonatically
  //key is md5 and value is serviceFather
  unordered_map<string, string> md5_to_service_father_;
  unordered_set<string> monitors_;
  vector<string> my_service_father_;

  void UpdateMd5ToServiceFather(const string& md5_path, const string& service_father) {
    slash::MutexLock l(&md5_to_service_father_lock_);
    md5_to_service_father_[md5_path] = service_father;
  }
 public:
  // Initial
  LoadBalance(MonitorOptions *options);
  int InitMonitor();
  int RegisterMonitor(const string &path);
  int GetMd5ToServiceFather();
  int GetMonitors();
  int DoBalance();

  void ProcessDeleteEvent(const string& path);
  void ProcessChildEvent(const string &path);
  void ProcessChangedEvent(const string &path);

  // Setter
  void SetReBalance() { need_rebalance_ = true; }
  // Getter
  bool NeedReBalance() { return need_rebalance_; }
  const vector<string> GetMyServiceFather() { return my_service_father_; }
  int GetMyServiceFatherNum() { return my_service_father_.size(); }
};
#endif
