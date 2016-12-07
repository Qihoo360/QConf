#include "pink_define.h"

#include <iostream>

#include "monitor_check_thread.h"
#include "monitor_work_thread.h"

WorkThread::WorkThread(MonitorOptions *options) :
  options_(options),
  waiting_index_(MAX_THREAD_NUM),
  should_exit_(false) {
  load_balance_ = new LoadBalance(options);
  service_listener_ = new ServiceListener(load_balance_, options_);
  update_thread_ = new pink::BGThread;
}

WorkThread::~WorkThread() {
  should_exit_ = true;
  delete load_balance_;
  delete service_listener_;
  delete update_thread_;
}

int WorkThread::InitEnv() {
  int ret = MONITOR_OK;
  if ((ret = load_balance_->InitMonitor()) != MONITOR_OK) {
    LOG(LOG_ERROR, "init load balance env failed");
    return ret;
  }

  if ((ret = service_listener_->InitListener()) != MONITOR_OK) {
    LOG(LOG_ERROR, "init service listener env failed");
    return ret;
  }

  if (update_thread_->StartThread() != pink::kSuccess) {
    LOG(LOG_ERROR, "create the update service thread error");
    return MONITOR_ERR_OTHER;
  }

  return ret;
}

int WorkThread::DoLoadBalance() {
    int ret = MONITOR_OK;
    if ((ret = load_balance_->GetMd5ToServiceFather()) != MONITOR_OK) {
        LOG(LOG_ERROR, "get md5 to service father failed");
        /* TODO (gaodq)
         * how to deal with this in a better way?
         * if the reason of failure is node not exist, we should restart main loop
        */
        return ret;
    }

    if ((ret = load_balance_->GetMonitors()) != MONITOR_OK) {
        LOG(LOG_ERROR, "get monitors failed");
        return ret;
    }

    if ((ret = load_balance_->DoBalance()) != MONITOR_OK) {
        LOG(LOG_ERROR, "balance failed");
        return ret;
    }
    return ret;
}

void WorkThread::LoadServiceToConf() {
    options_->ClearService();
    service_listener_->CleanServiceFatherToIp();
    service_listener_->GetAllIp();
    service_listener_->LoadAllService();
}

int WorkThread::Start() {
  if (InitEnv() != MONITOR_OK)
    return MONITOR_ERR_OTHER;

  /*
   * this loop is for load balance.
   * If rebalance is needed, the loop will be reiterate
   */
  while (!should_exit_) {
    LOG(LOG_INFO, " main loop start -> !!!!!!");
    if (DoLoadBalance() != MONITOR_OK)
      continue;
    // After load balance. Each monitor should load the service to Config
    LoadServiceToConf();

    service_father_ = load_balance_->GetMyServiceFather();
    CheckThread *check_threads[MAX_THREAD_NUM];
    int old_thread_num = 0;
    int new_thread_num = 0;
    while (!load_balance_->NeedReBalance()) {
      auto service_father_to_ip = service_listener_->GetServiceFatherToIp();

      // If the number of service father < MAX_THREAD_NUM, one service father one thread
      new_thread_num = min(static_cast<int>(service_father_to_ip.size()), MAX_THREAD_NUM);
      for (; old_thread_num< new_thread_num; ++old_thread_num) {
        check_threads[old_thread_num] = new CheckThread(old_thread_num, this, options_, service_listener_);
        check_threads[old_thread_num]->StartThread();
      }
      sleep(MONITOR_SLEEP);
    }

    for (int i = 0; i < old_thread_num; ++i) {
      delete check_threads[i];
      LOG(LOG_INFO, "exit check thread: %d", i);
    }
    sleep(MONITOR_SLEEP);
  }
  return MONITOR_ERR_OTHER;
}

int WorkThread::GetAndAddWaitingIndex() {
  int index;
  slash::MutexLock lw(&waiting_index_lock_);
  int service_father_num_ = service_father_.size();
  index = waiting_index_;
  waiting_index_ = (waiting_index_+1) % service_father_num_;
  slash::MutexLock lh(&has_thread_lock_);
  while (has_thread_[waiting_index_]) {
    waiting_index_ = (waiting_index_+1) % service_father_num_;
  }
  return index;
}
