#include <string.h>
#include <unistd.h>
#include <pthread.h>

#include <map>
#include <set>
#include <deque>
#include <vector>

#include "qconf_zoo.h"
#include "qconf_log.h"
#include "qconf_msg.h"
#include "qconf_shm.h"
#include "qconf_dump.h"
#include "qconf_const.h"
#include "qconf_script.h"
#include "qconf_format.h"
#include "qconf_config.h"
#include "qconf_watcher.h"
#include "qconf_feedback.h"
#include "qconf_feedback.h"
#include "qconf_gray.h"
#include "qconf_lock.h"

using namespace std;

/**
 * Global variable
 */
static qhasharr_t *_shm_tbl = NULL; //share memory table
static int _msg_queue_id = -1;  // message queue id for sending or receiving message
static string _register_node_path;
static int _recv_timeout = 3000; //zookeeper timeout
static int _scexec_timeout = 3000; //script execute timeout
static bool _stop_watcher_setting = false;  //stop flag
static bool _fb_enable = false;             //whether enable feedback
static bool _finish_process_tbl_sleep_setting = true; //process tbl thread finish sleep
static string _local_idc; //local idc

// key: zkhost => value: pointer to zhandle_t
static Mutex _ht_ih_mutex;
static std::map<string, zhandle_t*> _ht_idchost_handle;

// Key : zhandle_t address str =>  value: zkhost
static Mutex _ht_hi_mutex;
static std::map<unsigned long, string> _ht_handle_idchost;

// Nodes need to be get from zk and set into share memory
static Mutex _watch_nodes_mutex;
static CondVar _watch_nodes_cond(&_watch_nodes_mutex);
static deque<string> _need_watch_nodes;
static set<string> _exist_watch_nodes;

// Idcs need to be do gray process
static Mutex _gray_idcs_mutex;
static CondVar _gray_idcs_cond(&_gray_idcs_mutex);
static deque<string> _need_gray_idcs;
static set<string> _exist_gray_idcs;

// Nodes need to trigger dump, script or feedback
static Mutex _change_trigger_mutex;
static CondVar _change_trigger_cond(&_change_trigger_mutex);
static std::deque<std::string> _need_trigger_nodes;
static std::map<std::string, fb_val> _exist_trigger_nodes;

/**
 * Thread function
 */
static void *msg_process(void *p);
static void *assist_watcher_process(void *p);
static void *change_trigger_process(void *p);
static void *do_gray_process(void *p);
static void deque_process();

/**
 * Traverse share memory and update its item
 */
static int process_tbl();
static void msleep_interval(int num);

/**
 * Zookeeper watcher function
 */
static bool diff_with_zk(const string &tblkey, const string &tblval);
static int set_watcher_and_update_tbl(const string &tblkey);
static void global_watcher(zhandle_t *zh, int type, int state, const char *path, void *context);
static int watcher_reconnect_to_zookeeper(zhandle_t *zh);
static void process_deleted_event(const string &idc, const string &path);
static void process_created_event(const string &idc, const string &path);
static void process_changed_event(const string &idc, const string &path);
static void process_child_event(const string &idc, const string &path);
static void init_env_for_zk(zhandle_t *zh, const string &idc_host, const string &idc);

/**
 * Send node which need to update or remove to the thread who do that
 */
static void add_watcher_node(const string &key);
static int process_node(zhandle_t *zh, const string &tblkey, const string &path);
static int process_service(zhandle_t *zh, const string &tblkey, const string &path);
static int process_batch(zhandle_t *zh, const string &tblkey, const string &path);

static zhandle_t *get_zhandle_by_idc(const string &idc);
static int get_idc_by_zhandle(const zhandle_t *zh, string &idc, string &host);

/**
 * Send node which need to feedback, script execute or dump to thread who do that
 a trigger_type : QCONF_TRIGGER_TYPE_ADD_OR_MODIFY, QCONF_TRIGGER_TYPE_REMOVE or QCONF_TRIGGER_TYPE_RESET
 * fb_chds: string contains child nodes together with its status
 */
static int add_change_trigger_node(const string &tblkey, const string &tblval, char trigger_type, const string &fb_chds="");
static void trigger_dump(const string &tblkey, const string &tblval, char trigger_type);
static void trigger_script(char data_type, const string &idc, const string &path, char trigger_type);
#ifdef QCONF_CURL_ENABLE
static void trigger_feedback(char data_type, const string &idc, const string &path, const string &tblkey, const fb_val &mval, char trigger_type);
#endif

/**
 * Gray related function
 */
static void add_gray_idc(const string &key);

template<typename K, typename V>
static int lock_ht_find(const map<K, V> &ht, Mutex &mu, const K &key, V &val)
{
    int ret = QCONF_ERR_OTHER;
    typename map<K, V>::const_iterator it; 
    mu.Lock();
    it = ht.find(key);
    if (it != ht.end())
    {
        val = it->second;
        ret = QCONF_OK;
    }
    mu.Unlock();
    return ret;
}

