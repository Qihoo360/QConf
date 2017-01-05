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
