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

//config file name
const std::string kConfPath = "conf/monitor.conf";

//multi thread
#ifdef DEBUGT
constexpr int kMaxThreadNum = 1;
#else
constexpr int kMaxThreadNum = 64;
#endif

//config file keys
const std::string kDaemonMode = "daemon_mode";
const std::string kAutoRestart = "auto_restart";
const std::string kLogLevel = "log_level";
const std::string kConnRetryCount = "connect_retry_count";
const std::string kScanInterval = "scan_interval";
const std::string kInstanceName = "instance_name";
const std::string kZkHostPrefix = "zookeeper.";
const std::string kZkLogPath = "zk_log_path";
const std::string kZkRecvTimeout = "zk_recv_timeout";

constexpr int kMinLogLevel = 0;
constexpr int kMaxLogLevel = 6;

//znode and path
const std::string kLockRootDir = "/qconf_monitor_lock_node";
const std::string kDefaultInstanceName = "default_instance";
const std::string kMonitorList = "monitor_list";
const std::string kNodeList = "md5_list";

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
    return kLockRootDir + "/" + instance_name + "/" + kNodeList;
  }
  std::string GetMonitorList() {
    return kLockRootDir + "/" + instance_name + "/" + kMonitorList;
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
