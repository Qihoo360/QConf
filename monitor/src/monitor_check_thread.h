#ifndef MONITOR_CHECK_THREAD_H
#define MONITOR_CHECK_THREAD_H
#include "pink_thread.h"

#include <string>

#include "monitor_work_thread.h"

using namespace std;

class CheckThread : public pink::Thread {
public:
    CheckThread(int init_pos, WorkThread *workThread);
    ~CheckThread();
    bool isRunning() { return _should_exit; }

private:
    void *ThreadMain();
    int _tryConnect(const string &curServiceFather);
    void CronHandle();
    bool _isServiceExist(struct in_addr *addr, string host, int port, int timeout, int curStatus);

    int _service_pos;
    bool _should_exit;
    WorkThread *_workThread;
};

void updateServiceFunc(void *arg);

#endif
