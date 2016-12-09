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

CheckThread::CheckThread(int pos,
                         WorkThread *work_thread,
                         MonitorOptions *options,
                         ServiceListener *service_listener) :
  pink::Thread::Thread(options->ScanInterval()),
  service_pos_(pos),
  work_thread_(work_thread),
  options_(options),
  service_listener_(service_listener) {}

CheckThread::~CheckThread() {
  should_exit_ = true;
}

bool CheckThread::IsServiceExist(struct in_addr *addr, string host, int port, int timeout, int cur_status) {
  bool exist = false;
  int sock = -1, val = 1, ret = 0;
  struct timeval conn_tv;
  struct timeval recv_tv;
  struct sockaddr_in serv_addr;
  fd_set readfds, writefds, errfds;

  timeout = timeout <= 0 ? 1 : timeout;

  if((sock = socket(AF_INET, SOCK_STREAM, 0)) < 0) {
    LOG(LOG_ERROR, "socket failed. error:%s", strerror(errno));
    // return false is a good idea ?
    return false;
  }

  memset(&serv_addr, 0, sizeof(serv_addr));
  serv_addr.sin_family = AF_INET;
  serv_addr.sin_port = htons(port);
  serv_addr.sin_addr = *addr;

  // set socket non-block
  ioctl(sock, FIONBIO, &val);

  // set connect timeout
  conn_tv.tv_sec = timeout;
  conn_tv.tv_usec = 0;

  // set recv timeout
  recv_tv.tv_sec = 1;
  recv_tv.tv_sec = 0;
  setsockopt( sock, SOL_SOCKET, SO_RCVTIMEO, &recv_tv, sizeof(recv_tv));

  // connect
  if (connect(sock, (struct sockaddr *)&serv_addr, sizeof(serv_addr)) != 0) {
    if (errno != EINPROGRESS) {
      if (cur_status != STATUS_DOWN) {
        LOG(LOG_ERROR, "connect failed. host:%s port:%d error:%s",
            host.c_str(), port, strerror(errno));
      }
      close(sock);
      return false;
    }
  }
  FD_ZERO(&readfds);
  FD_SET(sock, &readfds);
  FD_ZERO(&writefds);
  FD_SET(sock, &writefds);
  FD_ZERO(&errfds);
  FD_SET(sock, &errfds);
  ret = select(sock+1, &readfds, &writefds, &errfds, &conn_tv);
  if ( ret <= 0 ){
    // connect timeout
    if (cur_status != STATUS_DOWN) {
      if (ret == 0) LOG(LOG_ERROR, "connect timeout. host:%s port:%d timeout:%d error:%s",
                        host.c_str(), port, timeout, strerror(errno));
      else LOG(LOG_ERROR, "select error. host:%s port:%d timeout:%d error:%s",
               host.c_str(), port, timeout, strerror(errno));
    }
    exist = false;
  }
  else {
    if (! FD_ISSET(sock, &readfds) && ! FD_ISSET(sock, &writefds)) {
      if (cur_status != STATUS_DOWN) {
        LOG(LOG_ERROR, "select not in read fds and write fds.host:%s port:%d error:%s",
            host.c_str(), port, strerror(errno));
      }
    }
    else if (FD_ISSET(sock, &errfds)) {
      exist = false;
    }
    else if (FD_ISSET(sock, &writefds) && FD_ISSET(sock, &readfds)) {
      exist = false;
    }
    else if (FD_ISSET(sock, &readfds) || FD_ISSET(sock, &writefds)) {
      exist = true;
    }
    else {
      exist = false;
    }
  }
  close(sock);
  return exist;
}

// Try to connect to the ip_port to see weather it's connecteble
int CheckThread::TryConnect(const string &cur_service_father) {
  auto service_father_to_ip = service_listener_->GetServiceFatherToIp();
  unordered_set<string> ip = service_father_to_ip[cur_service_father];
  int retry_count = options_->ConnRetryCount();
  for (auto it = ip.begin(); it != ip.end() && !should_exit_; ++it) {
    // It's important to get service_map in the loop to find zk's change in real time
    auto service_map = options_->GetServiceMap();
    string ip_port = cur_service_father + "/" + (*it);
    /*
       some service father don't have services and we add "" to service_father_to_ip
       so we need to judge weather It's a legal ip_port
       */
    if (service_map.find(ip_port) == service_map.end())
      continue;

    ServiceItem item = service_map[ip_port];
    int old_status = item.Status();
    //If the node is STATUS_UNKNOWN or STATUS_OFFLINE, we will ignore it
    if (old_status == STATUS_UNKNOWN || old_status == STATUS_OFFLINE)
      continue;

    struct in_addr addr;
    item.Addr(&addr);
    int cur_try_times = (old_status == STATUS_UP) ? 1 : 3;
    int timeout = item.ConnectTimeout() > 0 ? item.ConnectTimeout() : 3;
    int status = STATUS_DOWN;
    do {
      LOG(LOG_ERROR, "can not connect to service:%s, current try times:%d, max try times:%d", ip_port.c_str(), cur_try_times, retry_count);
      bool res = IsServiceExist(&addr, item.Host(), item.Port(), timeout, item.Status());
      status = (res) ? STATUS_UP : STATUS_DOWN;
    } while (cur_try_times < retry_count && status == STATUS_DOWN);

    LOG(LOG_INFO, "|checkService| service:%s, old status:%d, new status:%d. Have tried times:%d, max try times:%d", ip_port.c_str(), old_status, status, cur_try_times, retry_count);
    if (status != old_status) {
      UpdateServiceArgs *update_service_args =
        new UpdateServiceArgs(ip_port, status, options_, service_listener_);
      work_thread_->GetUpdateThread()->Schedule(UpdateServiceFunc, (void *)update_service_args);
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
  int timeout = cron_interval_;

  while (!should_exit_) {
    if (cron_interval_ > 0 ) {
      gettimeofday(&now, NULL);
      if (when.tv_sec > now.tv_sec || (when.tv_sec == now.tv_sec && when.tv_usec > now.tv_usec)) {
        timeout = (when.tv_sec - now.tv_sec) * 1000 + (when.tv_usec - now.tv_usec) / 1000;
      } else {
        when.tv_sec = now.tv_sec + (cron_interval_ / 1000);
        when.tv_usec = now.tv_usec + ((cron_interval_ % 1000 ) * 1000);
        CronHandle();
        timeout = cron_interval_;
      }
    }
    sleep(timeout);
  }
  return NULL;
}

void CheckThread::CronHandle() {
  auto service_father = work_thread_->GetServiceFather();
  int service_father_num = service_father.size();
  string cur_service_father = service_father[service_pos_];
  LOG(LOG_INFO, "|checkService| pthread id %x, pthread pos %d, current service father %s", \
      (unsigned int)this->thread_id(), (int)service_pos_, cur_service_father.c_str());

  TryConnect(cur_service_father);

  if (service_father_num > MAX_THREAD_NUM) {
    work_thread_->SetHasThread(service_pos_, false);
    service_pos_ = work_thread_->GetAndAddWaitingIndex();
    work_thread_->SetHasThread(service_pos_, true);
  }
}
