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
    ServiceListener* sl = ServiceListener::getInstance();
    bool ret = true;
    int alive = 0;
    size_t pos = node.rfind('/');
    string serviceFather = node.substr(0, pos);
    unordered_set<string> ips = (sl->getServiceFatherToIp())[serviceFather];
    for (auto it = ips.begin(); it != ips.end(); ++it) {
        string ipPath = serviceFather + "/" + (*it);
        int status = (Config::getInstance()->getServiceItem(ipPath)).getStatus();
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

static int updateZk(string node, int val) {
    Zk *zk = Zk::getInstance();
    string status = to_string(val);
    return zk->setZnode(node, status);
}

static int updateConf(string node, int val) {
    Config::getInstance()->setServiceMap(node, val);
    return 0;
}

//update service thread. comes first update first
void updateServiceFunc(void *arg) {
    LOG(LOG_INFO, "in update service");
    pair<string, int> *updateInfo = reinterpret_cast<pair<string, int> *>(arg);
    string key = updateInfo->first;
    int val = updateInfo->second;
    delete updateInfo;
    int oldStatus = (Config::getInstance()->getServiceItem(key)).getStatus();

    //compare the new status and old status to decide weather to update status
    if (val == STATUS_DOWN) {
        if (oldStatus == STATUS_UP) {
            if (isOnlyOneUp(key)) {
                LOG(LOG_FATAL_ERROR, "Maybe %s is the last server that is up. \
                    But monitor CAN NOT connect to it. its Status will not change!", key.c_str());
            }
            else {
                int res = updateZk(key, val);
                if (res != 0) {
                    LOG(LOG_ERROR, "update zk failed. server %s should be %d", key.c_str(), val);
                }
                else {
                    updateConf(key, val);
                }
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
            int res = updateZk(key, val);
            if (res != 0) {
                LOG(LOG_ERROR, "update zk failed. server %s should be %d", key.c_str(), val);
            }
            else {
                updateConf(key, val);
            }
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
