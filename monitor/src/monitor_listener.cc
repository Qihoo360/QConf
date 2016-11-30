#include <string>
#include <vector>
#include <iostream>

#include <signal.h>
#include <sys/types.h>
#include <netdb.h>

#include <zookeeper.h>
#include <zk_adaptor.h>

#include "monitor_config.h"
#include "monitor_service_item.h"
#include "monitor_listener.h"
#include "monitor_load_balance.h"
#include "monitor_log.h"
#include "monitor_const.h"
#include "monitor_util.h"

using namespace std;

ServiceListener::ServiceListener() {
    // It makes sense. Make all locks occur in one function
    modifyServiceFatherToIp(CLEAR, "");
    serviceFatherStatus.clear();
}

int ServiceListener::initListener() {
    return initEnv();
}

//path is the path of ipPort
void ServiceListener::modifyServiceFatherToIp(const string op, const string& path) {
    if (op == CLEAR) {
        slash::MutexLock l(&_serviceFatherToIpLock);
        serviceFatherToIp.clear();
    }
    size_t pos = path.rfind('/');
    string serviceFather = path.substr(0, pos);
    string ipPort = path.substr(pos + 1);
    size_t pos2 = ipPort.rfind(':');
    string ip = ipPort.substr(0, pos2);
    string port = ipPort.substr(pos2 + 1);
    if (op == ADD) {
        //If this ipPort has exist, no need to do anything
        if (ipExist(serviceFather, ipPort)) return;

        char status = STATUS_UNKNOWN;
        if (zk_get_service_status(path, status) != MONITOR_OK) return;
        struct in_addr addr;
        getAddrByHost(ip.c_str(), &addr);
        ServiceItem item(ip, &addr, atoi(port.c_str()), serviceFather, status);

        p_conf->deleteService(path);
        p_conf->addService(path, item);
        addIpPort(serviceFather, ipPort);
        modifyServiceFatherStatus(serviceFather, status, 1);
    }
    if (op == DELETE) {
        if (!serviceFatherExist(serviceFather)) {
            LOG(LOG_DEBUG, "service father: %s doesn't exist", serviceFather.c_str());
        }
        else if (!ipExist(serviceFather, ipPort)){
            LOG(LOG_DEBUG, "service father: %s doesn't have ipPort %s", serviceFather.c_str(), ipPort.c_str());
        }
        else {
            LOG(LOG_DEBUG, "delete service father %s, ip port %s", serviceFather.c_str(), ipPort.c_str());
            deleteIpPort(serviceFather, ipPort);
            int status = (p_conf->serviceItem(path)).status();
            modifyServiceFatherStatus(serviceFather, status, -1);
        }
        //uodate serviceMap
        p_conf->deleteService(path);
    }
}

void ServiceListener::processDeleteEvent(const string& path) {
    // It must be a service node. Because I do zoo_get only in service node
    // update serviceFatherToIp
    modifyServiceFatherToIp(DELETE, path);
}

void ServiceListener::processChildEvent(const string& path) {
    // It must be a service father node. Because I do zoo_get_children only in service father node
    struct String_vector children = {0};
    if (zk_get_chdnodes(path, children) == MONITOR_OK) {
        LOG(LOG_INFO, "get children success");
        if (children.count <= (int)getIpNum(path)) {
            LOG(LOG_INFO, "actually It's a delete event");
        } else {
            LOG(LOG_INFO, "add new service");
            for (int i = 0; i < children.count; ++i) {
                string ipPort = string(children.data[i]);
                modifyServiceFatherToIp(ADD, path + "/" + ipPort);
            }
        }
    }

    LOG(LOG_ERROR, "parameter error. zhandle is NULL");
    deallocate_String_vector(&children);
}

void ServiceListener::processChangedEvent(const string& path) {
    //ServiceListener* sl = ServiceListener::getInstance();
    //int oldStatus = (conf->getServiceItem(path)).getStatus();
    int newStatus = STATUS_UNKNOWN;
    string data;
    if (zk_get_node(path, data, 1) == MONITOR_OK) {
        newStatus = atoi(data.c_str());
        p_conf->setServiceMap(path, newStatus);
    }

    LOG(LOG_ERROR, "parameter error. zhandle is NULL");
}

int ServiceListener::addChildren(const string &serviceFather, struct String_vector &children) {
    if (children.count == 0) {
       addIpPort(serviceFather, "");
    }
    for (int i = 0; i < children.count; ++i) {
        string ip_port(children.data[i]);
        addIpPort(serviceFather, ip_port);
        LOG(LOG_INFO, "service father:%s, Ip:Port:%s", serviceFather.c_str(), ip_port.c_str());
    }
    return 0;
}

// Get all ip belong to my service father
int ServiceListener::getAllIp() {
    const vector<string> &serviceFather = p_loadBalance->myServiceFather();
    for (auto it = serviceFather.begin(); it != serviceFather.end(); ++it) {
        struct String_vector children = {0};
        // Get all ipPort belong to this serviceFather
        if (zk_get_chdnodes(*it, children) != M_OK) {
            LOG(LOG_ERROR, "get IP:Port failed. serviceFather:%s", (*it).c_str());
            addIpPort(*it, "");
        } else {
            // Add the serviceFather and ipPort to the map serviceFatherToIp
            addChildren(*it, children);
        }
        deallocate_String_vector(&children);
    }
    return MONITOR_OK;
}

