#include <stdio.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>
#include <assert.h>
#include <stdarg.h>

#include <string>
#include <vector>

#include "qconf_log.h"
#include "qconf_common.h"
#include "qconf_format.h"

using namespace std;

#if (BYTE_ORDER == LITTLE_ENDIAN)
#define QCONF_IS_LITTLE_ENDIAN true
#else
#define QCONF_IS_LITTLE_ENDIAN false
#endif

#define qconf_encode_num(buf, num, d_type)\
    if (QCONF_IS_LITTLE_ENDIAN)\
        memcpy(buf, &num, sizeof(d_type)); \
    else\
    {\
        for (size_t _i = 0; _i < sizeof(d_type); ++_i)\
            *(buf + _i) = (num >> _i * 8) & 0xff;\
    }

#define qconf_decode_num(buf, num, d_type)\
    num = 0;\
    if (QCONF_IS_LITTLE_ENDIAN)\
        memcpy(&num, buf, sizeof(d_type));\
    else\
    {\
        for (size_t _i = 0; _i < sizeof(d_type); ++_i)\
            num += (*reinterpret_cast<const uint8_t*>(buf + _i) << _i * 8);\
    }

#define qconf_string_append(dst, src, d_type)\
    {\
        d_type size = src.size();\
        char buf[sizeof(d_type)] = {0};\
        qconf_encode_num(buf, size, d_type);\
        dst.append(buf, sizeof(d_type));\
        dst.append(src);\
    }

#define qconf_string_sub(string, sub_pos, sub_string, d_type)\
    {\
        assert(string.size() >= sub_pos + sizeof(d_type));\
        d_type size = 0;\
        qconf_decode_num(string.data() + sub_pos, size, d_type);\
        sub_pos += sizeof(d_type);\
        assert(string.size() >= sub_pos + size);\
        sub_string.assign(string, sub_pos, size);\
        sub_pos += size;\
    }

static int tblval_to_vectorval(const string &tblval, char data_type, string_vector_t &nodes, string &idc, string &path);

static void qconf_append_idc(string &tblkey, const string &idc);
static void qconf_append_path(string &tblkey, const string &path);
static void qconf_append_host(string &tblkey, const string &host);
static void qconf_append_nodeval(string &tblkey, const string &nodeval);

static void qconf_sub_idc(const string &tblkey, size_t &pos, string &idc);
static void qconf_sub_path(const string &tblkey, size_t &pos, string &path);
static void qconf_sub_host(const string &tblkey, size_t &pos, string &host);
static void qconf_sub_nodeval(const string &tblkey, size_t &pos, string &nodeval);
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

    assert(tblkey.size() > 0);
    data_type = tblkey[0];

    pos = 1;

    switch (data_type)
    {
    case QCONF_DATA_TYPE_NODE:
    case QCONF_DATA_TYPE_SERVICE:
    case QCONF_DATA_TYPE_BATCH_NODE:
        qconf_sub_idc(tblkey, pos, idc);
        qconf_sub_path(tblkey, pos, path);
        return QCONF_OK;
    case QCONF_DATA_TYPE_ZK_HOST:
        qconf_sub_idc(tblkey, pos, idc);
        return QCONF_OK;
    case QCONF_DATA_TYPE_LOCAL_IDC:
        return QCONF_OK;
    default:
        return QCONF_ERR_DATA_FORMAT;
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
    uint16_t valid_cnt = 0;
    uint16_t node_size = 0;
    char buf[sizeof(uint16_t)] = {0};

    // set total children nodes count
    for (int i = 0; i < nodes.count; ++i)
    {
        if (STATUS_UP == valid_flg[i]) ++valid_cnt;
    }

    tblval.clear();
    qconf_encode_num(buf, valid_cnt, uint16_t);
    tblval.append(buf, sizeof(uint16_t));
    for (int i = 0; i < nodes.count; ++i)
    {
        if (STATUS_UP == valid_flg[i])
        {
            node_size = strlen(nodes.data[i]);
            qconf_encode_num(buf, node_size, uint16_t);
            tblval.append(buf, sizeof(uint16_t));
            tblval.append(nodes.data[i], node_size);
        }
    }
    tblval.append(key);

    return QCONF_OK;
}

