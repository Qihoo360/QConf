#ifndef SERVICEMAP_H
#define SERVICEMAP_H
#include <map>
#include <cstdio>
#include <string>
#include <iostream>

#include <netinet/in.h>

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
    ServiceItem(std::string host, struct in_addr *addr, int port, int connRetry, int timeout, std::string serviceFather, int status);
    ServiceItem();
    ~ServiceItem();

    int setStatus(int status);
    int getStatus();

    int setHost(string ip);
    string getHost();

    int setPort(int port);
    int getPort();

    int setAddr(struct in_addr* addr);
    void getAddr(struct in_addr* addr);

    int setConnectTimeout(int timeout);
    int getConnectTimeout();

    int setServiceFather(string serviceFather);
    const string getServiceFather();

    void clear();
};
#endif
