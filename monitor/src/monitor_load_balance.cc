#include <map>
#include <algorithm>
#include <string>
#include <iostream>
#include <cstdio>
#include <cstring>

#include <signal.h>
#include <sys/types.h>
#include <errno.h>
#include <pthread.h>

#include "monitor_const.h"
#include "monitor_log.h"
#include "monitor_load_balance.h"
#include "monitor_options.h"
#include "monitor_process.h"
#include "monitor_zk.h"

LoadBalance::LoadBalance(MonitorOptions *options)
      : options_(options) {
  monitor_zk_ = new MonitorZk(options, &cb_handle);
}

LoadBalance::~LoadBalance() {
  delete monitor_zk_;
}

int LoadBalance::Init() {
  int ret = kSuccess;

  // Init zookeeper handler
  if ((ret = monitor_zk_->InitEnv()) != kSuccess) return ret;
  cb_handle.options_ = options_;
  cb_handle.monitor_zk_ = monitor_zk_;
  cb_handle.md5_to_service_father_ = &md5_to_service_father_;

  // Create md5 list node add the watcher
  std::string md5_path = options_->GetNodeList();
  if ((ret = monitor_zk_->zk_create_node(md5_path, "", 0)) != kSuccess) {
    LOG(LOG_ERROR, "create znode %s failed", md5_path.c_str());
    return ret;
  }

  // Create monitor list node add the watcher
  std::string monitor_path = options_->GetMonitorList();
  if ((ret = monitor_zk_->zk_create_node(monitor_path, "", 0)) != kSuccess) {
    LOG(LOG_ERROR, "create znode %s failed", monitor_path.c_str());
    return ret;
  }

  // Monitor register, this function should in LoadBalance
  if ((ret = RegisterMonitor(monitor_path + "/monitor_")) != kSuccess) {
    LOG(LOG_ERROR, "Monitor register failed");
    return ret;
  }
  return kSuccess;
}

// Get my_service_fathers after registed
int LoadBalance::DoBalance() {
  options_->service_father_to_ip.clear();
  options_->my_service_fathers.clear();
  int ret = kOtherError;

  // Fill std::map<std::string, std::string> md5_to_service_father_;
  if ((ret = GetMd5ToServiceFather()) != kSuccess) {
    LOG(LOG_ERROR, "get md5 to service father failed");
    return ret;
  }

  // Get registed monitors
  size_t monitors_size = 0;
  size_t rank = 0;  // this monitor rank
  if ((ret = GetMonitors(monitors_size, rank)) != kSuccess) {
    LOG(LOG_ERROR, "get monitors failed");
    return ret;
  }

  // Save for random seek
  std::vector<std::string> md5_node;
  for (auto &it : md5_to_service_father_) {
    md5_node.push_back(it.first);
  }

  std::set<string> dummy_set;
  for (size_t i = rank; i < md5_node.size(); i += monitors_size) {
    std::string my_service_father = md5_to_service_father_[md5_node[i]];
    options_->service_father_to_ip[my_service_father] = dummy_set;
    options_->my_service_fathers.push_back(my_service_father);
    LOG(LOG_INFO, "my service father:%s", my_service_father.c_str());
  }

  options_->need_rebalance = false;
  return kSuccess;
}

int LoadBalance::RegisterMonitor(const std::string &path) {
  int ret = kOtherError;
  std::string &host_name = options_->monitor_host_name;
  for (int i = 0; i < MONITOR_GET_RETRIES; ++i) {
    memset(zk_lock_buf_, 0, sizeof(zk_lock_buf_));
    //register the monitor.
    ret = monitor_zk_->zk_create_node(path, host_name, ZOO_EPHEMERAL | ZOO_SEQUENCE,
                         zk_lock_buf_, sizeof(zk_lock_buf_));
    if (ret == kSuccess) {
      LOG(LOG_INFO, "registerMonitor: %s", zk_lock_buf_);
      return ret;
    }
  }
  LOG(LOG_ERROR, "create zookeeper node failed. API return : %d. node: %s ", ret, path.c_str());

  return ret;
}

int LoadBalance::GetMd5ToServiceFather() {
  md5_to_service_father_.clear();
  std::string node_list_path = options_->GetNodeList();
  struct String_vector md5_node = {0};
  std::string service_father;
  int ret = kSuccess;

  // Get all md5 node
  if ((ret = monitor_zk_->zk_get_chdnodes(node_list_path, md5_node)) != kSuccess) {
    LOG(LOG_ERROR, "get md5 list failes. path:%s", node_list_path.c_str());
    return ret;
  }

  for (int i = 0; i < md5_node.count; ++i) {
    std::string md5_node_str = std::string(md5_node.data[i]);
    std::string md5_path = node_list_path + "/" + md5_node_str;
    //get the value of md5Node which is serviceFather
    if (monitor_zk_->zk_get_node(md5_path, service_father, 1) != kSuccess) {
      LOG(LOG_ERROR, "get value of node:%s failed", md5_path.c_str());
      continue;
    }
    md5_to_service_father_[md5_node_str] = service_father;
    LOG(LOG_INFO, "md5: %s, service_father: %s", md5_path.c_str(), service_father.c_str());
  }

  deallocate_String_vector(&md5_node);
  return ret;
}

int LoadBalance::GetMonitors(size_t &monitors_size, size_t &rank) {
  std::string monitor_list_path = options_->GetMonitorList();
  struct String_vector monitor_nodes = {0};
  int ret = kSuccess;

  if ((ret = monitor_zk_->zk_get_chdnodes(monitor_list_path, monitor_nodes)) != kSuccess) {
    LOG(LOG_ERROR, "get monitors failes. path:%s", monitor_list_path.c_str());
    return ret;
  }

  monitors_size = monitor_nodes.count;

  std::string my_monitor = std::string(zk_lock_buf_);
  unsigned int my_seq = stoi(my_monitor.substr(my_monitor.size() - 10));

  for (size_t i = 0; i < monitors_size; ++i) {
    std::string monitor(monitor_nodes.data[i]);
    unsigned int tmp = stoi(monitor.substr(monitor.size() - 10));
    if (tmp == my_seq) break;
    rank++;
  }

  if (rank == monitors_size) {
    LOG(LOG_INFO, "Connect to zk. But the monitor registed is removed. Restart main loop");
    options_->need_rebalance = true;
    return kOtherError;
  }
  LOG(LOG_INFO, "There are %d monitors, I am %s, my rank is %llu", monitors_size, zk_lock_buf_, rank);

  deallocate_String_vector(&monitor_nodes);
  return ret;
}

void LoadBalance::BalanceZkHandle::ProcessDeleteEvent(const std::string &path) {
  if (path == options_->GetMonitorList()) {
    // Monitor dir is removed. Restart main loop
    options_->need_rebalance = true;
  }
}

void LoadBalance::BalanceZkHandle::ProcessChildEvent(const std::string &path) {
  // The number of registed monitors has changed. So we need rebalance
  if (path == options_->GetMonitorList() ||
      path == options_->GetNodeList())
    options_->need_rebalance = true;
}

void LoadBalance::BalanceZkHandle::ProcessChangedEvent(const std::string &path) {
  // service_father changed, need rebalance, load data in service_father_to_ip
  if (path == options_->GetNodeList()) 
    options_->need_rebalance = true;
}
