#ifndef ZK_H
#define ZK_H
#include <zookeeper.h>
#include <zk_adaptor.h>

#include <map>
#include <cstdio>
#include <string>
#include <iostream>

#include "monitor_config.h"
#include "monitor_service_item.h"

using namespace std;

// interface
class MonitorZk {
private:
    zhandle_t* _zh;
    int _recvTimeout;
    string _zkLogPath;
    string _zkHost;
    char *_zk_node_buffer;

protected:
    MonitorZk();
    Config* _conf;

    int initEnv();
    virtual ~MonitorZk();

    int zk_get_node(const string &path, string &buf, int watcher);
    int zk_create_node(const string &path, const string &value, int flags);
    int zk_create_node(const string &path, const string &value, int flags, char *path_buffer, int path_len);
    int zk_get_chdnodes(const string &path, String_vector &nodes);
    int zk_get_chdnodes_with_status(const string &path, String_vector &nodes, vector<char> &status);
    int zk_get_service_status(const string &path, char &status);
    bool zk_exists(const string &path);

    static void watcher(zhandle_t* zhandle, int type, int state, const char* node, void* context);
    virtual void processDeleteEvent(const string &path) = 0;
    virtual void processChildEvent(const string &path) = 0;
    virtual void processChangedEvent(const string &path) = 0;
public:
    int zk_modify(const std::string &path, const std::string &value);
};
#endif
