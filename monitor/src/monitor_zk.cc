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
  ZkCallBackHandle *cb_handle = static_cast<ZkCallBackHandle *>(context);
  if (cb_handle == NULL) {
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
      cb_handle->ProcessChildEvent(string(node));
      break;
    case CREATED_EVENT_DEF:
      LOG(LOG_DEBUG, "[ created event ]...");
      break;
    case DELETED_EVENT_DEF:
      LOG(LOG_DEBUG, "[ deleted event ] ...");
      cb_handle->ProcessDeleteEvent(string(node));
      break;
    case CHANGED_EVENT_DEF:
      LOG(LOG_DEBUG, "[ changed event ] ...");
      cb_handle->ProcessChangedEvent(string(node));
      break;
    default:
      break;
  }
}

MonitorZk::MonitorZk(MonitorOptions *options, ZkCallBackHandle *cb_handle)
  : zk_handle_(NULL),
    recv_timeout_(options->zk_recv_timeout),
    zk_host_(options->zk_host),
    zk_node_buf_(NULL),
    options_(options),
    cb_handle_(cb_handle) {
  zk_node_buf_ = new char[kMonitorMaxValueSize];
}

int MonitorZk::InitEnv() {
  if (zk_host_.size() <= 0) return kParamError;

  //init zookeeper handler
  zk_handle_ = zookeeper_init(zk_host_.c_str(), Watcher, recv_timeout_, NULL, (void *)cb_handle_, 0);
  if (!zk_handle_) {
    LOG(LOG_ERROR, "zookeeper_init failed. Check whether zk_host(%s) is correct or not", zk_host_.c_str());
    return kZkFailed;
  }

  return kSuccess;
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
  for (int i = 0; i < kMonitorGetRetries; ++i)
  {
    ret = zoo_set(zk_handle_, path.c_str(), value.c_str(), value.size(), -1);
    switch (ret)
    {
      case ZOK:
        return kSuccess;
      case ZNONODE:
        return kNotExist;
      case ZINVALIDSTATE:
      case ZMARSHALLINGERROR:
        continue;
      default:
        return kZkFailed;
    }
  }
  LOG(LOG_ERROR, "Failed to call zk_modify after retry. err:%s. path:%s",
      zerror(ret), path.c_str());
  return kZkFailed;
}

/**
 * Get znode from zookeeper, and set a watcher
 */
int MonitorZk::zk_get_node(const string &path, string &buf, int watcher) {
  int ret = 0;
  int buffer_len = kMonitorMaxValueSize;

  for (int i = 0; i < kMonitorGetRetries; ++i) {
    ret = zoo_get(zk_handle_, path.c_str(), watcher, zk_node_buf_, &buffer_len, NULL);
    switch (ret) {
      case ZOK:
        if (-1 == buffer_len) buffer_len = 0;
        buf.assign(zk_node_buf_, buffer_len);
        return kSuccess;
      case ZNONODE:
        return kNotExist;
      case ZINVALIDSTATE:
      case ZMARSHALLINGERROR:
        continue;
      default:
        LOG(LOG_ERROR, "Failed to call zoo_get. err:%s. path:%s",
            zerror(ret), path.c_str());
        return kZkFailed;
    }
  }

  LOG(LOG_ERROR, "Failed to call zoo_get after retry. err:%s. path:%s",
      zerror(ret), path.c_str());
  return kZkFailed;
}

/*
 * Create znode on zookeeper
 * and add watcher
 */
int MonitorZk::zk_create_node(const string &path, const string &value, int flags) {
  int ret = zk_create_node(path, value, flags, NULL, 0);
  string watch_path = ret == kSuccess ? path : path.substr(0, path.rfind('/'));
  return zk_exists(watch_path);
}

