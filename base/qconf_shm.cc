#include <errno.h>
#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>
#include <string.h>
#include <pthread.h>
#include <sys/ipc.h>
#include <sys/shm.h>
#include <sys/types.h>
#include <sys/file.h>

#include <vector>
#include <string>
#include <map>
#include <list>

#include "qconf_log.h"
#include "qconf_shm.h"
#include "qlibc/qlibc.h"
#include "qconf_common.h"
#include "qconf_format.h"

#define NEED_MD5_TBLLEN 1024
#define USE_MIXED_VERIFY

using namespace std;

// share memory set and remove lock
static pthread_mutex_t _qhasharr_op_mutex = PTHREAD_MUTEX_INITIALIZER;

static int hash_tbl_get_(qhasharr_t *tbl, const string &key, string &val);
static int hash_tbl_set_(qhasharr_t *tbl, const string &key, const string &val);
int maxSlotsNum = 0;
    
void qconf_destroy_qhasharr_lock()
{
    pthread_mutex_destroy(&_qhasharr_op_mutex);
}

int qconf_get_localidc(qhasharr_t *tbl, string &local_idc)
{
    if (NULL == tbl) return QCONF_ERR_PARAM;
    string idc, path, tblkey, tblval;
    serialize_to_tblkey(QCONF_DATA_TYPE_LOCAL_IDC, idc, path, tblkey);

    int ret = hash_tbl_get(tbl, tblkey, tblval);
    if (QCONF_OK == ret) ret = tblval_to_localidc(tblval, local_idc);

    return ret;
}

int qconf_update_localidc(qhasharr_t *tbl, const string &local_idc)
{
    if (NULL == tbl || local_idc.empty()) return QCONF_ERR_PARAM;
    string idc, path, tblkey, tblval;
    serialize_to_tblkey(QCONF_DATA_TYPE_LOCAL_IDC, idc, path, tblkey);

    int ret = localidc_to_tblval(tblkey, local_idc, tblval);
    if (QCONF_OK == ret) ret = hash_tbl_set(tbl, tblkey, tblval);
    ret = (QCONF_ERR_SAME_VALUE == ret) ? QCONF_OK : ret;
    return ret;
}

int qconf_exist_tblkey(qhasharr_t *tbl, const string &key, bool &status)
{
    if (NULL == tbl || key.empty()) return QCONF_ERR_PARAM;

    string val;

    int ret = hash_tbl_get(tbl, key, val);
    switch (ret)
    {
        case QCONF_OK:
            status = true;
            break;
        case QCONF_ERR_NOT_FOUND:
            status = false;
            break;
        default:
            return ret;
    }
    return QCONF_OK;
}

int create_hash_tbl(qhasharr_t *&tbl, key_t shmkey, mode_t mode)
{
    int shmid = -1;
    size_t memsize = 0;
    void* shmptr = NULL;

    memsize = qhasharr_calculate_memsize(maxSlotsNum);

    shmid = shmget(shmkey, memsize, IPC_CREAT | IPC_EXCL | mode);
    if (-1 == shmid)
    {
        if (EEXIST == errno)
        {
            return init_hash_tbl(tbl, shmkey, mode, 0);
        }

        LOG_FATAL_ERR("Failed to create share memory of key:%#x! errno:%d",
                  shmkey, errno);
        return QCONF_ERR_SHMGET;
    }

    shmptr = shmat(shmid, NULL, 0);
    if ((void*)-1 == shmptr)
    {
        LOG_FATAL_ERR("Failed to shmat key:%#x! errno:%d",
                  shmkey, errno);
        return QCONF_ERR_SHMAT;
    }
    
    tbl = qhasharr(shmptr, memsize);
    if (NULL == tbl)
    {
        LOG_FATAL_ERR("Failed to init shm of shmid:%d errno:%d", shmid, errno);
        return QCONF_ERR_SHMINIT;
    }

    return QCONF_OK;
}

