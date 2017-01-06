#include "pink_define.h"

#include <iostream>

#include "monitor_process.h"
#include "monitor_check_thread.h"
#include "monitor_work_thread.h"

MonitorZk* WorkThread::monitor_zk = NULL;

WorkThread::WorkThread(MonitorOptions *options)
      : options_(options),
        should_exit_(false) {
  load_balance_ = new LoadBalance(options);
  service_listener_ = new ServiceListener(options);
  update_thread_ = new pink::BGThread;
}

WorkThread::~WorkThread() {
  should_exit_ = true;
  delete load_balance_;
  delete service_listener_;
  delete update_thread_;
}

int WorkThread::Init() {
  int ret = kSuccess;
  if ((ret = load_balance_->Init()) != kSuccess) {
    LOG(LOG_ERROR, "init load balance env failed");
    return ret;
  }

  if ((ret = service_listener_->Init()) != kSuccess) {
    LOG(LOG_ERROR, "init service listener env failed");
    return ret;
  }

  if (update_thread_->StartThread() != pink::kSuccess) {
    LOG(LOG_ERROR, "create the update service thread error");
    return kOtherError;
  }

  process::need_restart = false;

  return ret;
}

int WorkThread::Start() {
  if (Init() != kSuccess)
    return kOtherError;

  /*
   * this loop is for load balance.
   * If rebalance is needed, the loop will be reiterate
   */
  while (!should_exit_) {
    LOG(LOG_INFO, " main loop start -> !!!!!!");

    int ret = kOtherError;
    if ((ret = load_balance_->DoBalance()) != kSuccess) {
      LOG(LOG_ERROR, "balance failed");
      // Maybe node doesn't exist, we sleep here.
      sleep(kMonitorSleep);
      continue;
    }

    // After load balance. Each monitor should load the service to Config
    service_listener_->LoadAllService();

    CheckThread *check_threads[kMaxThreadNum];

    // If the number of service father < MAX_THREAD_NUM, one service father one thread
    int thread_num = min(static_cast<int>(options_->service_father_to_ip.size()),
                         kMaxThreadNum);
    for (int i = 0; i < thread_num; ++i) {
      check_threads[i] = new CheckThread(i, update_thread_, options_);
      check_threads[i]->StartThread();
    }

    for (int i = 0; i < thread_num; ++i) {
      check_threads[i]->JoinThread();
      delete check_threads[i];
      LOG(LOG_INFO, "exit check thread: %d", i);
    }
    sleep(kMonitorSleep);
  }
  return kOtherError;
}