int ServiceListener::getAddrByHost(const char* host, struct in_addr* addr) {
    int ret = M_ERR;
    struct hostent *ht;
    if ((ht = gethostbyname(host)) !=  NULL) {
        *addr = *((struct in_addr *)ht->h_addr);
        ret = M_OK;
    }
    return ret;
}

// The args are repeat. But it's ok
int ServiceListener::loadService(string path, string serviceFather, string ipPort, vector<int>& st) {
    char status = STATUS_UNKNOWN;
    int ret = zk_get_service_status(path, status);
    if (ret != MONITOR_OK) {
        LOG(LOG_ERROR, "get service status failed. service:%s", path.c_str());
        return M_ERR;
    }
    if (status < -1 || status > 2) {
        status = -1;
    }
    ++(st[status + 1]);
    size_t pos = ipPort.find(':');
    string ip = ipPort.substr(0, pos);
    int port = atoi((ipPort.substr(pos+1)).c_str());
    struct in_addr addr;
    getAddrByHost(ip.c_str(), &addr);
    ServiceItem serviceItem(ip, &addr, port, serviceFather, status);
    p_conf->addService(path, serviceItem);
    LOG(LOG_INFO, "load service succeed, service:%s, status:%d", path.c_str(), status);
    return M_OK;
}

int ServiceListener::loadAllService() {
    // Here we need locks. Maybe we can remove it
    slash::MutexLock l(&_serviceFatherToIpLock);
    for (auto it1 = serviceFatherToIp.begin(); it1 != serviceFatherToIp.end(); ++it1) {
        string serviceFather = it1->first;
        unordered_set<string> ip_ports = it1->second;
        _serviceFatherToIpLock.Unlock();
        vector<int> status(4, 0);
        for (auto it2 = ip_ports.begin(); it2 != ip_ports.end(); ++it2) {
            string path = serviceFather + "/" + (*it2);
            loadService(path, serviceFather, *it2, status);
        }
        modifyServiceFatherStatus(serviceFather, status);
        _serviceFatherToIpLock.Lock();
    }
    /*
    serviceFatherStatus is not used for now
    for (auto it = serviceFatherStatus.begin(); it != serviceFatherStatus.end(); ++it) {
        cout << it->first << endl;
        for (auto it1 = (it->second).begin(); it1 != (it->second).end(); ++it1) {
            cout << *it1 << " ";
        }
        cout << endl;
    }
    */
    return M_OK;
}

//pay attention to locks
void ServiceListener::modifyServiceFatherStatus(const string& serviceFather, int status, int op) {
    slash::MutexLock l(&_serviceFatherStatusLock);
    serviceFatherStatus[serviceFather][status + 1] += op;
}

int ServiceListener::getServiceFatherStatus(const string& serviceFather, int status) {
    int ret;
    {
    slash::MutexLock l(&_serviceFatherStatusLock);
    ret = serviceFatherStatus[serviceFather][status + 1];
    }
    return ret;
}

void ServiceListener::modifyServiceFatherStatus(const string& serviceFather, vector<int>& statusv) {
    slash::MutexLock l(&_serviceFatherStatusLock);
    serviceFatherStatus[serviceFather] = statusv;
}

unordered_map<string, unordered_set<string>> ServiceListener::getServiceFatherToIp() {
    unordered_map<string, unordered_set<string>> ret;
    {
    slash::MutexLock l(&_serviceFatherToIpLock);
    ret = serviceFatherToIp;
    }
    return ret;
}

size_t ServiceListener::getIpNum(const string& serviceFather) {
    size_t ret = 0;
    {
    slash::MutexLock l(&_serviceFatherToIpLock);
    if (serviceFatherToIp.find(serviceFather) != serviceFatherToIp.end()) {
        ret = serviceFatherToIp[serviceFather].size();
    }
    }
    return ret;
}

bool ServiceListener::ipExist(const string& serviceFather, const string& ipPort) {
    bool ret = true;
    {
    slash::MutexLock l(&_serviceFatherToIpLock);
    if (serviceFatherToIp[serviceFather].find(ipPort) == serviceFatherToIp[serviceFather].end()) {
        ret = false;
    }
    }
    return ret;
}

bool ServiceListener::serviceFatherExist(const string& serviceFather) {
    bool ret = true;
    {
    slash::MutexLock l(&_serviceFatherToIpLock);
    if (serviceFatherToIp.find(serviceFather) == serviceFatherToIp.end())
        ret = false;
    }
    return ret;
}

void ServiceListener::addIpPort(const string& serviceFather, const string& ipPort) {
    slash::MutexLock l(&_serviceFatherToIpLock);
    serviceFatherToIp[serviceFather].insert(ipPort);
}

void ServiceListener::deleteIpPort(const string& serviceFather, const string& ipPort) {
    slash::MutexLock l(&_serviceFatherToIpLock);
    serviceFatherToIp[serviceFather].erase(ipPort);
}
