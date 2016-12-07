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
  //host_ is IP without port
  std::string host_;
  //addr_ is for net
  struct in_addr addr_;
  int port_;
  int conn_retry_;
  int conn_timeout_;
  std::string service_father_;
  //online or offline
  int status_;
 public:
  ServiceItem() :
    host_(""),
    port_(-1),
    conn_retry_(3),
    // default 3 seconds
    conn_timeout_(3),
    service_father_(""),
    status_(STATUS_DOWN) {
      memset(&addr_, 0, sizeof(struct in_addr));
    }

  ServiceItem(std::string host, struct in_addr *addr_, int port,
              std::string service_father, int status) :
    host_(host),
    addr_(*addr_),
    port_(port),
    conn_retry_(3),
    conn_timeout_(3),
    service_father_(service_father),
    status_(status) {}

  void SetStatus(int status) { status_ = status; }
  int Status() { return status_; }

  void SetHost(string ip) { host_ = ip; }
  string Host() { return host_; }

  void SetPort(int port) { port_ = port; }
  int Port() { return port_; }

  void SetAddr(struct in_addr* addr_) { memcpy(&addr_, addr_, sizeof(struct in_addr)); }
  void Addr(struct in_addr* addr_) { memcpy(addr_, &addr_, sizeof(struct in_addr)); }

  void SetConnectTimeout(int timeout) { conn_timeout_ = timeout; }
  int ConnectTimeout() { return conn_timeout_; }

  void SetServiceFather(string service_father) { service_father_ = service_father; }
  const string GetServiceFather() { return service_father_; }

  void clear() {
    memset(&addr_, 0, sizeof(struct in_addr));
    host_ = "";
    port_ = -1;
    conn_retry_ = 0;
    service_father_ = "";
    status_ = -1;
    conn_timeout_ = -1;
  }
};
#endif
