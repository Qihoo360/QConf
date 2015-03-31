#ifndef QCONF_ERRNO_H
#define QCONF_ERRNO_H

#define QCONF_ERR_OTHER                     -1
#define QCONF_OK                            0
#define QCONF_ERR_PARAM                     1
#define QCONF_ERR_MEM                       2 
#define QCONF_ERR_TBL_SET                   3
#define QCONF_ERR_GET_HOST                  4
#define QCONF_ERR_GET_IDC                   5
#define QCONF_ERR_BUF_NOT_ENOUGH            6
#define QCONF_ERR_DATA_TYPE                 7
#define QCONF_ERR_DATA_FORMAT               8
#define QCONF_ERR_NULL_VALUE                9
#define QCONF_ERR_NOT_FOUND                 10  //key is not found in hash table
#define QCONF_ERR_OPEN_DUMP                 11
#define QCONF_ERR_OPEN_TMP_DUMP             12
#define QCONF_ERR_NOT_IN_DUMP               13
#define QCONF_ERR_RENAME_DUMP               14
#define QCONF_ERR_WRITE_DUMP                15
#define QCONF_ERR_SAME_VALUE                16
#define QCONF_ERR_LEN_NON_POSITIVE          17 
#define QCONF_ERR_TBL_DATA_MESS             18

// error of number
#define QCONF_ERR_OUT_OF_RANGE              20
#define QCONF_ERR_NOT_NUMBER                21
#define QCONF_ERR_OTHRE_CHARACTER           22

// error of ip and port
#define QCONF_ERR_INVALID_IP                30
#define QCONF_ERR_INVALID_PORT              31

// error of msgsnd and msgrcv
#define QCONF_ERR_NO_MESSAGE                40
#define QCONF_ERR_E2BIG                     41
#define QCONF_ERR_MSGGET                    42
#define QCONF_ERR_MSGSND                    43
#define QCONF_ERR_MSGRCV                    44
#define QCONF_ERR_MSGIDRM                   45

// error of hostname
#define QCONF_ERR_HOSTNAME                  71

// error, not init while using qconf c library
#define QCONF_ERR_CC_NOT_INIT               81


// error, failed to operate message queue
#define QCONF_ERR_SEND_MSG_FAILED           91

//  operation if there is no value in share memory
#define QCONF_WAIT                          0
#define QCONF_NOWAIT                        1
#define QCONF_MAX_GET_TIMES                 100

#endif
