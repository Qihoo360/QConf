#include <signal.h>
#include <sys/types.h>
#include <errno.h>

#include <map>
#include <string>
#include <iostream>
#include <cstring>
#include <cstdio>

#include "monitor_const.h"
#include "monitor_config.h"
#include "monitor_util.h"
#include "monitor_process.h"
#include "monitor_log.h"
#include "monitor_zk.h"

using namespace std;

char _zkLockBuf[512] = {0};

Zk* Zk::zk = NULL;

Zk* Zk::getInstance() {
    if (!zk) {
        zk = new Zk();
    }
    return zk;
}

void Zk::processDeleteEvent(zhandle_t* zhandle, const string& path) {
    Zk* zk = Zk::getInstance();
    Config* conf = Config::getInstance();
    if (path == conf->getNodeList()) {
        LOG(LOG_INFO, "node %s is removed", path.c_str());
        zk->createZnode(path);
    }
    if (path == conf->getMonitorList()) {
        LOG(LOG_INFO, "monitor dir %s is removed. Restart main loop", path.c_str());
        Process::setStop();
    }
}

/*
The watcher in Zk will deal with events as follow:
1. zk disconnect with zookeeper server -> restart main loop
2. md5List is removed -> create it
3. monitorList is removed -> restart main loop
*/
void Zk::watcher(zhandle_t* zhandle, int type, int state, const char* node, void* context){
    dp();
    switch (type) {
        case SESSION_EVENT_DEF:
            if (state == ZOO_EXPIRED_SESSION_STATE) {
                LOG(LOG_DEBUG, "[session state: ZOO_EXPIRED_STATA: %d]", state);
                LOG(LOG_INFO, "restart the main loop!");
                kill(getpid(), SIGUSR2);
            }
            else {
                LOG(LOG_DEBUG, "[ session state: %d ]", state);
            }
            break;
        case CHILD_EVENT_DEF:
            LOG(LOG_DEBUG, "[ child event ] ...");
            break;
        case CREATED_EVENT_DEF:
            LOG(LOG_DEBUG, "[ created event ]...");
            break;
        case DELETED_EVENT_DEF:
            LOG(LOG_DEBUG, "[ deleted event ] ...");
            processDeleteEvent(zhandle, string(node));
            break;
        case CHANGED_EVENT_DEF:
            LOG(LOG_DEBUG, "[ changed event ] ...");
            break;
        default:
            break;
    }
}

Zk::Zk():_zh(NULL), _recvTimeout(3000), _zkLogPath(""), _zkHost(""), _zkLogFile(NULL) {
    conf = Config::getInstance();
}

/*
 * Initialize Zk api environment.
 *
 * zk_host       [in] zookeeper server host
 * log_path      [in] zookeerp api log path
 * recv_timeout  [in] receive timeout
 *
 */
int Zk::initEnv(const string zkHost, const string zkLogPath, const int recvTimeout) {
    if (zkLogPath.size() <= 0) {
        return M_ERR;
    }
    _zkLogFile = fopen(zkLogPath.c_str(), "a");
    if (!_zkLogFile) {
        LOG(LOG_ERROR, "log file open failed. path:%s. error:%s", zkLogPath.c_str(), strerror(errno));
        return M_ERR;
    }
    _zkLogPath = zkLogPath;
    //set the log file stream of zookeeper
    zoo_set_log_stream(_zkLogFile);
    LOG(LOG_INFO, "zoo_set_log_stream path:%s", zkLogPath.c_str());

    if (zkHost.size() <= 0) {
        LOG(LOG_ERROR, "zkHost %s wrong!", zkHost.c_str());
        fclose(_zkLogFile);
        return M_ERR;
    }
    _zkHost = zkHost;
    _recvTimeout = recvTimeout > 1000 ? recvTimeout : _recvTimeout;
    //init zookeeper handler
    _zh = zookeeper_init(zkHost.c_str(), watcher, _recvTimeout, NULL, NULL, 0);
    if (!_zh) {
        LOG(LOG_ERROR, "zookeeper_init failed. Check whether zk_host(%s) is correct or not", zkHost.c_str());
        fclose(_zkLogFile);
        return M_ERR;
    }
    return M_OK;
}

