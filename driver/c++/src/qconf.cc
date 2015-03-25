#include <errno.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <zookeeper.h>

#include <string>

#include "qconf.h"
#include "qconf_log.h"
#include "driver_api.h"
#include "qconf_errno.h"
#include "driver_common.h"

using namespace std;

#ifdef QCONF_INTERNAL
static int qconf_get_batch_keys_native_(const char *path, string_vector_t *nodes, const char *idc, int flags);
#endif

static int get_node_path(const string &path, string &real_path);
static int qconf_get_conf_(const char *path, char *buf, size_t buf_len, const char *idc, int flags);
static int qconf_get_batch_conf_(const char *path, qconf_batch_nodes *bnodes, const char *idc, int flags);
static int qconf_get_batch_keys_(const char *path, string_vector_t *nodes, const char *idc, int flags);
static int qconf_get_allhost_(const char *path, string_vector_t *nodes, const char *idc, int flags);
static int qconf_get_host_(const char *path, char *buf, size_t buf_len, const char *idc, int flags);

int qconf_init()
{
    srand(time(NULL));
    return init_qconf_env();
}

int qconf_destroy()
{
    return QCONF_OK;
}

int init_string_vector(string_vector_t *nodes)
{
    if (NULL == nodes) return QCONF_ERR_PARAM;

    memset((void*)nodes, 0, sizeof(string_vector_t));
    return QCONF_OK;
}

int destroy_string_vector(string_vector_t *nodes)
{
    if (NULL == nodes) return QCONF_ERR_PARAM;

    deallocate_String_vector((String_vector*)nodes);

    nodes->count = 0;
    nodes->data = NULL;
    return QCONF_OK;
}

int qconf_get_conf(const char *path, char *buf, unsigned int buf_len, const char *idc)
{
    return qconf_get_conf_(path, buf, buf_len, idc, QCONF_WAIT);
}

int qconf_aget_conf(const char *path, char *buf, unsigned int buf_len, const char *idc)
{
    return qconf_get_conf_(path, buf, buf_len, idc, QCONF_NOWAIT);
}

int qconf_get_batch_conf(const char *path, qconf_batch_nodes *bnodes, const char *idc)
{
    return qconf_get_batch_conf_(path, bnodes, idc, QCONF_WAIT);
}

int qconf_aget_batch_conf(const char *path, qconf_batch_nodes *bnodes, const char *idc)
{
    return qconf_get_batch_conf_(path, bnodes, idc, QCONF_NOWAIT);
}

int qconf_get_batch_keys(const char *path, string_vector_t *nodes, const char *idc)
{
    return qconf_get_batch_keys_(path, nodes, idc, QCONF_WAIT);
}

int qconf_aget_batch_keys(const char *path, string_vector_t *nodes, const char *idc)
{
    return qconf_get_batch_keys_(path, nodes, idc, QCONF_NOWAIT);
}

#ifdef QCONF_INTERNAL
int qconf_get_batch_keys_native(const char *path, string_vector_t *nodes, const char *idc)
{
    return qconf_get_batch_keys_native_(path, nodes, idc, QCONF_WAIT);
}

int qconf_aget_batch_keys_native(const char *path, string_vector_t *nodes, const char *idc)
{
    return qconf_get_batch_keys_native_(path, nodes, idc, QCONF_NOWAIT);
}
#endif

int qconf_get_allhost(const char *path, string_vector_t *nodes, const char *idc)
{
    return qconf_get_allhost_(path, nodes, idc, QCONF_WAIT);
}

int qconf_aget_allhost(const char *path, string_vector_t *nodes, const char *idc)
{
    return qconf_get_allhost_(path, nodes, idc, QCONF_NOWAIT);
}

int qconf_get_host(const char *path, char *buf, unsigned int buf_len, const char *idc)
{
    return qconf_get_host_(path, buf, buf_len, idc, QCONF_WAIT);
}

int qconf_aget_host(const char *path, char *buf, unsigned int buf_len, const char *idc)
{
    return qconf_get_host_(path, buf, buf_len, idc, QCONF_NOWAIT);
}

const char* qconf_version()
{
    return QCONF_DRIVER_CC_VERSION;
}

static int qconf_get_host_(const char *path, char *buf, size_t buf_len, const char *idc, int flags)
{
    if (NULL == path || '\0' == *path || NULL == buf)
        return QCONF_ERR_PARAM;

    int ret = QCONF_OK;
    string_vector_t nodes;

    init_string_vector(&nodes);
    ret = qconf_get_allhost_(path, &nodes, idc, flags); 
    if (QCONF_OK != ret)
    {
        LOG_ERR("Failed to call get_allhost_! ret:%d", ret);
        return ret;
    }

    if (0 == nodes.count)
    {
        buf[0] = '\0';
        return ret;
    }

    unsigned int r = rand() % nodes.count;
    size_t node_len = strlen(nodes.data[r]);
    if (node_len >= buf_len)
    {
        destroy_string_vector(&nodes);
        return QCONF_ERR_BUF_NOT_ENOUGH;
    }

    memcpy(buf, nodes.data[r], node_len);
    buf[node_len] = '\0';

    destroy_string_vector(&nodes);

    return ret;
}

