#ifndef __QCONF_MSG_H__
#define __QCONF_MSG_H__

#include <string>

#include "qconf_common.h"

#ifndef __PERMS_FLAGS__
#define __PERMS_FLAGS__
#define PERMS 0666
#endif

typedef struct
{
    long mtype;
    char mtext[QCONF_MAX_MSG_LEN];
} qconf_msgbuf;

int init_msg_queue(key_t key, int &msqid);
int create_msg_queue(key_t key, int &msqid);
int send_msg(int msqid, const std::string &msg);
int receive_msg(int msqid, std::string &msg);

#endif