void Zk::destroyEnv() {
    if (_zh) {
        LOG(LOG_DEBUG, "zookeeper close ...");
        zookeeper_close(_zh);
        _zh = NULL;
    }
    if (_zkLogFile) {
        // set the zookeeper log stream to be default stderr
        zoo_set_log_stream(NULL);
        LOG(LOG_DEBUG, "zkLog close ...");
        fclose(_zkLogFile);
        _zkLogFile = NULL;
    }
    zk = NULL;
}

Zk::~Zk(){
    destroyEnv();
};

void Zk::zErrorHandler(const int& ret) {
    if (ret == ZSESSIONEXPIRED ||  /*!< The session has been expired by the server */
        ret == ZSESSIONMOVED ||    /*!< session moved to another server, so operation is ignored */
        ret == ZOPERATIONTIMEOUT ||/*!< Operation timeout */
        ret == ZINVALIDSTATE)     /*!< Invliad zhandle state */
    {
        LOG(LOG_ERROR, "API return: %s. Reinitialize zookeeper handle.", zerror(ret));
    }
    else if (ret == ZCLOSING ||    /*!< ZooKeeper is closing */
            ret == ZCONNECTIONLOSS)  /*!< Connection to the server has been lost */
    {
        LOG(LOG_FATAL_ERROR, "connect to zookeeper Failed!. API return : %s. Try to initialize zookeeper handle", zerror(ret));
    }
}

bool Zk::znodeExist(const string& path) {
    if (!_zh) {
        LOG(LOG_ERROR, "_zh is NULL!");
        return false;
    }
    struct Stat stat;
    int ret = zoo_exists(_zh, path.c_str(), 1, &stat);
    if (ret == ZOK) {
        LOG(LOG_INFO, "node exist. node: %s", path.c_str());
        return true;
    }
    else if (ret == ZNONODE) {
        LOG(LOG_INFO, "node not exist. node: %s", path.c_str());
        return false;
    }
    else {
        LOG(LOG_ERROR, "zoo_exist failed. error: %s. node: %s", zerror(ret), path.c_str());
        zErrorHandler(ret);
        return false;
    }
}

//after create znode, set the watcher with zoo_exists
int Zk::createZnode(string path) {
    vector<string> nodeList = Util::split(path, '/');
    string node("");
    //root should sure be exist
    for (auto it = nodeList.begin(); it != nodeList.end(); ++it) {
        node += "/";
        node += (*it);
        int ret = zoo_create(_zh, node.c_str(), NULL, 0, &ZOO_OPEN_ACL_UNSAFE, 0, NULL, 0);
        if (ret == ZOK) {
            LOG(LOG_INFO, "Create node succeeded. node: %s", node.c_str());
        }
        else if (ret == ZNODEEXISTS) {
            LOG(LOG_INFO, "Create node .Node exists. node: %s", node.c_str());
        }
        else {
            LOG(LOG_ERROR, "create node failed. error: %s. node: %s ", zerror(ret), node.c_str());
            zErrorHandler(ret);
            return M_ERR;
        }
    }
    //add the watcher
    struct Stat stat;
    zoo_exists(_zh, node.c_str(), 1, &stat);
    return M_OK;
}

