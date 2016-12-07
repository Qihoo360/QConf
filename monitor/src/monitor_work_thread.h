#ifndef MULTITHREAD_H
#define MULTITHREAD_H
#include "bg_thread.h"
#include "slash_mutex.h"

#include <vector>
#include <string>

#include "monitor_options.h"
#include "monitor_const.h"
#include "monitor_log.h"
#include "monitor_load_balance.h"
#include "monitor_listener.h"

using namespace std;

class WorkThread {
  friend class Process;
 private:
  LoadBalance *load_balance_;
  ServiceListener *service_listener_;
  MonitorOptions *options_;
  pink::BGThread *update_thread_;

  //copy of myServiceFather in loadBalance
  vector<string> service_father_;

  //marked weather there is a thread checking this service father
  vector<bool> has_thread_;
  slash::Mutex has_thread_lock_;
  //the next service father waiting for check
  int waiting_index_;
  slash::Mutex waiting_index_lock_;

  bool should_exit_;
 public:
  WorkThread(MonitorOptions *options);
  ~WorkThread();
  int InitEnv();
  int Start();

  int DoLoadBalance();
  void LoadServiceToConf();

  int GetAndAddWaitingIndex();
  vector<string> GetServiceFather() { return service_father_; }
  void SetHasThread(int index, bool val) {
    slash::MutexLock l(&has_thread_lock_);
    has_thread_[index] = val;
  }

  pink::BGThread *GetUpdateThread() { return update_thread_; }
};

#endif
