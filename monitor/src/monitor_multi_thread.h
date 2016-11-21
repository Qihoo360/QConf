#ifndef MULTITHREAD_H
#define MULTITHREAD_H
#include <iostream>
#include <fstream>
#include <thread>
#include <vector>
#include <map>
#include <string>
#include <list>
#include <cstdio>

#include <pthread.h>

#include "monitor_util.h"
#include "monitor_config.h"
#include "monitor_const.h"
#include "monitor_log.h"
#include "monitor_process.h"
#include "monitor_zk.h"
#include "monitor_load_balance.h"
#include "monitor_listener.h"

using namespace std;

class MultiThread {
private:
    MultiThread();
    static bool threadError;
    static MultiThread* mlInstance;
    Config* conf;
    ServiceListener* sl;
    LoadBalance* lb;
    unordered_map<string, int> updateServiceInfo;
    list<string> priority;
    //key is the thread id of check thread. value is the index this thread in thread pool
    map<pthread_t, size_t> threadPos;
    pthread_mutex_t threadPosLock;
    Zk* zk;
    int serviceFatherNum;
    //copy of myServiceFather in loadBalance
    vector<string> serviceFathers;
    pthread_mutex_t serviceFathersLock;
    //marked weather there is a thread checking this service father
    vector<bool> hasThread;
    pthread_mutex_t hasThreadLock;
    //the next service father waiting for check
    int waitingIndex;
    pthread_mutex_t waitingIndexLock;

public:
    ~MultiThread();
    static MultiThread* getInstance();

    int runMainThread();
    static void* staticUpdateService(void* args);
    static void* staticCheckService(void* args);
    void checkService();
    void updateService();
    int tryConnect(string curServiceFather);
    int isServiceExist(struct in_addr *addr, char* host, int port, int timeout, int curStatus);

    int updateConf(string node, int val);
    int updateZk(string node, int val);
    bool isOnlyOneUp(string node, int val);
    bool isOnlyOneUp(string node);

    static bool isThreadError();
    static void setThreadError();
    static void clearThreadError();

    void setWaitingIndex(int val);
    int getAndAddWaitingIndex();

    void insertUpdateServiceInfo(const string& service, const int val);

    void clearHasThread(int sz);
    void setHasThread(int index, bool val);
    bool getHasThread(int index);
};
#endif