// Create znode on zookeeper
int MonitorZk::zk_create_node(const string &path, const string &value, int flags, char *path_buffer, int path_len) {
  int ret = 0;
  for (int i = 0; i < kMonitorGetRetries; ++i) {
    ret = zoo_create(zk_handle_, path.c_str(), value.c_str(), value.length(), &ZOO_OPEN_ACL_UNSAFE, flags, path_buffer, path_len);
    switch (ret) {
      case ZOK:
      case ZNODEEXISTS:
        return kSuccess;
      case ZNONODE:
      case ZNOCHILDRENFOREPHEMERALS:
      case ZBADARGUMENTS:
        LOG(LOG_ERROR, "Failed to call zoo_create. err:%s. path:%s",
            zerror(ret), path.c_str());
        return kZkFailed;
      default:
        continue;
    }
  }

  LOG(LOG_ERROR, "Failed to call zoo_create after retry. err:%s. path:%s",
      zerror(ret), path.c_str());
  return kZkFailed;
}

/**
 * Get children nodes from zookeeper and set a watcher
 */
int MonitorZk::zk_get_chdnodes(const string &path, String_vector &nodes) {
  if (NULL == zk_handle_ || path.empty()) return kParamError;

  int ret;
  for (int i = 0; i < kMonitorGetRetries; ++i) {
    ret = zoo_get_children(zk_handle_, path.c_str(), 1, &nodes);
    switch(ret) {
      case ZOK:
        return kSuccess;
      case ZNONODE:
        return kNotExist;
      case ZINVALIDSTATE:
      case ZMARSHALLINGERROR:
        continue;
      default:
        LOG(LOG_ERROR, "Failed to call zoo_get_children. err:%s. path:%s",
            zerror(ret), path.c_str());
        return kZkFailed;
    }
  }

  LOG(LOG_ERROR, "Failed to call zoo_get_children after retry. err:%s. path:%s",
      zerror(ret), path.c_str());
  return kZkFailed;
}

int MonitorZk::zk_get_chdnodes_with_status(const string &path, String_vector &nodes, vector<char> &status) {
  if (NULL == zk_handle_ || path.empty()) return kParamError;
  int ret = zk_get_chdnodes(path, nodes);
  if (kSuccess == ret) {
    string child_path;
    status.resize(nodes.count);
    for (int i = 0; i < nodes.count; ++i) {
      child_path = path + '/' + nodes.data[i];
      char s = 0;
      ret = zk_get_service_status(child_path, s);
      if (kSuccess != ret) return kOtherError;
      status[i] = s;
    }
  }
  return ret;
}

int MonitorZk::zk_get_service_status(const string &path, char &status) {
  if (NULL == zk_handle_ || path.empty()) return kParamError;

  string buf;
  if (kSuccess == zk_get_node(path, buf, 1)) {
    int value = kStatusUnknow;
    value = atoi(buf.c_str());
    switch(value) {
      case kStatusUp:
      case kStatusDown:
      case kStatusOffline:
        status = static_cast<char>(value);
        break;
      default:
        LOG(LOG_FATAL_ERROR, "Invalid service status of path:%s, status:%ld!",
            path.c_str(), value);
        return kOtherError;
    }
  } else {
    LOG(LOG_FATAL_ERROR, "Failed to get service status, path:%s", path.c_str());
    return kOtherError;
  }
  return kSuccess;
}

int MonitorZk::zk_exists(const string &path) {
  int ret = 0;

  for (int i = 0; i < kMonitorGetRetries; ++i) {
    ret = zoo_exists(zk_handle_, path.c_str(), 1, NULL);
    switch (ret) {
      case ZOK:
        return kSuccess;
      case ZNONODE:
        return kNotExist;
      case ZINVALIDSTATE:
      case ZMARSHALLINGERROR:
        continue;
      default:
        LOG(LOG_FATAL_ERROR, "Failed to call zoo_exists. err:%s. path:%s",
            zerror(ret), path.c_str());
        return kZkFailed;
    }
  }

  LOG(LOG_ERROR, "Failed to call zoo_exists after retry. err:%s. path:%s",
      zerror(ret), path.c_str());
  return kZkFailed;
}
