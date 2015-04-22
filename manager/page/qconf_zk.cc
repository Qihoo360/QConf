#include <errno.h>
#include <limits.h>
#include <regex.h>
#include <string.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <unistd.h>
#include <algorithm>
#include <string>
#include <sstream>
#include <iostream>
#include "qlibc.h"
#include "qconf_zk.h"


int QConfZK::zk_init(const std::string &host)
{
    if (host.empty()) return QCONF_ERR_PARAM;
    zh = zookeeper_init(host.c_str(), NULL, 30000, NULL, NULL, 0);
    if (NULL == zh) return QCONF_ERR_ZOO_FAILED;
    return QCONF_OK;
}

void QConfZK::zk_close()   
{
    zookeeper_close(zh);
}

int QConfZK::zk_create(const std::string &path, const std::string &value)
{
    for (int i = 0; i < QCONF_GET_RETRIES; ++i)
    {
        int ret = zoo_create(zh, path.c_str(), value.c_str(), value.size(), &ZOO_OPEN_ACL_UNSAFE, 0, NULL, -1);
        switch (ret)
        {
            case ZOK:
            case ZNODEEXISTS:
                return QCONF_OK;
            case ZNONODE:
                ret = zk_create(parent_node(path), "");
                if (QCONF_OK != ret) return QCONF_ERR_ZOO_FAILED;
                return zk_create(path, value);
            case ZINVALIDSTATE:
            case ZMARSHALLINGERROR:
                continue;
            default:
                return QCONF_ERR_ZOO_FAILED;
        }
    }
    return QCONF_ERR_ZOO_FAILED;
}

int QConfZK::zk_exist(const std::string &path, bool &exist)
{
    exist = false;
    for (int i = 0; i < QCONF_GET_RETRIES; ++i)
    {
        int ret = zoo_exists(zh, path.c_str(), 0, NULL);
        switch (ret)
        {
            case ZOK:
                exist = true;
            case ZNONODE:
                return QCONF_OK;
            case ZINVALIDSTATE:
            case ZMARSHALLINGERROR:
                continue;
            default:
                return QCONF_ERR_ZOO_FAILED;
        }
    }
    return QCONF_ERR_ZOO_FAILED;
}

int QConfZK::zk_modify(const std::string &path, const std::string &value)
{
    for (int i = 0; i < QCONF_GET_RETRIES; ++i)
    {
        int ret = zoo_set(zh, path.c_str(), value.c_str(), value.size(), -1);
        switch (ret)
        {
            case ZOK:
                return QCONF_OK;
            case ZNONODE:
                return QCONF_ERR_ZOO_NOT_EXIST;
            case ZINVALIDSTATE:
            case ZMARSHALLINGERROR:
                continue;
            default:
                return QCONF_ERR_ZOO_FAILED;
        }
    }
    return QCONF_ERR_ZOO_FAILED;
}

int QConfZK::zk_get(const std::string &path, std::string &value)
{
    for (int i = 0; i < QCONF_GET_RETRIES; ++i)
    {
        char buffer[QCONF_MAX_VALUE_SIZE];
        int buffer_len = QCONF_MAX_VALUE_SIZE;
        int ret = zoo_get(zh, path.c_str(), 0, buffer, &buffer_len, NULL);
        switch (ret)
        {
            case ZOK:
                if (-1 == buffer_len) buffer_len = 0;
                value.assign(buffer, buffer_len);
                return QCONF_OK;
            case ZNONODE:
                return QCONF_ERR_ZOO_NOT_EXIST;
            case ZINVALIDSTATE:
            case ZMARSHALLINGERROR:
                continue;
            default:
                return QCONF_ERR_ZOO_FAILED;
        }
    }
    return QCONF_ERR_ZOO_FAILED;
}

