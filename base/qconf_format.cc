#include <stdio.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>
#include <stdarg.h>

#include <string>
#include <vector>

#include "qconf_common.h"
#include "qconf_format.h"

using namespace std;

static int tblval_to_vectorval(const string &tblval, char data_type, string_vector_t &nodes, string &idc, string &path);

static void qconf_append_idc(string &tblkey, const string &idc);
static void qconf_append_path(string &tblkey, const string &path);
static void qconf_append_host(string &tblkey, const string &host);
static void qconf_append_nodeval(string &tblkey, const string &nodeval);

static int qconf_sub_idc(const string &tblkey, size_t &pos, string &idc);
static int qconf_sub_path(const string &tblkey, size_t &pos, string &path);
static int qconf_sub_host(const string &tblkey, size_t &pos, string &host);
static int qconf_sub_nodeval(const string &tblkey, size_t &pos, string &nodeval);
static int qconf_sub_vectorval(const string &tblval, size_t &pos, string_vector_t &nodes);

int serialize_to_tblkey(char data_type, const string &idc, const string &path, string &tblkey)
{
    tblkey.assign(1, data_type);
    switch (data_type)
    {
    case QCONF_DATA_TYPE_NODE:
    case QCONF_DATA_TYPE_SERVICE:
    case QCONF_DATA_TYPE_BATCH_NODE:
        qconf_append_idc(tblkey, idc);
        qconf_append_path(tblkey, path);
        return QCONF_OK;
    case QCONF_DATA_TYPE_ZK_HOST:
        qconf_append_idc(tblkey, idc);
        return QCONF_OK;
    case QCONF_DATA_TYPE_LOCAL_IDC:
        return QCONF_OK;
    default:
        return QCONF_ERR_DATA_TYPE;
    }
}

int deserialize_from_tblkey(const string &tblkey, char &data_type, string &idc, string &path)
{
    size_t pos = 0;

    if (tblkey.size() <= 0) return QCONF_ERR_DATA_FORMAT;
    data_type = tblkey[0];

    pos = 1;

    switch (data_type)
    {
        case QCONF_DATA_TYPE_NODE:
        case QCONF_DATA_TYPE_SERVICE:
        case QCONF_DATA_TYPE_BATCH_NODE:
            if (QCONF_OK != qconf_sub_idc(tblkey, pos, idc) ||
                    QCONF_OK != qconf_sub_path(tblkey, pos, path)) 
                return QCONF_ERR_DATA_FORMAT;
            return QCONF_OK;
        case QCONF_DATA_TYPE_ZK_HOST:
            if (QCONF_OK != qconf_sub_idc(tblkey, pos, idc))
                return QCONF_ERR_DATA_FORMAT;
            return QCONF_OK;
        case QCONF_DATA_TYPE_LOCAL_IDC:
            return QCONF_OK;
        default:
            return QCONF_ERR_DATA_TYPE;
    }
}

int localidc_to_tblval(const string &key, const string &local_idc, string &tblval)
{
    tblval.clear();
    qconf_append_idc(tblval, local_idc);
    tblval.append(key);

    return QCONF_OK;
}

int nodeval_to_tblval(const string &key, const string &nodeval, string &tblval)
{
    tblval.clear();
    qconf_append_nodeval(tblval, nodeval);
    tblval.append(key);

    return QCONF_OK;
}

int chdnodeval_to_tblval(const string &key, const string_vector_t &nodes, string &tblval, const vector<char> &valid_flg)
{
    QCONF_VECTOR_COUNT_TYPE valid_cnt = 0;
    QCONF_HOST_PATH_SIZE_TYPE node_size = 0;
    char buf[QCONF_VECTOR_COUNT_LEN] = {0};

    // set total children nodes count
    for (int i = 0; i < nodes.count; ++i)
    {
        if (STATUS_UP == valid_flg[i]) ++valid_cnt;
    }

    tblval.clear();
    qconf_encode_num(buf, valid_cnt, QCONF_VECTOR_COUNT_TYPE);
    tblval.append(buf, QCONF_VECTOR_COUNT_LEN);
    for (int i = 0; i < nodes.count; ++i)
    {
        if (STATUS_UP == valid_flg[i])
        {
            node_size = strlen(nodes.data[i]);
            qconf_encode_num(buf, node_size, QCONF_HOST_PATH_SIZE_TYPE);
            tblval.append(buf, QCONF_HOST_PATH_SIZE_LEN);
            tblval.append(nodes.data[i], node_size);
        }
    }
    tblval.append(key);

    return QCONF_OK;
}

