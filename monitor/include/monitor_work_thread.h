#ifndef MULTITHREAD_H
#define MULTITHREAD_H

#include "bg_thread.h"
#include "slash_mutex.h"

#include <vector>
#include <string>
#include <atomic>

#include "monitor_options.h"
#include "monitor_const.h"
#include "monitor_log.h"
#include "monitor_load_balance.h"
#include "monitor_listener.h"

const int kMonitorSleep = 2;

class WorkThread {
 public:
  WorkThread(MonitorOptions *options);
  ~WorkThread();
  int Init();
  int Start();

  void LoadServiceToConf();

 private:
  LoadBalance *load_balance_;
  ServiceListener *service_listener_;
  MonitorOptions *options_;
  pink::BGThread *update_thread_;

  std::atomic<bool> should_exit_;

 public:
  // Only used for update status in update thread.
  // does not need callback handle
  struct WorkCallbackHandle : public MonitorZk::ZkCallBackHandle {
    void ProcessDeleteEvent(const string& path) {}
    void ProcessChildEvent(const string &path) {}
    void ProcessChangedEvent(const string &path) {}
  };

  static MonitorZk *monitor_zk;
  static MonitorZk *GetZkInstance(MonitorOptions *options) {
    if (monitor_zk == NULL) {
      static WorkCallbackHandle cb_handle;
      monitor_zk = new MonitorZk(options, &cb_handle);
      int ret = kOtherError;
      if ((ret = monitor_zk->InitEnv()) != kSuccess) {
        LOG(LOG_ERROR, "Init zookeeper client failed");
      }
    }
    return monitor_zk;
  }
};

#endif  //  MULTITHREAD_H
