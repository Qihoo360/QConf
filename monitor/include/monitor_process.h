#ifndef PROCESS_H
#define PROCESS_H

#include <string>
#include <iostream>
#include <vector>
#include <cstdio>

#include "monitor_const.h"
#include "monitor_work_thread.h"
#include "monitor_options.h"
#include "monitor_listener.h"

namespace process{

//process name
const std::string kMonitorProcessName = "qconf_monitor";

//pid file
const std::string kPidFile = "monitor_pid";

//command file
const std::string kCmdFile = "tmp/cmd";

//command to process
const std::string kCmdReload = "reload";
const std::string kCmdList = "list";
const std::string kStatusListFile = "tmp/list";
const std::string kAll = "all";
const std::string kUp = "up";
const std::string kDown = "down";
const std::string kOffline = "offline";
const int kLineLength = 100;

extern MonitorOptions *options;
extern bool need_restart;

void HandleCmd(std::vector<std::string>& cmd);
void InitEnv(WorkThread *work_thread);
bool IsProcessRunning(const std::string& process_name);
int Daemonize();
int ProcessKeepalive(int& child_exit_status, const std::string pid_file);
void SigForward(const int sig);
void SigHandler(const int sig);
void trim(std::string &line);
int ProcessFileMsg(const std::string cmd_file);
void ProcessParam(const std::string& op);
void SetStop();
void ClearStop();

}  // namespace process

#endif  // PROCESS_H
