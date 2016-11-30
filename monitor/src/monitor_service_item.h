#ifndef SERVICEMAP_H
#define SERVICEMAP_H
#include <netinet/in.h>

#include <map>
#include <cstdio>
#include <string>
#include <iostream>
#include <cstring>

#include "monitor_const.h"

using namespace std;

class ServiceItem {
private:
    //_host is IP without port
    std::string _host;
    //_addr is for net
    struct in_addr _addr;
    int _port;
    int _connRetry;
    int _connTimeout;
    std::string _serviceFather;
    //online or offline
    int _status;
public:
    ServiceItem() :
        _host(""),
        _port(-1),
        _connRetry(3),
        // default 3 seconds
        _connTimeout(3),
        _serviceFather(""),
        _status(STATUS_DOWN) {
        memset(&_addr, 0, sizeof(struct in_addr));
    }

    ServiceItem(std::string host, struct in_addr *addr, int port,
                std::string serviceFather, int status) :
        _host(host),
        _addr(*addr),
        _port(port),
        _connRetry(3),
        _connTimeout(3),
        _serviceFather(serviceFather),
        _status(status) {}

    void setStatus(int status) { _status = status; }
    int status() { return _status; }

    void setHost(string ip) { _host = ip; }
    string host() { return _host; }

    void setPort(int port) { _port = port; }
    int port() { return _port; }

    void setAddr(struct in_addr* addr) { memcpy(&_addr, addr, sizeof(struct in_addr)); }
    void addr(struct in_addr* addr) { memcpy(addr, &_addr, sizeof(struct in_addr)); }

    void setConnectTimeout(int timeout) { _connTimeout = timeout; }
    int connectTimeout() { return _connTimeout; }

    void setServiceFather(string serviceFather) { _serviceFather = serviceFather; }
    const string serviceFather() { return _serviceFather; }

    void clear() {
        memset(&_addr, 0, sizeof(struct in_addr));
        _host = "";
        _port = -1;
        _connRetry = 0;
        _serviceFather = "";
        _status = -1;
        _connTimeout = -1;
    }
};
#endif
