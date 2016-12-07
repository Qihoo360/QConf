#ifndef ZK_H
#define ZK_H
#include <zookeeper.h>
#include <zk_adaptor.h>

#include <map>
#include <cstdio>
#include <string>
#include <iostream>

#include "monitor_options.h"
#include "monitor_service_item.h"

using namespace std;

// interface
class MonitorZk {
 private:
  zhandle_t* zk_handle_;
  int recv_timeout_;
  string zk_host_;
  char *zk_node_buf_;

 protected:
  MonitorZk(MonitorOptions *options);
  MonitorOptions* options_;

  int InitEnv();
  virtual ~MonitorZk();

  int zk_get_node(const string &path, string &buf, int watcher);
  int zk_create_node(const string &path, const string &value, int flags);
  int zk_create_node(const string &path, const string &value, int flags, char *path_buffer, int path_len);
  int zk_get_chdnodes(const string &path, String_vector &nodes);
  int zk_get_chdnodes_with_status(const string &path, String_vector &nodes, vector<char> &status);
  int zk_get_service_status(const string &path, char &status);
  int zk_exists(const string &path);

  static void Watcher(zhandle_t* zhandle, int type, int state, const char* node, void* context);
  virtual void ProcessDeleteEvent(const string &path) = 0;
  virtual void ProcessChildEvent(const string &path) = 0;
  virtual void ProcessChangedEvent(const string &path) = 0;

 public:
  int zk_modify(const std::string &path, const std::string &value);
};
#endif
