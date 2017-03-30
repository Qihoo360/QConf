#include <gdbm.h>
#include <errno.h>
#include <unistd.h>
#include <string.h>
#include <stdlib.h>
#include <assert.h>
#include <pthread.h>
#include <sys/stat.h>
#include <sys/ipc.h>

#include <string>
#include <iostream>

#include "qconf_log.h"
#include "qconf_shm.h"
#include "qconf_dump.h"
#include "qconf_common.h"
#include "qconf_format.h"

using namespace std;

const string QCONF_DUMP_DIR("/dumps");
const string QCONF_DUMP_PATH("/_dump.gdbm");

static string _qconf_dump_file;
static GDBM_FILE _qconf_dbf = NULL;
static pthread_mutex_t _qconf_dump_mutx = PTHREAD_MUTEX_INITIALIZER;

static int qconf_init_dbf_(int flags);

int qconf_init_dump_file(const string &agent_dir)
{
    _qconf_dump_file = agent_dir + QCONF_DUMP_DIR;
    mode_t mode = 0755;
    if (-1 == access(_qconf_dump_file.c_str(), F_OK))
    {
        if (-1 == mkdir(_qconf_dump_file.c_str(), mode))
        {
            LOG_ERR("Failed to create dump directory! errno:%d", errno);
            return QCONF_ERR_MKDIR;
        }
    }

    _qconf_dump_file.append(QCONF_DUMP_PATH);

    return qconf_init_dbf();
}

int qconf_init_dbf()
{
    return qconf_init_dbf_(GDBM_WRCREAT);
}

static int qconf_init_dbf_(int flags)
{
    if (_qconf_dump_file.empty())
    {
        LOG_ERR("Not init dump_file\n");
        return QCONF_ERR_OPEN_DUMP;
    }

    int mode = 0644;
    _qconf_dbf = gdbm_open(_qconf_dump_file.c_str(), 0, flags, mode, NULL);
    if (NULL == _qconf_dbf)
    {
        LOG_FATAL_ERR("Failed to open gdbm file:%s; gdbm err:%s", 
                _qconf_dump_file.c_str(), gdbm_strerror(gdbm_errno));
        return QCONF_ERR_OPEN_DUMP;
    }

    int value = 1;
    int ret = gdbm_setopt(_qconf_dbf, GDBM_COALESCEBLKS, &value, sizeof(value));
    if (ret != 0) {
        LOG_FATAL_ERR("Failed to setopt of gdbm file:%s; gdbm err:%s", 
                _qconf_dump_file.c_str(), gdbm_strerror(gdbm_errno));
        return QCONF_ERR_OPEN_DUMP;
    }

    return QCONF_OK;
}

void qconf_destroy_dump_lock()
{
    pthread_mutex_destroy(&_qconf_dump_mutx);
}

void qconf_destroy_dbf()
{
    if (NULL != _qconf_dbf)
    {
        gdbm_close(_qconf_dbf);
        _qconf_dbf = NULL;
    }
}

int qconf_dump_get(const string &tblkey, string &tblval)
{
    if (tblkey.empty()) return QCONF_ERR_PARAM;

    datum gdbm_key;
    datum gdbm_val;

    gdbm_key.dptr = (char*)tblkey.data();
    gdbm_key.dsize = tblkey.size();

    pthread_mutex_lock(&_qconf_dump_mutx);
    assert(NULL != _qconf_dbf);

    gdbm_val = gdbm_fetch(_qconf_dbf, gdbm_key);
    if (NULL == gdbm_val.dptr)
    {
        LOG_ERR_KEY_INFO(tblkey, "Failed to gdbm fetch! gdbm err:%s",
                gdbm_strerror(gdbm_errno));

        pthread_mutex_unlock(&_qconf_dump_mutx);
        return QCONF_ERR_NOT_FOUND;
    }
    pthread_mutex_unlock(&_qconf_dump_mutx);

    tblval.assign(gdbm_val.dptr, gdbm_val.dsize);
    free(gdbm_val.dptr);

    return QCONF_OK;
}

int qconf_dump_set(const string &tblkey, const string &tblval)
{
    if (tblkey.empty()) return QCONF_ERR_PARAM;

    datum gdbm_key;
    datum gdbm_val;
    int ret = QCONF_OK;

    gdbm_key.dptr = (char*)tblkey.data();
    gdbm_key.dsize = tblkey.size();
    gdbm_val.dptr = (char*)tblval.data();
    gdbm_val.dsize = tblval.size();

    pthread_mutex_lock(&_qconf_dump_mutx);
    assert(NULL != _qconf_dbf);

    ret = gdbm_store(_qconf_dbf, gdbm_key, gdbm_val, GDBM_REPLACE);
    if (0 != ret)
    {
        LOG_ERR_KEY_INFO(tblkey, "Failed to gdbm store! gdbm err:%s",
                gdbm_strerror(gdbm_errno));

        pthread_mutex_unlock(&_qconf_dump_mutx);
        return QCONF_ERR_WRITE_DUMP;
    }
    gdbm_sync(_qconf_dbf);
    pthread_mutex_unlock(&_qconf_dump_mutx);

    return QCONF_OK;
}

int qconf_dump_delete(const string &tblkey)
{
    if (tblkey.empty()) return QCONF_ERR_PARAM;

    datum gdbm_key;
    int ret = QCONF_OK;

    gdbm_key.dptr = (char*)tblkey.data();
    gdbm_key.dsize = tblkey.size();

    pthread_mutex_lock(&_qconf_dump_mutx);
    assert(NULL != _qconf_dbf);

    ret = gdbm_delete(_qconf_dbf, gdbm_key);
    if (0 != ret && GDBM_ITEM_NOT_FOUND != gdbm_errno)
    {
        LOG_ERR_KEY_INFO(tblkey, "Failed to gdbm delete! gdbm err:%s",
                gdbm_strerror(gdbm_errno));

        pthread_mutex_unlock(&_qconf_dump_mutx);
        return QCONF_ERR_DEL_DUMP;
    }
    gdbm_sync(_qconf_dbf);
    pthread_mutex_unlock(&_qconf_dump_mutx);

    return QCONF_OK;
}

int qconf_dump_clear()
{
    pthread_mutex_lock(&_qconf_dump_mutx);
    if (NULL != _qconf_dbf)
    {
        gdbm_close(_qconf_dbf);
        _qconf_dbf = NULL;
    }
    int ret = qconf_init_dbf_(GDBM_NEWDB);
    pthread_mutex_unlock(&_qconf_dump_mutx);
    
    return ret;
}

int qconf_dump_tbl(qhasharr_t *tbl)
{
    if (NULL == tbl) return QCONF_ERR_PARAM;

    string tblkey;
    string tblval;
    int ret = QCONF_OK;
    int max_slots = 0, used_slots = 0;

    // get key count
    hash_tbl_get_count(tbl, max_slots, used_slots);

    for (int idx = 0; idx < max_slots;)
    {
        ret = hash_tbl_getnext(tbl, tblkey, tblval, idx);

        if (QCONF_OK == ret) 
        {
            // write dump
            ret = qconf_dump_set(tblkey, tblval);
            if (QCONF_OK != ret) break;
        }
        else if (QCONF_ERR_TBL_END == ret)
        {
            ret = QCONF_OK;
        }
        else
        {
            LOG_ERR("Failed to hash_tbl_getnext!");
        }
    }

    return ret;
}
