#ifndef MULTITHREAD_H
#define MULTITHREAD_H
#include "bg_thread.h"
#include "slash_mutex.h"

#include <vector>
#include <string>

#include "monitor_config.h"
#include "monitor_const.h"
#include "monitor_log.h"
#include "monitor_process.h"
#include "monitor_load_balance.h"
#include "monitor_listener.h"

using namespace std;

class WorkThread {
private:
    //copy of myServiceFather in loadBalance
    vector<string> _serviceFathers;
    int _serviceFatherNum;

    //marked weather there is a thread checking this service father
    vector<bool> _hasThread;
    slash::Mutex _hasThreadLock;
    //the next service father waiting for check
    int _waitingIndex;
    slash::Mutex _waitingIndexLock;

    pink::BGThread *_updateThread;
public:
    WorkThread();
    ~WorkThread();
    int Start();

    int getAndAddWaitingIndex();
    void setHasThread(int index, bool val);

    pink::BGThread *getUpdateThread() { return _updateThread; }
};
#endif