template<typename K, typename V>
static void lock_ht_update(map<K, V> &ht, Mutex &mu, const K &key, const V &val)
{
    mu.Lock();
    ht[key] = val;
    mu.Unlock();
}

template<typename K, typename V>
static void lock_ht_delete(map<K, V> &ht, Mutex &mu, const K &key)
{
    mu.Lock();
    ht.erase(key);
    mu.Unlock();
}

template<typename K, typename V>
static void lock_ht_keys(const map<K, V> &ht, Mutex &mu, vector<K> &idcs)
{
    typename map<K, V>::const_iterator it;
    idcs.clear();

    mu.Lock();
    for (it = ht.begin(); it != ht.end(); ++it)
    {
        idcs.push_back(it->first);
    }
    mu.Unlock();
}

int qconf_init_shm_tbl()
{
    int ret = create_hash_tbl(_shm_tbl, QCONF_DEFAULT_SHM_KEY, 0644);
    if (ret == QCONF_OK) {
        bool initRet = LRU::getInstance()->initLruMem(_shm_tbl);
        if (!initRet) {
            LOG_ERR("Init LRU memory failed");
            ret = QCONF_ERR_MEM; 
        }
    }
    return ret;
}

void qconf_clear_shm_tbl()
{
    hash_tbl_clear(_shm_tbl);
}

int qconf_init_msg_key()
{
    return create_msg_queue(QCONF_DEFAULT_MSG_QUEUE_KEY, _msg_queue_id);
}

/**
 * signal all conditions
 */
void qconf_thread_exit()
{
    _stop_watcher_setting = true;
    send_msg(_msg_queue_id, QCONF_STOP_MSG);
    _watch_nodes_cond.SignalAll();
    _change_trigger_cond.SignalAll();
    _gray_idcs_cond.SignalAll();
}

/**
 * Close zookeeper handler and destroy the watcher tables
 */
void qconf_destroy_zk()
{
    _ht_ih_mutex.Lock();
    for (map<string, zhandle_t*>::iterator it = _ht_idchost_handle.begin(); it != _ht_idchost_handle.end(); ++it)
    {
        zookeeper_close(it->second);
        it->second = NULL;
    }
    _ht_ih_mutex.Unlock();
}

/**
 * Initialize the register node prefix
 */
void qconf_init_rgs_node_pfx(const string &node_prefix)
{
    char hostname[256] = {0};
    gethostname(hostname, sizeof(hostname));
    _register_node_path = node_prefix;
    _register_node_path += string("/") + hostname;
}

void qconf_init_recv_timeout(int timeout)
{
    _recv_timeout = timeout < 1000 ? 1000 : timeout;
    if (_recv_timeout != timeout)
        LOG_ERR("Error zookeeper receive timeout! timeout:%d, use default:%d", timeout, _recv_timeout);
}

void qconf_init_fb_flg(bool enable_flags)
{
    _fb_enable = enable_flags;
}

int qconf_init_local_idc(const string &idc)
{
    _local_idc = idc;
    int ret = qconf_update_localidc(_shm_tbl, _local_idc);
    return ret;
}

void qconf_init_scexec_timeout(int timeout)
{
    _scexec_timeout = (timeout < 500) ? 500 : timeout;
    _scexec_timeout = (_scexec_timeout > 30000) ? 30000 : _scexec_timeout;
    if (_scexec_timeout != timeout)
        LOG_ERR("Error script execute timeout! timeout:%d, use default:%d", timeout, _scexec_timeout);
}

int watcher_setting_start()
{
    int ret = 0;
    pthread_t assist_watcher_thread, msg_thread, change_trigger_thread, gray_thread;

    // Assist watcher thread, scan share tbl regularly
    ret = pthread_create(&assist_watcher_thread, NULL, assist_watcher_process, NULL);
    if (0 != ret)
    {
        LOG_FATAL_ERR("Failed to create assist_watcher_thread! errno:%d", ret);
        return QCONF_ERR_OTHER;
    }

    // Msg thread, dispose message from message queue
    ret = pthread_create(&msg_thread, NULL, msg_process, NULL);	
    if (0 != ret)
    {
        LOG_FATAL_ERR("Failed to create msg_thread! errno:%d", ret);
        qconf_thread_exit();
        pthread_join(assist_watcher_thread, NULL);
        return QCONF_ERR_OTHER;
    }

    // Change trigger thread, trigger process like feedback, execute script and dump
    ret = pthread_create(&change_trigger_thread, NULL, change_trigger_process, NULL);
    if (0 != ret)
    {
        LOG_FATAL_ERR("Failed create change_trigger_thread! errno: %d", ret);
        qconf_thread_exit();
        pthread_join(msg_thread, NULL);
        pthread_join(assist_watcher_thread, NULL);
        return QCONF_ERR_OTHER;
    }

    // Change trigger thread, trigger process like feedback, execute script and dump
    ret = pthread_create(&gray_thread, NULL, do_gray_process, NULL);
    if (0 != ret)
    {
        LOG_FATAL_ERR("Failed create gray_thread! errno: %d", ret);
        qconf_thread_exit();
        pthread_join(change_trigger_thread, NULL);
        pthread_join(msg_thread, NULL);
        pthread_join(assist_watcher_thread, NULL);
        return QCONF_ERR_OTHER;
    }

    // Main thread, set wather on zookeeper and write share table
    deque_process();

    qconf_thread_exit();
    pthread_join(gray_thread, NULL);
    pthread_join(change_trigger_thread, NULL);
    pthread_join(msg_thread, NULL);
    pthread_join(assist_watcher_thread, NULL);

    return QCONF_OK;
}