int batchnodeval_to_tblval(const string &key, const string_vector_t &nodes, string &tblval)
{
    QCONF_VECTOR_COUNT_TYPE size = 0;
    char buf[QCONF_VECTOR_COUNT_LEN] = {0};

    tblval.clear();
    size = nodes.count;
    qconf_encode_num(buf, size, QCONF_VECTOR_COUNT_TYPE);
    tblval.append(buf, QCONF_VECTOR_COUNT_LEN);
    for (int i = 0; i < nodes.count; ++i)
    {
        size = strlen(nodes.data[i]);
        qconf_encode_num(buf, size, QCONF_HOST_PATH_SIZE_TYPE);
        tblval.append(buf, QCONF_HOST_PATH_SIZE_LEN);
        tblval.append(nodes.data[i], size);
    }
    tblval.append(key);

    return QCONF_OK;
}

int idcval_to_tblval(const string &key, const string &host, string &tblval)
{
    tblval.clear();
    qconf_append_host(tblval, host);
    tblval.append(key);

    return QCONF_OK;
}

char get_data_type(const string &value)
{
    if (value.empty()) return QCONF_DATA_TYPE_UNKNOWN;
    return value[0];
}

int tblval_to_localidc(const string &tblval, string &idc)
{
    size_t pos = 0;

    // idc
    if (QCONF_OK != qconf_sub_idc(tblval, pos, idc)) return QCONF_ERR_DATA_FORMAT;

    // data type
    if (tblval.size() < pos + 1 || tblval[pos] != QCONF_DATA_TYPE_LOCAL_IDC)
        return QCONF_ERR_DATA_FORMAT;

    return QCONF_OK;
}

int tblval_to_idcval(const string &tblval, string &host)
{
    size_t pos = 0;

    // host
    if (QCONF_OK != qconf_sub_host(tblval, pos, host)) return QCONF_ERR_DATA_FORMAT;
    
    // data type
    if (tblval.size() < pos + 1 || tblval[pos] != QCONF_DATA_TYPE_ZK_HOST)
        return QCONF_ERR_DATA_FORMAT;

    return QCONF_OK;
}

int tblval_to_idcval(const string &tblval, string &host, string &idc)
{
    size_t pos = 0;

    // host
    if (QCONF_OK != qconf_sub_host(tblval, pos, host)) return QCONF_ERR_DATA_FORMAT;

    // data type
    if (tblval.size() < pos + 1 || tblval[pos] != QCONF_DATA_TYPE_ZK_HOST)
        return QCONF_ERR_DATA_FORMAT;
    pos++;

    // idc
    if (QCONF_OK != qconf_sub_idc(tblval, pos, idc)) return QCONF_ERR_DATA_FORMAT;

    return QCONF_OK;
}

int tblval_to_nodeval(const string &tblval, string &nodeval)
{
    size_t pos = 0;
    int ret = QCONF_ERR_OTHER;
    
    // nodeval
    if (QCONF_OK != (ret = qconf_sub_nodeval(tblval, pos, nodeval)))
        return ret;
    
    // data type
    if (tblval.size() < pos + 1 || tblval[pos] != QCONF_DATA_TYPE_NODE)
        return QCONF_ERR_DATA_FORMAT;
    
    return QCONF_OK;
}

int tblval_to_nodeval(const string &tblval, string &nodeval, string &idc, string &path)
{
    size_t pos = 0;
    int ret = QCONF_ERR_OTHER;

    // nodeval
    if (QCONF_OK != (ret = qconf_sub_nodeval(tblval, pos, nodeval)))
        return ret;

    // data type
    if (tblval.size() < pos + 1 || tblval[pos] != QCONF_DATA_TYPE_NODE)
        return QCONF_ERR_DATA_FORMAT;
    pos++;

    // idc, path
    if (QCONF_OK != qconf_sub_idc(tblval, pos, idc) || 
            QCONF_OK != qconf_sub_path(tblval, pos, path))
        return QCONF_ERR_DATA_FORMAT;

    return QCONF_OK;
}

