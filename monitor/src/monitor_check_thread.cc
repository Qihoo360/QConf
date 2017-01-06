#include <signal.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/socket.h>
#include <netdb.h>
#include <sys/ioctl.h>
#include <fcntl.h>
#include <time.h>
#include <errno.h>

#include <unordered_set>
#include <string>
#include <utility>
#include <map>
#include <cstring>

#include "monitor_check_thread.h"
#include "monitor_options.h"
#include "monitor_const.h"
#include "monitor_log.h"
#include "monitor_process.h"
#include "monitor_load_balance.h"
#include "monitor_listener.h"

CheckThread::CheckThread(int init_pos,
                         pink::BGThread *update_thread,
                         MonitorOptions *options)
      : pink::Thread::Thread(options->scan_interval),
        service_pos_(init_pos),
        options_(options),
        update_thread_(update_thread) {
  pcli_ = new TestCli();
  pcli_->set_connect_timeout(3);
}

CheckThread::~CheckThread() {
  should_exit_ = true;
}

// Try to connect to the ip_port to see weather it's connecteble
int CheckThread::TryConnect(const std::string &cur_service_father) {
  std::set<std::string> &ip = options_->service_father_to_ip[cur_service_father];
  int retry_count = options_->conn_retry_count;
  for (auto it = ip.begin(); it != ip.end() && !should_exit_; ++it) {
    // It's important to get service_map in the loop to find zk's change in real time
    std::string ip_port = cur_service_father + "/" + (*it);
    /*
     * some service father don't have services and we add "" to service_father_to_ip
     * so we need to judge weather It's a legal ip_port
     */
    if (options_->service_map.find(ip_port) == options_->service_map.end())
      continue;

    ServiceItem item = options_->service_map[ip_port];
    int old_status = item.status;
    //If the node is STATUS_UNKNOWN or STATUS_OFFLINE, we will ignore it
    if (old_status == STATUS_UNKNOWN || old_status == STATUS_OFFLINE)
      continue;

    int cur_try_times = (old_status == STATUS_UP) ? 1 : 3;
    int status = STATUS_DOWN;
    do {
      pcli_->Connect(item.host, item.port);
      status = (pcli_->Available()) ? STATUS_UP : STATUS_DOWN;
      cur_try_times++;
    } while (cur_try_times < retry_count && status == STATUS_DOWN);
    pcli_->Close();

    LOG(LOG_INFO, "|checkService| service:%s, old status:%d, new status:%d. Have tried times:%d, max try times:%d", ip_port.c_str(), old_status, status, cur_try_times, retry_count);
    if (status != old_status) {
      UpdateServiceArgs *update_service_args = new UpdateServiceArgs(ip_port, status, options_);
      update_thread_->Schedule(UpdateServiceFunc, (void *)update_service_args);
    }
  }
  return 0;
}

void *CheckThread::ThreadMain() {
  struct timeval when;
  gettimeofday(&when, NULL);
  struct timeval now = when;

  when.tv_sec += (cron_interval_ / 1000);
  when.tv_usec += ((cron_interval_ % 1000 ) * 1000);
  int cron_timeout = cron_interval_;

  while (!should_exit_ && !options_->need_rebalance && !process::need_restart) {
    if (cron_interval_ > 0 ) {
      gettimeofday(&now, NULL);
      if (when.tv_sec > now.tv_sec || (when.tv_sec == now.tv_sec && when.tv_usec > now.tv_usec)) {
        cron_timeout = (when.tv_sec - now.tv_sec) * 1000 + (when.tv_usec - now.tv_usec) / 1000;
      } else {
        when.tv_sec = now.tv_sec + (cron_interval_ / 1000);
        when.tv_usec = now.tv_usec + ((cron_interval_ % 1000 ) * 1000);
        CronHandle();
        cron_timeout = cron_interval_;
      }
    }
    sleep(cron_timeout);
  }
  return NULL;
}

void CheckThread::CronHandle() {
  int service_father_num = options_->my_service_fathers.size();
  std::string &cur_service_father = options_->my_service_fathers[service_pos_];
  LOG(LOG_INFO, "|checkService| pthread id %x, pthread pos %d, current service father %s", \
      (unsigned int)this->thread_id(), (int)service_pos_, cur_service_father.c_str());

  TryConnect(cur_service_father);

  if (service_father_num > MAX_THREAD_NUM) {
    options_->SetHasThread(service_pos_, false);
    service_pos_ = options_->GetAndAddWaitingIndex();
    options_->SetHasThread(service_pos_, true);
  }
}
