#include <string>
#include <utility>

#include "monitor_check_thread.h"
#include "monitor_listener.h"
#include "monitor_config.h"
#include "monitor_const.h"
#include "monitor_log.h"
#include "monitor_zk.h"

//traversal all the service under the service father to judge weather it's only one service up
static bool isOnlyOneUp(string node) {
    bool ret = true;
    int alive = 0;
    size_t pos = node.rfind('/');
    string serviceFather = node.substr(0, pos);
    unordered_set<string> ips = (p_serviceListerner->getServiceFatherToIp())[serviceFather];
    for (auto it = ips.begin(); it != ips.end(); ++it) {
        string ipPath = serviceFather + "/" + (*it);
        int status = (p_conf->serviceItem(ipPath)).status();
        if (status == STATUS_UP) {
            ++alive;
        }
        if (alive > 1) {
            ret = false;
            break;
        }
    }
    if (alive > 1) {
        ret = false;
    }
    return ret;
}

//update service thread. comes first update first
void updateServiceFunc(void *arg) {
    LOG(LOG_INFO, "in update service");
    pair<string, int> *updateInfo = reinterpret_cast<pair<string, int> *>(arg);
    string key = updateInfo->first;
    int val = updateInfo->second;
    delete updateInfo;
    int oldStatus = (p_conf->serviceItem(key)).status();

    //compare the new status and old status to decide weather to update status
    if (val == STATUS_DOWN) {
        if (oldStatus == STATUS_UP) {
            if (isOnlyOneUp(key)) {
                LOG(LOG_FATAL_ERROR, "Maybe %s is the last server that is up. \
                    But monitor CAN NOT connect to it. its Status will not change!", key.c_str());
            }
            else {
                if (p_serviceListerner->zk_modify(key, to_string(val)) != MONITOR_OK)
                    LOG(LOG_ERROR, "update zk failed. server %s should be %d", key.c_str(), val);
                else
                    p_conf->setServiceMap(key, val);
            }
        }
        else if (oldStatus == STATUS_DOWN) {
            LOG(LOG_INFO, "service %s keeps down", key.c_str());
        }
        else if (oldStatus == STATUS_OFFLINE) {
            LOG(LOG_INFO, "service %s is off line and it can't be connected", key.c_str());
        }
        else {
            LOG(LOG_WARNING, "status: %d should not exist!", oldStatus);
        }
    }
    else if (val == STATUS_UP) {
        if (oldStatus == STATUS_DOWN) {
            if (p_serviceListerner->zk_modify(key, to_string(val)) != MONITOR_OK)
                LOG(LOG_ERROR, "update zk failed. server %s should be %d", key.c_str(), val);
            else
                p_conf->setServiceMap(key, val);
        }
        else if (oldStatus == STATUS_UP) {
            LOG(LOG_INFO, "service %s keeps up", key.c_str());
        }
        else if (oldStatus == STATUS_OFFLINE) {
            LOG(LOG_INFO, "service %s is off line and it can be connected", key.c_str());
        }
        else {
            LOG(LOG_WARNING, "status: %d should not exist!", oldStatus);
        }
    }
    else {
        LOG(LOG_INFO, "should not come here");
    }
    usleep(1000);
}
