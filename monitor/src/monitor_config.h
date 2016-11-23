#ifndef CONFIG_H
#define CONFIG_H
#include "base_conf.h"

#include <string>
#include <unordered_set>
#include <map>
#include <vector>
#include <unordered_map>

#include <pthread.h>

#include "monitor_service_item.h"

using namespace std;

class Config : public slash::BaseConf {
private:
    static Config* _instance;

    pthread_mutex_t serviceMapLock;

    bool _daemonMode;
    bool _autoRestart;
    string _monitorHostname;
    int _logLevel;
    int _connRetryCount;
    int _scanInterval;
    string _instanceName;
    string _zkHost;
    string _zkLogPath;
    int _zkRecvTimeout;

    //core data. the key is the full path of ipPort and the value is serviceItem of this ipPort
    map<string, ServiceItem> _serviceMap;

    Config(const string &confPath);
public:
    ~Config();
    static Config* getInstance();
    int load();
    int resetConfig();

    bool isDaemonMode() { return _daemonMode; }
    bool isAutoRestart() { return _autoRestart; }
    string getMonitorHostname() { return _monitorHostname; }
    int getLogLevel() { return _logLevel; }
    int getConnRetryCount() { return _connRetryCount; }
    int getScanInterval() { return _scanInterval; }
    string getInstanceName() { return _instanceName; }
    string getZkHost() { return _zkHost; }
    string getZkLogPath() { return _zkLogPath; }
    int getZkRecvTimeout() { return _zkRecvTimeout; }
    string getNodeList();
    string getMonitorList();
    int printConfig();

    map<string, ServiceItem> getServiceMap();
    int setServiceMap(string node, int val);
    void clearServiceMap();

    int addService(string ipPath, ServiceItem serviceItem);
    void deleteService(const string& ipPath);

    ServiceItem getServiceItem(const string& ipPath);
    int printServiceMap();
};
#endif
