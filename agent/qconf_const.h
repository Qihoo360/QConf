#ifndef __QCONF_CONST_H__
#define __QCONF_CONST_H__

#include "qconf_common.h"

// agent version
#define QCONF_AGENT_VERSION                 "1.0.0"

#define QCONF_AGENT_NAME                    "qconf_agent"

// status of process exit
#define QCONF_EXIT_NORMAL                   51

// error: zookeeper operation failed
#define QCONF_ERR_ZOO_FAILED                61

// error: failed to execute command
#define QCONF_ERR_CMD                       71

// error: local is is null
#define QCONF_ERR_NULL_LOCAL_IDC            81

// error: failed to operate file
#define QCONF_ERR_FAILED_OPEN_FILE          101
#define QCONF_ERR_FAILED_READ_FILE          102

// error: feedback timeout
#define QCONF_ERR_FB_TIMEOUT                111

// error: script callback
#define QCONF_ERR_SCRIPT_TIMEOUT            115
#define QCONF_ERR_SCRIPT_NOT_EXIST          116
#define QCONF_ERR_SCRIPT_DIR_NOT_INIT       117


// status for getting data from zookeeper
#define QCONF_INVALID_LEN                   -1
#define QCONF_NODE_NOT_EXIST                -2

#define QCONF_SEND_BUF_MAX_LEN              4096
#define QCONF_IP_MAX_LEN                    256

#define QCONF_ZOO_LOG                       "zoo.err.log"

// zookeeper default recv timeout(unit:millisecond)
#define QCONF_ZK_DEFAULT_RECV_TIMEOUT       3000

#define QCONF_DEFAULT_REGIESTER_PREFIX      "/qconf/__qconf_register_hosts"

#define QCONF_STATUS_CHK_PREFIX             "/qconf/__status_chk"

#define	QCONF_KEY_DAEMON_MODE               "daemon_mode"
#define	QCONF_KEY_ZKRECVTIMEOUT             "zookeeper_recv_timeout"
#define QCONF_KEY_REGISTER_NODE_PREFIX 	    "register_node_prefix"
#define QCONF_KEY_ZKLOG_PATH                "zk_log"
#define QCONF_KEY_FEEDBACK_ENABLE           "feedback_enable"
#define QCONF_KEY_FEEDBACK_URL              "feedback_url"

// no need feedbcak nodes
#define QCONF_ANCHOR_NODE                   "/qconf/__qconf_anchor_node"

#define QCONF_GET_VALUE_RETRY_TIMES         3

// valid conf key name
#define QCONF_KEY_DAEMON_MODE               "daemon_mode"
#define QCONF_KEY_LOG_FMT                   "log_fmt"
#define QCONF_KEY_LOG_LEVEL                 "log_level"
#define QCONF_KEY_QCONF_DIR                 "qconf_dir"
#define QCONF_KEY_CHECK_IDC                 "idc"
#define QCONF_KEY_SHM_KEY                   "shm_key"
#define QCONF_KEY_MSG_QUEUE_KEY             "msg_queue_key"
#define QCONF_KEY_MAX_REPEAT_READ_TIMES     "max_repeat_read_times"
#define QCONF_KEY_LOCAL_IDC                 "local_idc"

/* trigger type */
#define QCONF_TRIGGER_TYPE_ADD              '0'
#define QCONF_TRIGGER_TYPE_CHANGE           '1'
#define QCONF_TRIGGER_TYPE_REMOVE           '2'
#define QCONF_TRIGGER_TYPE_RESET            '3'

/* zk constants */
#define QCONF_GET_RETRIES                   3

/* log constans */
#define QCONF_LOG_DIR                       "logs"

/* script callback constants */
#define QCONF_SCRIPT_SEPARATOR              '#'
#define QCONF_SCRIPT_SUFFIX                 ".sh"
#define QCONF_SCRIPT_TIMEOUT_SECOND         30

/* feedback constants */
#define QCONF_FB_RETRIES                    3
#define QCONF_FB_RESULT                     "0"

#endif