static void *assist_watcher_process(void *p)
{
    int interval = 1800000 + rand() % 1800000;

    while (!_stop_watcher_setting)
    {
        msleep_interval(interval);
        if (_stop_watcher_setting) break;
        process_tbl();
    }
    pthread_exit(NULL);
}

static void msleep_interval(int msecond)
{
    int count = 0, num = msecond / 10;
    while (!_stop_watcher_setting && count++ < num && !_finish_process_tbl_sleep_setting)
    {
        usleep(10000);
    }
    _finish_process_tbl_sleep_setting = false;
}

static void *msg_process(void *p)
{
    string key;
    while (!_stop_watcher_setting)
    {
        int ret = receive_msg(_msg_queue_id, key);
        if (_stop_watcher_setting) break;
        if (QCONF_OK == ret)
        {
            add_watcher_node(key);
        }
        else if (QCONF_ERR_MSGIDRM == ret)
        {
            LOG_FATAL_ERR("Msg queue:%d has been removed! now recreate!",
                    _msg_queue_id);
            ret = qconf_init_msg_key();
            if (QCONF_OK != ret)
            {
                LOG_FATAL_ERR("Failed to recreate msg queue! ret:%d", ret);
                exit(-1);
            }
        }
        else
            LOG_ERR("Failed to get message from msq:%d!", _msg_queue_id);
    }
    pthread_exit(NULL);
}

static void deque_process()
{
    while (!_stop_watcher_setting)
    {
        string tblkey;
        _watch_nodes_mutex.Lock();
        while (!_stop_watcher_setting && _need_watch_nodes.size() == 0)
        {
            _watch_nodes_cond.Wait();
        }

        if (!_need_watch_nodes.empty())
        {
            tblkey = _need_watch_nodes.front();
            _need_watch_nodes.pop_front();
            _exist_watch_nodes.erase(tblkey);
        }
        _watch_nodes_mutex.Unlock();
        if (!tblkey.empty() && QCONF_OK != set_watcher_and_update_tbl(tblkey))
        {
            LOG_ERR_KEY_INFO(tblkey, "Failed to set watcher and update tbl!");
        }
    }
}

static int process_tbl()
{
    // Check share memory
    int max_slots = 0, used_slots = 0;
    hash_tbl_get_count(_shm_tbl, max_slots, used_slots);

    string tblkey, tblval;
    for (int idx = 0; idx < max_slots && !_stop_watcher_setting; ) 
    {
        int ret = hash_tbl_getnext(_shm_tbl, tblkey, tblval, idx);
        if (QCONF_OK == ret)
        {
            // do dump everytime
            if (QCONF_OK != qconf_dump_set(tblkey, tblval))
            {
                LOG_ERR_KEY_INFO(tblkey, "Failed to set dump when traverse tbl!");
            }

            // send to deque process thread only when value changed
            if (diff_with_zk(tblkey, tblval)) 
            {
                LOG_ERR_KEY_INFO(tblkey, "Checked inconformity with zookeeper!");
                add_watcher_node(tblkey);
            }
        }
        else if (QCONF_ERR_TBL_END == ret)
        {}
        else
        {
            LOG_ERR_KEY_INFO(tblkey, "Failed to get next item in shmtbl");
        }
    }

    // Watch notify node for current machine
    zhandle_t *zh = NULL;
    vector<string> idcs;
    vector<string>::iterator it;
    lock_ht_keys(_ht_idchost_handle, _ht_ih_mutex, idcs);
    
    for (it = idcs.begin(); it != idcs.end(); ++it)
    {
        string cidc = *it;
        if (QCONF_OK == lock_ht_find(_ht_idchost_handle, _ht_ih_mutex, cidc, zh) && NULL != zh)
        {
            switch (watch_notify_node(zh))
            {
                case QCONF_OK:
                    add_gray_idc(cidc);
                    break;
                case QCONF_NODE_NOT_EXIST:
                    break;
                default:
                    LOG_FATAL_ERR("Failed to set watcher for notify node on idc: %s!", cidc.c_str());
            }
        }
    }
    return QCONF_OK;
}

