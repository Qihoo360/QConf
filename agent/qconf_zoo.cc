#include <errno.h>
#include <stdlib.h>
#include <string.h>
#include <pthread.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <zookeeper.h>

#include <string>

#include "qlibc.h"
#include "qconf_zoo.h"
#include "qconf_log.h"
#include "qconf_const.h"
#include "qconf_config.h"

using namespace std;

static FILE *_zoo_log_fp = NULL;
static pthread_key_t _qconf_safe_key;
static pthread_once_t _qconf_once_control = PTHREAD_ONCE_INIT;

static int zk_get_service_status(zhandle_t *zh, const string &path, char &status);
static int children_node_cmp(const void* p1, const void* p2);

static char *zk_get_node_buf_();
static void destroy_safe_key_(void *p);
static void init_once_routine_();

static char *zk_get_node_buf_()
{
    char *buf = NULL;
    
    int ret = pthread_once(&_qconf_once_control, init_once_routine_);
    if (0 != ret)
    {
        LOG_ERR("Failed to exec pthread_once! ret:%d", ret);
        return NULL;
    }

    buf = (char*)pthread_getspecific(_qconf_safe_key);
    if (NULL == buf)
    {
        buf = (char*)calloc(QCONF_MAX_VALUE_SIZE, sizeof(char));
        if (NULL == buf)
        {
            LOG_ERR("Failed to malloc space! errno:%d", errno);
            return NULL;
        }
        ret = pthread_setspecific(_qconf_safe_key, (void*)buf);
        if (0 != ret)
        {
            LOG_ERR("Faield to exec pthread_setspecific! ret:%d", ret);
            free(buf);
            return NULL;
        }
    }

    return buf;
}

static void init_once_routine_()
{
    pthread_key_create(&_qconf_safe_key,  destroy_safe_key_);
}

static void destroy_safe_key_(void *p)
{
    free(p);
}

/**
 * Get znode from zookeeper, and set a watcher
 */
int zk_get_node(zhandle_t *zh, const string &path, string &buf, int watcher)
{
    int ret = 0;
    int buffer_len = QCONF_MAX_VALUE_SIZE;

    char *buffer = zk_get_node_buf_();
    if (NULL == buffer)
    {
        LOG_ERR("Failed to get zk node buf");
        return QCONF_ERR_MEM;
    }

    for (int i = 0; i < QCONF_GET_RETRIES; ++i)
    {
        ret = zoo_get(zh, path.c_str(), watcher, buffer, &buffer_len, NULL);
        switch (ret)
        {
            case ZOK:
                if (-1 == buffer_len) buffer_len = 0;
                buf.assign(buffer, buffer_len);
                return QCONF_OK;
            case ZNONODE:
                return QCONF_NODE_NOT_EXIST;
            case ZINVALIDSTATE:
            case ZMARSHALLINGERROR:
                continue;
            default:
                LOG_ERR("Failed to call zoo_get. err:%s. path:%s", 
                        zerror(ret), path.c_str());
                return QCONF_ERR_ZOO_FAILED;
        }
    }

    LOG_ERR("Failed to call zoo_get after retry. err:%s. path:%s", 
            zerror(ret), path.c_str());
    return QCONF_ERR_ZOO_FAILED;
}

/**
 * Create znode on zookeeper
 */
int zk_create_node(zhandle_t *zh, const string &path, const string &value, int flags)
{
    int ret = 0;
    for (int i = 0; i < QCONF_GET_RETRIES; ++i)
    {
        ret = zoo_create(zh, path.c_str(), value.c_str(), value.length(), &ZOO_OPEN_ACL_UNSAFE, flags, NULL, 0);
        switch (ret)
        {
            case ZOK:
                return QCONF_OK;
            case ZNODEEXISTS:
                return QCONF_NODE_EXIST;
            case ZNONODE:
            case ZNOCHILDRENFOREPHEMERALS:
            case ZBADARGUMENTS:
                LOG_ERR("Failed to call zoo_create. err:%s. path:%s", 
                        zerror(ret), path.c_str());
                return QCONF_ERR_ZOO_FAILED;
            default:
                continue;
        }
    }

    LOG_ERR("Failed to call zoo_create after retry. err:%s. path:%s", 
            zerror(ret), path.c_str());
    return QCONF_ERR_ZOO_FAILED;
}

/**
 * Get children nodes from zookeeper and set a watcher
 */
int zk_get_chdnodes(zhandle_t *zh, const string &path, string_vector_t &nodes)
{
    if (NULL == zh || path.empty()) return QCONF_ERR_PARAM;

    int ret;
    for (int i = 0; i < QCONF_GET_RETRIES; ++i)
    {
        ret = zoo_get_children(zh, path.c_str(), 1, &nodes);
        switch(ret)
        {
            case ZOK:
                qsort(nodes.data, nodes.count, sizeof(char*), children_node_cmp);
                return QCONF_OK;
            case ZNONODE:
                return QCONF_NODE_NOT_EXIST;
            case ZINVALIDSTATE:
            case ZMARSHALLINGERROR:
                continue;
            default:
                LOG_ERR("Failed to call zoo_get_children. err:%s. path:%s",
                        zerror(ret), path.c_str());
                return QCONF_ERR_ZOO_FAILED;
        }
    }

    LOG_ERR("Failed to call zoo_get_children after retry. err:%s. path:%s",
            zerror(ret), path.c_str());
    return QCONF_ERR_ZOO_FAILED;
}

