#include <unistd.h>
#include <pthread.h>

#include <fstream>
#include <string>
#include <unordered_map>
#include <unordered_set>
#include <map>
#include <set>
#include <vector>
#include <iostream>

#include "monitor_config.h"
#include "monitor_util.h"
#include "monitor_log.h"
#include "monitor_const.h"

using namespace std;

Config* Config::_instance = NULL;

Config::Config(const string &confPath) :
    slash::BaseConf(confPath) {
    pthread_mutex_init(&serviceMapLock, NULL);
    resetConfig();
}

Config::~Config() {
    delete _instance;
}

Config* Config::getInstance() {
    if (!_instance) {
        _instance = new Config(CONF_PATH);
    }
    return _instance;
}

int Config::resetConfig(){
    _autoRestart = 1;
    _daemonMode = 0;
    _logLevel = 2;
    _zkHost = "127.0.0.1:2181";
    _zkLogPath = "";
    _monitorHostname = "";
    _connRetryCount = 3;
    _scanInterval = 3;
    _serviceMap.clear();
    _zkRecvTimeout = 3000;
    return M_OK;
}

int Config::load() {
    int ret;
    ret = this->LoadConf();
    if (ret != 0)
        return M_ERR;

    Log::init(MAX_LOG_LEVEL);

    // _daemonMode
    GetConfBool(DAEMON_MODE, &_daemonMode);
    // _autoRestart
    GetConfBool(AUTO_RESTART, &_autoRestart);
    // _logLevel
    GetConfInt(LOG_LEVEL, &_logLevel);
    if (_logLevel < MIN_LOG_LEVEL) {
        _logLevel = MIN_LOG_LEVEL;
        LOG(LOG_WARNING, "log level has been setted MIN_LOG_LEVEL");
    } else if (_logLevel > MAX_LOG_LEVEL) {
        _logLevel = MAX_LOG_LEVEL;
        LOG(LOG_WARNING, "log level has been setted MAX_LOG_LEVEL");
    }
    // Find the zk host this monitor should focus on. Their idc should be the same
    // _monitorHostname
    char hostname[128] = {0};
    if (gethostname(hostname, sizeof(hostname)) != 0) {
        LOG(LOG_ERROR, "get host name failed");
        return M_ERR;
    }
    _monitorHostname.assign(hostname);

    // _zkHost
    vector<string> word = Util::split(_monitorHostname, '.');
    bool find_zk_host = false;
    for (auto iter = word.begin(); iter != word.end(); iter++) {
        if (GetConfStr(ZK_HOST + *iter, &_zkHost)) {
            find_zk_host = true;
            break;
        }
    }
    if (!find_zk_host) {
        LOG(LOG_ERROR, "get zk host name failed");
        return M_ERR;
    }

    // _logLevel
    GetConfInt(LOG_LEVEL, &_logLevel);
    // _connRetryCount
    GetConfInt(CONN_RETRY_COUNT, &_connRetryCount);
    // _scanInterval
    GetConfInt(SCAN_INTERVAL, &_scanInterval);
    // _instanceName
    GetConfStr(INSTANCE_NAME, &_instanceName);
    // _zkLogPath
    GetConfStr(ZK_LOG_PATH, &_zkLogPath);
    // _zkRecvTimeout
    GetConfInt(ZK_RECV_TIMEOUT, &_zkRecvTimeout);

    //reload the config result in the change of loglevel in Log
    Log::init(_instance->getLogLevel());
    return M_OK;
}

int Config::printConfig() {
    LOG(LOG_INFO, "daemonMode: %d", _daemonMode);
    LOG(LOG_INFO, "autoRestart: %d", _autoRestart);
    LOG(LOG_INFO, "logLevel: %d", _logLevel);
    LOG(LOG_INFO, "connRetryCount: %d", _connRetryCount);
    LOG(LOG_INFO, "scanInterval: %d", _scanInterval);
    LOG(LOG_INFO, "instanceName: %s", _instanceName.c_str());
    LOG(LOG_INFO, "zkHost: %s", _zkHost.c_str());
    LOG(LOG_INFO, "zkLogPath: %s", _zkLogPath.c_str());
    return M_OK;
}

string Config::getNodeList() {
    //should judge weather instanceName is empty
    if (_instanceName.empty()) {
        LOG(LOG_ERROR, "instance name is empty. using default_instance");
        return LOCK_ROOT_DIR + SLASH + "default_instance" + SLASH + NODE_LIST;
    }
    return LOCK_ROOT_DIR + SLASH + _instanceName + SLASH + NODE_LIST;
}

string Config::getMonitorList() {
    if (_instanceName.empty()) {
        LOG(LOG_ERROR, "instance name is empty. using default_instance");
        return LOCK_ROOT_DIR + SLASH + "default_instance" + SLASH + MONITOR_LIST;
    }
    return LOCK_ROOT_DIR + SLASH + _instanceName + SLASH + MONITOR_LIST;
}

int Config::addService(string ipPath, ServiceItem serviceItem) {
    pthread_mutex_lock(&serviceMapLock);
    _serviceMap[ipPath] = serviceItem;
    pthread_mutex_unlock(&serviceMapLock);
    return 0;
}

void Config::deleteService(const string& ipPath) {
    pthread_mutex_lock(&serviceMapLock);
    _serviceMap.erase(ipPath);
    pthread_mutex_unlock(&serviceMapLock);
}

map<string, ServiceItem> Config::getServiceMap() {
    map<string, ServiceItem> ret;
    pthread_mutex_lock(&serviceMapLock);
    ret = _serviceMap;
    pthread_mutex_unlock(&serviceMapLock);
    return ret;
}

int Config::setServiceMap(string node, int val) {
    pthread_mutex_lock(&serviceMapLock);
    _serviceMap[node].setStatus(val);
    pthread_mutex_unlock(&serviceMapLock);
    return 0;
}

//no necessity to add lock
void Config::clearServiceMap() {
    _serviceMap.clear();
}

ServiceItem Config::getServiceItem(const string& ipPath) {
    ServiceItem ret;
    pthread_mutex_lock(&serviceMapLock);
    ret = _serviceMap[ipPath];
    pthread_mutex_unlock(&serviceMapLock);
    return ret;
}

int Config::printServiceMap() {
    for (auto it = _serviceMap.begin(); it != _serviceMap.end(); ++it) {
        LOG(LOG_INFO, "path: %s", (it->first).c_str());
        LOG(LOG_INFO, "host: %s", (it->second).getHost().c_str());
        LOG(LOG_INFO, "port: %d", (it->second).getPort());
        LOG(LOG_INFO, "service father: %s", (it->second).getServiceFather().c_str());
        LOG(LOG_INFO, "status: %d", (it->second).getStatus());
    }
    return 0;
}