int tblval_to_chdnodeval(const string &tblval, string_vector_t &nodes)
{
    size_t pos = 0;
    int ret = qconf_sub_vectorval(tblval, pos, nodes);
    
    if (QCONF_OK != ret) return ret;
    
    // data type
    if (tblval.size() < pos + 1 || tblval[pos] != QCONF_DATA_TYPE_SERVICE)
        return QCONF_ERR_DATA_FORMAT;
    
    return QCONF_OK;
}

int tblval_to_batchnodeval(const string &tblval, string_vector_t &nodes)
{
    size_t pos = 0;
    int ret = qconf_sub_vectorval(tblval, pos, nodes);
    
    if (QCONF_OK != ret) return ret;
    
    // data type
    if (tblval.size() < pos + 1 || tblval[pos] != QCONF_DATA_TYPE_BATCH_NODE)
        return QCONF_ERR_DATA_FORMAT;

    return QCONF_OK;
}

int tblval_to_chdnodeval(const string &tblval, string_vector_t &nodes, string &idc, string &path)
{
    return tblval_to_vectorval(tblval, QCONF_DATA_TYPE_SERVICE, nodes, idc, path);
}

int tblval_to_batchnodeval(const string &tblval, string_vector_t &nodes, string &idc, string &path)
{
    return tblval_to_vectorval(tblval, QCONF_DATA_TYPE_BATCH_NODE, nodes, idc, path);
}

static int tblval_to_vectorval(const string &tblval, char data_type, string_vector_t &nodes, string &idc, string &path)
{
    size_t pos = 0;
    int ret = QCONF_OK;

    // nodes
    ret = qconf_sub_vectorval(tblval, pos, nodes);
    if (QCONF_OK != ret) return ret;

    // data type
    if (tblval.size() < pos + 1 || tblval[pos] != data_type)
        return QCONF_ERR_DATA_FORMAT;
    pos++;

    // idc
    if (QCONF_OK != qconf_sub_idc(tblval, pos, idc) || 
            QCONF_OK != qconf_sub_path(tblval, pos, path))
        return QCONF_ERR_DATA_FORMAT;

    return QCONF_OK;
}

void serialize_to_idc_host(const string &idc, const string &host, string &dest)
{
    dest.clear();
    qconf_append_idc(dest, idc);
    qconf_append_host(dest, host);
}

int deserialize_from_idc_host(const string &idc_host, string &idc, string &host)
{
    size_t pos = 0;
    if (QCONF_OK != qconf_sub_idc(idc_host, pos, idc) || 
            QCONF_OK != qconf_sub_host(idc_host, pos, host))
        return QCONF_ERR_DATA_FORMAT;

    return QCONF_OK;
}

static void qconf_append_idc(string &tblkey, const string &idc)
{
    qconf_string_append(tblkey, idc, QCONF_IDC_SIZE_TYPE);
}

static void qconf_append_path(string &tblkey, const string &path)
{
    qconf_string_append(tblkey, path, QCONF_HOST_PATH_SIZE_TYPE);
}

static void qconf_append_host(string &tblkey, const string &host)
{
    qconf_string_append(tblkey, host, QCONF_HOST_PATH_SIZE_TYPE);
}

static void qconf_append_nodeval(string &tblkey, const string &nodeval)
{
    qconf_string_append(tblkey, nodeval, QCONF_VALUE_SIZE_TYPE);
}

static int qconf_sub_idc(const string &tblkey, size_t &pos, string &idc)
{
    int ret = QCONF_ERR_OTHER;
    qconf_string_sub(tblkey, pos, idc, QCONF_IDC_SIZE_TYPE, ret);
    return ret;
}

static int qconf_sub_host(const string &tblkey, size_t &pos, string &host)
{
    int ret = QCONF_ERR_OTHER;
    qconf_string_sub(tblkey, pos, host, QCONF_HOST_PATH_SIZE_TYPE, ret);
    return ret;
}

