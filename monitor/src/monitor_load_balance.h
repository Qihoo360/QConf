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
private:
    char _zkLockBuf[512] = {0};
    bool _needReBalance;
    slash::Mutex _md5ToServiceFatherLock;


    //use map but not unordered_map so it can be sorted autonatically
    //key is md5 and value is serviceFather
    unordered_map<string, string> _md5ToServiceFather;
    unordered_set<string> _monitors;
    vector<string> _myServiceFather;

    void _updateMd5ToServiceFather(const string& md5Path, const string& serviceFather);
public:
    // Initial
    LoadBalance();
    int initMonitor();
    int registerMonitor(const string &path);
    int getMd5ToServiceFather();
    int getMonitors();
    int balance();

    void processDeleteEvent(const string& path);
    void processChildEvent(const string &path);
    void processChangedEvent(const string &path);

    // Setter
    void setReBalance() { _needReBalance = true; }
    // Getter
    bool needReBalance() { return _needReBalance; }
    const vector<string> &myServiceFather() { return _myServiceFather; }
};
extern LoadBalance *p_loadBalance;
#endif