int QConfZK::zk_delete(const std::string &path)
{
    //add force delete
    for (int i = 0; i < QCONF_GET_RETRIES; ++i)
    {
        int ret = zoo_delete(zh, path.c_str(), -1);
        switch (ret)
        {
            case ZOK:
            case ZNONODE:
                return QCONF_OK;
            case ZNOTEMPTY:
                return QCONF_ERR_ZOO_NOTEMPTY;
            case ZINVALIDSTATE:
            case ZMARSHALLINGERROR:
                continue;
            default:
                return QCONF_ERR_ZOO_FAILED;
        }
    }
    return QCONF_ERR_ZOO_FAILED;
}


int QConfZK::zk_node_delete(const std::string &node)
{
    std::string path;
    if (NULL == zh || QCONF_OK != zk_path(node, path)) return QCONF_ERR_PARAM;
    
    return zk_delete(path);
}

/**
 * Set node value, add if not exist
 */
int QConfZK::zk_node_set(const std::string &node, const std::string &value)
{
    std::string path;
    if (NULL == zh || QCONF_OK != zk_path(node, path)) return QCONF_ERR_PARAM;
    int ret = zk_modify(path, value);
    if (QCONF_ERR_ZOO_NOT_EXIST == ret)
        return zk_create(path, value); //create node if not exists

    return ret;
}

/**
 * Set services, add if not exit 
 */
int QConfZK::zk_services_set(const std::string &node, const std::map<std::string, char> &servs)
{
    //parameter check
    std::string path;
    if (NULL == zh || QCONF_OK != zk_path(node, path) || servs.empty()) return QCONF_ERR_PARAM;
    std::map<std::string, char>::const_iterator it = servs.begin();
    for (; it != servs.end(); ++it)
    {
        if (!check_service(it->first, it->second)) return QCONF_ERR_PARAM;
    }

    //set serivice monitor node path
    if (QCONF_OK != zk_monitor_node_create(path)) return QCONF_ERR_ZOO_FAILED;

    //modify or add new services, and get services exists on zk but not in new services union
    int ret = 0;
    std::set<std::string> old_servs, new_servs, delete_servs;
    switch (zk_list(path, old_servs))
    {
        case QCONF_OK:
            break;
        case QCONF_ERR_ZOO_NOT_EXIST:
            if (QCONF_OK != zk_create(path, "")) return QCONF_ERR_OTHER;
            break;
        default:
            return QCONF_ERR_OTHER;
    
    }
    
    for (it = servs.begin(); it !=servs.end(); ++it)
    {
        // add or modify service
        ret = zk_service_add(path, it->first, it->second);
        if (QCONF_OK != ret) return ret;
        new_servs.insert(it->first);
    }

    //get services need to be deleted
    std::set_difference(old_servs.begin(), old_servs.end(), new_servs.begin(), new_servs.end(), 
            std::inserter(delete_servs, delete_servs.begin()));

    //delete old service
    std::set<std::string>::iterator sit;
    for (sit = delete_servs.begin(); sit != delete_servs.end(); ++sit)
    {
        std::string child_path = serv_path(path, *sit);
        ret = zk_delete(child_path);
        if (QCONF_OK != ret) return ret;
    }
    return ret;
}

/**
 * Add one service
 */
int QConfZK::zk_service_add(const std::string &node, const std::string &serv, const char &status)
{
    std::string path;
    if (NULL == zh || QCONF_OK != zk_path(node, path) || !check_service(serv, status)) return QCONF_ERR_PARAM;

    //set serivice monitor node path
    if (QCONF_OK != zk_monitor_node_create(path)) return QCONF_ERR_ZOO_FAILED;
    
    std::string spath = serv_path(path, serv);
    std::string sstatus = integer_to_string(status);
    int ret = zk_modify(spath, sstatus);
    if (QCONF_ERR_ZOO_NOT_EXIST == ret)
        return zk_create(spath, sstatus);
    return ret;
}

/**
 * Delete one sevice
 */
