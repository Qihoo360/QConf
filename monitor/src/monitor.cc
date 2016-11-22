#include <thread>
#include <vector>
#include <map>
#include <string>
#include <fstream>
#include <iostream>
#include <cstdio>

#include "monitor_util.h"
#include "monitor_config.h"
#include "monitor_const.h"
#include "monitor_log.h"
#include "monitor_process.h"
#include "monitor_zk.h"
#include "monitor_load_balance.h"
#include "monitor_listener.h"
#include "monitor_multi_thread.h"

using namespace std;

// TODO (gaodunqiao@360.cn) classify error
int initMonitorEnv(Zk* _zk) {
    Config* conf = Config::getInstance();
    Process::clearStop();
    conf->clearServiceMap();
    string zkHost = conf->getZkHost();
    string zkLogPath = conf->getZkLogPath();
    int recvTimeout = conf->getZkRecvTimeout();

    // init zookeeper handler
    if (_zk->initEnv(zkHost, zkLogPath, recvTimeout) != M_OK) {
        LOG(LOG_ERROR, "Zk init env failed, host:%s, zk log path:%s", zkHost.c_str(), zkLogPath.c_str());
        return M_ERR;
    }

    //check qconf_monitor_lock_node/default_instance/md5_list
    if (_zk->createZnode2(conf->getNodeList()) != M_OK) {
        LOG(LOG_ERROR, "create znode %s failed", (conf->getNodeList()).c_str());
        return M_ERR;
    }

    //check qconf_monitor_lock_node/default_instance/monitor_list
    //if(_zk->checkAndCreateZnode(conf->getMonitorList()) == M_OK) {
    if (_zk->createZnode2(conf->getMonitorList()) != M_OK) {
        LOG(LOG_ERROR, "create znode %s failed", (conf->getMonitorList()).c_str());
        return M_ERR;
    }

    // monitor register, this function should in LoadBalance
    if (_zk->registerMonitor(conf->getMonitorList() + "/monitor_") != M_OK) {
        LOG(LOG_ERROR, "Monitor register failed");
        return M_ERR;
    }
    return M_OK;
}

// TODO classify error
int doLoadBalance(LoadBalance *lb) {
    LoadBalance::clearReBalance();
    //load balance
    if (lb->initEnv() != M_OK) {
        LOG(LOG_ERROR, "init load balance env failed");
        return M_ERR;
    }

    if (lb->getMd5ToServiceFather() != M_OK) {
        LOG(LOG_ERROR, "get md5 to service father failed");
        /*
           how to deal with this in a better way?
           if the reason of failure is node not exist, we should restart main loop
           */
        return M_ERR;
    }

    if (lb->getMonitors() != M_OK) {
        LOG(LOG_ERROR, "get monitors failed");
        return M_ERR;
    }

    if (lb->balance() != M_OK) {
        LOG(LOG_ERROR, "balance failed");
        return M_ERR;
    }
    return M_OK;
}

int loadServiceToConf(ServiceListener *sl) {
    if (sl->initEnv() != M_OK) {
        LOG(LOG_ERROR, "init service listener env failed");
        return M_ERR;
    }
    sl->getAllIp();
    sl->loadAllService();
    return M_OK;
}

int main(int argc, char** argv) {
    Config* conf = Config::getInstance();

    if (Process::isProcessRunning(MONITOR_PROCESS_NAME)) {
        LOG(LOG_ERROR, "Monitor is already running.");
        return M_ERR;
    }
    if (conf->isDaemonMode()) {
        Process::daemonize();
    }
    if (conf->isAutoRestart()) {
        int childExitStatus = -1;
        int ret = Process::processKeepalive(childExitStatus, PIDFILE);
        //parent process
        if (ret > 0) {
            return childExitStatus;
        }
        else if (ret < 0) {
            return M_ERR;
        }
        else {
            //child process write pid to PIDFILE
            if (Util::writePid(PIDFILE.c_str()) != M_OK) {
                return M_ERR;
            }
        }
    }

    while (1) {
        LOG(LOG_INFO, " main loop start -> !!!!!!");
        Zk* _zk = Zk::getInstance();
        if (initMonitorEnv(_zk) != M_OK) {
            delete _zk;
            LOG(LOG_ERROR, "init monitor env failed");
            return M_ERR;
        }
        /*
        this loop is for load balance.
        If rebalance is needed, the loop will be reiterate
        */
        while (!Process::isStop() && !MultiThread::isThreadError()) {
            LOG(LOG_INFO, " second loop start -> !!!!!!");

            LoadBalance* lb = LoadBalance::getInstance();
            if (doLoadBalance(lb) != M_OK) {
                delete lb;
                sleep(2);
                continue;
            }

            //after load balance. Each monitor should load the service to Config
            ServiceListener* sl = ServiceListener::getInstance();
            if (loadServiceToConf(sl) != M_OK) {
                delete sl;
                delete lb;
                sleep(2);
                continue;
            }

            //multiThread module
            MultiThread* ml = MultiThread::getInstance();
            ml->runMainThread();

            //It's important !! Remember to close it always
            delete lb;
            delete sl;
            delete ml;
        }
    }
    LOG(LOG_ERROR, "EXIT main loop!!!");
    return 0;
}
