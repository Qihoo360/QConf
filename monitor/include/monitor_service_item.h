#ifndef SERVICE_ITEM_H
#define SERVICE_ITEM_H

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
  }

  ServiceItem(std::string &_host, int _port,
              std::string &_service_father, int _status)
    : host(_host),
      port(_port),
      conn_retry(3),
      conn_timeout(3),
      service_father(_service_father),
      status(_status) {
  }

  std::string host;
  int port;
  int conn_retry;
  int conn_timeout;
  std::string service_father;
  // online or offline
  int status;
};

#endif  // SERVICE_ITEM_H