int QConfZK::zk_service_delete(const std::string &node, const std::string &serv)
{
    int ret = QCONF_ERR_OTHER;
    std::string path;
    if (NULL == zh ||QCONF_OK != zk_path(node, path)) return QCONF_ERR_PARAM;
    
    char data_type = QCONF_NODE_TYPE_NODE;
    if (QCONF_OK != (ret = check_node_type(path, data_type))) return ret;
    if (QCONF_NODE_TYPE_NODE == data_type) return QCONF_ERR_NODE_TYPE;

    return zk_delete(serv_path(path, serv));
}

/**
 * Clear sevices
 */
int QConfZK::zk_service_clear(const std::string &node)
{
    std::string path;
    if (NULL == zh || QCONF_OK != zk_path(node, path)) return QCONF_ERR_PARAM;
    
    int ret = 0;
    char data_type = QCONF_NODE_TYPE_NODE;
    if (QCONF_OK != (ret = check_node_type(path, data_type))) return ret;
    if (QCONF_NODE_TYPE_NODE == data_type) return QCONF_ERR_NODE_TYPE;

    std::set<std::string> servs;
    ret = zk_list(path, servs);
    if (QCONF_OK != ret) return ret;
    
    std::string child_path;
    std::set<std::string>::iterator it = servs.begin();
    for(; it != servs.end(); ++it)
    {
        child_path = serv_path(path, *it);
        ret = zk_delete(child_path);
        if (QCONF_OK != ret) return ret;
    }
    return QCONF_OK;
}

/**
 * Upline one service
 */
int QConfZK::zk_service_up(const std::string &node, const std::string &serv)
{
    int ret = QCONF_ERR_OTHER;
    std::string path;
    if (NULL == zh || QCONF_OK != zk_path(node, path)) return QCONF_ERR_PARAM;
    
    char data_type = QCONF_NODE_TYPE_NODE;
    if (QCONF_OK != (ret = check_node_type(path, data_type))) return ret;
    if (QCONF_NODE_TYPE_NODE == data_type) return QCONF_ERR_NODE_TYPE;

    return zk_modify(serv_path(path, serv), integer_to_string(STATUS_UP));
}

/**
 * Offline one service
 */
int QConfZK::zk_service_offline(const std::string &node, const std::string &serv)
{
    int ret = QCONF_ERR_OTHER;
    std::string path;
    if (NULL == zh || QCONF_OK != zk_path(node, path)) return QCONF_ERR_PARAM;
    
    char data_type = QCONF_NODE_TYPE_NODE;
    if (QCONF_OK != (ret = check_node_type(path, data_type))) return ret;
    if (QCONF_NODE_TYPE_NODE == data_type) return QCONF_ERR_NODE_TYPE;

    return zk_modify(serv_path(path, serv), integer_to_string(STATUS_OFFLINE));
}

/**
 *  Get conf from zookeeper
 */
int QConfZK::zk_node_get(const std::string &node, std::string &buf)
{
    std::string path;

    if (NULL == zh || QCONF_OK != zk_path(node, path)) return QCONF_ERR_PARAM;

    return zk_get(path, buf);
}

/**
 *  Get all services 
 */
int QConfZK::zk_services_get(const std::string &node, std::set<std::string> &servs)
{
    int ret = QCONF_ERR_OTHER;
    std::string path;
    if (NULL == zh || QCONF_OK != zk_path(node, path)) return QCONF_ERR_PARAM;

    char data_type = QCONF_NODE_TYPE_NODE;
    if (QCONF_OK != (ret = check_node_type(path, data_type))) return ret;
    if (QCONF_NODE_TYPE_NODE == data_type) return QCONF_ERR_NODE_TYPE;
    
    return zk_list(path, servs);
}

/**
 *  Get all services together with their status
 */
