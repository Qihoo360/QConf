#include <map>
#include <string>
#include <iostream>
#include <cstdio>
#include <cstring>

#include <netinet/in.h>

#include "monitor_const.h"
#include "monitor_service_item.h"

using namespace std;

ServiceItem::ServiceItem():
    _host(""),
    _port(-1),
    _connRetry(0),
    // default 3 seconds
    _connTimeout(3),
    _serviceFather(""),
    _status(STATUS_DOWN)
{
    memset(&_addr, 0, sizeof(struct in_addr));
}

ServiceItem::ServiceItem(std::string host, struct in_addr *addr, int port, int connRetry, int timeout, std::string serviceFather, int status):
    _host(host),
    _addr(*addr),
    _port(port),
    _connRetry(connRetry),
    _connTimeout(timeout),
    _serviceFather(serviceFather),
    _status(status) {}

void ServiceItem::clear() {
    memset(&_addr, 0, sizeof(struct in_addr));
    _host = "";
    _port = -1;
    _connRetry = 0;
    _serviceFather = "";
    _status = -1;
    _connTimeout = -1;
}

ServiceItem::~ServiceItem() {}

int ServiceItem::setStatus(int status) {
    _status = status;
    return 0;
}

int ServiceItem::setHost(string ip) {
    _host = ip;
    return 0;
}

int ServiceItem::setPort(int port) {
    _port = port;
    return 0;
}

int ServiceItem::setAddr(struct in_addr* addr) {
    memcpy(&_addr, addr, sizeof(struct in_addr));
    return 0;
}

int ServiceItem::setServiceFather(string serviceFather) {
    _serviceFather = serviceFather;
    return 0;
}

void ServiceItem::getAddr(struct in_addr* addr) {
    memcpy(addr, &_addr, sizeof(struct in_addr));
}

string ServiceItem::getHost() {
    return _host;
}

int ServiceItem::getPort() {
    return _port;
}

int ServiceItem::getConnectTimeout() {
    return _connTimeout;
}

int ServiceItem::getStatus() {
    return _status;
}

int ServiceItem::setConnectTimeout(int timeout) {
    _connTimeout = timeout;
    return 0;
}

const string ServiceItem::getServiceFather() {
    return _serviceFather;
}
