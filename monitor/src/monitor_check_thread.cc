#include "monitor_check_thread.h"
#include "monitor_options.h"
#include "monitor_process.h"
#include "monitor_const.h"
#include "monitor_log.h"

CheckThread::CheckThread(int init_pos,
                         pink::BGThread *update_thread,
                         MonitorOptions *options)
      : pink::Thread::Thread(options->scan_interval),
        service_pos_(init_pos),
        options_(options),
        update_thread_(update_thread) {
  pcli_ = new TestCli();
  pcli_->set_connect_timeout(3);
  if (cron_interval_ <= 0) cron_interval_ = 3;
}

CheckThread::~CheckThread() {
  should_exit_ = true;
  delete pcli_;
}

// Try to connect to the ip_port to see weather it's connecteble
int CheckThread::TryConnect(const std::string &cur_service_father) {
  std::set<std::string> &ip_ports = options_->service_father_to_ip[cur_service_father];
  int retry_count = options_->conn_retry_count;
  for (auto &ip_port : ip_ports) {
    if (should_exit_) break;
    std::string ip_port_path = cur_service_father + "/" + ip_port;
    /*
     * some service father don't have services and we add "" to service_father_to_ip
     * so we need to judge weather It's a legal ip_port
     */
    if (options_->service_map.find(ip_port_path) == options_->service_map.end())
      continue;

    ServiceItem item = options_->service_map[ip_port_path];
    int old_status = item.status;
    // If the node is kStatusUnknow or kStatusOffline, we will ignore it
    if (old_status == kStatusUnknow || old_status == kStatusOffline)
      continue;

    int cur_try_times = (old_status == kStatusUp) ? 1 : 3;
    int status = kStatusDown;
    do {
      pcli_->Connect(item.host, item.port);
      status = (pcli_->Available()) ? kStatusUp : kStatusDown;
      cur_try_times++;
    } while (cur_try_times < retry_count && status == kStatusDown);
    pcli_->Close();

    LOG(LOG_INFO, "|checkService| service:%s, old status:%d, new status:%d. Have tried times:%d, max try times:%d",
        ip_port_path.c_str(), old_status, status, cur_try_times, retry_count);

    if (status != old_status) {
      UpdateServiceArgs *update_service_args = new UpdateServiceArgs(ip_port_path, status, options_);
      update_thread_->Schedule(UpdateServiceFunc, (void *)update_service_args);
    }
  }
  return 0;
}

void *CheckThread::ThreadMain() {
  int try_times = 0;
  while (!should_exit_ && !options_->need_rebalance && !process::need_restart) {
    try_times++;
    if (try_times >= cron_interval_) {
        CronHandle();
        try_times = 0;
    }
    sleep(1);
  }
  return NULL;
}

void CheckThread::CronHandle() {
  int service_father_num = options_->my_service_fathers.size();
  std::string &cur_service_father = options_->my_service_fathers[service_pos_];
  LOG(LOG_INFO, "|checkService| pthread id %x, pthread pos %d, current service father %s", \
      (unsigned int)this->thread_id(), (int)service_pos_, cur_service_father.c_str());

  TryConnect(cur_service_father);

  if (service_father_num > kMaxThreadNum) {
    options_->SetHasThread(service_pos_, false);
    service_pos_ = options_->GetAndAddWaitingIndex();
    options_->SetHasThread(service_pos_, true);
  }
}