int QConfZK::zk_services_get_with_status(const std::string &node, std::map<std::string, char> &servs)
{
    int ret = QCONF_ERR_OTHER;
    std::string path;
    if (NULL == zh || QCONF_OK != zk_path(node, path)) return QCONF_ERR_PARAM;

    char data_type = QCONF_NODE_TYPE_NODE;
    if (QCONF_OK != (ret = check_node_type(path, data_type))) return ret;
    if (QCONF_NODE_TYPE_NODE == data_type) return QCONF_ERR_NODE_TYPE;
    
    std::set<std::string> nodes;
    ret = zk_services_get(path, nodes);
    if (QCONF_OK == ret)
    {
        std::string child_path;
        std::set<std::string>::iterator it = nodes.begin();
        for (; it != nodes.end(); ++it)
        {
            child_path = serv_path(path, *it);
            char s = 0;
            ret = zk_service_status_get(child_path, s);
            if (QCONF_OK != ret) return QCONF_ERR_OTHER;
            servs.insert(std::make_pair(*it, s));
        }
    }
    return ret;
}

/**
 * List all children nodes 
 */
int QConfZK::zk_list(const std::string &node, std::set<std::string> &children)
{
    std::string path;
    if (NULL == zh || QCONF_OK != zk_path(node, path)) return QCONF_ERR_PARAM;
    int ret;
    String_vector chdnodes;
    memset(&chdnodes, 0, sizeof(chdnodes));
    children.clear();

    for (int i = 0; i < QCONF_GET_RETRIES; ++i)
    {
        ret = zoo_get_children(zh, path.c_str(), 0, &chdnodes);
        switch(ret)
        {
            case ZOK:
                for (int j = 0; j < chdnodes.count; ++j)
                {
                    children.insert(chdnodes.data[j]);
                }
                deallocate_String_vector(&chdnodes);
                return QCONF_OK;
            case ZNONODE:
                return QCONF_ERR_ZOO_NOT_EXIST;
            case ZINVALIDSTATE:
            case ZMARSHALLINGERROR:
                continue;
            default:
                return QCONF_ERR_ZOO_FAILED;
        }
    }

    return QCONF_ERR_ZOO_FAILED;
}

/**
 * List all children nodes together with their values
 */
int QConfZK::zk_list_with_values(const std::string &node, std::map<std::string, std::string> &children)
{
    std::string path;
    if (NULL == zh || QCONF_OK != zk_path(node, path)) return QCONF_ERR_PARAM;
    
    std::set<std::string> nodes;
    int ret = zk_list(path, nodes);
    if (QCONF_OK == ret)
    {
        std::string child_path;
        std::set<std::string>::iterator it;
        for (it = nodes.begin(); it != nodes.end(); ++it)
        {
            child_path = path + '/' + *it;
            std::string value;
            ret = zk_get(child_path, value);
            if (QCONF_OK != ret) return ret;
            children.insert(std::make_pair(*it, value));
        }
    }
    return ret;
}

int QConfZK::zk_service_status_get(const std::string &path, char &status)
{
    std::string buf;
    if (QCONF_OK == zk_get(path, buf))
    {
        long value = STATUS_UNKNOWN;
        if(QCONF_OK != string_to_integer(buf, value)) return QCONF_ERR_OTHER;
        switch(value)
        {
            case STATUS_UP:
            case STATUS_DOWN:
            case STATUS_OFFLINE:
                status = static_cast<char>(value);
                break;
            default:          
                return QCONF_ERR_OTHER;
        }
    }
    else
    {
        return QCONF_ERR_OTHER;
    }
    return QCONF_OK;
}

/**
 * Judge whether the content can be converted to integer
 */