int init_hash_tbl(qhasharr_t *&tbl, key_t shmkey, mode_t mode, int flags)
{
    int shmid = -1;

    shmid = shmget(shmkey, 0, mode);
    if (-1 == shmid) return QCONF_ERR_SHMGET;

    tbl = (qhasharr_t*)shmat(shmid, NULL, flags);
    if ((void*)-1 == (void*)tbl)
    {
        tbl = NULL;
        return QCONF_ERR_SHMAT;
    }
    return QCONF_OK;
}

int hash_tbl_get_count(qhasharr_t *tbl, int &max_slots, int &used_slots)
{
    if (NULL == tbl) return -1;
    return qhasharr_size(tbl, &max_slots, &used_slots);
}

static int hash_tbl_get_(qhasharr_t *tbl, const string &key, string &val) 
{
    if (key.empty()) return QCONF_ERR_PARAM;

    char *val_tmp = NULL;
    size_t val_tmp_len = 0;

    val_tmp = (char*)qhasharr_get(tbl, key.data(), key.size(), &val_tmp_len);
    if (NULL == val_tmp) return QCONF_ERR_NOT_FOUND;

    val.assign(val_tmp, val_tmp_len);
    free(val_tmp);
    val_tmp = NULL;

    return QCONF_OK;
}

int hash_tbl_get(qhasharr_t *tbl, const string &key, string &val)
{
    if (NULL == tbl || key.empty()) return QCONF_ERR_PARAM;

    int ret = hash_tbl_get_(tbl, key, val);
    if (QCONF_OK != ret)
        return ret;

    return qconf_verify(val);
}

#ifdef USE_MIXED_VERIFY
int qconf_verify(string &tblval)
{
    size_t tblval_size = tblval.size();
    QCONF_VALUE_SIZE_TYPE val_size = 0;
    const char *valstr = NULL, *veristr = NULL;
    char val_md5[QCONF_MD5_INT_LEN] = {0};

    qconf_decode_num(tblval.data(), val_size, QCONF_VALUE_SIZE_TYPE);
    valstr = tblval.data() + QCONF_VALUE_SIZE_LEN;
    veristr = valstr + val_size;

    // verify MD5 code
    if (val_size > NEED_MD5_TBLLEN)
    {
        if (tblval_size < QCONF_VALUE_SIZE_LEN + val_size + QCONF_MD5_INT_LEN)
            return QCONF_ERR_TBL_DATA_MESS;
        qhashmd5(valstr, val_size, val_md5);

        if (0 == memcmp(val_md5, veristr, QCONF_MD5_INT_LEN))
        {
            tblval.assign(valstr, val_size);
            return QCONF_OK;
        }
    }
    // verify original value
    else
    {
        if (tblval_size < QCONF_VALUE_SIZE_LEN + val_size * 2)
            return QCONF_ERR_TBL_DATA_MESS;
        if (0 == memcmp(valstr, veristr, val_size))
        {
            tblval.assign(valstr, val_size);
            return QCONF_OK;
        }
    }

    return QCONF_ERR_TBL_DATA_MESS;
}
#else
int qconf_verify(string &val)
{
    if (val.size() < QCONF_MD5_INT_LEN)
        return QCONF_ERR_TBL_DATA_MESS;

    char val_md5[QCONF_MD5_INT_LEN] = {0};

    qhashmd5(val.data(), val.size() - QCONF_MD5_INT_LEN, val_md5);

    if (0 == memcmp(val_md5, val.data() + val.size() - QCONF_MD5_INT_LEN, QCONF_MD5_INT_LEN))
    {
        val.resize(val.size() - QCONF_MD5_INT_LEN);
        return QCONF_OK;
    }

    return QCONF_ERR_TBL_DATA_MESS;
}
#endif

static int hash_tbl_set_(qhasharr_t *tbl, const string &key, const string &val)
{
    if (key.empty()) return QCONF_ERR_PARAM;
    pthread_mutex_lock(&_qhasharr_op_mutex);
    bool ret = qhasharr_put(tbl, key.data(), key.size(), val.data(), val.size());
    pthread_mutex_unlock(&_qhasharr_op_mutex);

    while (!ret && errno == ENOBUFS) {
        string removeKey = (LRU::getInstance())->getRemoveKey();
        errno = 0;
        bool removeRet = hash_tbl_remove(tbl, removeKey);
        if (removeRet == QCONF_OK) {
            LRU::getInstance()->removeKey();
        }
        else {
            LOG_ERR("remove key from shared memory failed");
            break;
        }
        pthread_mutex_lock(&_qhasharr_op_mutex);
        ret = qhasharr_put(tbl, key.data(), key.size(), val.data(), val.size());
        pthread_mutex_unlock(&_qhasharr_op_mutex);
    }
    if (ret) {
        LRU::getInstance()->visitKey(key);
    }

    return ret ? QCONF_OK : QCONF_ERR_TBL_SET;
}

