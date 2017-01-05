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

struct ServiceItem {
  ServiceItem()
    : host(""),
      port(-1),
      conn_retry(3),
      // default 3 seconds
      conn_timeout(3),
      service_father(""),
      status(STATUS_DOWN) {
    memset(&addr, 0, sizeof(struct in_addr));
  }

  ServiceItem(std::string _host, struct in_addr *_addr, int _port,
              std::string _service_father, int _status)
    : host(_host),
      addr(*_addr),
      port(_port),
      conn_retry(3),
      conn_timeout(3),
      service_father(_service_father),
      status(_status) {
  }

  // host is IP without port
  std::string host;
  // addr is for net
  struct in_addr addr;
  int port;
  int conn_retry;
  int conn_timeout;
  std::string service_father;
  // online or offline
  int status;
};
#endif
