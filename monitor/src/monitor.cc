#include <thread>
#include <vector>
#include <map>
#include <string>
#include <fstream>
#include <iostream>
#include <cstdio>

#include "monitor_util.h"
#include "monitor_options.h"
#include "monitor_const.h"
#include "monitor_log.h"
#include "monitor_process.h"
#include "monitor_zk.h"
#include "monitor_load_balance.h"
#include "monitor_listener.h"
#include "monitor_work_thread.h"

using namespace std;

int main(int argc, char** argv) {
  int ret = MONITOR_OK;
  MonitorOptions options(CONF_PATH);
  if ((ret = options.Load()) != MONITOR_OK)
    return ret;

  WorkThread work_thread(&options);

  Process::InitEnv(&work_thread);
  if (Process::IsProcessRunning(MONITOR_PROCESS_NAME)) {
    LOG(LOG_ERROR, "Monitor is already running.");
    return MONITOR_ERR_OTHER;
  }
  if (options.IsDaemonMode())
    Process::Daemonize();

  if (options.IsAutoRestart()) {
    int child_exit_status = -1;
    int ret = Process::ProcessKeepalive(child_exit_status, PIDFILE);
    // Parent process
    if (ret > 0)
      return child_exit_status;
    else if (ret < 0)
      return MONITOR_ERR_OTHER;
    else if ((ret = Util::writePid(PIDFILE.c_str())) != MONITOR_OK)
      return ret;
  }

  work_thread.Start();

  return 0;
}