int hash_tbl_set(qhasharr_t *tbl, const string &key, const string &val)
{
    if (NULL == tbl || key.empty()) return QCONF_ERR_PARAM;

    string val_tmp;
    string val_in_mem;
    int ret = QCONF_OK;
    char val_md5[QCONF_MD5_INT_LEN] = {0};

    ret = hash_tbl_get(tbl, key, val_in_mem);

    if (QCONF_OK == ret && 0 == val.compare(val_in_mem))
        return QCONF_ERR_SAME_VALUE;

#ifdef USE_MIXED_VERIFY
    /*        __________
     *       |          |
     *       v          |
     *  | value len | value | verification code |
     */
    QCONF_VALUE_SIZE_TYPE val_size = val.size();
    char buf[QCONF_VALUE_SIZE_LEN] = {0};
    qconf_encode_num(buf, val_size, QCONF_VALUE_SIZE_TYPE);
    val_tmp.assign(buf, QCONF_VALUE_SIZE_LEN);
    val_tmp.append(val);

    // Use MD5 as verification code
    if (val_size > NEED_MD5_TBLLEN) {
        qhashmd5(val.data(), val_size, val_md5);
        val_tmp.append(val_md5, QCONF_MD5_INT_LEN);
    }
    // Use original value as verification code
    else
        val_tmp += val;
#else
    qhashmd5(val.data(), val.size(), val_md5);

    val_tmp.assign(val);
    val_tmp.append(val_md5, QCONF_MD5_INT_LEN);
#endif

    ret = hash_tbl_set_(tbl, key, val_tmp);

    return ret;
}

bool hash_tbl_exist(qhasharr_t *tbl, const string &key)
{
    if (NULL == tbl || key.empty()) return false;
    return qhasharr_exist(tbl, key.data(), key.size());
}

int hash_tbl_getnext(qhasharr_t *tbl, string &tblkey, string &tblval, int &idx)
{
    if (NULL == tbl) return QCONF_ERR_PARAM; 
    qnobj_t obj;
    char data_type;
    int ret = QCONF_OK;
    string idc, path, host;

    memset(&obj, 0, sizeof(qnobj_t));
    bool status = qhasharr_getnext(tbl, &obj, &idx);
    if (!status)
    {
        idx++;
        if (ENOENT == errno && idx == (tbl->maxslots+1)) return QCONF_ERR_TBL_END; 

        LOG_ERR("Failed to qhasharr_getnext! idx:%d; errno:%d", idx-1, errno);
        return QCONF_ERR_NOT_FOUND;
    }

    tblval.assign((char*)obj.data, obj.data_size);
    ret = qconf_verify(tblval);
    if (QCONF_OK != ret)
    {
        free(obj.name);
        free(obj.data);
        return ret;
    }

    tblkey.assign(obj.name, obj.name_size);
    data_type = get_data_type(tblkey);

    // get idc and path
    if (QCONF_DATA_TYPE_NODE == data_type)
    {
        string nodeval;
        ret = tblval_to_nodeval(tblval, nodeval, idc, path);
        if (QCONF_OK != ret)
            LOG_ERR("Failed to get nodeval! idx:%d", idx-1);
    }
    else if (QCONF_DATA_TYPE_SERVICE == data_type)
    {
        string_vector_t chdnodes;
        memset(&chdnodes, 0, sizeof(string_vector_t));
        ret = tblval_to_chdnodeval(tblval, chdnodes, idc, path);
        if (QCONF_OK != ret)
            LOG_ERR("Failed to get children service nodes! idx:%d", idx-1);

        if (chdnodes.count > 0) free_string_vector(chdnodes, chdnodes.count);
    }
    else if (QCONF_DATA_TYPE_BATCH_NODE == data_type)
    {
        string_vector_t batchnodes;
        memset(&batchnodes, 0, sizeof(string_vector_t));
        ret = tblval_to_batchnodeval(tblval, batchnodes, idc, path);
        if (QCONF_OK != ret)
            LOG_ERR("Failed to get batch nodes! idx:%d", idx-1);

        if (batchnodes.count > 0) free_string_vector(batchnodes, batchnodes.count);
    }
    else if (QCONF_DATA_TYPE_ZK_HOST == data_type)
    {
        ret = tblval_to_idcval(tblval, host, idc);
        if (QCONF_OK != ret)
            LOG_ERR("Failed to get host of idc! idx:%d", idx-1);
    }
    else if (QCONF_DATA_TYPE_LOCAL_IDC == data_type)
        ret = QCONF_OK;
    else
        ret = QCONF_ERR_DATA_TYPE;

    if (QCONF_OK == ret) serialize_to_tblkey(data_type, idc, path, tblkey);

    free(obj.name);
    free(obj.data);

    return ret;
}

