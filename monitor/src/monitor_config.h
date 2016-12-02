#ifndef CONFIG_H
#define CONFIG_H
#include "base_conf.h"
#include "slash_mutex.h"

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

    FILE *_zkLogFile;
    int _setZkLog();

    // Core data. the key is the full path of ipPort and the value is serviceItem of this ipPort
    map<string, ServiceItem> _serviceMap;
    slash::Mutex _serviceMapLock;

public:
    Config(const string &confPath);
    ~Config();
    int Load();
    int resetConfig();

    bool isDaemonMode() { return _daemonMode; }
    bool isAutoRestart() { return _autoRestart; }
    string monitorHostname() { return _monitorHostname; }
    int logLevel() { return _logLevel; }
    int connRetryCount() { return _connRetryCount; }
    int scanInterval() { return _scanInterval; }
    string instanceName() { return _instanceName; }
    string zkHost() { return _zkHost; }
    string zkLogPath() { return _zkLogPath; }
    int zkRecvTimeout() { return _zkRecvTimeout; }
    string nodeList();
    string monitorList();
    int printConfig();

    map<string, ServiceItem> serviceMap();
    int setServiceMap(string node, int val);

    int addService(string ipPath, ServiceItem serviceItem);
    void deleteService(const string& ipPath);

    ServiceItem serviceItem(const string& ipPath);
    int printServiceMap();
};
extern Config *p_conf;
#endif
