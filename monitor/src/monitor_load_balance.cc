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
#include "monitor_util.h"
#include "monitor_log.h"
#include "monitor_load_balance.h"
#include "monitor_options.h"
#include "monitor_process.h"
#include "monitor_zk.h"

using namespace std;

LoadBalance::LoadBalance(MonitorOptions *options) :
  MonitorZk(options),
  need_rebalance_(false) {}

int LoadBalance::InitMonitor() {
  int ret = MONITOR_OK;

  // Init zookeeper handler
  if ((ret = InitEnv()) != MONITOR_OK) return ret;

  // Create md5 list node add the watcher
  string md5_path = options_->NodeList();
  if ((ret = zk_create_node(md5_path, "", 0)) != MONITOR_OK) {
    LOG(LOG_ERROR, "create znode %s failed", md5_path.c_str());
    return ret;
  }

  // Create monitor list node add the watcher
  string monitor_path = options_->MonitorList();
  if ((ret = zk_create_node(monitor_path, "", 0)) != MONITOR_OK) {
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

int LoadBalance::RegisterMonitor(const string &path) {
  int ret = MONITOR_ERR_OTHER;
  string host_name = options_->MonitorHostname();
  for (int i = 0; i < MONITOR_GET_RETRIES; ++i) {
    memset(zk_lock_buf_, 0, sizeof(zk_lock_buf_));
    //register the monitor.
    ret = zk_create_node(path, host_name, ZOO_EPHEMERAL | ZOO_SEQUENCE,
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
  string node_list_path = options_->NodeList();
  struct String_vector md5_node = {0};
  string service_father;
  int ret = MONITOR_OK;

  //get all md5 node
  if ((ret = zk_get_chdnodes(node_list_path, md5_node)) != MONITOR_OK) {
    LOG(LOG_ERROR, "get md5 list failes. path:%s", node_list_path.c_str());
    return ret;
  }

  md5_to_service_father_.clear();
  my_service_father_.clear();
  for (int i = 0; i < md5_node.count; ++i) {
    string md5_node_str = string(md5_node.data[i]);
    string md5_path = node_list_path + "/" + md5_node_str;
    //get the value of md5Node which is serviceFather
    if (zk_get_node(md5_path, service_father, 1) != MONITOR_OK) {
      LOG(LOG_ERROR, "get value of node:%s failed", md5_path.c_str());
      continue;
    }
    UpdateMd5ToServiceFather(md5_node_str, service_father);
    LOG(LOG_INFO, "md5: %s, service_father: %s", md5_path.c_str(), service_father.c_str());
  }

  deallocate_String_vector(&md5_node);
  return ret;
}

int LoadBalance::GetMonitors() {
  string monitor_list_path = options_->MonitorList();
  struct String_vector monitor_nodes = {0};
  int ret = MONITOR_OK;

  if ((ret = zk_get_chdnodes(monitor_list_path, monitor_nodes)) != MONITOR_OK) {
    LOG(LOG_ERROR, "get monitors failes. path:%s", monitor_list_path.c_str());
    return ret;
  }

  monitors_.clear();
  for (int i = 0; i < monitor_nodes.count; ++i)
    monitors_.insert(string(monitor_nodes.data[i]));
  LOG(LOG_INFO, "There are %d monitors, I am %s", monitors_.size(), zk_lock_buf_);

  deallocate_String_vector(&monitor_nodes);
  return ret;
}

int LoadBalance::DoBalance() {
  vector<string> md5_node;
  { // MutexLock
    slash::MutexLock l(&md5_to_service_father_lock_);
    for (auto it = md5_to_service_father_.begin();
         it != md5_to_service_father_.end();
         ++it) {
      md5_node.push_back(it->first);
    }
  }

  vector<unsigned int> sequence;
  for (auto it = monitors_.begin(); it != monitors_.end(); ++it) {
    unsigned int tmp = stoi((*it).substr((*it).size() - 10));
    sequence.push_back(tmp);
  }

  sort(sequence.begin(), sequence.end());

  string monitor = string(zk_lock_buf_);
  unsigned int my_seq = stoi(monitor.substr(monitor.size() - 10));
  size_t rank = 0;
  for (; rank < sequence.size() && sequence[rank] != my_seq; ++rank) ;
  if (rank == sequence.size()) {
    LOG(LOG_INFO, "I'm connect to zk. But the monitor registed is removed. Restart main loop");
    Process::SetStop();
    return MONITOR_ERR_OTHER;
  }

  slash::MutexLock l(&md5_to_service_father_lock_);
  for (size_t i = rank; i < md5_node.size(); i += monitors_.size()) {
    my_service_father_.push_back(md5_to_service_father_[md5_node[i]]);
    LOG(LOG_INFO, "my service father:%s", my_service_father_.back().c_str());
  }

  need_rebalance_ = false;
  return MONITOR_OK;
}

void LoadBalance::ProcessDeleteEvent(const string &path) {
  if (path == options_->MonitorList()) {
    // Monitor dir is removed. Restart main loop
    Process::SetStop();
  }
}

void LoadBalance::ProcessChildEvent(const string &path) {
  // The number of registed monitors has changed. So we need rebalance
  if (path == options_->MonitorList() ||
      path == options_->NodeList())
    SetReBalance();
}

void LoadBalance::ProcessChangedEvent(const string &path) {
  string service_father;
  if (zk_get_node(path, service_father, 1) == MONITOR_OK) {
    LOG(LOG_INFO, "get data success");
    string md5_path = path.substr(path.rfind('/') + 1);
    UpdateMd5ToServiceFather(md5_path, string(service_father));
  }
}
