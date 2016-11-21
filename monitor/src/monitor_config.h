#ifndef CONFIG_H
#define CONFIG_H
#include <string>
#include <unordered_set>
#include <map>
#include <vector>
#include <unordered_map>

#include <pthread.h>

#include "monitor_service_item.h"

using namespace std;

class Config {
private:
    static Config* _instance;

    pthread_mutex_t serviceMapLock;

    int _daemonMode;
    string _monitorHostname;
    int _autoRestart;
    int _logLevel;
    int _connRetryCount;
    int _scanInterval;
    std::string _instanceName;
    std::string _zkHost;
    std::string _zkLogPath;
    int _zkRecvTimeout;

    //core data. the key is the full path of ipPort and the value is serviceItem of this ipPort
    map<string, ServiceItem> _serviceMap;

    Config();
    int setValueInt(const string& key, const string& value);
    int setValueStr(const string& key, const string& value);

public:
    ~Config();
    static Config* getInstance();
    int load();
    int resetConfig();
    int isDaemonMode();
    int isAutoRestart();

    string getMonitorHostname();
    int getLogLevel();
    int getConnRetryCount();
    int getScanInterval();
    string getInstanceName();
    string getZkHost();
    string getZkLogPath();
    int getZkRecvTimeout();
    string getNodeList();
    string getMonitorList();

    map<string, ServiceItem> getServiceMap();
    int setServiceMap(string node, int val);
    void clearServiceMap();

    int addService(string ipPath, ServiceItem serviceItem);
    void deleteService(const string& ipPath);

    ServiceItem getServiceItem(const string& ipPath);
    int printMap();
};
#endif
