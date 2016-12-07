#include <signal.h>
#include <sys/types.h>
#include <errno.h>

#include <map>
#include <string>
#include <iostream>
#include <cstring>
#include <cstdio>

#include "monitor_const.h"
#include "monitor_options.h"
#include "monitor_process.h"
#include "monitor_log.h"
#include "monitor_zk.h"

using namespace std;

void MonitorZk::Watcher(zhandle_t* zhandle, int type, int state, const char* node, void* context) {
  MonitorZk *object = static_cast<MonitorZk *>(context);
  if (object == NULL) {
    LOG(LOG_FATAL_ERROR, "error in watcher.");
    return;
  }
  switch (type) {
    case SESSION_EVENT_DEF:
      if (state == ZOO_EXPIRED_SESSION_STATE) {
        LOG(LOG_DEBUG, "[session state: ZOO_EXPIRED_STATA: %d]", state);
        LOG(LOG_INFO, "restart the main loop!");
        kill(getpid(), SIGUSR2);
      }
      else {
        LOG(LOG_DEBUG, "[ session state: %d ]", state);
      }
      break;
    case CHILD_EVENT_DEF:
      LOG(LOG_DEBUG, "[ child event ] ...");
      object->ProcessChildEvent(string(node));
      break;
    case CREATED_EVENT_DEF:
      LOG(LOG_DEBUG, "[ created event ]...");
      break;
    case DELETED_EVENT_DEF:
      LOG(LOG_DEBUG, "[ deleted event ] ...");
      object->ProcessDeleteEvent(string(node));
      break;
    case CHANGED_EVENT_DEF:
      LOG(LOG_DEBUG, "[ changed event ] ...");
      object->ProcessChangedEvent(string(node));
      break;
    default:
      break;
  }
}

MonitorZk::MonitorZk(MonitorOptions *options):
  zk_handle_(NULL),
  zk_node_buf_(NULL),
  options_(options) {
    recv_timeout_ = options_->ZkRecvTimeout();
    zk_host_ = options_->ZkHost();
  }

int MonitorZk::InitEnv() {
  if (zk_host_.size() <= 0) return MONITOR_ERR_PARAM;

  //init zookeeper handler
  zk_handle_ = zookeeper_init(zk_host_.c_str(), Watcher, recv_timeout_, NULL, (void *)this, 0);
  if (!zk_handle_) {
    LOG(LOG_ERROR, "zookeeper_init failed. Check whether zk_host(%s) is correct or not", zk_host_.c_str());
    return MONITOR_ERR_ZOO_FAILED;
  }

  zk_node_buf_ = new char[MONITOR_MAX_VALUE_SIZE];
  if (NULL == zk_node_buf_) {
    LOG(LOG_ERROR, "Failed to get zk node buf");
    return MONITOR_ERR_MEM;
  }

  return MONITOR_OK;
}

MonitorZk::~MonitorZk(){
  if (zk_handle_) {
    LOG(LOG_DEBUG, "zookeeper close ...");
    zookeeper_close(zk_handle_);
    zk_handle_ = NULL;
  }
  delete[] zk_node_buf_;
};

int MonitorZk::zk_modify(const std::string &path, const std::string &value)
{
  int ret = 0;
  for (int i = 0; i < MONITOR_GET_RETRIES; ++i)
  {
    ret = zoo_set(zk_handle_, path.c_str(), value.c_str(), value.size(), -1);
    switch (ret)
    {
      case ZOK:
        return MONITOR_OK;
      case ZNONODE:
        return MONITOR_NODE_NOT_EXIST;
      case ZINVALIDSTATE:
      case ZMARSHALLINGERROR:
        continue;
      default:
        return MONITOR_ERR_ZOO_FAILED;
    }
  }
  LOG(LOG_ERROR, "Failed to call zk_modify after retry. err:%s. path:%s",
      zerror(ret), path.c_str());
  return MONITOR_ERR_ZOO_FAILED;
}

/**
 * Get znode from zookeeper, and set a watcher
 */
int MonitorZk::zk_get_node(const string &path, string &buf, int watcher) {
  int ret = 0;
  int buffer_len = MONITOR_MAX_VALUE_SIZE;

  for (int i = 0; i < MONITOR_GET_RETRIES; ++i) {
    ret = zoo_get(zk_handle_, path.c_str(), watcher, zk_node_buf_, &buffer_len, NULL);
    switch (ret) {
      case ZOK:
        if (-1 == buffer_len) buffer_len = 0;
        buf.assign(zk_node_buf_, buffer_len);
        return MONITOR_OK;
      case ZNONODE:
        return MONITOR_NODE_NOT_EXIST;
      case ZINVALIDSTATE:
      case ZMARSHALLINGERROR:
        continue;
      default:
        LOG(LOG_ERROR, "Failed to call zoo_get. err:%s. path:%s",
            zerror(ret), path.c_str());
        return MONITOR_ERR_ZOO_FAILED;
    }
  }

  LOG(LOG_ERROR, "Failed to call zoo_get after retry. err:%s. path:%s",
      zerror(ret), path.c_str());
  return MONITOR_ERR_ZOO_FAILED;
}