int Zk::createZnode2(string path) {
    if (path.size() <= 0) {
        LOG(LOG_ERROR, "parameter error, path is empty");
        return M_ERR;
    }
    if (path[0] != '/') {
        path = '/' + path;
    }
    if (path.back() == '/') {
        path.pop_back();
    }
    string node = path;
    while (1) {
        int ret = zoo_create(_zh, node.c_str(), NULL, 0, &ZOO_OPEN_ACL_UNSAFE, 0, NULL, 0);
        if (ret == ZOK) {
            LOG(LOG_INFO, "Create node succeeded. node: %s", path.c_str());
            if (node == path) {
                break;
            }
            else {
                node = path;
            }
        }
        else if (ret == ZNODEEXISTS) {
            LOG(LOG_INFO, "Create node .Node exists. node: %s", path.c_str());
            if (node == path) {
                break;
            }
            else {
                node = path;
            }
        }
        else if (ret == ZNONODE) {
            size_t pos = node.rfind('/');
            node = node.substr(0, pos);
        }
        else {
            LOG(LOG_ERROR, "create node failed. error: %s. node: %s ", zerror(ret), node.c_str());
            zErrorHandler(ret);
            return M_ERR;
        }
    }
    //add the watcher
    struct Stat stat;
    zoo_exists(_zh, node.c_str(), 1, &stat);
    return M_OK;
}

int Zk::checkAndCreateZnode(string path) {
    if (path.size() <= 0) {
        return M_ERR;
    }
    if (path[0] != '/') {
        path = '/' + path;
    }
    if (path.back() == '/') {
        path.pop_back();
    }
    // check weather the node exist
    if (znodeExist(path)) {
        return M_OK;
    }
    else {
        if (createZnode(path) == M_OK) {
            LOG(LOG_INFO, "create znode succeeded, path:%s", path.c_str());
            return M_OK;
        }
        else {
            LOG(LOG_ERROR, "create znode failed, path:%s", path.c_str());
            return M_ERR;
        }
    }
}

int Zk::registerMonitor(string path) {
    int ret = ZOK;
    while (_zh) {
        memset(_zkLockBuf, 0, sizeof(_zkLockBuf));
        string hostName = conf->getMonitorHostname();
        //register the monitor.
        if (hostName.empty()) {
            ret = zoo_create(_zh, path.c_str(), NULL, 0, &ZOO_OPEN_ACL_UNSAFE,
                ZOO_EPHEMERAL | ZOO_SEQUENCE, _zkLockBuf, sizeof(_zkLockBuf));
        }
        //if set the value of monitor node. It may be used in rebalance
        else {
            ret = zoo_create(_zh, path.c_str(), hostName.c_str(), hostName.length(), &ZOO_OPEN_ACL_UNSAFE,
                ZOO_EPHEMERAL | ZOO_SEQUENCE, _zkLockBuf, sizeof(_zkLockBuf));
        }

        if (ret == ZOK) {
            LOG(LOG_INFO, "Create zookeeper node succeeded. node: %s", _zkLockBuf);
        }
        else if (ret == ZNODEEXISTS) {
            LOG(LOG_INFO, "Create zookeeper node .Node exists. node: %s", _zkLockBuf);
        }
        else {
            LOG(LOG_ERROR, "create zookeeper node failed. API return : %d. node: %s ", ret, path.c_str());
            zErrorHandler(ret);
            // wait a second
            sleep(1);
            continue;
        }
        break;
    }
    if (!_zh) {
        LOG(LOG_TRACE, "zkLock...out...error return...");
        return  M_ERR;
    }
    return M_OK;
}

int Zk::setZnode(string node, string data) {
    int ver = -1; //will not check the version of node
    if (!_zh) {
        LOG(LOG_FATAL_ERROR, "_zh is NULL. restart the main loop!");
        Process::setStop();
        return -1;
    }
    int status = zoo_set(_zh, node.c_str(), data.c_str(), data.length(), ver);
    if (status == ZOK) {
        return 0;
    }
    else {
        LOG(LOG_ERROR, "%s failed when zoo_set node(%s), data(%d), error:%s", \
            __FUNCTION__, node.c_str(), data.c_str(), zerror(status));
        return -1;
    }
}
