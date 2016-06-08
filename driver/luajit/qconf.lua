--[[
-- 1. current qconf just supports luajit
--
-- 2. before the program, please make sure that libqconf.so has been installed
--
--]]

local ffi = require('ffi')
-- local qconf = ffi.load("/absolute/libqconf.so")
local qconf = ffi.load("libqconf.so")

ffi.cdef[[
struct string_vector
{
    int count;      // the number of services
    char **data;    // the array of services
};
typedef struct string_vector string_vector_t;

typedef struct qconf_node
{
    char *key;
    char *value;
} qconf_node;

typedef struct qconf_batch_nodes
{
    int count;
    qconf_node *nodes;
} qconf_batch_nodes;

int qconf_init();
int qconf_destroy();
int qconf_get_conf(const char *path, char *buf, int buf_len, const char *idc);
int qconf_get_batch_conf(const char *path, qconf_batch_nodes *bnodes, const char *idc);
int qconf_get_batch_keys(const char *path, string_vector_t *nodes, const char *idc);

int init_string_vector(string_vector_t *nodes);
int destroy_string_vector(string_vector_t *nodes);
int init_qconf_batch_nodes(qconf_batch_nodes *bnodes);
int destroy_qconf_batch_nodes(qconf_batch_nodes *bnodes);
int qconf_get_allhost(const char *path, string_vector_t *nodes, const char *idc);
int qconf_get_host(const char *path, char *buf, int buf_len, const char *idc);

const char* qconf_version();
]]

-- define the max length of conf buffer which is 1M
-- define the max length of one ip and port which is 256
local qconf_conf_buf_max_len = 1048576
local qconf_host_buf_max_len = 256

-- define qconf lua version
local qconf_version = "1.0.0"

-- define the qconf errors flags
local qconf_errors = {
    ["-1"]     = 'QCONF_ERR_OTHER',
    ["0"]      = 'QCONF_OK',
    ["1"]      = 'QCONF_ERR_PARAM',
    ["2"]      = 'QCONF_ERR_MEM',
    ["3"]      = 'QCONF_ERR_TBL_SET',
    ["4"]      = 'QCONF_ERR_GET_HOST',
    ["5"]      = 'QCONF_ERR_GET_IDC',
    ["6"]      = 'QCONF_ERR_BUF_NOT_ENOUGH',
    ["7"]      = 'QCONF_ERR_DATA_TYPE',
    ["8"]      = 'QCONF_ERR_DATA_FORMAT',
    ["9"]      = 'QCONF_ERR_NULL_VALUE',
    -- key is not found in hash table
    ["10"]     = 'QCONF_ERR_NOT_FOUND',
    ["11"]     = 'QCONF_ERR_OPEN_DUMP',
    ["12"]     = 'QCONF_ERR_OPEN_TMP_DUMP',
    ["13"]     = 'QCONF_ERR_NOT_IN_DUMP',
    ["14"]     = 'QCONF_ERR_RENAME_DUMP',
    ["15"]     = 'QCONF_ERR_WRITE_DUMP',
    ["16"]     = 'QCONF_ERR_SAME_VALUE',
    -- error of number
    ["20"]     = 'QCONF_ERR_OUT_OF_RANGE',
    ["21"]     = 'QCONF_ERR_NOT_NUMBER',
    ["22"]     = 'QCONF_ERR_OTHRE_CHARACTER',
    -- error of ip and port
    ["30"]     = 'QCONF_ERR_INVALID_IP',
    ["31"]     = 'QCONF_ERR_INVALID_PORT',
    -- error of msgsnd and msgrcv
    ["40"]     = 'QCONF_ERR_NO_MESSAGE',
    ["41"]     = 'QCONF_ERR_E2BIG',
    -- error of hostname
    ["71"]     = 'QCONF_ERR_HOSTNAME',
    -- error, not init while using qconf c library
    ["81"]     = 'QCONF_ERR_CC_NOT_INIT',
}

-- init the qconf environment
local ret = qconf.qconf_init()
if ret ~= 0 then
    os.exit(-1)
end

-- qconf interned buff
-- Lua string is an (interned) copy of the data and bears no relation to the original data area anymore. 
-- http://luajit.org/ext_ffi_api.html#ffi_string
local buff = ffi.new("char[?]", qconf_conf_buf_max_len, {0})

-- qconf get_conf function
local function get_conf(key, idc)
    local ret = qconf.qconf_get_conf(key, buff, qconf_conf_buf_max_len, idc)

    if ret == 0 then
        return ret, ffi.string(buff)
    else
        return ret, qconf_errors[tostring(ret)]
    end
end

-- qconf get_allhost function
local function get_allhost(key, idc)
    local nodes = ffi.new("string_vector_t")
    local ret = qconf.init_string_vector(nodes)
    if ret ~= 0 then
        return ret, qconf_errors[tostring(ret)]
    end

    ret = qconf.qconf_get_allhost(key, nodes, idc)
    if ret ~= 0 then
        return ret, qconf_errors[tostring(ret)]
    end

    local arr = {}

    for v = 0, nodes.count - 1 do
        arr[v + 1] = ffi.string(nodes.data[v])
    end

    qconf.destroy_string_vector(nodes)

    return ret, arr
end

-- qconf get_host function
local function get_host(key, idc)
    local ret = qconf.qconf_get_host(key, buff, qconf_host_buf_max_len, idc)

    if ret == 0 then
        return ret, ffi.string(buff)
    else
        return ret, qconf_errors[tostring(ret)]
    end
end

-- qconf get_batch_conf function
local function get_batch_conf(key, idc)
    local bnodes = ffi.new("qconf_batch_nodes")
    local ret = qconf.init_qconf_batch_nodes(bnodes)
    if ret ~= 0 then
        return ret, qconf_errors[tostring(ret)]
    end

    ret = qconf.qconf_get_batch_conf(key, bnodes, idc)
    if ret ~= 0 then
        return ret, qconf_errors[tostring(ret)]
    end

    local arr = {}

    for v = 0, bnodes.count - 1 do
        arr[ffi.string(bnodes.nodes[v].key)] = ffi.string(bnodes.nodes[v].value)
    end

    qconf.destroy_qconf_batch_nodes(bnodes)

    return ret, arr
end

-- qconf get_batch_keys function
local function get_batch_keys(key, idc)
    local nodes = ffi.new("string_vector_t")
    local ret = qconf.init_string_vector(nodes)
    if ret ~= 0 then
        return ret, qconf_errors[tostring(ret)]
    end

    ret = qconf.qconf_get_batch_keys(key, nodes, idc)
    if ret ~= 0 then
        return ret, qconf_errors[tostring(ret)]
    end

    local arr = {}

    for v = 0, nodes.count - 1 do
        arr[v + 1] = ffi.string(nodes.data[v])
    end

    qconf.destroy_string_vector(nodes)

    return ret, arr
end

-- qconf version
local function version()
    return 0, qconf_version
end

return {
    get_conf = get_conf,
    get_allhost = get_allhost,
    get_host = get_host,
    get_batch_conf = get_batch_conf,
    get_batch_keys = get_batch_keys,
    version = version
}
