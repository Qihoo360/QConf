#include <errno.h>
#include <limits.h>
#include <regex.h>
#include <string.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <sys/time.h>
#include <unistd.h>
#include <time.h>
#include <stdlib.h>
#include <algorithm>
#include <string>
#include <sstream>
#include "qlibc.h"
#include "qconf_zk.h"
#include "qconf_format.h"

#include <iostream>

// Error number
#define QCONF_ERR_OTHER                     -1
#define QCONF_ERR_MEM                       2 
#define QCONF_ERR_OUT_OF_RANGE              20              
#define QCONF_ERR_NOT_NUMBER                21
#define QCONF_ERR_OTHRE_CHARACTER           22

// Node type on zookeeper
#define QCONF_NODE_TYPE_NODE                '2'
#define QCONF_NODE_TYPE_SERVICE             '3'

// Data type in serialized content
#define QCONF_DATA_TYPE_NODE                '2'
#define QCONF_DATA_TYPE_SERVICE             '3'

#define QCONF_GET_RETRIES                   3
#define QCONF_MAX_VALUE_SIZE                1048577
#define QCONF_SERV_GROUP_PREFIX             "/qconf_monitor_lock_node/default_instance/md5_list"
#define QCONF_MD5_SIZE                      32

// Gray related
#define QCONF_NOTIFY_CLIENT_PREFIX          "/qconf/__qconf_notify/client"
#define QCONF_NOTIFY_CONTENT_PREFIX         "/qconf/__qconf_notify/content"
#define QCONF_NOTIFY_BACKLINK_PREFIX        "/qconf/__qconf_notify/backlink"
#define QCONF_NOTIFY_BACKLINK_DELIM         ';'
#define QCONF_GRAY_CONTENT_MAX_SIZE         102400 //must be less than QCONF_MAX_VALUE_SIZE

#ifndef __STRING_VECTOR_T_FLAGS__
#define __STRING_VECTOR_T_FLAGS__
typedef struct String_vector string_vector_t;
#endif


int QConfZK::zk_init(const std::string &host)
{
    if (host.empty()) return QCONF_ERR_PARAM;
    zkLog = fopen("/dev/null", "a");  
    if (NULL != zkLog)  
    {  
        zoo_set_log_stream(zkLog);  
    }  
    zh = zookeeper_init(host.c_str(), NULL, 29999, NULL, NULL, 0);
    if (NULL == zh) return QCONF_ERR_ZOO_FAILED;
    return QCONF_OK;
}

void QConfZK::zk_close()   
{
    zookeeper_close(zh);
    if (NULL != zkLog)
    {
        zoo_set_log_stream(NULL);  
        fclose(zkLog);
        zkLog = NULL;
    }
}

