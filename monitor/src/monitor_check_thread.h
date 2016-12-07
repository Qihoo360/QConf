#ifndef MONITOR_CHECK_THREAD_H
#define MONITOR_CHECK_THREAD_H
#include "pink_thread.h"

#include <string>

#include "monitor_work_thread.h"

using namespace std;

class CheckThread : public pink::Thread {
 public:
  CheckThread(int init_pos,
              WorkThread *work_thread,
              MonitorOptions *options,
              ServiceListener *service_listener);
  ~CheckThread();

 private:
  void *ThreadMain();
  int TryConnect(const string &cur_service_father);
  void CronHandle();
  bool IsServiceExist(struct in_addr *addr, string host, int port, int timeout, int cur_status);

  int service_pos_;
  bool should_exit_;
  WorkThread *work_thread_;
  MonitorOptions *options_;
  ServiceListener *service_listener_;
};

struct UpdateServiceArgs {
  explicit UpdateServiceArgs(string ip_port, int new_status,
                    MonitorOptions *options,
                    ServiceListener *service_listener) :
    ip_port(ip_port),
    new_status(new_status),
    options(options),
    service_listener(service_listener) {}
  string ip_port;
  int new_status;
  MonitorOptions *options;
  ServiceListener *service_listener;
};

void UpdateServiceFunc(void *arg);

#endif
