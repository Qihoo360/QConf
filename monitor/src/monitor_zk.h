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

class Zk{
private:
    Config* conf;
    static Zk* zk;

    zhandle_t* _zh;
    int _recvTimeout;
    string _zkLogPath;
    string _zkHost;
    FILE* _zkLogFile;
    Zk();
public:
    ~Zk();
    static Zk* getInstance();
    int initEnv(const string zkHost, const string zkLogPath, const int recvTimeout);
    void destroyEnv();

    int checkAndCreateZnode(string path);
    int createZnode(string path);
    int createZnode2(string path);
    int setZnode(string node, string data);
    int registerMonitor(string path);
    bool znodeExist(const string& path);
    void zErrorHandler(const int& ret);

    static void watcher(zhandle_t* zhandle, int type, int state, const char* node, void* context);
    static void processDeleteEvent(zhandle_t* zhandle, const string& path);
};
#endif