int QConfZK::string_to_integer(const std::string &cnt, long &integer)
{
    if(cnt.empty()) return QCONF_ERR_PARAM;
    errno = 0;
    char *endptr = NULL;
    long value = strtol(cnt.c_str(), &endptr, 0);

    //over range
	if ((LONG_MAX == value || LONG_MIN == value) && ERANGE == errno)
        return QCONF_ERR_OUT_OF_RANGE;
    //Illegal number
    if (cnt == endptr || (0 == value && EINVAL == errno))
        return QCONF_ERR_NOT_NUMBER;
    //further characters exists
    if ('\0' != *endptr) return QCONF_ERR_OTHRE_CHARACTER;

    integer = value;
    return QCONF_OK;
}

inline std::string QConfZK::integer_to_string(const long &integer)
{
    std::stringstream ss;
    ss << integer;
    return ss.str();
}

/**
 * Get the parent node
 */
inline std::string QConfZK::parent_node(const std::string &path)
{
    std::string parent(path, 0, path.find_last_of('/'));
    return parent;
}

/**
 * Get the parent node
 */
inline std::string QConfZK::serv_path(const std::string &node, const std::string serv)
{
    return node + "/" + serv; 
}

/**
 * Set monitor node for services
 */
inline int QConfZK::zk_monitor_node_create(const std::string &path)
{
    return  zk_create(get_monitor_node(path), path);
}

/**
 * Remove monitor node for services
 */
inline int QConfZK::zk_monitor_node_remove(const std::string &path)
{
    return  zk_delete(get_monitor_node(path));
}

/**
 * Genereate the monitor node path, all service node should has been register the monitor node
 */
std::string QConfZK::get_monitor_node(const std::string &path)
{
    unsigned char md5hash[16] = {0};
    qhashmd5(path.c_str(), path.size(), md5hash);

    char md5ascii[QCONF_MD5_SIZE] = {0};
    qhashmd5_bin_to_hex(md5ascii, md5hash, 16);

    std::string new_path(QCONF_SERV_GROUP_PREFIX);
    new_path.append("/");
    new_path.append(md5ascii, sizeof(md5ascii));
    return new_path;
}

int QConfZK::zk_path(const std::string &path, std::string &new_path)
{
    if (path.empty()) return QCONF_ERR_PARAM;
    // check path format TODO check multi slash
    int status = 0;
    regex_t reg = {0};
    regmatch_t pmatch[1];
    const size_t nmatch = 1;
    char errbuf[1024] = {0};
    int cflags = REG_EXTENDED | REG_NOSUB;
    const char * pattern = "^[A-Za-z0-9_:/\\.\\-]+$";

    status = regcomp(&reg, pattern, cflags);
    if (0 != status)
    {
        regerror(status, &reg, errbuf, sizeof(errbuf));
        return QCONF_ERR_OTHER;
    }
    
    status = regexec(&reg, path.c_str(), nmatch, pmatch, 0);
    if (0 != status)
    {
        regerror(status, &reg, errbuf, sizeof(errbuf));
        regfree(&reg);
        return QCONF_ERR_OTHER;
    }
    regfree(&reg);

    //add slash at begin and remove slash at end
    new_path.assign("/");
    size_t pos_begin, pos_end;
    pos_begin = path.find_first_not_of("/");
    pos_end = path.find_last_not_of("/");
    if (path.npos == pos_end) return QCONF_ERR_OTHER; //root
    new_path.append(path.substr(pos_begin, pos_end - pos_begin + 1));
    return QCONF_OK;
}

int QConfZK::check_node_type(const std::string &path, char &node_type)
{
    bool exist = false;
    if (QCONF_OK != zk_exist(get_monitor_node(path), exist)) return QCONF_ERR_OTHER;   
    node_type = exist ? QCONF_NODE_TYPE_SERVICE : QCONF_NODE_TYPE_NODE;
    return QCONF_OK;
}

bool QConfZK::check_service(const std::string &path, const char status)
{
    if (path.empty()) return false;
    switch (status)
    {
        case STATUS_UNKNOWN:
        case STATUS_UP:
        case STATUS_DOWN:
        case STATUS_OFFLINE:
            return true;
        default:
            return false;
    }
}
