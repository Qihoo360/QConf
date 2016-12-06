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
    _modifyServiceFatherToIp(CLEAR, "");
    serviceFatherStatus.clear();
}

int ServiceListener::initListener() {
    return initEnv();
}

// Get all ip belong to my service father
void ServiceListener::getAllIp() {
    auto serviceFather = p_loadBalance->myServiceFather();
    for (auto it = serviceFather.begin(); it != serviceFather.end(); ++it) {
        struct String_vector children = {0};
        // Get all ipPort belong to this serviceFather
        if (zk_get_chdnodes(*it, children) != MONITOR_OK) {
            LOG(LOG_ERROR, "get IP:Port failed. serviceFather:%s", (*it).c_str());
            _addIpPort(*it, "");
        } else {
            // Add the serviceFather and ipPort to the map serviceFatherToIp
            _addChildren(*it, children);
        }
        deallocate_String_vector(&children);
    }
}

void ServiceListener::loadAllService() {
    // Here we need locks. Maybe we can remove it
    slash::MutexLock l(&_serviceFatherToIpLock);
    for (auto it1 = serviceFatherToIp.begin(); it1 != serviceFatherToIp.end(); ++it1) {
        string serviceFather = it1->first;
        unordered_set<string> ip_ports = it1->second;
        _serviceFatherToIpLock.Unlock();
        vector<int> status(4, 0);
        for (auto it2 = ip_ports.begin(); it2 != ip_ports.end(); ++it2) {
            string path = serviceFather + "/" + (*it2);
            _loadService(path, serviceFather, *it2, status);
        }
        _serviceFatherToIpLock.Lock();
    }
}

void ServiceListener::processDeleteEvent(const string& path) {
    // It must be a service node. Because I do zoo_get only in service node
    // update serviceFatherToIp
    _modifyServiceFatherToIp(DELETE, path);
}

void ServiceListener::processChildEvent(const string& path) {
    // It must be a service father node. Because I do zoo_get_children only in service father node
    struct String_vector children = {0};
    if (zk_get_chdnodes(path, children) == MONITOR_OK) {
        LOG(LOG_INFO, "get children success");
        if (children.count <= _getIpNum(path)) {
            LOG(LOG_INFO, "actually It's a delete event");
        } else {
            LOG(LOG_INFO, "add new service");
            for (int i = 0; i < children.count; ++i) {
                string ipPort = string(children.data[i]);
                _modifyServiceFatherToIp(ADD, path + "/" + ipPort);
            }
        }
    }
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
}

int ServiceListener::_addChildren(const string &serviceFather, struct String_vector &children) {
    if (children.count == 0) _addIpPort(serviceFather, "");
    for (int i = 0; i < children.count; ++i)
        _addIpPort(serviceFather, string(children.data[i]));
    return 0;
}

int ServiceListener::_getAddrByHost(const char* host, struct in_addr* addr) {
    int ret = MONITOR_ERR_OTHER;
    struct hostent *ht;
    if ((ht = gethostbyname(host)) !=  NULL) {
        *addr = *((struct in_addr *)ht->h_addr);
        ret = MONITOR_OK;
    }
    return ret;
}

// The args are repeat. But it's ok
int ServiceListener::_loadService(string path, string serviceFather, string ipPort, vector<int>& st) {
    char status = STATUS_UNKNOWN;
    int ret = MONITOR_OK;
    if ((ret = zk_get_service_status(path, status))  != MONITOR_OK) {
        LOG(LOG_ERROR, "get service status failed. service:%s", path.c_str());
        return ret;
    }
    if (status < -1 || status > 2) {
        status = -1;
    }
    ++(st[status + 1]);
    size_t pos = ipPort.find(':');
    string ip = ipPort.substr(0, pos);
    int port = atoi((ipPort.substr(pos+1)).c_str());
    struct in_addr addr;
    _getAddrByHost(ip.c_str(), &addr);
    ServiceItem serviceItem(ip, &addr, port, serviceFather, status);
    p_conf->addService(path, serviceItem);
    LOG(LOG_INFO, "load service succeed, service:%s, status:%d", path.c_str(), status);
    return MONITOR_OK;
}

unordered_map<string, unordered_set<string>> ServiceListener::getServiceFatherToIp() {
    unordered_map<string, unordered_set<string>> ret;
    slash::MutexLock l(&_serviceFatherToIpLock);
    ret = serviceFatherToIp;
    return ret;
}

int ServiceListener::_getIpNum(const string& serviceFather) {
    int ret = 0;
    slash::MutexLock l(&_serviceFatherToIpLock);
    if (serviceFatherToIp.find(serviceFather) != serviceFatherToIp.end())
        ret = serviceFatherToIp[serviceFather].size();
    return ret;
}

bool ServiceListener::_ipExist(const string& serviceFather, const string& ipPort) {
    bool ret = true;
    slash::MutexLock l(&_serviceFatherToIpLock);
    if (serviceFatherToIp[serviceFather].find(ipPort) == serviceFatherToIp[serviceFather].end())
        ret = false;
    return ret;
}

//path is the path of ipPort
void ServiceListener::_modifyServiceFatherToIp(const string &op, const string& path) {
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
        if (_ipExist(serviceFather, ipPort)) return;

        char status = STATUS_UNKNOWN;
        if (zk_get_service_status(path, status) != MONITOR_OK) return;
        struct in_addr addr;
        _getAddrByHost(ip.c_str(), &addr);
        ServiceItem item(ip, &addr, atoi(port.c_str()), serviceFather, status);

        p_conf->addService(path, item);
        _addIpPort(serviceFather, ipPort);
    }
    if (op == DELETE) {
        _deleteIpPort(serviceFather, ipPort);
        p_conf->deleteService(path);
    }
}

bool ServiceListener::_serviceFatherExist(const string& serviceFather) {
    bool ret = true;
    slash::MutexLock l(&_serviceFatherToIpLock);
    if (serviceFatherToIp.find(serviceFather) == serviceFatherToIp.end())
        ret = false;
    return ret;
}

void ServiceListener::_addIpPort(const string& serviceFather, const string& ipPort) {
    slash::MutexLock l(&_serviceFatherToIpLock);
    serviceFatherToIp[serviceFather].insert(ipPort);
}

void ServiceListener::_deleteIpPort(const string& serviceFather, const string& ipPort) {
    slash::MutexLock l(&_serviceFatherToIpLock);
    serviceFatherToIp[serviceFather].erase(ipPort);
}
