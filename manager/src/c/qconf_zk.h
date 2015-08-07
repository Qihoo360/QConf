#ifndef QCONF_ZK_H
#define QCONF_ZK_H

#include <set>
#include <map>
#include <vector>
#include <string>
#include <zookeeper.h>

// Return code
#define QCONF_OK                            0
#define QCONF_ERR_PARAM                     1               /* Ilegal parameter */
#define QCONF_ERR_ZOO_FAILED                61              /* Zookeeper Operation failed */
#define QCONF_ERR_ZOO_NOTEMPTY              100             /* Child node exist on Zookeeper */
#define QCONF_ERR_ZOO_NOT_EXIST             102             /* Zookeeper Node not exist */
#define QCONF_ERR_ZOO_ALREADY_EXIST         103
#define QCONF_ERR_NODE_TYPE                 110             /* Error node type for current Zookeeper operation */

// error: gray level submission
#define QCONF_ERR_GRAY_SERIALIZE            122             /* Error happened when serialize gray content */
#define QCONF_ERR_GRAY_SET_CONTENT          123             /* Error happened when set content nodes */
#define QCONF_ERR_GRAY_SET_CLIENTS          125             /* Error happened when set client nodes */
#define QCONF_ERR_GRAY_SET_BACKLINK         126             /* Error happened when set backlink node */
#define QCONF_ERR_GRAY_GET_CONTENT          127             /* Error happened when get notify content */
#define QCONF_ERR_GRAY_GET_CLIENTS          129             /* Error happened when get clients */
#define QCONF_ERR_GRAY_GET_BACKLINK         130             /* Error happened when get backlink */
#define QCONF_ERR_GRAY_DELETE_CONTENT       131             /* Error happened when delete content nodes */
#define QCONF_ERR_GRAY_DELETE_CLIENTS        133             /* Error happened when delete clinet nodes */
#define QCONF_ERR_GRAY_DELETE_BACKLINK      134             /* Error happened when delete backlink node */
#define QCONF_ERR_GRAY_COMMIT               135             /* Error happened when commit gray content */

// server status define
#define STATUS_UP                           0
#define STATUS_OFFLINE                      1
#define STATUS_DOWN                         2


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
         * Upline one service
         */
        int zk_service_down(const std::string &node, const std::string &serv);
        
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
         * Check path, return QCONF_OK if correct and new_path is the formated path
         */
        int zk_path(const std::string &path, std::string &new_path);

        /**
         * Begin gray commit
         */
        int zk_gray_begin(const std::map<std::string, std::string> &nodes, const std::vector<std::string> &machines, std::string &gray_id);

        /**
         * Rollback gray commit
         */
        int zk_gray_rollback(const std::string &gray_id);
        
        /**
         * Commit gray commit
         */
        int zk_gray_commit(const std::string &gray_id);
        
    private:
        zhandle_t *zh;
        FILE *zkLog;
        int zk_service_status_get(const std::string &path, char &status);
        int zk_create(const std::string &path, const std::string &value);
        int zk_modify(const std::string &path, const std::string &value);
        int zk_delete(const std::string &path);
        int zk_delete_exist(const std::string &path);
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
        int path_normalize(const std::string &path, std::string &new_path);
        
        // Gray related
        int zk_gray_get_content(const std::string &gray_id, std::vector<std::pair<std::string, std::string> > &nodes);
        int zk_gray_delete_notify(const std::string &gray_id);
        int zk_gray_check_nodes(const std::map<std::string, std::string> &raw_nodes, std::map<std::string, std::string> &nodes);
        int zk_gray_check_chients(const std::vector<std::string> &clients);
        std::string gray_generate_id();
        int gray_serialize_content(const std::map<std::string, std::string> &nodes, std::string &content);
        int gray_deserialize_content(const std::string &content, std::vector<std::pair<std::string, std::string> > &nodes);
};

#endif
