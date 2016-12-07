#ifndef CONFIG_H
#define CONFIG_H
#include "base_conf.h"
#include "slash_mutex.h"

#include <string>
#include <unordered_set>
#include <map>
#include <vector>
#include <unordered_map>

#include <pthread.h>

#include "monitor_service_item.h"

using namespace std;

//znode and path
const string LOCK_ROOT_DIR = "/qconf_monitor_lock_node";
const string DEFAULT_INSTANCE_NAME = "default_instance";
const string MONITOR_LIST = "monitor_list";
const string SLASH = "/";
const string NODE_LIST = "md5_list";

class MonitorOptions : public slash::BaseConf {
 private:
  bool daemon_mode_;
  bool auto_restart_;
  string monitor_host_name_;
  int log_level_;
  int conn_retry_count_;
  int scan_interval_;
  string instance_name_;
  string zk_host_;
  string zk_log_path_;
  int zk_recv_timeout_;

  FILE * zk_log_file_;
  int SetZkLog();

  // Core data. the key is the full path of ipPort and the value is serviceItem of this ipPort
  map<string, ServiceItem> service_map_;
  slash::Mutex service_map_lock_;

 public:
  MonitorOptions(const string &conf_path);
  ~MonitorOptions();
  int Load();

  bool IsDaemonMode() { return daemon_mode_; }
  bool IsAutoRestart() { return auto_restart_; }
  string MonitorHostname() { return monitor_host_name_; }
  int LogLevel() { return log_level_; }
  int ConnRetryCount() { return conn_retry_count_; }
  int ScanInterval() { return scan_interval_; }
  string InstanceName() { return instance_name_; }
  string ZkHost() { return zk_host_; }
  string ZkLogPath() { return zk_log_path_; }
  int ZkRecvTimeout() { return zk_recv_timeout_; }
  string NodeList() {
    return LOCK_ROOT_DIR + SLASH + instance_name_ + SLASH + NODE_LIST;
  }
  string MonitorList() {
    return LOCK_ROOT_DIR + SLASH + instance_name_ + SLASH + MONITOR_LIST;
  }

  map<string, ServiceItem> GetServiceMap() {
    slash::MutexLock l(&service_map_lock_);
    return service_map_;
  }

  void SetServiceMap(string node, int val) {
    slash::MutexLock l(&service_map_lock_);
    service_map_[node].SetStatus(val);
  }

  void AddService(string ip_path, ServiceItem item) {
    slash::MutexLock l(&service_map_lock_);
    service_map_[ip_path] = item;
  }

  void DeleteService(const string& ip_path) {
    slash::MutexLock l(&service_map_lock_);
    service_map_.erase(ip_path);
  }

  void ClearService() { service_map_.clear(); }
  ServiceItem GetServiceItem(const string& ip_path) {
    slash::MutexLock l(&service_map_lock_);
    return service_map_[ip_path];
  }
  void DebugServiceMap();

  void Debug();
};
#endif
