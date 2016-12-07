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

using namespace std;

struct Process{
    static WorkThread *work_thread_;
    static MonitorOptions *options_;
    static ServiceListener *service_listener_;

    static void HandleCmd(vector<string>& cmd);
    static void InitEnv(WorkThread *work_thread);
    static bool IsProcessRunning(const string& process_name);
    static int Daemonize();
    static int ProcessKeepalive(int& child_exit_status, const string pid_file);
    static void SigForward(const int sig);
    static void SigHandler(const int sig);
    static int ProcessFileMsg(const string cmd_file);
    static void ProcessParam(const string& op);
    static void SetStop();
    static void ClearStop();
};
#endif