static int qconf_get_allhost_(const char *path, string_vector_t *nodes, const char *idc, int flags)
{
    if (NULL == path || '\0' == *path || NULL == nodes)
        return QCONF_ERR_PARAM;

    string tmp_buf;
    string tmp_idc;
    int ret = QCONF_OK;
    string real_path;

    ret = get_node_path(string(path), real_path);
    if (QCONF_OK != ret) return ret;

    if (0 != nodes->count) destroy_string_vector(nodes);

    if (NULL != idc) tmp_idc.assign(idc);

    ret = qconf_get_children(real_path, *nodes, tmp_idc, flags);
    if (QCONF_OK != ret)
    {
        LOG_ERR("Failed to get children services! ret:%d", ret);
        return ret;
    }

    return ret;
}

static int qconf_get_conf_(const char *path, char *buf, size_t buf_len, const char *idc, int flags)
{
    if (NULL == path || '\0' == *path || NULL == buf)
        return QCONF_ERR_PARAM;

    string tmp_buf;
    string tmp_idc;
    int ret = QCONF_OK;
    string real_path;

    ret = get_node_path(string(path), real_path);
    if (QCONF_OK != ret) return ret;

    if (NULL != idc) tmp_idc.assign(idc);

    ret = qconf_get(real_path, tmp_buf, tmp_idc, flags);
    if (QCONF_OK != ret) return ret;

    if (tmp_buf.size() >= buf_len)
    {
        LOG_ERR("buf is not enough! value len:%zd, buf len:%zd",
                tmp_buf.size(), buf_len);
        return QCONF_ERR_BUF_NOT_ENOUGH;
    }

    memcpy(buf, tmp_buf.data(), tmp_buf.size());
    buf[tmp_buf.size()] = '\0';

    return ret;
}

static int qconf_get_batch_conf_(const char *path, qconf_batch_nodes *bnodes, const char *idc, int flags)
{
    if (NULL == path || '\0' == *path || NULL == bnodes)
        return QCONF_ERR_PARAM;

    string tmp_buf;
    string tmp_idc;
    int ret = QCONF_OK;
    string real_path;

    ret = get_node_path(string(path), real_path);
    if (QCONF_OK != ret) return ret;

    if (0 != bnodes->count) destroy_qconf_batch_nodes(bnodes);

    if (NULL != idc) tmp_idc.assign(idc);

    ret = qconf_get_batchnode(real_path, *bnodes, tmp_idc, flags);
    if (QCONF_OK != ret)
    {
        LOG_ERR("Failed to call qconf_get_batchnode! ret:%d", ret);
        return ret;
    }

    return ret;
}

static int qconf_get_batch_keys_(const char *path, string_vector_t *nodes, const char *idc, int flags)
{
    if (NULL == path || '\0' == *path || NULL == nodes)
        return QCONF_ERR_PARAM;

    string tmp_buf;
    string tmp_idc;
    int ret = QCONF_OK;
    string real_path;

    ret = get_node_path(string(path), real_path);
    if (QCONF_OK != ret) return ret;

    if (0 != nodes->count) destroy_string_vector(nodes);

    if (NULL != idc) tmp_idc.assign(idc);

    ret = qconf_get_batchnode_keys(real_path, *nodes, tmp_idc, flags);
    if (QCONF_OK != ret)
    {
        LOG_ERR("Failed to get batchnode keys! ret:%d", ret);
        return ret;
    }

    return ret;
}

#ifdef QCONF_INTERNAL
static int qconf_get_batch_keys_native_(const char *path, string_vector_t *nodes, const char *idc, int flags)
{
    if (NULL == path || '\0' == *path || NULL == nodes)
        return QCONF_ERR_PARAM;

    int ret = QCONF_OK;
    string tmp_idc;

    if (0 != nodes->count) destroy_string_vector(nodes);

    if (NULL != idc) tmp_idc.assign(idc);

    ret = qconf_get_batchnode_keys(string(path), *nodes, tmp_idc, flags);
    if (QCONF_OK != ret)
    {
        LOG_ERR("Failed to get batchnode keys! ret:%d", ret);
        return ret;
    }

    return ret;
}
#endif

static int get_node_path(const string &path, string &real_path)
{
    if (0 == path.size()) return QCONF_ERR_PARAM;

    char delim = '/';
    size_t deal_path_len = 0;
    const char *end_pos = NULL;
    const char *start_pos = NULL;

    start_pos = path.data();
    end_pos = path.data() + path.size() - 1;

    while (*start_pos == delim && start_pos <= end_pos)
    {
        start_pos++;
    }
    while (*end_pos == delim && start_pos <= end_pos)
    {
        end_pos--;
    }
    
    if (start_pos > end_pos)
    {
        LOG_ERR("path:%s is not right", path.c_str());
        return QCONF_ERR_PARAM;
    }

    deal_path_len = end_pos + 1 - start_pos;

#ifdef QCONF_INTERNAL
    real_path.assign(QCONF_PREFIX);
    real_path.append(start_pos, deal_path_len);
#else
    real_path.assign(1, delim);
    real_path.append(start_pos, deal_path_len);
#endif
   
    return QCONF_OK;
}
