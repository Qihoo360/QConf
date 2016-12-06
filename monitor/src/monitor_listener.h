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

    int _addChildren(const string &serviceFather, struct String_vector &children);
    int _getAddrByHost(const char* host, struct in_addr* addr);
    int _loadService(string path, string serviceFather, string ipPort, vector<int>& );
    int _getIpNum(const string& serviceFather);
    bool _ipExist(const string& serviceFather, const string& ipPort);
    void _modifyServiceFatherToIp(const string &op, const string& path);
    bool _serviceFatherExist(const string& serviceFather);
    void _addIpPort(const string& serviceFather, const string& ipPort);
    void _deleteIpPort(const string& serviceFather, const string& ipPort);

    slash::Mutex _serviceFatherToIpLock;
    slash::Mutex _serviceFatherStatusLock;

public:
    ServiceListener();
    int initListener();
    void getAllIp();
    void loadAllService();
    void cleanServiceFatherToIp() { serviceFatherToIp.clear(); }

    unordered_map<string, unordered_set<string>> getServiceFatherToIp();

    void processDeleteEvent(const string& path);
    void processChildEvent(const string& path);
    void processChangedEvent(const string& path);
};
extern ServiceListener *p_serviceListerner;
#endif