int zk_get_chdnodes_with_status(zhandle_t *zh, const string &path, string_vector_t &nodes, vector<char> &status)
{
    if (NULL == zh || path.empty()) return QCONF_ERR_PARAM;
    int ret = zk_get_chdnodes(zh, path, nodes);
    if (QCONF_OK == ret)
    {
        string child_path;
        status.resize(nodes.count);
        for (int i = 0; i < nodes.count; ++i)
        {
            child_path = path + '/' + nodes.data[i];
            char s = 0;
            ret = zk_get_service_status(zh, child_path, s);
            if (QCONF_OK != ret) return QCONF_ERR_OTHER;
            status[i] = s;
        }
    }
    return ret;
}

static int zk_get_service_status(zhandle_t *zh, const string &path, char &status)
{
    if (NULL == zh || path.empty()) return QCONF_ERR_PARAM;

    string buf;
    if (QCONF_OK == zk_get_node(zh, path, buf, 1))
    {
        long value = STATUS_UNKNOWN;
        get_integer(buf, value);
        switch(value)
        {
            case STATUS_UP:
            case STATUS_DOWN:
            case STATUS_OFFLINE:
                status = static_cast<char>(value);
                break;
            default:          
                LOG_FATAL_ERR("Invalid service status of path:%s, status:%ld!",
                        path.c_str(), value);
                return QCONF_ERR_OTHER;
        }
    }
    else
    {
        LOG_ERR( "Failed to get service status, path:%s", path.c_str());
        return QCONF_ERR_OTHER;
    }
    return QCONF_OK;
}

static int children_node_cmp(const void* p1, const void* p2)
{
    char **s1 = (char**)p1;
    char **s2 = (char**)p2;

    return strcmp(*s1, *s2);
}

int zk_register_ephemeral(zhandle_t *zh, const string &path, const string &value)
{
    if (NULL == zh || path.empty() || value.empty()) return QCONF_ERR_PARAM;

    string cur_path;
    int ret = QCONF_ERR_OTHER;
    size_t pos  = path.find_first_of('/', 1);
    while (string::npos != pos)
    {
        cur_path = path.substr(0, pos);
        ret = zk_create_node(zh, cur_path, "", 0);
        if (QCONF_OK != ret && QCONF_NODE_EXIST != ret)
        {
            LOG_ERR("Failed register ephemeral node:%s!", cur_path.c_str());
            return QCONF_ERR_ZOO_FAILED;
        }
        pos = path.find_first_of('/', pos + 1);
    }

    ret = zk_create_node(zh, path, value, ZOO_EPHEMERAL);
    if (QCONF_NODE_EXIST == ret)
    {
        LOG_INFO("Ephemeral node:%s already EXIST!", path.c_str());
        ret = QCONF_OK;
    }

    if (QCONF_OK != ret)
    {
        LOG_ERR("Failed to register ephemeral node:%s!", path.c_str());
    }
    return ret;
}

int qconf_init_zoo_log(const string &log_dir, const string &zoo_log)
{
    if (log_dir.empty() || zoo_log.empty()) return QCONF_ERR_PARAM;

    string log_path = log_dir + "/" + zoo_log;
    umask(0);
    _zoo_log_fp = fopen(log_path.c_str(), "a+");
    if (NULL == _zoo_log_fp)
    {
        LOG_ERR("Failed to open zoo log file:%s, errno:%d", log_path.c_str(), errno);
        return QCONF_ERR_FAILED_OPEN_FILE;
    }
    zoo_set_log_stream(_zoo_log_fp);
    zoo_set_debug_level(ZOO_LOG_LEVEL_WARN);
    return QCONF_OK;
}

void qconf_destroy_zoo_log()
{
    if (_zoo_log_fp != NULL)
    {
        fclose(_zoo_log_fp);
        _zoo_log_fp = NULL;
    }
}

int zk_exists(zhandle_t *zh, const string &path)
{
    int ret = 0;

    for (int i = 0; i < QCONF_GET_RETRIES; ++i)
    {
        ret = zoo_exists(zh, path.c_str(), 1, NULL);
        switch (ret)
        {
            case ZOK:
                return QCONF_OK;
            case ZNONODE:
                return QCONF_NODE_NOT_EXIST;
            case ZINVALIDSTATE:
            case ZMARSHALLINGERROR:
                continue;
            default:
                LOG_ERR("Failed to call zoo_exists. err:%s. path:%s", 
                        zerror(ret), path.c_str());
                return QCONF_ERR_ZOO_FAILED;
        }
    }

    LOG_ERR("Failed to call zoo_exists after retry. err:%s. path:%s", 
            zerror(ret), path.c_str());
    return QCONF_ERR_ZOO_FAILED;
}

