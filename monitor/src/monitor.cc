#include "env.h"
#include "slash_status.h"

#include <string>
#include <fstream>

#include "monitor_options.h"
#include "monitor_const.h"
#include "monitor_log.h"
#include "monitor_process.h"
#include "monitor_load_balance.h"
#include "monitor_listener.h"
#include "monitor_work_thread.h"

int main(int argc, char** argv) {
  int ret = kSuccess;
  MonitorOptions *options = new MonitorOptions(CONF_PATH);
  if ((ret = options->Load()) != 0)
    return ret;

  WorkThread *work_thread = new WorkThread(options);

  process::options = options;
  process::need_restart = false;
  if (process::IsProcessRunning(MONITOR_PROCESS_NAME)) {
    LOG(LOG_ERROR, "Monitor is already running.");
    return kOtherError;
  }
  if (options->daemon_mode)
    process::Daemonize();

  if (options->auto_restart) {
    int child_exit_status = -1;
    int ret = process::ProcessKeepalive(child_exit_status, PIDFILE);
    // Parent process
    if (ret > 0)
      return child_exit_status;
    else if (ret < 0)
      return kOtherError;
    else {
      std::ofstream pidstream(PIDFILE);
      pidstream << getpid() << endl;
      pidstream.close();
    }
  }

  work_thread->Start();
  delete work_thread;

  return 0;
}
