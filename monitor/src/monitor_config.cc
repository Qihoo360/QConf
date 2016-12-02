#include "zookeeper.h"

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

Config::Config(const string &confPath) :
    slash::BaseConf(confPath) {
    resetConfig();
    _serviceMap.clear();
}

Config::~Config() {
    if (_zkLogFile) {
        // set the zookeeper log stream to be default stderr
        zoo_set_log_stream(NULL);
        LOG(LOG_DEBUG, "zkLog close ...");
        fclose(_zkLogFile);
        _zkLogFile = NULL;
    }
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
    return MONITOR_OK;
}

int Config::Load() {
    if (this->LoadConf() != 0);
        return MONITOR_ERR_OTHER;

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
        return MONITOR_ERR_MEM;
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
        return MONITOR_ERR_OTHER;
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

    // Reload the config result in the change of loglevel in Log
    Log::init(this->logLevel());

    // Set zookeeper log path
    int ret;
    if ((ret = _setZkLog()) != MONITOR_OK) {
        LOG(LOG_ERROR, "set zk log path failed");
        return ret;
    }
    return MONITOR_OK;
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
    return MONITOR_OK;
}

string Config::nodeList() {
    //should judge weather instanceName is empty
    if (_instanceName.empty()) {
        LOG(LOG_ERROR, "instance name is empty. using default_instance");
        return LOCK_ROOT_DIR + SLASH + "default_instance" + SLASH + NODE_LIST;
    }
    return LOCK_ROOT_DIR + SLASH + _instanceName + SLASH + NODE_LIST;
}

string Config::monitorList() {
    if (_instanceName.empty()) {
        LOG(LOG_ERROR, "instance name is empty. using default_instance");
        return LOCK_ROOT_DIR + SLASH + "default_instance" + SLASH + MONITOR_LIST;
    }
    return LOCK_ROOT_DIR + SLASH + _instanceName + SLASH + MONITOR_LIST;
}

int Config::addService(string ipPath, ServiceItem serviceItem) {
    slash::MutexLock l(&_serviceMapLock);
    _serviceMap[ipPath] = serviceItem;
    return 0;
}

void Config::deleteService(const string& ipPath) {
    slash::MutexLock l(&_serviceMapLock);
    _serviceMap.erase(ipPath);
}

map<string, ServiceItem> Config::serviceMap() {
    map<string, ServiceItem> ret;
    slash::MutexLock l(&_serviceMapLock);
    ret = _serviceMap;
    return ret;
}

int Config::setServiceMap(string node, int val) {
    slash::MutexLock l(&_serviceMapLock);
    _serviceMap[node].setStatus(val);
    return 0;
}

ServiceItem Config::serviceItem(const string& ipPath) {
    ServiceItem ret;
    slash::MutexLock l(&_serviceMapLock);
    ret = _serviceMap[ipPath];
    return ret;
}

int Config::printServiceMap() {
    for (auto it = _serviceMap.begin(); it != _serviceMap.end(); ++it) {
        LOG(LOG_INFO, "path: %s", (it->first).c_str());
        LOG(LOG_INFO, "host: %s", (it->second).host().c_str());
        LOG(LOG_INFO, "port: %d", (it->second).port());
        LOG(LOG_INFO, "service father: %s", (it->second).serviceFather().c_str());
        LOG(LOG_INFO, "status: %d", (it->second).status());
    }
    return 0;
}

int Config::_setZkLog() {
    if (_zkLogPath.size() <= 0) {
        return MONITOR_ERR_ZOO_FAILED;
    }
    _zkLogFile = fopen(_zkLogPath.c_str(), "a+");
    if (!_zkLogFile) {
        LOG(LOG_ERROR, "log file open failed. path:%s. error:%s", _zkLogPath.c_str(), strerror(errno));
        return MONITOR_ERR_FAILED_OPEN_FILE;
    }
    //set the log file stream of zookeeper
    zoo_set_log_stream(_zkLogFile);
    zoo_set_debug_level(ZOO_LOG_LEVEL_WARN);
    LOG(LOG_INFO, "zoo_set_log_stream path:%s", _zkLogPath.c_str());

    return MONITOR_OK;
}
