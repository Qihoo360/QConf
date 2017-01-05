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
  int ret = MONITOR_OK;

  // Init zookeeper handler
  if ((ret = monitor_zk_->InitEnv()) != MONITOR_OK) return ret;
  cb_handle.options_ = options_;
  cb_handle.monitor_zk_ = monitor_zk_;
  cb_handle.md5_to_service_father_ = &md5_to_service_father_;

  // Create md5 list node add the watcher
  std::string md5_path = options_->GetNodeList();
  if ((ret = monitor_zk_->zk_create_node(md5_path, "", 0)) != MONITOR_OK) {
    LOG(LOG_ERROR, "create znode %s failed", md5_path.c_str());
    return ret;
  }

  // Create monitor list node add the watcher
  std::string monitor_path = options_->GetMonitorList();
  if ((ret = monitor_zk_->zk_create_node(monitor_path, "", 0)) != MONITOR_OK) {
    LOG(LOG_ERROR, "create znode %s failed", monitor_path.c_str());
    return ret;
  }

  // Monitor register, this function should in LoadBalance
  if ((ret = RegisterMonitor(monitor_path + "/monitor_")) != MONITOR_OK) {
    LOG(LOG_ERROR, "Monitor register failed");
    return ret;
  }
  return MONITOR_OK;
}

// Get my_service_fathers
int LoadBalance::DoBalance() {
  options_->service_father_to_ip.clear();
  options_->service_fathers.clear();
  int ret = MONITOR_ERR_OTHER;
  if ((ret = GetMd5ToServiceFather()) != MONITOR_OK) {
    LOG(LOG_ERROR, "get md5 to service father failed");
    /* TODO (gaodq)
     * how to deal with this in a better way?
     * if the reason of failure is node not exist, we should restart main loop
     */
    return ret;
  }

  if ((ret = GetMonitors()) != MONITOR_OK) {
    LOG(LOG_ERROR, "get monitors failed");
    return ret;
  }

  // TODO gdq 不需拷贝直接遍历md5_to_servi...
  std::vector<std::string> md5_node;
  { // MutexLock
    slash::MutexLock l(&md5_to_service_father_lock_);
    for (auto it = md5_to_service_father_.begin();
         it != md5_to_service_father_.end();
         ++it) {
      md5_node.push_back(it->first);
    }
  }

  std::vector<unsigned int> sequence;
  for (auto it = monitors_.begin(); it != monitors_.end(); ++it) {
    unsigned int tmp = stoi((*it).substr((*it).size() - 10));
    sequence.push_back(tmp);
  }

  sort(sequence.begin(), sequence.end());

  std::string monitor = std::string(zk_lock_buf_);
  unsigned int my_seq = stoi(monitor.substr(monitor.size() - 10));
  size_t rank = 0;
  for (; rank < sequence.size() && sequence[rank] != my_seq; ++rank) ;
  if (rank == sequence.size()) {
    LOG(LOG_INFO, "I'm connect to zk. But the monitor registed is removed. Restart main loop");
    options_->need_rebalance = true;
    return MONITOR_ERR_OTHER;
  }

  slash::MutexLock l(&md5_to_service_father_lock_);
  std::set<string> dummy_set;
  for (size_t i = rank; i < md5_node.size(); i += monitors_.size()) {
    std::string my_service_father = md5_to_service_father_[md5_node[i]];
    options_->service_father_to_ip[my_service_father] = dummy_set;
    options_->service_fathers.push_back(my_service_father);
    LOG(LOG_INFO, "my service father:%s", my_service_father.c_str());
  }

  options_->need_rebalance = false;
  return MONITOR_OK;
}

int LoadBalance::RegisterMonitor(const std::string &path) {
  int ret = MONITOR_ERR_OTHER;
  std::string &host_name = options_->monitor_host_name;
  for (int i = 0; i < MONITOR_GET_RETRIES; ++i) {
    memset(zk_lock_buf_, 0, sizeof(zk_lock_buf_));
    //register the monitor.
    ret = monitor_zk_->zk_create_node(path, host_name, ZOO_EPHEMERAL | ZOO_SEQUENCE,
                         zk_lock_buf_, sizeof(zk_lock_buf_));
    if (ret == MONITOR_OK) {
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
  int ret = MONITOR_OK;

  // Get all md5 node
  if ((ret = monitor_zk_->zk_get_chdnodes(node_list_path, md5_node)) != MONITOR_OK) {
    LOG(LOG_ERROR, "get md5 list failes. path:%s", node_list_path.c_str());
    return ret;
  }

  for (int i = 0; i < md5_node.count; ++i) {
    std::string md5_node_str = std::string(md5_node.data[i]);
    std::string md5_path = node_list_path + "/" + md5_node_str;
    //get the value of md5Node which is serviceFather
    if (monitor_zk_->zk_get_node(md5_path, service_father, 1) != MONITOR_OK) {
      LOG(LOG_ERROR, "get value of node:%s failed", md5_path.c_str());
      continue;
    }
    // TODO gdq Need lock ?
    md5_to_service_father_[md5_node_str] = service_father;
    LOG(LOG_INFO, "md5: %s, service_father: %s", md5_path.c_str(), service_father.c_str());
  }

  deallocate_String_vector(&md5_node);
  return ret;
}

int LoadBalance::GetMonitors() {
  monitors_.clear();
  std::string monitor_list_path = options_->GetMonitorList();
  struct String_vector monitor_nodes = {0};
  int ret = MONITOR_OK;

  if ((ret = monitor_zk_->zk_get_chdnodes(monitor_list_path, monitor_nodes)) != MONITOR_OK) {
    LOG(LOG_ERROR, "get monitors failes. path:%s", monitor_list_path.c_str());
    return ret;
  }

  for (int i = 0; i < monitor_nodes.count; ++i) {
    monitors_.insert(std::string(monitor_nodes.data[i]));
  }
  LOG(LOG_INFO, "There are %d monitors, I am %s", monitors_.size(), zk_lock_buf_);

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
