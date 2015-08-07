#include <errno.h>
#include <unistd.h>
#include <string.h>
#include <sys/msg.h>

#include <string>

#include "qconf_log.h"
#include "qconf_msg.h"
#include "qconf_format.h"

using namespace std;


int init_msg_queue(key_t key, int &msqid)
{
    msqid = msgget(key, PERMS);
    if (-1 == msqid)
    {
        return QCONF_ERR_MSGGET;
    }

    return QCONF_OK;
}

int create_msg_queue(key_t key, int &msqid)
{
    msqid = msgget(key, PERMS | IPC_CREAT);
    if (-1 == msqid)
    {
        LOG_FATAL_ERR("Faield to create msg queue of key:%#x! errno:%d", key, errno);
        return QCONF_ERR_MSGGET;
    }

    return QCONF_OK;
}

int send_msg(int msqid, const string &msg)
{
    int ret = QCONF_OK;
    int try_send_times = 1;

    if (msg.empty()) return QCONF_ERR_PARAM;

    if (msg.size() >= QCONF_MAX_MSG_LEN)
    {
        return QCONF_ERR_E2BIG;
    }

    qconf_msgbuf msgbuf;
    msgbuf.mtype = QCONF_MSG_TYPE;
    memcpy(msgbuf.mtext, msg.data(), msg.size());

    while (true)
    {
        ret = msgsnd(msqid, (void*)&msgbuf, msg.size(), IPC_NOWAIT);
        if (0 == ret) return QCONF_OK;

        if (EAGAIN == errno)
        {
            if (try_send_times < QCONF_MAX_SEND_MSG_TIMES)
            {
                usleep(5000);
                try_send_times++;
                continue;
            }
            return QCONF_ERR_MSGFULL;
        }
        return QCONF_ERR_MSGSND;
    }

    return QCONF_OK;
}

int receive_msg(int msqid, string &msg)
{
    ssize_t len = 0;
    qconf_msgbuf msgbuf;

    len = msgrcv(msqid, (void*)&msgbuf, QCONF_MAX_MSG_LEN, QCONF_MSG_TYPE, 0);
    if (-1 == len)
    {
        if (EIDRM == errno)
        {
            LOG_FATAL_ERR("Msg queue has been removed! msqid:%d, errno:%d", msqid, errno);
            return QCONF_ERR_MSGIDRM;
        }

        LOG_ERR("Failed to get message! msqid:%d, errno:%d", msqid, errno);
        return QCONF_ERR_MSGRCV;
    }

    msg.assign(msgbuf.mtext, len);
    return QCONF_OK;
}
