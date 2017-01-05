#ifndef MONITOR_CHECK_THREAD_H
#define MONITOR_CHECK_THREAD_H
#include "pink_thread.h"

#include <string>

#include "monitor_options.h"
#include "monitor_work_thread.h"

using namespace std;

class CheckThread : public pink::Thread {
 public:
  CheckThread(int init_pos, pink::BGThread *update_thread, MonitorOptions *options);
  ~CheckThread();

 private:
  void *ThreadMain();
  int TryConnect(const string &cur_service_father);
  void CronHandle();
  bool IsServiceExist(struct in_addr *addr, string host, int port, int timeout, int cur_status);

  int service_pos_;
  MonitorOptions *options_;
  pink::BGThread *update_thread_;
};

struct UpdateServiceArgs {
  explicit UpdateServiceArgs(string ip_port, int new_status, MonitorOptions *options)
      : ip_port(ip_port),
        new_status(new_status),
        options(options) {
  }
  string ip_port;
  int new_status;
  MonitorOptions *options;
};

void UpdateServiceFunc(void *arg);

#endif  // MONITOR_CHECK_THREAD_H