static bool diff_with_zk(const string &tblkey, const string &tblval)
{
    vector<char> status;
    string_vector_t chdnodes;
    int ret = QCONF_ERR_OTHER;
    string idc, path, zkval, tblval_new;
    char data_type = QCONF_DATA_TYPE_UNKNOWN;

    memset(&chdnodes, 0, sizeof(string_vector_t));
    deserialize_from_tblkey(tblkey, data_type, idc, path);
    if (QCONF_DATA_TYPE_ZK_HOST == data_type ||
        QCONF_DATA_TYPE_LOCAL_IDC == data_type)
    {
        return false;
    }

    zhandle_t *zh = get_zhandle_by_idc(idc);
    if (NULL != zh)
    {
        switch (data_type)
        {
        case QCONF_DATA_TYPE_NODE:
            ret = zk_get_node(zh, path, zkval, 1);
            if (QCONF_NODE_NOT_EXIST == ret) return true;
            if (QCONF_OK == ret)
            {
                nodeval_to_tblval(tblkey, zkval, tblval_new);
                if (tblval != tblval_new) return true;
            }
            break;
        case QCONF_DATA_TYPE_SERVICE:
            ret = zk_get_chdnodes_with_status(zh, path, chdnodes, status);
            if (QCONF_NODE_NOT_EXIST == ret) return true;
            if (QCONF_OK == ret)
            {
                chdnodeval_to_tblval(tblkey, chdnodes, tblval_new, status);
                deallocate_String_vector(&chdnodes);
                if (tblval != tblval_new) return true;
            }
            break;
        case QCONF_DATA_TYPE_BATCH_NODE:
            ret = zk_get_chdnodes(zh, path, chdnodes);
            if (QCONF_NODE_NOT_EXIST == ret) return true;
            if (QCONF_OK == ret)
            {
                batchnodeval_to_tblval(tblkey, chdnodes, tblval_new);
                deallocate_String_vector(&chdnodes);
                if (tblval != tblval_new) return true;
            }
            break;
        case QCONF_DATA_TYPE_ZK_HOST:
        case QCONF_DATA_TYPE_LOCAL_IDC:
            break;
        default:
            LOG_ERR("Invalid data_type:%c", data_type);
        }
    }
    return false;
}

static int set_watcher_and_update_tbl(const string &tblkey)
{
    string idc, path, gray_value;
    int ret = QCONF_ERR_OTHER;
    char data_type = QCONF_DATA_TYPE_UNKNOWN;
    // If the node is gray nodes
    if (is_gray_node(tblkey, gray_value))
    {
        ret = hash_tbl_set(_shm_tbl, tblkey, gray_value);
        if (QCONF_OK == ret)
        {
            add_change_trigger_node(tblkey, gray_value, QCONF_TRIGGER_TYPE_ADD_OR_MODIFY);
        }
        if (QCONF_OK != ret && QCONF_ERR_SAME_VALUE != ret)
        {
            LOG_ERR("Failed to set share memory for gray node. ret:%d", ret);
            return QCONF_ERR_GRAY_FAKE_SHM_FAILED;
        }
        return QCONF_OK;
    }

    deserialize_from_tblkey(tblkey, data_type, idc, path);

    zhandle_t *zh = get_zhandle_by_idc(idc);
    if (zh != NULL)
    {
        switch (data_type)
        {
        case QCONF_DATA_TYPE_NODE:
            ret = process_node(zh, tblkey, path);
            break;
        case QCONF_DATA_TYPE_SERVICE:
            ret = process_service(zh, tblkey, path);
            break;
        case QCONF_DATA_TYPE_BATCH_NODE:
            ret = process_batch(zh, tblkey, path);
            break;
        default:
            LOG_ERR("Invalid data_type:%c", data_type);
            return QCONF_ERR_DATA_TYPE;
        }
    }
    
    if (QCONF_ERR_ZOO_FAILED == ret) //read from dump only when zk failed
    {
        string tblval;
        if (QCONF_OK != qconf_dump_get(tblkey, tblval))
        {
            LOG_ERR_KEY_INFO(tblkey, "Failed to get value from dump!");
            return QCONF_ERR_OTHER;
        }
        ret = hash_tbl_set(_shm_tbl, tblkey, tblval);
        ret = (QCONF_ERR_SAME_VALUE == ret) ? QCONF_OK : ret;
        return ret;
    }
    
    return ret;
}

