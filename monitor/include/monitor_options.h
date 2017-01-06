#ifndef MONITOR_OPTIONS_H
#define MONITOR_OPTIONS_H
#include "base_conf.h"
#include "slash_mutex.h"

#include <string>
#include <set>
#include <map>
#include <vector>
#include <atomic>

#include <pthread.h>

#include "monitor_service_item.h"

//znode and path
const std::string LOCK_ROOT_DIR = "/qconf_monitor_lock_node";
const std::string DEFAULT_INSTANCE_NAME = "default_instance";
const std::string MONITOR_LIST = "monitor_list";
const std::string SLASH = "/";
const std::string NODE_LIST = "md5_list";

struct MonitorOptions {
  slash::BaseConf *base_conf;
  bool daemon_mode;
  bool auto_restart;
  std::string monitor_host_name;
  int log_level;
  int conn_retry_count;
  int scan_interval;
  std::string instance_name;
  std::string zk_host;
  std::string zk_log_path;
  int zk_recv_timeout;

  FILE * zk_log_file;

  // Core data. key is the full path of ip_port and value is ServiceItem of this ip_port
  std::map<std::string, ServiceItem> service_map;
  // Core data
  // key is service_father and value is a set of ipPort
  std::vector<std::string> my_service_fathers;
  std::map<std::string, std::set<std::string>> service_father_to_ip;

  // Marked weather there is a thread checking this service father
  std::vector<bool> has_thread;
  slash::Mutex has_thread_lock;
  // The next service father waiting for check
  int waiting_index;
  slash::Mutex waiting_index_lock;

  std::atomic<bool> need_rebalance;

  MonitorOptions(const std::string &conf_path);
  ~MonitorOptions();
  int Load();

  int SetZkLog();

  std::string GetNodeList() {
    return LOCK_ROOT_DIR + SLASH + instance_name + SLASH + NODE_LIST;
  }
  std::string GetMonitorList() {
    return LOCK_ROOT_DIR + SLASH + instance_name + SLASH + MONITOR_LIST;
  }

  void DebugServiceMap();
  void Debug();

  int GetAndAddWaitingIndex();
  void SetHasThread(int index, bool val) {
    slash::MutexLock l(&has_thread_lock);
    has_thread[index] = val;
  }
};

#endif  // MONITOR_OPTIONS_H
