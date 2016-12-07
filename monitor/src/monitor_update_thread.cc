#include <string>
#include <utility>

#include "monitor_check_thread.h"
#include "monitor_listener.h"
#include "monitor_options.h"
#include "monitor_const.h"
#include "monitor_log.h"
#include "monitor_zk.h"

//traversal all the service under the service father to judge weather it's only one service up
static bool IsOnlyOneUp(const string &node, ServiceListener *service_listener, MonitorOptions *options) {
  bool ret = true;
  int alive = 0;
  string ip_path;
  string service_father = node.substr(0, node.rfind('/'));
  unordered_set<string> ips = (service_listener->GetServiceFatherToIp())[service_father];
  for (auto it = ips.begin(); it != ips.end(); ++it) {
    ip_path = service_father + "/" + (*it);
    int status = (options->GetServiceItem(ip_path)).Status();
    if (status == STATUS_UP) ++alive;
    if (alive > 1) return false;
  }
  return ret;
}

//update service thread. comes first update first
void UpdateServiceFunc(void *arg) {
  UpdateServiceArgs *update_service_args= static_cast<UpdateServiceArgs *>(arg);
  string ip_port = update_service_args->ip_port;
  int new_status = update_service_args->new_status;
  MonitorOptions *options = update_service_args->options;
  ServiceListener *service_listener = update_service_args->service_listener;
  delete update_service_args;

  int old_status = (options->GetServiceItem(ip_port)).Status();

  //compare the new status and old status to decide weather to update status
  if (new_status == STATUS_DOWN && old_status == STATUS_UP &&
      IsOnlyOneUp(ip_port, service_listener, options)) {
    LOG(LOG_FATAL_ERROR, "Maybe %s is the last server that is up. \
        But monitor CAN NOT connect to it. its Status will not change!", ip_port.c_str());
    return;
  }

  if (service_listener->zk_modify(ip_port, to_string(new_status)) == MONITOR_OK)
    options->SetServiceMap(ip_port, new_status);
}