static int process_node(zhandle_t *zh, const string &tblkey, const string &path)
{
    string val, tblval;
    int ret = zk_get_node(zh, path, val, 1);
    
    switch (ret)
    {
    case QCONF_OK:
        nodeval_to_tblval(tblkey, val, tblval);
        ret = hash_tbl_set(_shm_tbl, tblkey, tblval);
        if (QCONF_OK == ret)
        {
            add_change_trigger_node(tblkey, tblval, QCONF_TRIGGER_TYPE_ADD_OR_MODIFY);
        }
        ret = (QCONF_ERR_SAME_VALUE == ret) ? QCONF_OK : ret;
        return ret;
    case QCONF_NODE_NOT_EXIST:
        ret = hash_tbl_remove(_shm_tbl, tblkey);
        add_change_trigger_node(tblkey, tblval, QCONF_TRIGGER_TYPE_REMOVE);
        return ret;
    default:
        LOG_ERR("Failed to get node value! path:%s", path.c_str());
        return ret;
    }
}

static int process_service(zhandle_t *zh, const string &tblkey, const string &path)
{
    vector<char> status; 
    string tblval, fb_val;
    string_vector_t chdnodes;
    memset(&chdnodes, 0, sizeof(string_vector_t));

    int ret = zk_get_chdnodes_with_status(zh, path, chdnodes, status);
    switch (ret)
    {
    case QCONF_OK:
        chdnodeval_to_tblval(tblkey, chdnodes, tblval, status);
        ret = hash_tbl_set(_shm_tbl, tblkey, tblval);
        if (QCONF_OK == ret)
        {
#ifdef QCONF_CURL_ENABLE
            if (_fb_enable) feedback_generate_chdval(chdnodes, status, fb_val);
#endif
            add_change_trigger_node(tblkey, tblval, QCONF_TRIGGER_TYPE_ADD_OR_MODIFY, fb_val);
        }
        deallocate_String_vector(&chdnodes);
        ret = (QCONF_ERR_SAME_VALUE == ret) ? QCONF_OK : ret;
        return ret;
    case QCONF_NODE_NOT_EXIST:
        ret = hash_tbl_remove(_shm_tbl, tblkey);
        add_change_trigger_node(tblkey, tblval, QCONF_TRIGGER_TYPE_REMOVE);
        return ret;
    default:
        return ret;
    }
}

static int process_batch(zhandle_t *zh, const string &tblkey, const string &path)
{
    string_vector_t nodes;
    memset(&nodes, 0, sizeof(string_vector_t));
    string tblval, fb_val;
    int ret = zk_get_chdnodes(zh, path, nodes);
    switch (ret)
    {
    case QCONF_OK:
        batchnodeval_to_tblval(tblkey, nodes, tblval);
        ret = hash_tbl_set(_shm_tbl, tblkey, tblval);
        if (QCONF_OK == ret)
        {
#ifdef QCONF_CURL_ENABLE
            if (_fb_enable) feedback_generate_batchval(nodes, fb_val);
#endif
            add_change_trigger_node(tblkey, tblval, QCONF_TRIGGER_TYPE_ADD_OR_MODIFY, fb_val);
        }
        deallocate_String_vector(&nodes);
        ret = (QCONF_ERR_SAME_VALUE == ret) ? QCONF_OK : ret;
        return ret;
    case QCONF_NODE_NOT_EXIST:
        ret = hash_tbl_remove(_shm_tbl, tblkey);
        add_change_trigger_node(tblkey, tblval, QCONF_TRIGGER_TYPE_REMOVE);
        return ret;
    default:
        return ret;
    }
}

static zhandle_t *get_zhandle_by_idc(const string &idc)
{
    if (idc.empty()) return NULL;
    string host, idc_host;

    if (QCONF_OK != get_idc_conf(idc, host))
    {
        LOG_ERR("Failed to get host by idc:%s", idc.c_str());
        return NULL;
    }

    serialize_to_idc_host(idc, host, idc_host);
    zhandle_t *zh = NULL;
    lock_ht_find(_ht_idchost_handle, _ht_ih_mutex, idc_host, zh);
    if (NULL == zh)
    {
        zh = zookeeper_init(host.c_str(), global_watcher, _recv_timeout, NULL, NULL, 0);
        if (NULL == zh)
        {
            LOG_ERR("Failed to initial zookeeper. host:%s timeout:%d",
                    host.c_str(), _recv_timeout);
            return NULL;
        }
        init_env_for_zk(zh, idc_host, idc);
    }
    return zh;
}

static int get_idc_by_zhandle(const zhandle_t *zh, string &idc, string &host)
{
    string idc_host;
    unsigned long htkey = reinterpret_cast<unsigned long>(zh);
    if (QCONF_OK != lock_ht_find(_ht_handle_idchost, _ht_hi_mutex, htkey, idc_host)) return QCONF_ERR_OTHER;
    deserialize_from_idc_host(idc_host, idc, host);
    return QCONF_OK;
}

