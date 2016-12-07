#include "zookeeper.h"
#include "slash_string.h"

#include <unistd.h>
#include <pthread.h>

#include <fstream>
#include <string>
#include <unordered_map>
#include <unordered_set>
#include <map>
#include <set>
#include <vector>
#include <iostream>

#include "monitor_options.h"
#include "monitor_log.h"
#include "monitor_const.h"

using namespace std;

MonitorOptions::MonitorOptions(const string &confPath) :
  slash::BaseConf(confPath),
  daemon_mode_(0),
  auto_restart_(0),
  monitor_host_name_(""),
  log_level_(2),
  conn_retry_count_(3),
  scan_interval_(3),
  zk_host_("127.0.0.1:2181"),
  zk_log_path_("logs"),
  zk_recv_timeout_(3000) {
    service_map_.clear();
  }

MonitorOptions::~MonitorOptions() {
  if (zk_log_file_) {
    // set the zookeeper log stream to be default stderr
    zoo_set_log_stream(NULL);
    LOG(LOG_DEBUG, "zkLog close ...");
    fclose(zk_log_file_);
    zk_log_file_ = NULL;
  }
}

int MonitorOptions::Load() {
  if (this->LoadConf() != 0)
    return MONITOR_ERR_OTHER;

  Log::init(MAX_LOG_LEVEL);

  GetConfBool(DAEMON_MODE, &daemon_mode_);
  GetConfBool(AUTO_RESTART, &auto_restart_);
  GetConfInt(LOG_LEVEL, &log_level_);
  log_level_ = max(log_level_, MIN_LOG_LEVEL);
  log_level_ = min(log_level_, MAX_LOG_LEVEL);
  // Find the zk host this monitor should focus on. Their idc should be the same
  char hostname[128] = {0};
  if (gethostname(hostname, sizeof(hostname)) != 0) {
    LOG(LOG_ERROR, "get host name failed");
    return MONITOR_ERR_MEM;
  }
  monitor_host_name_.assign(hostname);

  vector<string> word;
  slash::StringSplit(monitor_host_name_, '.', word);
  bool find_zk_host = false;
  for (auto iter = word.begin(); iter != word.end(); iter++) {
    if (GetConfStr(ZK_HOST + *iter, &zk_host_)) {
      find_zk_host = true;
      break;
    }
  }
  if (!find_zk_host) {
    LOG(LOG_ERROR, "get zk host name failed");
    return MONITOR_ERR_OTHER;
  }

  GetConfInt(CONN_RETRY_COUNT, &conn_retry_count_);
  GetConfInt(SCAN_INTERVAL, &scan_interval_);
  GetConfStr(INSTANCE_NAME, &instance_name_);
  instance_name_ = instance_name_.empty() ? DEFAULT_INSTANCE_NAME : instance_name_;
  GetConfStr(ZK_LOG_PATH, &zk_log_path_);
  GetConfInt(ZK_RECV_TIMEOUT, &zk_recv_timeout_);

  // Reload the config result in the change of loglevel in Log
  Log::init(log_level_);

  // Set zookeeper log path
  int ret;
  if ((ret = SetZkLog()) != MONITOR_OK) {
    LOG(LOG_ERROR, "set zk log path failed");
    return ret;
  }
  return MONITOR_OK;
}

int MonitorOptions::SetZkLog() {
  if (zk_log_path_.size() <= 0) {
    return MONITOR_ERR_ZOO_FAILED;
  }
  zk_log_file_ = fopen(zk_log_path_.c_str(), "a+");
  if (!zk_log_file_) {
    LOG(LOG_ERROR, "log file open failed. path:%s. error:%s", zk_log_path_.c_str(), strerror(errno));
    return MONITOR_ERR_FAILED_OPEN_FILE;
  }
  //set the log file stream of zookeeper
  zoo_set_log_stream(zk_log_file_);
  zoo_set_debug_level(ZOO_LOG_LEVEL_WARN);
  LOG(LOG_INFO, "zoo_set_log_stream path:%s", zk_log_path_.c_str());

  return MONITOR_OK;
}

void MonitorOptions::Debug() {
  LOG(LOG_INFO, "daemonMode: %d", daemon_mode_);
  LOG(LOG_INFO, "autoRestart: %d", auto_restart_);
  LOG(LOG_INFO, "logLevel: %d", log_level_);
  LOG(LOG_INFO, "connRetryCount: %d", conn_retry_count_);
  LOG(LOG_INFO, "scanInterval: %d", scan_interval_);
  LOG(LOG_INFO, "instanceName: %s", instance_name_.c_str());
  LOG(LOG_INFO, "zkHost: %s", zk_host_.c_str());
  LOG(LOG_INFO, "zkLogPath: %s", zk_log_path_.c_str());
}

void MonitorOptions::DebugServiceMap() {
  for (auto it = service_map_.begin(); it != service_map_.end(); ++it) {
    LOG(LOG_INFO, "path: %s", (it->first).c_str());
    LOG(LOG_INFO, "host: %s", (it->second).Host().c_str());
    LOG(LOG_INFO, "port: %d", (it->second).Port());
    LOG(LOG_INFO, "service father: %s", (it->second).GetServiceFather().c_str());
    LOG(LOG_INFO, "status: %d", (it->second).Status());
  }
}