/*
 * Create znode on zookeeper
 * and add watcher
 */
int MonitorZk::zk_create_node(const string &path, const string &value, int flags) {
  int ret = zk_create_node(path, value, flags, NULL, 0);
  string watch_path = ret == MONITOR_OK ? path : path.substr(0, path.rfind('/'));
  return zk_exists(watch_path);
}

// Create znode on zookeeper
int MonitorZk::zk_create_node(const string &path, const string &value, int flags, char *path_buffer, int path_len) {
  int ret = 0;
  for (int i = 0; i < MONITOR_GET_RETRIES; ++i) {
    ret = zoo_create(zk_handle_, path.c_str(), value.c_str(), value.length(), &ZOO_OPEN_ACL_UNSAFE, flags, path_buffer, path_len);
    switch (ret) {
      case ZOK:
      case ZNODEEXISTS:
        return MONITOR_OK;
      case ZNONODE:
      case ZNOCHILDRENFOREPHEMERALS:
      case ZBADARGUMENTS:
        LOG(LOG_ERROR, "Failed to call zoo_create. err:%s. path:%s",
            zerror(ret), path.c_str());
        return MONITOR_ERR_ZOO_FAILED;
      default:
        continue;
    }
  }

  LOG(LOG_ERROR, "Failed to call zoo_create after retry. err:%s. path:%s",
      zerror(ret), path.c_str());
  return MONITOR_ERR_ZOO_FAILED;
}

/**
 * Get children nodes from zookeeper and set a watcher
 */
int MonitorZk::zk_get_chdnodes(const string &path, String_vector &nodes) {
  if (NULL == zk_handle_ || path.empty()) return MONITOR_ERR_PARAM;

  int ret;
  for (int i = 0; i < MONITOR_GET_RETRIES; ++i) {
    ret = zoo_get_children(zk_handle_, path.c_str(), 1, &nodes);
    switch(ret) {
      case ZOK:
        return MONITOR_OK;
      case ZNONODE:
        return MONITOR_NODE_NOT_EXIST;
      case ZINVALIDSTATE:
      case ZMARSHALLINGERROR:
        continue;
      default:
        LOG(LOG_ERROR, "Failed to call zoo_get_children. err:%s. path:%s",
            zerror(ret), path.c_str());
        return MONITOR_ERR_ZOO_FAILED;
    }
  }

  LOG(LOG_ERROR, "Failed to call zoo_get_children after retry. err:%s. path:%s",
      zerror(ret), path.c_str());
  return MONITOR_ERR_ZOO_FAILED;
}

int MonitorZk::zk_get_chdnodes_with_status(const string &path, String_vector &nodes, vector<char> &status) {
  if (NULL == zk_handle_ || path.empty()) return MONITOR_ERR_PARAM;
  int ret = zk_get_chdnodes(path, nodes);
  if (MONITOR_OK == ret) {
    string child_path;
    status.resize(nodes.count);
    for (int i = 0; i < nodes.count; ++i) {
      child_path = path + '/' + nodes.data[i];
      char s = 0;
      ret = zk_get_service_status(child_path, s);
      if (MONITOR_OK != ret) return MONITOR_ERR_OTHER;
      status[i] = s;
    }
  }
  return ret;
}

int MonitorZk::zk_get_service_status(const string &path, char &status) {
  if (NULL == zk_handle_ || path.empty()) return MONITOR_ERR_PARAM;

  string buf;
  if (MONITOR_OK == zk_get_node(path, buf, 1)) {
    int value = STATUS_UNKNOWN;
    value = atoi(buf.c_str());
    switch(value) {
      case STATUS_UP:
      case STATUS_DOWN:
      case STATUS_OFFLINE:
        status = static_cast<char>(value);
        break;
      default:
        LOG(LOG_FATAL_ERROR, "Invalid service status of path:%s, status:%ld!",
            path.c_str(), value);
        return MONITOR_ERR_OTHER;
    }
  } else {
    LOG(LOG_FATAL_ERROR, "Failed to get service status, path:%s", path.c_str());
    return MONITOR_ERR_OTHER;
  }
  return MONITOR_OK;
}

int MonitorZk::zk_exists(const string &path) {
  int ret = 0;

  for (int i = 0; i < MONITOR_GET_RETRIES; ++i) {
    ret = zoo_exists(zk_handle_, path.c_str(), 1, NULL);
    switch (ret) {
      case ZOK:
        return MONITOR_OK;
      case ZNONODE:
        return MONITOR_NODE_NOT_EXIST;
      case ZINVALIDSTATE:
      case ZMARSHALLINGERROR:
        continue;
      default:
        LOG(LOG_FATAL_ERROR, "Failed to call zoo_exists. err:%s. path:%s",
            zerror(ret), path.c_str());
        return MONITOR_ERR_ZOO_FAILED;
    }
  }

  LOG(LOG_ERROR, "Failed to call zoo_exists after retry. err:%s. path:%s",
      zerror(ret), path.c_str());
  return MONITOR_ERR_ZOO_FAILED;
}
