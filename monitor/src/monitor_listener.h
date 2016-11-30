#ifndef SERVICELISTENER_H
#define SERVICELISTENER_H
#include <zookeeper.h>
#include <zk_adaptor.h>
#include "slash_mutex.h"

#include <pthread.h>

#include <set>
#include <unordered_set>
#include <map>
#include <unordered_map>
#include <string>
#include <vector>
#include <iostream>

#include "monitor_config.h"
#include "monitor_service_item.h"
#include "monitor_load_balance.h"

using namespace std;

class ServiceListener : public MonitorZk {
private:
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
    //there exist some common method. I should make a base class maybe
    size_t getIpNum(const string& serviceFather);

    slash::Mutex _serviceFatherToIpLock;
    slash::Mutex _serviceFatherStatusLock;

public:
    ServiceListener();
    int initListener();
    int getAllIp();

    int loadService(string path, string serviceFather, string ipPort, vector<int>& );
    int loadAllService();

    int getAddrByHost(const char* host, struct in_addr* addr);

    void modifyServiceFatherToIp(const string op, const string& path);
    unordered_map<string, unordered_set<string>> getServiceFatherToIp();
    bool ipExist(const string& serviceFather, const string& ipPort);
    bool serviceFatherExist(const string& serviceFather);
    int addChildren(const string &serviceFather, struct String_vector &children);
    void addIpPort(const string& serviceFather, const string& ipPort);
    void deleteIpPort(const string& serviceFather, const string& ipPort);

    void processDeleteEvent(const string& path);
    void processChildEvent(const string& path);
    void processChangedEvent(const string& path);

    void modifyServiceFatherStatus(const string& serviceFather, int status, int op);
    void modifyServiceFatherStatus(const string& serviceFather, vector<int>& statusv);
    int getServiceFatherStatus(const string& serviceFather, int status);

    //这个标记一开始是用来区分zk节点的值是由monitor去改变的还是zk自己改变的
    //是为了使用serviceFatherStatus来判断是否仅剩一个up的服务节点的
    //最后发现这样还是不可行，因为网络的原因等，还是无法确认每次设置了标记位之后就清除，再设置，暂时无用
    /* pthread_mutex_t watchFlagLock;
    bool watchFlag;
    void setWatchFlag();
    void clearWatchFlag();
    bool getWatchFlag(); */
};
extern ServiceListener *p_serviceListerner;
#endif
