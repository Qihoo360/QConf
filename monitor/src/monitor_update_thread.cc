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
    string ipPath;
    string serviceFather = node.substr(0, node.rfind('/'));
    unordered_set<string> ips = (p_serviceListerner->getServiceFatherToIp())[serviceFather];
    for (auto it = ips.begin(); it != ips.end(); ++it) {
        ipPath = serviceFather + "/" + (*it);
        int status = (p_conf->serviceItem(ipPath)).status();
        if (status == STATUS_UP) ++alive;
        if (alive > 1) return false;
    }
    return ret;
}

//update service thread. comes first update first
void updateServiceFunc(void *arg) {
    LOG(LOG_INFO, "in update service");
    pair<string, int> *updateInfo = reinterpret_cast<pair<string, int> *>(arg);
    string ipPort = updateInfo->first;
    int newStatus = updateInfo->second;
    delete updateInfo;
    int oldStatus = (p_conf->serviceItem(ipPort)).status();

    //compare the new status and old status to decide weather to update status
    if (newStatus == STATUS_DOWN && oldStatus == STATUS_UP &&
        isOnlyOneUp(ipPort)) {
        LOG(LOG_FATAL_ERROR, "Maybe %s is the last server that is up. \
            But monitor CAN NOT connect to it. its Status will not change!", ipPort.c_str());
        return;
    }

    if (p_serviceListerner->zk_modify(ipPort, to_string(newStatus)) == MONITOR_OK)
        p_conf->setServiceMap(ipPort, newStatus);
}
