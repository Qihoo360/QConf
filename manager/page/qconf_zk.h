#ifndef QCONF_ZK_H
#define QCONF_ZK_H

#include <string>
#include <set>
#include <map>
#include <zookeeper.h>

#define QCONF_OK                            0
#define QCONF_ERR_OTHER                     -1
#define QCONF_ERR_PARAM                     1
#define QCONF_ERR_ZOO_FAILED                61

#define QCONF_ERR_ZOO_NOTEMPTY              100
#define QCONF_ERR_ZOO_UNINITIAL             101
#define QCONF_ERR_ZOO_NOT_EXIST             102
#define QCONF_ERR_NODE_TYPE                 110

// server status define
#define STATUS_UNKNOWN                      -1
#define STATUS_UP                           0
#define STATUS_OFFLINE                      1
#define STATUS_DOWN                         2

#define QCONF_GET_RETRIES                   3
#define QCONF_MAX_VALUE_SIZE                1048577

// error of number
#define QCONF_ERR_OUT_OF_RANGE              20
#define QCONF_ERR_NOT_NUMBER                21
#define QCONF_ERR_OTHRE_CHARACTER           22

#define QCONF_NODE_TYPE_NODE                '2'
#define QCONF_NODE_TYPE_SERVICE             '3'

#define QCONF_SERV_GROUP_PREFIX             "/qconf_monitor_lock_node/default_instance/md5_list"

#define QCONF_MD5_SIZE                      32

class QConfZK
{
    public:
        QConfZK():zh(NULL){}

        /**
         * Init zookeeper
         */
        int zk_init(const std::string &host);

        /**
         * Destory zookeeper environment
         */
        void zk_close();

        /**
         * Set node value, add if not exist
         */
        int zk_node_set(const std::string &node, const std::string &value);

        /**
         * Delete node
         */
        int zk_node_delete(const std::string &node);

        /**
         * Set services, add if not exit 
         */
        int zk_services_set(const std::string &node, const std::map<std::string, char> &servs);

        /**
         * Add one service
         */
        int zk_service_add(const std::string &node, const std::string &serv, const char &status);

        /**
         * Delete one sevice
         */
        int zk_service_delete(const std::string &node, const std::string &serv);

        /**
         * Delete one sevice
         */
        int zk_service_clear(const std::string &node);

        /**
         * Upline one service
         */
        int zk_service_up(const std::string &node, const std::string &serv);

        /**
         * Offline one service
         */
        int zk_service_offline(const std::string &node, const std::string &serv);

        /**
         *  Get conf from zookeeper
         */
        int zk_node_get(const std::string &node, std::string &buf);

        /**
         *  Get all services 
         */
        int zk_services_get(const std::string &node, std::set<std::string> &servs);

        /**
         *  Get all services together with their status
         */
        int zk_services_get_with_status(const std::string &node, std::map<std::string, char> &servs);

        /**
         * List all children nodes 
         */
        int zk_list(const std::string &node, std::set<std::string> &children);

        /**
         * List all children nodes together with their values
         */
        int zk_list_with_values(const std::string &node, std::map<std::string, std::string> &children);


        /**
         * check path, return QCONF_OK if correct and new_path is the formated path
         */
        int zk_path(const std::string &path, std::string &new_path);

    private:
        zhandle_t *zh;
        int zk_service_status_get(const std::string &path, char &status);
        int zk_create(const std::string &path, const std::string &value);
        int zk_modify(const std::string &path, const std::string &value);
        int zk_delete(const std::string &path);
        int zk_exist(const std::string &path, bool &exist);
        int zk_get(const std::string &path, std::string &value);
        int zk_monitor_node_create(const std::string &path);
        int zk_monitor_node_remove(const std::string &path);
        int check_node_type(const std::string &path, char &node_type);
        bool check_service(const std::string &path, char status);

        std::string parent_node(const std::string &path);
        std::string serv_path(const std::string &node, const std::string serv);
        std::string get_monitor_node(const std::string &path);
        int string_to_integer(const std::string &cnt, long &integer);
        std::string integer_to_string(const long &integer);
};


#endif