int batchnodeval_to_tblval(const string &key, const string_vector_t &nodes, string &tblval)
{
    uint16_t size = 0;
    char buf[sizeof(uint16_t)] = {0};

    tblval.clear();
    size = nodes.count;
    qconf_encode_num(buf, size, uint16_t);
    tblval.append(buf, sizeof(uint16_t));
    for (int i = 0; i < nodes.count; ++i)
    {
        size = strlen(nodes.data[i]);
        qconf_encode_num(buf, size, uint16_t);
        tblval.append(buf, sizeof(uint16_t));
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
    assert(!value.empty());
    return value[0];
}

int tblval_to_localidc(const string &tblval, string &idc)
{
    size_t pos = 0;

    // idc
    qconf_sub_idc(tblval, pos, idc);

    // data type
    assert(tblval.size() >= pos + 1);
    assert(tblval[pos] == QCONF_DATA_TYPE_LOCAL_IDC);

    return QCONF_OK;
}

int tblval_to_idcval(const string &tblval, string &host)
{
    size_t pos = 0;

    // host
    qconf_sub_host(tblval, pos, host);
    
    // data type
    assert(tblval.size() >= pos + 1);
    assert(tblval[pos] == QCONF_DATA_TYPE_ZK_HOST);

    return QCONF_OK;
}

int tblval_to_idcval(const string &tblval, string &host, string &idc)
{
    size_t pos = 0;

    // host
    qconf_sub_host(tblval, pos, host);

    // data type
    assert(tblval.size() >= pos + 1);
    assert(tblval[pos] == QCONF_DATA_TYPE_ZK_HOST);
    pos++;

    // idc
    qconf_sub_idc(tblval, pos, idc);

    return QCONF_OK;
}

int tblval_to_nodeval(const string &tblval, string &nodeval)
{
    size_t pos = 0;

    // nodeval
    qconf_sub_nodeval(tblval, pos, nodeval);

    // data type
    assert(tblval.size() >= pos + 1);
    assert(tblval[pos] == QCONF_DATA_TYPE_NODE);
    
    return QCONF_OK;
}

int tblval_to_nodeval(const string &tblval, string &nodeval, string &idc, string &path)
{
    size_t pos = 0;

    // nodeval
    qconf_sub_nodeval(tblval, pos, nodeval);

    // data type
    assert(tblval.size() >= pos + 1);
    assert(tblval[pos] == QCONF_DATA_TYPE_NODE);
    pos++;

    // idc
    qconf_sub_idc(tblval, pos, idc);

    // path
    qconf_sub_path(tblval, pos, path);

    return QCONF_OK;
}

int tblval_to_chdnodeval(const string &tblval, string_vector_t &nodes)
{
    size_t pos = 0;
    int ret = qconf_sub_vectorval(tblval, pos, nodes);
    
    // data type
    assert(tblval.size() >= pos + 1);
    assert(tblval[pos] == QCONF_DATA_TYPE_SERVICE);
    
    return ret;
}

int tblval_to_batchnodeval(const string &tblval, string_vector_t &nodes)
{
    size_t pos = 0;
    int ret = qconf_sub_vectorval(tblval, pos, nodes);
    
    // data type
    assert(tblval.size() >= pos + 1);
    assert(tblval[pos] == QCONF_DATA_TYPE_BATCH_NODE);

    return ret;
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
    assert(tblval.size() >= pos + 1);
    assert(tblval[pos] == data_type);
    pos++;

    // idc
    qconf_sub_idc(tblval, pos, idc);

    // path
    qconf_sub_path(tblval, pos, path);

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
    qconf_sub_idc(idc_host, pos, idc);
    qconf_sub_host(idc_host, pos, host);

    return QCONF_OK;
}

static void qconf_append_idc(string &tblkey, const string &idc)
{
    qconf_string_append(tblkey, idc, uint8_t);
}

static void qconf_append_path(string &tblkey, const string &path)
{
    qconf_string_append(tblkey, path, uint16_t);
}

static void qconf_append_host(string &tblkey, const string &host)
{
    qconf_string_append(tblkey, host, uint16_t);
}

static void qconf_append_nodeval(string &tblkey, const string &nodeval)
{
    qconf_string_append(tblkey, nodeval, uint32_t);
}

static void qconf_sub_idc(const string &tblkey, size_t &pos, string &idc)
{
    qconf_string_sub(tblkey, pos, idc, uint8_t);
}

static void qconf_sub_host(const string &tblkey, size_t &pos, string &host)
{
    qconf_string_sub(tblkey, pos, host, uint16_t);
}

static void qconf_sub_path(const string &tblkey, size_t &pos, string &path)
{
    qconf_string_sub(tblkey, pos, path, uint16_t);
}

static void qconf_sub_nodeval(const string &tblkey, size_t &pos, string &nodeval)
{
    qconf_string_sub(tblkey, pos, nodeval, uint32_t);
}

static int qconf_sub_vectorval(const string &tblval, size_t &pos, string_vector_t &nodes)
{
    uint16_t size = 0;

    // nodes
    assert(tblval.size() >= pos + sizeof(uint16_t));
    qconf_decode_num(tblval.data() + pos, size, uint16_t);
    nodes.count = size;
    pos += sizeof(uint16_t);
    if (0 == nodes.count)
    {
        nodes.data = NULL;
        return QCONF_OK;
    }

    nodes.data = (char**)calloc(nodes.count, sizeof(char*));
    if (NULL == nodes.data)
    {
        LOG_ERR("calloc string_vector_t data failed! count:%d", nodes.count);
        return QCONF_ERR_MEM;
    }

    for (int i = 0; i < nodes.count; ++i)
    {
        assert(tblval.size() >= pos + sizeof(uint16_t));
        qconf_decode_num(tblval.data() + pos, size, uint16_t);
        pos += sizeof(uint16_t);
        assert(tblval.size() >= pos + size);

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

void qconf_print_key_info(const char* file_path, int line_no, const string &tblkey, const char *format, ...)
{
    string idc;
    string path;
    char data_type;
    char buf[QCONF_MAX_BUF_LEN] = {0};

    va_list arg_ptr;
    va_start(arg_ptr, format);
    int n = vsnprintf(buf, sizeof(buf), format, arg_ptr);
    va_end(arg_ptr);

    if (n >= (int)sizeof(buf)) return;

    deserialize_from_tblkey(tblkey, data_type, idc, path);
    snprintf(buf + n, sizeof(buf) - n, "; data type:%c, idc:%s, path:%s",
            data_type, idc.c_str(), path.c_str());
    qconf_print_log(file_path, line_no, QCONF_LOG_ERR, "%s", buf);
}