static void global_watcher(zhandle_t *zh, int type, int state, const char *path, void *context)
{
    LOG_TRACE("Global_watcher received event. type:%d state:%d path:%s", type, state, path);
    
    string idc, host;
    if (QCONF_OK != get_idc_by_zhandle(zh, idc, host)) return;
    
    // do gray process when the event is happened on notify node
    if ((CREATED_EVENT_DEF == type || DELETED_EVENT_DEF == type) && is_notify_node(path))
    {
        add_gray_idc(idc);
    }

    switch (type)
    {
    case SESSION_EVENT_DEF:
        LOG_DEBUG("Process session watcher...");
        if (ZOO_EXPIRED_SESSION_STATE == state)
        {
            LOG_ERR("[session state: ZOO_EXPIRED_SESSION_STATE], now reconnect to zookeeper!");
            watcher_reconnect_to_zookeeper(zh);
        }
        else if (ZOO_CONNECTED_STATE == state)
        {
            LOG_INFO("[session state: ZOO_CONNECTED_STATE]");
        }
        else if (ZOO_CONNECTING_STATE == state)
        {
            LOG_INFO("[session state: ZOO_CONNECTING_STATE]");
        }
        break;
    case CHILD_EVENT_DEF:
        process_child_event(idc, path);
        break;
    case CREATED_EVENT_DEF:
        process_created_event(idc, path);
        break;
    case DELETED_EVENT_DEF:
        process_deleted_event(idc, path);
        break;
    case CHANGED_EVENT_DEF:
        process_changed_event(idc, path);
        break;
    }
}

static int watcher_reconnect_to_zookeeper(zhandle_t *zh)
{
    if (NULL == zh) return QCONF_ERR_OTHER;

    zhandle_t *hthandle = NULL;
    string idc_host, idc, host;
    int ret = QCONF_ERR_OTHER;
    unsigned long htkey = reinterpret_cast<unsigned long>(zh);
    if (QCONF_OK == lock_ht_find(_ht_handle_idchost, _ht_hi_mutex, htkey, idc_host))
    {
        lock_ht_delete(_ht_handle_idchost, _ht_hi_mutex, htkey);
        lock_ht_delete(_ht_idchost_handle, _ht_ih_mutex, idc_host);

        // close old handle
        zookeeper_close(zh);

        deserialize_from_idc_host(idc_host, idc, host);
        hthandle = zookeeper_init(host.c_str(), global_watcher, _recv_timeout, NULL, NULL, 0);
        if (NULL != hthandle)
        {
            // init env zk related
            init_env_for_zk(hthandle, idc_host, idc);

            // reset the table watcher
            _finish_process_tbl_sleep_setting = true;
            ret = QCONF_OK;
        }
        else
        {
            LOG_ERR("Failed to initial zookeeper. host:%s", host.c_str());
        }
    }
    else
    {
        LOG_ERR("Failed to find in [handle to host] table.");
    }
    return ret;
}

static void init_env_for_zk(zhandle_t *zh, const string &idc_host, const string &idc)
{
    // Update table
    unsigned long htkey_new = reinterpret_cast<unsigned long>(zh);
    lock_ht_update(_ht_handle_idchost, _ht_hi_mutex, htkey_new, idc_host);
    lock_ht_update(_ht_idchost_handle, _ht_ih_mutex, idc_host, zh);

    // Reregister Current Host on Zookeeper host
    zk_register_ephemeral(zh, _register_node_path, QCONF_AGENT_VERSION);

    // Watch notify node for current machine
    switch (watch_notify_node(zh))
    {
        case QCONF_OK:
            add_gray_idc(idc);
            break;
        case QCONF_NODE_NOT_EXIST:
            break;
        default:
            LOG_FATAL_ERR("Failed to set watcher for notify node on idc: %s!", idc.c_str());
    }
}

/**
 * After receive delete event, then set node watcher off, and delete the node
 */
static void process_deleted_event(const string &idc, const string &path)
{
    //if (QCONF_NODE_NOT_EXIST != zk_exists(zh, path)) return;
    
    string tblkey;
    serialize_to_tblkey(QCONF_DATA_TYPE_NODE, idc, path, tblkey); 
    if (hash_tbl_exist(_shm_tbl, tblkey))
    {
        add_watcher_node(tblkey);
    }

    serialize_to_tblkey(QCONF_DATA_TYPE_SERVICE, idc, path, tblkey);
    if (hash_tbl_exist(_shm_tbl, tblkey))
    {
        add_watcher_node(tblkey);
    }

    serialize_to_tblkey(QCONF_DATA_TYPE_BATCH_NODE, idc, path, tblkey);
    if (hash_tbl_exist(_shm_tbl, tblkey))
    {
        add_watcher_node(tblkey);
    }
}

