#ifndef __QCONF_WATCHER_H__
#define __QCONF_WATCHER_H__

#include <string>

/* zookeeper state constants */
#define EXPIRED_SESSION_STATE_DEF -112
#define AUTH_FAILED_STATE_DEF -113
#define CONNECTING_STATE_DEF 1
#define ASSOCIATING_STATE_DEF 2
#define CONNECTED_STATE_DEF 3
#define NOTCONNECTED_STATE_DEF 999

/* zookeeper event type constants */
#define CREATED_EVENT_DEF 1
#define DELETED_EVENT_DEF 2
#define CHANGED_EVENT_DEF 3
#define CHILD_EVENT_DEF 4
#define SESSION_EVENT_DEF -1
#define NOTWATCHING_EVENT_DEF -2



/**
 * Initialize current hash table
 */
int qconf_init_shm_tbl();

/**
 * Clear current hash table
 */
void qconf_clear_shm_tbl();

/**
 * Initialize the message key
 */
int qconf_init_msg_key();

/**
 * Initialize the mutex lock
 */
int qconf_init_mutex_lock();

/**
 * Destroy the mutex lock
 */
void qconf_destroy_mutex_lock();

/**
 * Close zookeeper handler and destory the watcher tables
 */
void qconf_destroy_zk();

/**
 * Initialize the register node prefix
 */
void qconf_init_rgs_node_pfx(const std::string &node_prefix);

/**
 * Initialize the zookeeper timeout
 */
void qconf_init_recv_timeout(int timeout);

/**
 * Initialize the local idc
 */
int qconf_init_local_idc(const std::string &idc);

/**
 * Initialize the feedback enable flags
 */
void qconf_init_fb_flg(bool enable_flags);

/**
 * The wachter process thread
 */
int watcher_setting_start();

#endif
