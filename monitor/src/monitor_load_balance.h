#ifndef LOADBALANCE_H
#define LOADBALANCE_H
#include "slash_mutex.h"

#include <unordered_set>
#include <unordered_map>
#include <string>
#include <vector>
#include <iostream>

#include <pthread.h>

#include <zookeeper.h>
#include <zk_adaptor.h>

#include "monitor_config.h"
#include "monitor_zk.h"

using namespace std;

// Implement Zk interface
class LoadBalance : public MonitorZk {
public:
    char _zkLockBuf[512] = {0};
    bool _needReBalance;
    slash::Mutex _md5ToServiceFatherLock;


    //use map but not unordered_map so it can be sorted autonatically
    //key is md5 and value is serviceFather
    unordered_map<string, string> _md5ToServiceFather;
    unordered_set<string> _monitors;
    vector<string> _myServiceFather;

    LoadBalance();
public:
    int initMonitor();
    int registerMonitor(const string &path);

    int getMd5ToServiceFather();
    void updateMd5ToServiceFather(const string& md5Path, const string& serviceFather);
    int getMonitors();
    int balance();
    const vector<string> &myServiceFather() { return _myServiceFather; }

    void processDeleteEvent(const string& path);
    void processChildEvent(const string &path);
    void processChangedEvent(const string &path);

    void setReBalance() { _needReBalance = true; }
    bool needReBalance() { return _needReBalance; }
};
extern LoadBalance *p_loadBalance;
#endif