/**
 * After receive the created event, then set node watcher off
 */
static void process_created_event(const string &idc, const string &path)
{
    // There is no need operation for created event
    // For it is user's duty to decide to get the nodes
}

/**
 * After received children event, then set the node watcher off
 */
static void process_changed_event(const string &idc, const string &path)
{
    string tblkey;
    serialize_to_tblkey(QCONF_DATA_TYPE_NODE, idc, path, tblkey);
    if (hash_tbl_exist(_shm_tbl, tblkey))
    {
        add_watcher_node(tblkey);
    }

    // if child node changed, then set the parent service node flags
    size_t pos = path.rfind('/');
    if (string::npos != pos)
    {
        string parent_tblkey;   
        serialize_to_tblkey(QCONF_DATA_TYPE_SERVICE, idc, path.substr(0, pos), parent_tblkey);
        if (hash_tbl_exist(_shm_tbl, parent_tblkey))
        {
            add_watcher_node(parent_tblkey);
        }
    }
}

/**
 * After received children event, Set current path watcher off
 */
static void process_child_event(const string &idc, const string &path)
{
    string tblkey;

    serialize_to_tblkey(QCONF_DATA_TYPE_SERVICE, idc, path, tblkey);
    if (hash_tbl_exist(_shm_tbl, tblkey))
    {
        add_watcher_node(tblkey);
    }

    serialize_to_tblkey(QCONF_DATA_TYPE_BATCH_NODE, idc, path, tblkey);
    if (hash_tbl_exist(_shm_tbl, tblkey))
    {
        add_watcher_node(tblkey);
    }
}

static void add_watcher_node(const string &key)
{
    if (key.empty()) return;
    _watch_nodes_mutex.Lock();
    if (_exist_watch_nodes.find(key) == _exist_watch_nodes.end())
    {
        _need_watch_nodes.push_back(key);
        _exist_watch_nodes.insert(key);
        _watch_nodes_cond.SignalAll();
    }
    _watch_nodes_mutex.Unlock();
}

static void add_gray_idc(const string &key)
{
    if (key.empty()) return;
    _gray_idcs_mutex.Lock();
    if (_exist_gray_idcs.find(key) == _exist_gray_idcs.end())
    {
        _need_gray_idcs.push_back(key);
        _exist_gray_idcs.insert(key);
        _gray_idcs_cond.SignalAll();
    }
    _gray_idcs_mutex.Unlock();
}

static int add_change_trigger_node(const string &tblkey, const string &tblval, char trigger_type, const string &fb_chds)
{
    if (tblkey.empty()) return QCONF_ERR_OTHER;
    string mkey = trigger_type + tblkey;  //append trigger type before tblkey
    fb_val mval = {tblval, fb_chds};

    _change_trigger_mutex.Lock();
    if (_exist_trigger_nodes.find(mkey) == _exist_trigger_nodes.end())
    {
        _need_trigger_nodes.push_back(mkey);
        _exist_trigger_nodes.insert(pair<string, fb_val>(mkey, mval));
        _change_trigger_cond.SignalAll();
    }
    _change_trigger_mutex.Unlock();
    return QCONF_OK;
}

static void* change_trigger_process(void *p)
{
    while (!_stop_watcher_setting)
    {
        string mkey;
        fb_val mval;

        _change_trigger_mutex.Lock();
        while (!_stop_watcher_setting && _need_trigger_nodes.size() == 0)
        {
            _change_trigger_cond.Wait();   // awake when the change trigger queue not empty
        }
        // get key need process
        if (_need_trigger_nodes.size() > 0)
        {
            mkey = _need_trigger_nodes.front();
            _need_trigger_nodes.pop_front();
            mval = _exist_trigger_nodes.find(mkey)->second;
            _exist_trigger_nodes.erase(mkey);
        }
        _change_trigger_mutex.Unlock();

        // begin process
        if (!mkey.empty())
        {
            char data_type = 0;
            char trigger_type = mkey[0];
            string idc, path, tblkey, tblval;
            tblkey = mkey.substr(1);
            deserialize_from_tblkey(tblkey, data_type, idc, path);
            tblval = mval.tblval;
            
            switch (trigger_type)
            {
                case QCONF_TRIGGER_TYPE_RESET:
                    break;
                case QCONF_TRIGGER_TYPE_ADD_OR_MODIFY:
                case QCONF_TRIGGER_TYPE_REMOVE:
                    trigger_script(data_type, idc, path, trigger_type);
                    trigger_dump(tblkey, tblval, trigger_type);
                    break;
                default:
                    LOG_ERR("Unknown trigger type:%c", trigger_type);
            }
#ifdef QCONF_CURL_ENABLE
            if (QCONF_TRIGGER_TYPE_REMOVE != trigger_type && _fb_enable)
            {
                trigger_feedback(data_type, idc, path, tblkey, mval, trigger_type);
            }
#endif
        }
        else
        {
            LOG_INFO("Change trigger thread awaked, but no message found, maybe exit!");
            continue;
        }
    }
    pthread_exit(NULL);
}

