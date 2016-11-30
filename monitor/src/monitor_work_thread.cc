#include "pink_define.h"

#include <iostream>

#include "monitor_check_thread.h"
#include "monitor_work_thread.h"

WorkThread::WorkThread() :
    _is_running(false),
    _waitingIndex(MAX_THREAD_NUM),
    _updateThread(NULL) {
    _serviceFathers = p_loadBalance->myServiceFather();
    _serviceFatherNum = _serviceFathers.size();
    _hasThread.resize(_serviceFatherNum, false);
}

WorkThread::~WorkThread() {
}

int WorkThread::Start() {
    CheckThread *_checkThreads[MAX_THREAD_NUM];

    _updateThread = new pink::BGThread;
    if (_updateThread->StartThread() != pink::kSuccess) {
        this->Exit();
        LOG(LOG_ERROR, "create the update service thread error");
    }

    int oldThreadNum = 0;
    int newThreadNum = 0;
    //If the number of service father < MAX_THREAD_NUM, one service father one thread
    while (!Process::isStop() && !p_loadBalance->needReBalance() && !isRunning()) {
        auto serviceFatherToIp = p_serviceListerner->getServiceFatherToIp();

        newThreadNum = min(static_cast<int>(serviceFatherToIp.size()), MAX_THREAD_NUM);
        for (; oldThreadNum < newThreadNum; ++oldThreadNum) {
            _checkThreads[oldThreadNum] = new CheckThread(oldThreadNum, this);
            _checkThreads[oldThreadNum]->StartThread();
        }
        sleep(2);
    }

    for (int i = 0; i < oldThreadNum; ++i) {
        delete _checkThreads[i];
        LOG(LOG_INFO, "exit check thread: %d", i);
    }
    Exit();
    return M_ERR;
}

int WorkThread::getAndAddWaitingIndex() {
    int index;
    slash::MutexLock l(&_waitingIndexLock);
    index = _waitingIndex;
    _waitingIndex = (_waitingIndex+1) % _serviceFatherNum;
    {
    slash::MutexLock l(&_hasThreadLock);
    while (_hasThread[_waitingIndex]) {
        _waitingIndex = (_waitingIndex+1) % _serviceFatherNum;
    }
    }
    return index;
}

void WorkThread::setHasThread(int index, bool val) {
    slash::MutexLock l(&_hasThreadLock);
    _hasThread[index] = val;
}
