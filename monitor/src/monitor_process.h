#ifndef PROCESS_H
#define PROCESS_H
#include <string>
#include <iostream>
#include <vector>
#include <cstdio>

#include "monitor_const.h"

using namespace std;

class Process{
private:
    Process();
    ~Process();
    //if _zk disconnect with server. _stop will be true and main loop will be reiterate
    static bool stop;
    static void handleCmd(vector<string>& cmd);

public:
    static bool isProcessRunning(const string& processName);
    static int daemonize();
    static int processKeepalive(int& childExitStatus, const string pidFile);
    static void sigForward(const int sig);
    static void sigHandler(const int sig);
    static int processFileMsg(const string cmdFile);
    static void processParam(const string& op);
    static bool isStop();
    static void setStop();
    static void clearStop();
};
#endif