static void trigger_dump(const string &tblkey, const string &tblval, char trigger_type)
{
    if (tblkey.empty()) return;
    switch (trigger_type)
    {
        case QCONF_TRIGGER_TYPE_ADD_OR_MODIFY:
            if (QCONF_OK != qconf_dump_set(tblkey, tblval))
            {
                LOG_ERR_KEY_INFO(tblkey, "Failed to set dump!");
            }
            break;
        case QCONF_TRIGGER_TYPE_REMOVE:
            if (QCONF_OK != qconf_dump_delete(tblkey))
            {
                LOG_ERR_KEY_INFO(tblkey, "Failed to delete key in dump!");
            }
            break;
        default:
            LOG_ERR("Unknown atrigger type for dump, trigger_type:%c!", trigger_type);
    }
}

static void trigger_script(char data_type, const string &idc, const string &path, char trigger_type)
{
    if (idc.empty() || path.empty()) return;
    
    string head, path_no_prefix, content;
    //generate script head
#ifdef QCONF_INTERNAL
    int ret = path.compare(0, QCONF_PREFIX_LEN, QCONF_PREFIX); 
    path_no_prefix = (0 == ret) ? path.substr(QCONF_PREFIX_LEN) : path;
#else
    path_no_prefix = path;
#endif
    head += "qconf_path=" + path_no_prefix + ";qconf_idc=" + idc
         + ";qconf_type=" + data_type + ";qconf_event=" + trigger_type + ";";

    //get script content
    if (QCONF_OK != find_script(path_no_prefix, content)) return; 

    //execute script
    if (QCONF_OK != execute_script(head + content, _scexec_timeout))
    {
        LOG_ERR("Failed to execute script, script:(%s) of path:%s", content.c_str(), path.c_str());
    }
}

#ifdef QCONF_CURL_ENABLE
static void trigger_feedback(char data_type, const string &idc, const string &path, const string &tblkey, const fb_val &mval, char trigger_type)
{
    if (idc.empty() || path.empty()) return;

    // generate feedback content and do feedback
    string content;
    zhandle_t *zh = get_zhandle_by_idc(idc);
    if (NULL == zh) 
    {
        LOG_ERR("Failed to get zhandle by idc! idc:%s", idc.c_str());
        return;
    }
    // generate ip
    static string ip;
    if (ip.empty())
    {
        if(QCONF_OK != get_feedback_ip(zh, ip)) return;
    }

    int ret = feedback_generate_content(ip, data_type, idc, path, mval, content);
    if (QCONF_OK == ret)
    {
        int ret = feedback_process(content);
        if (QCONF_ERR_FB_TIMEOUT == ret)
        {
            add_change_trigger_node(tblkey, mval.tblval, QCONF_TRIGGER_TYPE_RESET, mval.fb_chds);
        }
        else if (QCONF_OK != ret)
        {
            LOG_ERR("Failed to process feedback, idc:%s, path:%s!", idc.c_str(), path.c_str());
        }
    }
    else
    {
        LOG_ERR("Failed to generate feedback content! ret:%d, idc:%s, path:%s",
                ret, idc.c_str(), path.c_str());
    }
}
#endif

static void *do_gray_process(void *p)
{
    while (!_stop_watcher_setting)
    {
        string gray_idc;
        _gray_idcs_mutex.Lock();
        while (!_stop_watcher_setting && _need_gray_idcs.size() == 0)
        {
            _gray_idcs_cond.Wait();
        }

        if (!_need_gray_idcs.empty())
        {
            gray_idc = _need_gray_idcs.front();
            _need_gray_idcs.pop_front();
            _exist_gray_idcs.erase(gray_idc);
        }
        _gray_idcs_mutex.Unlock();

        if (!gray_idc.empty()) 
        {
            zhandle_t *zh = get_zhandle_by_idc(gray_idc);
            if (NULL == zh) 
            {
                LOG_FATAL_ERR("Failed to get zk handle for gray idc!");
            }

            vector< pair<string, string> > gray_nodes;
            if (QCONF_OK != gray_process(zh, gray_idc, gray_nodes))
            {
                LOG_FATAL_ERR("Failed to do the gray release process!");
            }
            // send to the main thread to modify the share memory
            for (vector< pair<string, string> >::const_iterator it = gray_nodes.begin(); it != gray_nodes.end(); ++it)
            {
                add_watcher_node((*it).first);
            }
        }
        // eles maybe signaled by _stop_watcher_setting
    }
    pthread_exit(NULL);
}
