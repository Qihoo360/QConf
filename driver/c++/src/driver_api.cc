#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/ioctl.h>
#include <sys/poll.h>
#include <sys/time.h>
#include <sys/shm.h>
#include <errno.h>

#include <iostream>
#include <vector>
#include <string>

#include "qconf_log.h"
#include "qconf_shm.h"
#include "qconf_msg.h"
#include "qconf_errno.h"
#include "qconf_format.h"
#include "driver_api.h"

using namespace std;

static qhasharr_t *_qconf_hashtbl  = NULL;
static key_t _qconf_hashtbl_key    = QCONF_DEFAULT_SHM_KEY;

static int _qconf_msqid            = QCONF_INVALID_SEM_ID;
static key_t _qconf_msqid_key      = QCONF_DEFAULT_MSG_QUEUE_KEY;

static int init_shm(); 
static int init_msg();
static int send_msg_to_agent(int msqid, const string &idc, const string &path, char data_type);
static int qconf_get_(const string &path, string &tblval, char dtype, const string &idc, int flags);


int init_qconf_env()
{
    int ret = init_shm();
    if (QCONF_OK != ret) return ret;

    ret = init_msg();

    return ret; 
}

static int init_shm()
{
    int ret = QCONF_OK;

    if (NULL != _qconf_hashtbl) return ret;

    ret = init_hash_tbl(_qconf_hashtbl, _qconf_hashtbl_key, 0444, SHM_RDONLY);
    if (QCONF_OK != ret)
    {
        LOG_FATAL_ERR("Failed to init hash table! key:%#x", _qconf_hashtbl_key);
        return ret; 
    }

    return ret;
}

static int init_msg()
{
    int ret = QCONF_OK;

    if (QCONF_INVALID_SEM_ID != _qconf_msqid) return ret;

    ret = init_msg_queue(_qconf_msqid_key, _qconf_msqid);
    if (QCONF_OK != ret)
    {
        LOG_FATAL_ERR("Failed to init msg queue! key:%#x", _qconf_msqid_key);
        return ret;
    }

    return ret;
}

/**
 * Get child nodes from hash table(share memory)
 */
int qconf_get_children(const string &path, string_vector_t &nodes, const string &idc, int flags)
{
    if (path.empty()) return QCONF_ERR_PARAM;

    string tblval;

    int ret = qconf_get_(path, tblval, QCONF_DATA_TYPE_SERVICE, idc, flags);
    if (QCONF_OK != ret) return ret;

    ret = tblval_to_chdnodeval(tblval, nodes);
    return ret;
}

int qconf_get_batchnode(const string &path, qconf_batch_nodes &bnodes, const string &idc, int flags)
{
    if (path.empty()) return QCONF_ERR_PARAM;

    string buf;
    string child_path;
    int ret = QCONF_OK;
    string_vector_t nodes;

    memset(&nodes, 0, sizeof(string_vector_t));
    ret = qconf_get_batchnode_keys(path, nodes, idc, flags);

    if (QCONF_OK == ret)
    {
        bnodes.count = nodes.count;
        if (0 == bnodes.count)
        {
            bnodes.nodes = NULL;
            return ret;
        }

        bnodes.nodes = (qconf_node*) calloc(bnodes.count, sizeof(qconf_node));
        if (NULL == bnodes.nodes)
        {
            LOG_ERR("Failed to malloc bnodes->nodes! errno:%d", errno);
            free_string_vector(nodes, nodes.count);
            bnodes.count = 0;
            return QCONF_ERR_MEM;
        }

        for (int i = 0; i < nodes.count; i++)
        {
            child_path = path + "/" + nodes.data[i];

            ret = qconf_get(child_path, buf, idc, flags);
            if (QCONF_OK == ret)
            {
                bnodes.nodes[i].value = strndup(buf.c_str(), buf.size() + 1);
                if (NULL == bnodes.nodes[i].value)
                {
                    LOG_ERR("Failed to strdup value of path:%s! errno:%d",
                            child_path.c_str(), errno);
                    free_string_vector(nodes, nodes.count);
                    free_qconf_batch_nodes(&bnodes, i);
                    return QCONF_ERR_MEM;
                }

                bnodes.nodes[i].key = nodes.data[i];
                nodes.data[i] = NULL;
            }
            else
            {
                LOG_ERR("Failed to call qconf_get! ret:%d", ret);
                free_string_vector(nodes, nodes.count);
                free_qconf_batch_nodes(&bnodes, i);
                return ret;
            }
        }

        free_string_vector(nodes, 0);
    }
    else
    {
        LOG_ERR("Failed to get batch node keys! path:%s, idc:%s ret:%d",
                path.c_str(), idc.c_str(), ret);
    }

    return ret;
}

