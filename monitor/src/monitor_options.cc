#include "zookeeper.h"
#include "slash_string.h"

#include <iostream>

#include "monitor_options.h"
#include "monitor_log.h"
#include "monitor_const.h"

MonitorOptions::MonitorOptions(const std::string &conf_path)
      : base_conf(new slash::BaseConf(conf_path)),
        daemon_mode(0),
        auto_restart(0),
        monitor_host_name(""),
        log_level(2),
        conn_retry_count(3),
        scan_interval(3),
        zk_host("127.0.0.1:2181"),
        zk_log_path("logs"),
        zk_recv_timeout(3000),
        zk_log_file(nullptr),
        waiting_index(kMaxThreadNum),
        need_rebalance(false) {
  service_map.clear();
}

MonitorOptions::~MonitorOptions() {
  if (zk_log_file) {
    // Set the zookeeper log stream to be default stderr
    zoo_set_log_stream(nullptr);
    LOG(LOG_DEBUG, "zkLog close ...");
    fclose(zk_log_file);
    zk_log_file = nullptr;
  }
  delete base_conf;
}

int MonitorOptions::Load() {
  if (base_conf->LoadConf() != 0)
    return kOtherError;

  mlog::Init(kMaxLogLevel);

  base_conf->GetConfBool(kDaemonMode, &daemon_mode);
  base_conf->GetConfBool(kAutoRestart, &auto_restart);
  base_conf->GetConfInt(kLogLevel, &log_level);
  log_level = max(log_level, kMinLogLevel);
  log_level = min(log_level, kMaxLogLevel);

  // Find the zk host this monitor should focus on. Their idc should be the same
  char hostname[128] = {0};
  if (gethostname(hostname, sizeof(hostname)) != 0) {
    LOG(LOG_ERROR, "get host name failed");
    return kOtherError;
  }
  monitor_host_name.assign(hostname);

  std::vector<std::string> word;
  slash::StringSplit(monitor_host_name, '.', word);
  bool find_zk_host = false;
  for (auto iter = word.begin(); iter != word.end(); iter++) {
    if (base_conf->GetConfStr(kZkHostPrefix + *iter, &zk_host)) {
      find_zk_host = true;
      break;
    }
  }
  if (!find_zk_host) {
    LOG(LOG_ERROR, "get zk host name failed");
    return kOtherError;
  }

  base_conf->GetConfInt(kConnRetryCount, &conn_retry_count);
  base_conf->GetConfInt(kScanInterval, &scan_interval);
  base_conf->GetConfStr(kInstanceName, &instance_name);
  instance_name = instance_name.empty() ? kDefaultInstanceName : instance_name;
  base_conf->GetConfStr(kZkLogPath, &zk_log_path);
  base_conf->GetConfInt(kZkRecvTimeout, &zk_recv_timeout);

  // Reload the config result in the change of loglevel in Log
  mlog::Init(log_level);

  // Set zookeeper log path
  int ret;
  if ((ret = SetZkLog()) != kSuccess) {
    LOG(LOG_ERROR, "set zk log path failed");
    return ret;
  }
  return kSuccess;
}

int MonitorOptions::SetZkLog() {
  if (zk_log_path.size() <= 0) {
    return kZkFailed;
  }
  zk_log_file = fopen(zk_log_path.c_str(), "a+");
  if (!zk_log_file) {
    LOG(LOG_ERROR, "log file open failed. path:%s. error:%s",
        zk_log_path.c_str(), strerror(errno));
    return kOpenFileFailed;
  }
  //set the log file stream of zookeeper
  zoo_set_log_stream(zk_log_file);
  zoo_set_debug_level(ZOO_LOG_LEVEL_WARN);
  LOG(LOG_INFO, "zoo_set_log_stream path:%s", zk_log_path.c_str());

  return kSuccess;
}

void MonitorOptions::Debug() {
  LOG(LOG_INFO, "daemonMode: %d", daemon_mode);
  LOG(LOG_INFO, "autoRestart: %d", auto_restart);
  LOG(LOG_INFO, "logLevel: %d", log_level);
  LOG(LOG_INFO, "connRetryCount: %d", conn_retry_count);
  LOG(LOG_INFO, "scanInterval: %d", scan_interval);
  LOG(LOG_INFO, "instanceName: %s", instance_name.c_str());
  LOG(LOG_INFO, "zkHost: %s", zk_host.c_str());
  LOG(LOG_INFO, "zkLogPath: %s", zk_log_path.c_str());
}

void MonitorOptions::DebugServiceMap() {
  for (auto &item : service_map) {
    LOG(LOG_INFO, "path: %s", item.first.c_str());
    LOG(LOG_INFO, "host: %s", item.second.host.c_str());
    LOG(LOG_INFO, "port: %d", item.second.port);
    LOG(LOG_INFO, "service father: %s", item.second.service_father.c_str());
    LOG(LOG_INFO, "status: %d", item.second.status);
  }
}

int MonitorOptions::GetAndAddWaitingIndex() {
  int index;
  int service_father_num = service_father_to_ip.size();
  slash::MutexLock lw(&waiting_index_lock);
  index = waiting_index;
  waiting_index = (waiting_index+1) % service_father_num;
  slash::MutexLock lh(&has_thread_lock);
  while (has_thread[waiting_index]) {
    waiting_index = (waiting_index + 1) % service_father_num;
  }
  return index;
}