static int qconf_sub_path(const string &tblkey, size_t &pos, string &path)
{
    int ret = QCONF_ERR_OTHER;
    qconf_string_sub(tblkey, pos, path, QCONF_HOST_PATH_SIZE_TYPE, ret);
    return ret;
}

static int qconf_sub_nodeval(const string &tblkey, size_t &pos, string &nodeval)
{
    int ret = QCONF_ERR_OTHER;
    qconf_string_sub(tblkey, pos, nodeval, QCONF_VALUE_SIZE_TYPE, ret);
    return ret;
}

static int qconf_sub_vectorval(const string &tblval, size_t &pos, string_vector_t &nodes)
{
    QCONF_VECTOR_COUNT_TYPE size = 0;

    // nodes
    if (tblval.size() < pos + QCONF_VECTOR_COUNT_LEN)
        return QCONF_ERR_DATA_FORMAT;

    qconf_decode_num(tblval.data() + pos, size, QCONF_VECTOR_COUNT_TYPE);
    nodes.count = size;
    pos += QCONF_VECTOR_COUNT_LEN;
    if (0 == nodes.count)
    {
        nodes.data = NULL;
        return QCONF_OK;
    }

    nodes.data = (char**)calloc(nodes.count, sizeof(char*));
    if (NULL == nodes.data) return QCONF_ERR_MEM;

    for (int i = 0; i < nodes.count; ++i)
    {
        if (tblval.size() < pos + QCONF_HOST_PATH_SIZE_LEN) 
            return QCONF_ERR_DATA_FORMAT;
        qconf_decode_num(tblval.data() + pos, size, QCONF_HOST_PATH_SIZE_TYPE);
        pos += QCONF_HOST_PATH_SIZE_LEN;
        if (tblval.size() < pos + size) return QCONF_ERR_DATA_FORMAT;

        nodes.data[i] = (char*)calloc(size + 1, sizeof(char));
        if (NULL == nodes.data[i])
        {
            free_string_vector(nodes, i);
            return QCONF_ERR_MEM;
        }
        memcpy(nodes.data[i], tblval.data() + pos, size);
        nodes.data[i][size] = '\0';
        pos += size;
    }

    return QCONF_OK;
}

int graynodeval_to_tblval(const set<string> &nodes, string &tblval)
{
    QCONF_VECTOR_COUNT_TYPE size = 0;
    char buf[QCONF_VECTOR_COUNT_LEN] = {0};

    tblval.clear();
    size = nodes.size();
    qconf_encode_num(buf, size, QCONF_VECTOR_COUNT_TYPE);
    tblval.append(buf, QCONF_VECTOR_COUNT_LEN);
    for (set<string>::const_iterator it = nodes.begin(); it != nodes.end(); ++it)
    {
        QCONF_VALUE_SIZE_TYPE len= 0;
        len = (*it).size();
        qconf_encode_num(buf, len, QCONF_VALUE_SIZE_TYPE);
        tblval.append(buf, QCONF_VALUE_SIZE_LEN);
        tblval.append(*it);
    }
    return QCONF_OK;
}

int tblval_to_graynodeval(const string &tblval, set<string> &nodes)
{
    nodes.clear();

    // nodes
    size_t pos = 0;
    QCONF_VECTOR_COUNT_TYPE size = 0;
    if (tblval.size() < pos + QCONF_VECTOR_COUNT_LEN) return QCONF_ERR_OTHER;
    qconf_decode_num(tblval.data() + pos, size, QCONF_VECTOR_COUNT_TYPE);
    pos += QCONF_VECTOR_COUNT_LEN;
    if (0 == size) return QCONF_OK;

    for (int i = 0; i < size; ++i)
    {
        QCONF_VALUE_SIZE_TYPE len = 0;
        if (tblval.size() < pos + QCONF_VALUE_SIZE_LEN) return QCONF_ERR_OTHER;
        qconf_decode_num(tblval.data() + pos, len, QCONF_VALUE_SIZE_TYPE);
        pos += QCONF_VALUE_SIZE_LEN;
        if (tblval.size() < pos + len) return QCONF_ERR_OTHER;

        nodes.insert(tblval.substr(pos, len));
        pos += len;
    }
    return QCONF_OK;
}
