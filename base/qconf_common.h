#ifndef QCONF_COMMON_H
#define QCONF_COMMON_H

#include <zookeeper.h>

#ifndef __STRING_VECTOR_T_FLAGS__
#define __STRING_VECTOR_T_FLAGS__
typedef struct String_vector string_vector_t;
#endif

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
//key is not found in hash table
#define QCONF_ERR_NOT_FOUND                 10  
#define QCONF_ERR_OPEN_DUMP                 11
#define QCONF_ERR_OPEN_TMP_DUMP             12
#define QCONF_ERR_NOT_IN_DUMP               13
#define QCONF_ERR_RENAME_DUMP               14
#define QCONF_ERR_WRITE_DUMP                15
#define QCONF_ERR_SAME_VALUE                16
#define QCONF_ERR_LEN_NON_POSITIVE          17 
#define QCONF_ERR_TBL_DATA_MESS             18
#define QCONF_ERR_TBL_END                   19

// error of number
#define QCONF_ERR_OUT_OF_RANGE              20
#define QCONF_ERR_NOT_NUMBER                21
#define QCONF_ERR_OTHRE_CHARACTER           22

// error of ip and port
#define QCONF_ERR_INVALID_IP                30
#define QCONF_ERR_INVALID_PORT              31
#define QCONF_ERR_SET                       32

// error of msgsnd and msgrcv
#define QCONF_ERR_NO_MESSAGE                40
#define QCONF_ERR_E2BIG                     41
#define QCONF_ERR_MSGGET                    42
#define QCONF_ERR_MSGSND                    43
#define QCONF_ERR_MSGRCV                    44
#define QCONF_ERR_MSGIDRM                   45

// error of file
#define QCONF_ERR_OPEN                      51
#define QCONF_ERR_DEL_DUMP                  52 
#define QCONF_ERR_LOCK                      53
#define QCONF_ERR_MKDIR                     54
#define QCONF_ERR_READ                      55
#define QCONF_ERR_WRITE                     56

// error of hashmap
#define QCONF_ERR_INIT_MAP                  91

// error of share memory 
#define QCONF_ERR_SHMGET                    201
#define QCONF_ERR_SHMAT                     202
#define QCONF_ERR_SHMINIT                   203

#define QCONF_ERR_LOG_LEVEL                 211

// the buf length
#define QCONF_PATH_BUF_LEN                  2048
#define QCONF_IDC_MAX_LEN                   512
#define QCONF_HOST_MAX_LEN                  1024
#define QCONF_TBLKEY_MAX_LEN                2048
#define QCONF_MAX_BUF_LEN                   2048

// server status define
#define STATUS_UNKNOWN                      -1
#define STATUS_UP                           0
#define STATUS_OFFLINE                      1
#define STATUS_DOWN                         2

// define the md5 string length
#define QCONF_MD5_INT_LEN                   16
#define QCONF_MD5_INT_HEAD_LEN              19
#define QCONF_MD5_STR_LEN                   32
#define QCONF_MD5_STR_HEAD_LEN              35

#define QCONF_INVALID_SEM_ID                -1
#define QCONF_INVALID_KEY                   -1

// qconf share memory key
#define QCONF_DEFAULT_SHM_KEY               0x10cf21d1
#define QCONF_MAX_SLOTS_NUM                 800000 

#define QCONF_FILE_PATH_LEN                 2048

// data type
// '0' - '9' : type of users' data
// 'a' - 'z' : type of information that are only used by qconf 
// 'A' - 'Z' : default type
#define QCONF_DATA_TYPE_UNKNOWN             '0'
#define QCONF_DATA_TYPE_ZK_HOST             '1'
#define QCONF_DATA_TYPE_NODE                '2'
#define QCONF_DATA_TYPE_SERVICE             '3'
#define QCONF_DATA_TYPE_BATCH_NODE          '4'
#define QCONF_DATA_TYPE_LOCAL_IDC           'a'

// zookeeper default recv timeout(unit:millisecond)
#define QCONF_ZK_DEFAULT_RECV_TIMEOUT       3000

// qconf msg queue key
#define QCONF_DEFAULT_MSG_QUEUE_KEY         0x10cf56fe

// qconf message type
#define QCONF_MSG_TYPE                      0x10c

// max send repeat times when the queue     is full
#define QCONF_MAX_SEND_MSG_TIMES            3

// max length of one message
#define QCONF_MAX_MSG_LEN                   2048

// get the max number of messages one time
#define QCONF_MAX_NUM_MSG                   200

// max read times from the share memory
#define QCONF_MAX_READ_TIMES                100
// max read times when value is empty from the share memory
#define QCONF_MAX_EMPTY_READ_TIMES          5

// max value size of node info or children services
#define QCONF_MAX_VALUE_SIZE                1048577
#define QCONF_MAX_CHD_NODE_CNT              65535

#define QCONF_PREFIX                        "/qconf/"
#define QCONF_PREFIX_LEN                    (sizeof(QCONF_PREFIX) - 1)

#endif
