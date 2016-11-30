#include <map>
#include <algorithm>
#include <string>
#include <iostream>
#include <cstdio>
#include <cstring>

#include <signal.h>
#include <sys/types.h>
#include <errno.h>
#include <pthread.h>

#include "monitor_const.h"
#include "monitor_util.h"
#include "monitor_log.h"
#include "monitor_process.h"
#include "monitor_load_balance.h"
#include "monitor_config.h"
#include "monitor_zk.h"

using namespace std;

LoadBalance::LoadBalance() :
    _needReBalance(false) {
    _md5ToServiceFather.clear();
    _monitors.clear();
    _myServiceFather.clear();
}

void LoadBalance::processDeleteEvent(const string &path) {
    if (path == p_conf->nodeList()) {
        LOG(LOG_INFO, "node %s is removed", path.c_str());
        // TODO createZnode(path);
    }
    if (path == p_conf->monitorList()) {
        LOG(LOG_INFO, "monitor dir %s is removed. Restart main loop", path.c_str());
        Process::setStop();
    }
}

void LoadBalance::processChildEvent(const string &path) {
    //the number of registed monitors has changed. So we need rebalance
    LOG(LOG_INFO, "%s has changed. Rebalance...", path.c_str());
    setReBalance();
}

void LoadBalance::processChangedEvent(const string &path) {
    LOG(LOG_INFO, "the data of node %s changed.", path.c_str());
    string serviceFather;

    if (zk_get_node(path, serviceFather, 1) == MONITOR_OK) {
        LOG(LOG_INFO, "get data success");
        size_t pos = path.rfind('/');
        string md5Path = path.substr(pos + 1);
        updateMd5ToServiceFather(md5Path, string(serviceFather));
    }
}

int LoadBalance::getMd5ToServiceFather() {
    string node_list_path = p_conf->nodeList();
    struct String_vector md5Node = {0};
    string serviceFather;

    //get all md5 node
    if (zk_get_chdnodes(node_list_path, md5Node) != MONITOR_OK) {
        LOG(LOG_ERROR, "get md5 list failes. path:%s", node_list_path.c_str());
        return M_ERR;
    }

    for (int i = 0; i < md5Node.count; ++i) {
        string md5NodeStr = string(md5Node.data[i]);
        string md5Path = node_list_path + "/" + md5NodeStr;
        //get the value of md5Node which is serviceFather
        if (zk_get_node(md5Path, serviceFather, 1) != MONITOR_OK) {
            LOG(LOG_ERROR, "get value of node:%s failed", md5Path.c_str());
            continue;
        }
        updateMd5ToServiceFather(md5NodeStr, serviceFather);
        LOG(LOG_INFO, "md5: %s, serviceFather: %s", md5Path.c_str(), serviceFather.c_str());
    }

    deallocate_String_vector(&md5Node);
    return M_OK;
}

int LoadBalance::getMonitors() {
    string monitor_list_path = p_conf->monitorList();
    struct String_vector monitorNode = {0};

    if (zk_get_chdnodes(monitor_list_path, monitorNode) != MONITOR_OK) {
        LOG(LOG_ERROR, "get monitors failes. path:%s", monitor_list_path.c_str());
        return M_ERR;
    }

    for (int i = 0; i < monitorNode.count; ++i)
        _monitors.insert(string(monitorNode.data[i]));
    LOG(LOG_INFO, "There are %d monitors, I am %s", _monitors.size(), _zkLockBuf);

    deallocate_String_vector(&monitorNode);
    return M_OK;
}

int LoadBalance::balance() {
    vector<string> md5Node;
    {slash::MutexLock l(&_md5ToServiceFatherLock);
    for (auto it = _md5ToServiceFather.begin(); it != _md5ToServiceFather.end(); ++it) {
        md5Node.push_back(it->first);
    }}

    vector<unsigned int> sequence;
    for (auto it = _monitors.begin(); it != _monitors.end(); ++it) {
        unsigned int tmp = stoi((*it).substr((*it).size() - 10));
        sequence.push_back(tmp);
    }

    sort(sequence.begin(), sequence.end());

    string monitor = string(_zkLockBuf);
    unsigned int mySeq = stoi(monitor.substr(monitor.size() - 10));
    size_t rank = 0;
    for (; rank < sequence.size(); ++rank) {
        if (sequence[rank] == mySeq) {
            break;
        }
    }
    if (rank == sequence.size()) {
        LOG(LOG_INFO, "I'm connect to zk. But the monitor registed is removed. Restart main loop");
        Process::setStop();
        return M_ERR;
    }
    {slash::MutexLock l(&_md5ToServiceFatherLock);
    for (size_t i = rank; i < md5Node.size(); i += _monitors.size()) {
        _myServiceFather.push_back(_md5ToServiceFather[md5Node[i]]);
        LOG(LOG_INFO, "my service father:%s", _myServiceFather.back().c_str());
    }}

    return M_OK;
}

void LoadBalance::updateMd5ToServiceFather(const string& md5Path, const string& serviceFather) {
    if (serviceFather.size() <= 0) return;
    slash::MutexLock l(&_md5ToServiceFatherLock);
    _md5ToServiceFather[md5Path] = serviceFather;
}

int LoadBalance::initMonitor() {
    int ret = M_ERR;
    Process::clearStop();
    p_conf->clearServiceMap();

    // init zookeeper handler
    if (initEnv() != M_OK) return M_ERR;

    //check qconf_monitor_lock_node/default_instance/md5_list
    string md5_path = p_conf->nodeList();
    zk_create_node(md5_path, NULL, 0);
    // Add the watcher
    ret = zk_exists(md5_path);
    if (ret != MONITOR_OK) {
        LOG(LOG_ERROR, "create znode %s failed", md5_path.c_str());
        return M_ERR;
    }

    // Check qconf_monitor_lock_node/default_instance/monitor_list
    string monitor_path = p_conf->monitorList();
    zk_create_node(monitor_path, NULL, 0);
    // Add the watcher
    ret = zk_exists(monitor_path);
    if (ret != MONITOR_OK) {
        LOG(LOG_ERROR, "create znode %s failed", monitor_path.c_str());
        return M_ERR;
    }

    // Monitor register, this function should in LoadBalance
    if (registerMonitor(monitor_path + "/monitor_") != M_OK) {
        LOG(LOG_ERROR, "Monitor register failed");
        return M_ERR;
    }
    return M_OK;
}

int LoadBalance::registerMonitor(const string &path) {
    int ret = MONITOR_ERR_OTHER;
    string hostName = p_conf->monitorHostname();
    for (int i = 0; i < MONITOR_GET_RETRIES; ++i) {
        memset(_zkLockBuf, 0, sizeof(_zkLockBuf));
        //register the monitor.
        ret = zk_create_node(path, hostName, ZOO_EPHEMERAL | ZOO_SEQUENCE,
                             _zkLockBuf, sizeof(_zkLockBuf));
        if (ret == MONITOR_OK) return MONITOR_OK;
    }
    LOG(LOG_ERROR, "create zookeeper node failed. API return : %d. node: %s ", ret, path.c_str());

    return MONITOR_ERR_OTHER;
}