int qconf_get_batchnode_keys(const string &path, string_vector_t &nodes, const string &idc, int flags)
{
    if (path.empty()) return QCONF_ERR_PARAM;

    string tblval;

    int ret = qconf_get_(path, tblval, QCONF_DATA_TYPE_BATCH_NODE, idc, flags);
    if (QCONF_OK != ret) return ret;

    ret = tblval_to_batchnodeval(tblval, nodes);
    return ret;
}

/**
 * Get child nodes from hash table(share memory)
 */
int qconf_get(const string &path, string &buf, const string &idc, int flags)
{
    if (path.empty()) return QCONF_ERR_PARAM;

    string tblval;

    int ret = qconf_get_(path, tblval, QCONF_DATA_TYPE_NODE, idc, flags);
    if (QCONF_OK != ret) return ret;

    ret = tblval_to_nodeval(tblval, buf);
    return ret;
}

static int qconf_get_(const string &path, string &tblval, char dtype, const string &idc, int flags)
{
    int count = 0;
    string tblkey;
    int ret = QCONF_OK;

    ret = init_qconf_env();
    if (QCONF_OK != ret) return ret;

    string tmp_idc(idc);
    if (idc.empty())
    {
        ret = qconf_get_localidc(_qconf_hashtbl, tmp_idc);
        if (QCONF_OK != ret)
        {
            LOG_ERR("Failed to get local idc! ret:%d", ret);
            return ret;
        }
    }

    ret = serialize_to_tblkey(dtype, tmp_idc, path, tblkey);
    if (QCONF_OK != ret) return ret;

    // get value from tbl
    ret = hash_tbl_get(_qconf_hashtbl, tblkey, tblval);
    if (QCONF_OK == ret) return ret;

    // Not get batch keys from share memory, then send message to agent
    int ret_snd = send_msg_to_agent(_qconf_msqid, tmp_idc, path, dtype);
    if (QCONF_OK != ret_snd)
    {
        LOG_ERR("Failed to send message to agent, ret:%d", ret_snd);
        return ret_snd;
    }

    // If not wait, then return directly
    if (QCONF_NOWAIT == flags) return ret;

    while (count < QCONF_MAX_GET_TIMES)
    {
        usleep(5000);
        count++;
        
        ret = hash_tbl_get(_qconf_hashtbl, tblkey, tblval);
        if (QCONF_OK == ret)
        {
            LOG_ERR("Wait time:%d*5ms, type:%c, idc:%s, path:%s", 
                    count, dtype, tmp_idc.c_str(), path.c_str());
            return ret;
        }
    }

    if (count >= QCONF_MAX_GET_TIMES)
    {
        LOG_FATAL_ERR("Failed to get value! wait time:%d*5ms, type:%c, idc:%s, path:%s, ret:%d",
                count, dtype, tmp_idc.c_str(), path.c_str(), ret);
    }

    return ret;
}

static int send_msg_to_agent(int msqid, const string &idc, const string &path, char data_type)
{
    string tblkey;
    serialize_to_tblkey(data_type, idc, path, tblkey);
    
    int ret = send_msg(msqid, tblkey);
    return ret;
}
