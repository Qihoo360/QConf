#ifndef SERVICELISTENER_H
#define SERVICELISTENER_H
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
#include "monitor_service_item.h"
#include "monitor_load_balance.h"

using namespace std;

class ServiceListener {
//private:
public:
    ServiceListener();
    static ServiceListener* slInstance;
    zhandle_t* zh;
    /*
    core data
    key is serviceFather and value is a set of ipPort
    */
    unordered_map<string, unordered_set<string>> serviceFatherToIp;
    /*
    core data
    key is service father and value is the number of ipPort with different types.
    It's used for check weather there are only one service alive
    */
    unordered_map<string, vector<int>> serviceFatherStatus;

    Config* conf;
    LoadBalance* lb;
    //there exist some common method. I should make a base class maybe
    int initEnv();
    int destroyEnv();
    int zkGetChildren(const string path, struct String_vector* children);
    size_t getIpNum(const string& serviceFather);

    pthread_mutex_t serviceFatherToIpLock;
    pthread_mutex_t serviceFatherStatusLock;
    pthread_mutex_t watchFlagLock;

    bool watchFlag;

public:
    static ServiceListener* getInstance();
    ~ServiceListener();

    int getAllIp();

    int loadService(string path, string serviceFather, string ipPort, vector<int>& );
    int loadAllService();

    int zkGetNode(const char* path, char* data, int* dataLen);
    int addChildren(const string serviceFather, struct String_vector children);

    int getAddrByHost(const char* host, struct in_addr* addr);

    void modifyServiceFatherToIp(const string op, const string& path);
    unordered_map<string, unordered_set<string>> getServiceFatherToIp();
    bool ipExist(const string& serviceFather, const string& ipPort);
    bool serviceFatherExist(const string& serviceFather);
    void addIpPort(const string& serviceFather, const string& ipPort);
    void deleteIpPort(const string& serviceFather, const string& ipPort);

    static void watcher(zhandle_t* zhandle, int type, int state, const char* node, void* context);
    static void processDeleteEvent(zhandle_t* zhandle, const string& path);
    static void processChildEvent(zhandle_t* zhandle, const string& path);
    static void processChangedEvent(zhandle_t* zhandle, const string& path);

    int modifyServiceFatherStatus(const string& serviceFather, int status, int op);
    int modifyServiceFatherStatus(const string& serviceFather, vector<int>& statusv);
    int getServiceFatherStatus(const string& serviceFather, int status);

    void setWatchFlag();
    void clearWatchFlag();
    bool getWatchFlag();
};
#endif