int hash_tbl_remove(qhasharr_t *tbl, const string &key)
{
    if (NULL == tbl || key.empty()) return QCONF_ERR_PARAM;

    pthread_mutex_lock(&_qhasharr_op_mutex);
    bool ret = qhasharr_remove(tbl, key.data(), key.size());
    pthread_mutex_unlock(&_qhasharr_op_mutex);

    if (!ret) return (ENOENT == errno) ? QCONF_OK : QCONF_ERR_OTHER;

    return QCONF_OK;
}

int hash_tbl_clear(qhasharr_t *tbl)
{
    if (NULL == tbl) return QCONF_ERR_PARAM;
    
    pthread_mutex_lock(&_qhasharr_op_mutex);
    qhasharr_clear(tbl);
    pthread_mutex_unlock(&_qhasharr_op_mutex);

    return QCONF_OK;
}

LRU* LRU::lruInstance = NULL;

LRU::LRU() {
    lruMem.clear();
    keyToIterator.clear();
}

LRU::~LRU() {
    delete lruInstance;
    lruInstance = NULL;
}

LRU* LRU::getInstance() {
    if (!lruInstance) {
        lruInstance = new LRU();
    }
    return lruInstance;
}

string LRU::getRemoveKey() {
    if (lruMem.empty()) {
        LOG_ERR("Memory is empty nothing to remove. Maybe it's too small");
        return "";
    }
    return lruMem.back();
}

string LRU::removeKey() {
    string key = lruMem.back();
    lruMem.pop_back();
    if (keyToIterator.find(key) != keyToIterator.end()) {
        keyToIterator.erase(key);
    }

    return key;
}

void LRU::visitKey(string key) {
    if (key == QCONF_KEY_TYPE_LOCAL_IDC) {
        return;
    }
    if (keyToIterator.find(key) == keyToIterator.end()) {
        lruMem.push_front(key);
        keyToIterator[key] = lruMem.begin();
    }
    else {
        list<string>::iterator it = keyToIterator[key];
        lruMem.erase(it);
        lruMem.push_front(key);
        keyToIterator[key] = lruMem.begin();
    }
    return;
}


bool LRU::initLruMem(qhasharr_t* tbl) {
    int max_slots = 0, used_slots = 0;
    hash_tbl_get_count(tbl, max_slots, used_slots);

    string tblkey, tblval;

    for (int idx = 0; idx < max_slots;) {
        int ret = hash_tbl_getnext(tbl, tblkey, tblval, idx);
        if (ret == QCONF_OK) {

            char data_type;
            string idc, path;
            deserialize_from_tblkey(tblkey, data_type, idc, path);
            if (!path.empty()) {
                visitKey(tblkey);
            }

        }
        else if (QCONF_ERR_TBL_END == ret){}
        else{
            LOG_ERR_KEY_INFO(tblkey, "Failed to get next item in shmtbl");
            return false;
        }
    }
    return true;
}

