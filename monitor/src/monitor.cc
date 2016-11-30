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
#include "monitor_work_thread.h"

using namespace std;

Config *p_conf = NULL;
LoadBalance *p_loadBalance = NULL;
ServiceListener *p_serviceListerner = NULL;

int doLoadBalance() {
    //load balance
    if (p_loadBalance->initMonitor() != M_OK) {
        LOG(LOG_ERROR, "init load balance env failed");
        return M_ERR;
    }

    if (p_loadBalance->getMd5ToServiceFather() != M_OK) {
        LOG(LOG_ERROR, "get md5 to service father failed");
        /* TODO
         * how to deal with this in a better way?
         * if the reason of failure is node not exist, we should restart main loop
        */
        return M_ERR;
    }

    if (p_loadBalance->getMonitors() != M_OK) {
        LOG(LOG_ERROR, "get monitors failed");
        return M_ERR;
    }

    if (p_loadBalance->balance() != M_OK) {
        LOG(LOG_ERROR, "balance failed");
        return M_ERR;
    }
    return M_OK;
}

int loadServiceToConf() {
    if (p_serviceListerner->initListener() != M_OK) {
        LOG(LOG_ERROR, "init service listener env failed");
        return M_ERR;
    }
    p_serviceListerner->getAllIp();
    p_serviceListerner->loadAllService();
    return M_OK;
}

int main(int argc, char** argv) {
    p_conf = new Config(CONF_PATH);
    if (p_conf->Load() != M_OK) return M_ERR;
    p_conf->DumpConf();
    p_conf->printConfig();

    if (Process::isProcessRunning(MONITOR_PROCESS_NAME)) {
        LOG(LOG_ERROR, "Monitor is already running.");
        return M_ERR;
    }
    if (p_conf->isDaemonMode())
        Process::daemonize();

    if (p_conf->isAutoRestart()) {
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

    /*
     * this loop is for load balance.
     * If rebalance is needed, the loop will be reiterate
    */
    while (!Process::isStop()) {
        LOG(LOG_INFO, " main loop start -> !!!!!!");
        p_loadBalance = new LoadBalance();
        if (!p_loadBalance || doLoadBalance() != M_OK) {
            delete p_loadBalance;
            continue;
        }

        // After load balance. Each monitor should load the service to Config
        p_serviceListerner = new ServiceListener();
        if (!p_serviceListerner || loadServiceToConf() != M_OK) {
            delete p_serviceListerner;
            delete p_loadBalance;
            continue;
        }

        //multiThread module
        WorkThread workThread;
        workThread.Start();
    }

    LOG(LOG_ERROR, "EXIT main loop!!!");
    delete p_conf;
    return 0;
}
