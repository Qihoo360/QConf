#ifndef MONITOR_ZK_H
#define MONITOR_ZK_H

#include <zookeeper.h>
#include <zk_adaptor.h>

#include <map>
#include <cstdio>
#include <string>
#include <iostream>

#include "monitor_options.h"
#include "monitor_service_item.h"

const int kMonitorGetRetries = 3;
const int kMonitorMaxValueSize = 1048577;

class MonitorZk {
 public:
  // Interface for callback handle
  class ZkCallBackHandle {
   public:
    virtual void ProcessDeleteEvent(const std::string &path) = 0;
    virtual void ProcessChildEvent(const std::string &path) = 0;
    virtual void ProcessChangedEvent(const std::string &path) = 0;
  };

  MonitorZk(MonitorOptions *options, ZkCallBackHandle *cb_handle);

  int InitEnv();
  virtual ~MonitorZk();

  int zk_get_node(const std::string &path, std::string &buf, int watcher);
  int zk_create_node(const std::string &path, const std::string &value, int flags);
  int zk_create_node(const std::string &path, const std::string &value, int flags,
                     char *path_buffer, int path_len);
  int zk_get_chdnodes(const std::string &path, String_vector &nodes);
  int zk_get_chdnodes_with_status(const std::string &path, String_vector &nodes, vector<char> &status);
  int zk_get_service_status(const std::string &path, char &status);
  int zk_exists(const std::string &path);
  int zk_modify(const std::string &path, const std::string &value);

  static void Watcher(zhandle_t* zhandle, int type, int state, const char* node, void* context);

 private:
  zhandle_t* zk_handle_;
  int recv_timeout_;
  std::string zk_host_;
  char *zk_node_buf_;
  MonitorOptions* options_;
  ZkCallBackHandle* const cb_handle_;
};

#endif  // MONITOR_ZK_H