int QConfZK::zk_create(const std::string &path, const std::string &value)
{
    for (int i = 0; i < QCONF_GET_RETRIES; ++i)
    {
        int ret = zoo_create(zh, path.c_str(), value.c_str(), value.size(), &ZOO_OPEN_ACL_UNSAFE, 0, NULL, -1);
        switch (ret)
        {
            case ZOK:
                return QCONF_OK;
            case ZNODEEXISTS:
                return QCONF_ERR_ZOO_ALREADY_EXIST;
            case ZNONODE:
                ret = zk_create(parent_node(path), "");
                if (QCONF_OK != ret && QCONF_ERR_ZOO_ALREADY_EXIST != ret) return ret;
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

int QConfZK::zk_delete_exist(const std::string &path)
{
    for (int i = 0; i < QCONF_GET_RETRIES; ++i)
    {
        int ret = zoo_delete(zh, path.c_str(), -1);
        switch (ret)
        {
            case ZOK:
                return QCONF_OK;
            case ZNONODE:
                return QCONF_ERR_ZOO_NOT_EXIST;
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

int QConfZK::zk_delete(const std::string &path)
{
    int ret = QCONF_ERR_OTHER;
    if (QCONF_ERR_ZOO_NOT_EXIST == (ret = zk_delete_exist(path)))
        ret = QCONF_OK;
    return ret;
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
    switch (ret = zk_list(path, old_servs))
    {
        case QCONF_OK:
            break;
        case QCONF_ERR_ZOO_NOT_EXIST:
            ret = zk_create(path, "");
            if (QCONF_OK != ret) return ret;
            break;
        default:
            return ret;
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
 * Up one service
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
 * Down one service
 */
int QConfZK::zk_service_down(const std::string &node, const std::string &serv)
{
    int ret = QCONF_ERR_OTHER;
    std::string path;
    if (NULL == zh || QCONF_OK != zk_path(node, path)) return QCONF_ERR_PARAM;
    
    char data_type = QCONF_NODE_TYPE_NODE;
    if (QCONF_OK != (ret = check_node_type(path, data_type))) return ret;
    if (QCONF_NODE_TYPE_NODE == data_type) return QCONF_ERR_NODE_TYPE;

    return zk_modify(serv_path(path, serv), integer_to_string(STATUS_DOWN));
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
    if (QCONF_OK != (ret = check_node_type(path, data_type))) return QCONF_ERR_PARAM;
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
            if (QCONF_OK != ret) return QCONF_ERR_ZOO_FAILED;
            servs.insert(std::pair<std::string, char>(*it, s));
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
            children.insert(std::pair<std::string, std::string>(*it, value));
        }
    }
    return ret;
}

int QConfZK::zk_service_status_get(const std::string &path, char &status)
{
    std::string buf;
    if (QCONF_OK == zk_get(path, buf))
    {
        long value = -1;
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
    int ret = zk_create(get_monitor_node(path), path);
    if (QCONF_ERR_ZOO_ALREADY_EXIST == ret) ret = QCONF_OK;
    return ret;
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
        return QCONF_ERR_PARAM;
    }
    
    status = regexec(&reg, path.c_str(), nmatch, pmatch, 0);
    if (0 != status)
    {
        regerror(status, &reg, errbuf, sizeof(errbuf));
        regfree(&reg);
        return QCONF_ERR_PARAM;
    }
    regfree(&reg);

    return path_normalize(path, new_path);
}

int QConfZK::path_normalize(const std::string &path, std::string &new_path)
{
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
    if (QCONF_OK != zk_exist(get_monitor_node(path), exist)) return QCONF_ERR_ZOO_FAILED;   
    node_type = exist ? QCONF_NODE_TYPE_SERVICE : QCONF_NODE_TYPE_NODE;
    return QCONF_OK;
}

bool QConfZK::check_service(const std::string &path, const char status)
{
    if (path.empty()) return false;
    switch (status)
    {
        case STATUS_UP:
        case STATUS_DOWN:
        case STATUS_OFFLINE:
            return true;
        default:
            return false;
    }
}

/**
 * Begin gray begin
 */
int QConfZK::zk_gray_begin(const std::map<std::string, std::string> &raw_nodes, 
        const std::vector<std::string> &clients, std::string &gray_id)
{
    std::map<std::string, std::string> nodes;
    if (QCONF_OK != zk_gray_check_nodes(raw_nodes, nodes) || QCONF_OK != zk_gray_check_chients(clients)) 
        return QCONF_ERR_PARAM;

    // Generate gray id
    std::string tmp_gray_id = gray_generate_id();
    
    // Serialize to gray content
    std::string content;
    if (QCONF_OK != gray_serialize_content(nodes, content)) return QCONF_ERR_GRAY_SERIALIZE;
    
    // Create content nodes
    std::string content_node_pfx(QCONF_NOTIFY_CONTENT_PREFIX);
    unsigned int node_num = 0, index = 0;
    do
    {
        std::string current_content(content, index, QCONF_GRAY_CONTENT_MAX_SIZE);
        std::string content_node = content_node_pfx + "/" + tmp_gray_id + "_" + integer_to_string(node_num++);
        if (QCONF_OK != zk_create(content_node, current_content)) return QCONF_ERR_GRAY_SET_CONTENT;
        
        index += QCONF_GRAY_CONTENT_MAX_SIZE;

    } while (index < content.size());
    
    // Create client nodes
    std::string client_node_pfx(QCONF_NOTIFY_CLIENT_PREFIX);
    std::string bl_content;
    for (std::vector<std::string>::const_iterator it = clients.begin(); it != clients.end(); ++it)
    {
        if (QCONF_OK != zk_create(client_node_pfx + "/" + (*it), tmp_gray_id)) 
            return QCONF_ERR_GRAY_SET_CLIENTS;
        bl_content += (*it) + QCONF_NOTIFY_BACKLINK_DELIM;
    }

    // Create backlink nodes
    std::string backlink_node_pfx(QCONF_NOTIFY_BACKLINK_PREFIX);
    if (QCONF_OK != zk_create(backlink_node_pfx + "/" + tmp_gray_id, bl_content)) 
        return QCONF_ERR_GRAY_SET_BACKLINK;
    
    gray_id = tmp_gray_id;

    return QCONF_OK;
}

/**
 * Rollback gray rollback
 */
int QConfZK::zk_gray_rollback(const std::string &gray_id)
{
    // Delete notify node
    return zk_gray_delete_notify(gray_id);
}

/**
 * Commit gray commit
 */
int QConfZK::zk_gray_commit(const std::string &gray_id)
{
    // Get notify content
    std::vector<std::pair<std::string, std::string> > nodes;
    if (QCONF_OK != zk_gray_get_content(gray_id, nodes)) return QCONF_ERR_GRAY_GET_CONTENT;
    
    // Modify znode with new value
    std::vector<std::pair<std::string, std::string> >::iterator it;
    for (it = nodes.begin(); it != nodes.end(); ++it)
    {
        if (QCONF_OK != zk_node_set((*it).first, (*it).second)) 
            return QCONF_ERR_GRAY_COMMIT;
    }
    
    // Delete notify node
    return zk_gray_delete_notify(gray_id);
}

/**
 * Get all gray content nodes
 */
int QConfZK::zk_gray_get_content(const std::string &gray_id, std::vector<std::pair<std::string, std::string> > &nodes)
{
    std::string content_node_pfx(QCONF_NOTIFY_CONTENT_PREFIX);
    content_node_pfx += "/" + gray_id;

    // Get the serialized content
    std::string scontent;
    int index = 0, ret = QCONF_ERR_OTHER;
    do
    {
        std::string tmp_content, 
            content_node = content_node_pfx + "_" + integer_to_string(index++);
        switch (ret = zk_get(content_node, tmp_content))
        {
            case QCONF_OK:
                scontent += tmp_content;
            case QCONF_ERR_ZOO_NOT_EXIST:
                break;
            default:
                return ret;
        }
    } while(QCONF_OK == ret);

    // Deserialize to gray nodes
    if (QCONF_OK != gray_deserialize_content(scontent, nodes)) return QCONF_ERR_OTHER;
    
    return QCONF_OK;
}

/**
 * Delete all related nodes for indicated gray_id
 */
int QConfZK::zk_gray_delete_notify(const std::string &gray_id)
{
    // Delete clients
    std::string backlink_node_pfx(QCONF_NOTIFY_BACKLINK_PREFIX);
    std::string client_node_pfx(QCONF_NOTIFY_CLIENT_PREFIX);
    std::string bl_content;
    if (QCONF_OK != zk_get(backlink_node_pfx + "/" + gray_id, bl_content))
        return QCONF_ERR_GRAY_GET_BACKLINK;

    std::stringstream ss(bl_content);
    std::string item;
    while (std::getline(ss, item, QCONF_NOTIFY_BACKLINK_DELIM)) 
    {
        if (QCONF_OK != zk_delete(client_node_pfx + "/" + item))
            return QCONF_ERR_GRAY_DELETE_CLIENTS;
    }

    // Delete content nodes
    std::string content_node_pfx(QCONF_NOTIFY_CONTENT_PREFIX);
    content_node_pfx += "/" + gray_id;
    
    int index = 0, ret = QCONF_ERR_OTHER;
    while(QCONF_OK == (ret = zk_delete_exist(content_node_pfx + "_" + integer_to_string(index))))
    {
        ++index;
    }

    if (QCONF_ERR_ZOO_NOT_EXIST != ret) return QCONF_ERR_GRAY_DELETE_CONTENT;

    // Delete backlink node
    if (QCONF_OK != zk_delete(backlink_node_pfx + "/" + gray_id))
        return QCONF_ERR_GRAY_DELETE_BACKLINK;

    return QCONF_OK;
}

/**
 * Check wether all nodes is exists
 */
int QConfZK::zk_gray_check_nodes(const std::map<std::string, std::string> &nodes, 
        std::map<std::string, std::string> &new_nodes)
{
    if (nodes.empty()) return QCONF_ERR_PARAM;
    
    new_nodes.clear();
    std::map<std::string, std::string>::const_iterator it;
    for (it = nodes.begin(); it != nodes.end(); ++it)
    {
        std::string zpath;
        bool exist = false;
        
        if (QCONF_OK != path_normalize((*it).first, zpath)               || /* path error */
                QCONF_OK != zk_exist(zpath, exist)              || /* check exist failed*/
                !exist                                          || /* path not exist */
                ((*it).second).size() > QCONF_MAX_VALUE_SIZE - 100)        /* value to large */ 
        {
            new_nodes.clear();
            return QCONF_ERR_OTHER;   
        }

        new_nodes.insert(std::pair<std::string, std::string>(zpath, (*it).second));
    }

    return QCONF_OK;
}

/**
 * Check wether the clients are already in gray process
 */
int QConfZK::zk_gray_check_chients(const std::vector<std::string> &clients)
{
    if (clients.empty()) return QCONF_ERR_PARAM;
    
    std::string client_node_pfx(QCONF_NOTIFY_CLIENT_PREFIX);
    for (std::vector<std::string>::const_iterator it = clients.begin(); it != clients.end(); ++it)
    {
        bool exist = true;
        if (QCONF_OK != zk_exist(client_node_pfx + "/" + (*it), exist)      ||
                exist)
        {
            return QCONF_ERR_OTHER;
        }
           
    }
    return QCONF_OK;
}

/**
 * Generate gray id
 */
std::string QConfZK::gray_generate_id()
{
    // Generate id
    std::string id;
    struct timeval tv = {0};
    gettimeofday(&tv, NULL);
    std::stringstream ss;
    ss << "GRAYID" << tv.tv_sec << "-" << tv.tv_usec;
    ss >> id;
    return id;
}

/**
 * Serialize into gray content
 */
int QConfZK::gray_serialize_content(const std::map<std::string, std::string> &nodes, std::string &content)
{
    if (nodes.empty()) return QCONF_ERR_PARAM;

    std::set<std::string> graynodes;
    std::map<std::string, std::string>::const_iterator it;
    for (it = nodes.begin(); it != nodes.end(); ++it)
    {
        std::string tblkey, tblval;
        serialize_to_tblkey(QCONF_DATA_TYPE_NODE, "idc", (*it).first, tblkey);
        nodeval_to_tblval(tblkey, (*it).second, tblval);
        graynodes.insert(tblval);
    }
    
    return graynodeval_to_tblval(graynodes, content);
}

/**
 * Deserialize from gray content
 */
int QConfZK::gray_deserialize_content(const std::string &content, std::vector<std::pair<std::string, std::string> > &nodes)
{
    if (content.empty()) return QCONF_ERR_PARAM;

    std::set<std::string> vnodes;
    if (QCONF_OK != tblval_to_graynodeval(content, vnodes)) return QCONF_ERR_OTHER;

    for (std::set<std::string>::const_iterator i = vnodes.begin(); i != vnodes.end(); ++i)
    {
        std::string gray_path, gray_idc, gray_val;
        tblval_to_nodeval((*i), gray_val, gray_idc, gray_path);
        nodes.push_back(std::pair<std::string, std::string>(gray_path, gray_val));
    }

    return QCONF_OK;
}
