#ifndef LOADBALANCE_H
#define LOADBALANCE_H
#include <set>
#include <unordered_set>
#include <map>
#include <unordered_map>
#include <string>
#include <vector>
#include <iostream>

#include <pthread.h>

#include <zookeeper.h>
#include <zk_adaptor.h>

#include "monitor_config.h"

using namespace std;

class LoadBalance {
public:
    zhandle_t* zh;
    static bool reBalance;
    static LoadBalance* lbInstance;
    Config* conf;

    pthread_mutex_t md5ToServiceFatherLock;

    //use map but not unordered_map so it can be sorted autonatically
    //key is md5 and value is serviceFather
    map<string, string> md5ToServiceFather;
    unordered_set<string> monitors;
    vector<string> myServiceFather;

    LoadBalance();
    int destroyEnv();

public:
    ~LoadBalance();
    int initEnv();
    static LoadBalance* getInstance();

    int zkGetChildren(const string path, struct String_vector* children);
    int zkGetNode(const char* md5Path, char* serviceFather, int* dataLen);

    int getMd5ToServiceFather();
    void updateMd5ToServiceFather(const string& md5Path, const string& serviceFather);
    int getMonitors();
    int balance();
    vector<string> getMyServiceFather();

    static void watcher(zhandle_t* zhandle, int type, int state, const char* path, void* context);
    static void processChildEvent(zhandle_t* zhandle, const string path);
    static void processChangedEvent(zhandle_t* zhandle, const string path);

    static void setReBalance();
    static void clearReBalance();
    static bool getReBalance();
};
#endif
