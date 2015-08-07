#include <unistd.h>
#include <map>
#include <set>
#include <vector>
#include <string>
#include <sstream>
#include <iostream>

#include "qconf_config.h"
#include "qconf_const.h"
#include "qconf_log.h"
#include "qconf_zoo.h"
#include "qconf_format.h"
#include "qconf_log.h"
#include "qconf_lock.h"
#include "qconf_gray.h"

using namespace std;

static string _notify_node;
static Mutex _gray_nodes_mutex;
static map<string, string> _gray_nodes;

static int read_notify_content(zhandle_t *zh, const string &notify_id, const string &idc, vector< pair<string, string> > &nodes);
static void get_notify_path();
static void gray_nodes_set(const vector< pair<string, string> > &nodes);
static void gray_nodes_get(vector< pair<string, string> > &nodes);
static void gray_nodes_clear();

bool is_notify_node(const string &node_path)
{
    return node_path == _notify_node;
}

bool is_gray_node(const string &mkey, string &mval)
{
    map<string, string>::const_iterator it; 
    bool exist = false;
    _gray_nodes_mutex.Lock();
    it = _gray_nodes.find(mkey);
    if (it != _gray_nodes.end()) 
    {
        exist = true;
        mval.assign(it->second);
    }
    _gray_nodes_mutex.Unlock();
    return exist;
}

int watch_notify_node(zhandle_t *zh)
{
    if (_notify_node.empty()) get_notify_path();
    
    return zk_exists(zh, _notify_node);
}

static void get_notify_path()
{
    char hostname[256] = {0};
    gethostname(hostname, sizeof(hostname));
    _notify_node.assign(QCONF_NOTIFY_CLIENT_PREFIX);
    _notify_node += string("/") + hostname;
}

int gray_process(zhandle_t *zh, const string &idc, vector< pair<string, string> > &nodes)
{
    if (NULL == zh || idc.empty()) return QCONF_ERR_PARAM;

    // notify node => notify id
    if (_notify_node.empty()) get_notify_path();
    
    string notify_id;
    int ret = QCONF_ERR_OTHER;
    switch (zk_get_node(zh, _notify_node, notify_id, 1))
    {
        case QCONF_OK:
            if (QCONF_OK != read_notify_content(zh, notify_id, idc, nodes))
            {
                LOG_ERR("Failed to get notify node content! notify_id:%s", notify_id.c_str());
                return QCONF_ERR_GRAY_GET_NOTIFY_CONTENT;
            }
            gray_nodes_set(nodes);
            break;
        case QCONF_NODE_NOT_EXIST:
            ret = watch_notify_node(zh); //watch the notify node 
            if (QCONF_OK != ret && QCONF_NODE_NOT_EXIST != ret)
            {
                LOG_FATAL_ERR("Failed to set watcher for notify node!");
            }
            gray_nodes_get(nodes);
            gray_nodes_clear();
            break;
        default:
            LOG_ERR("Failed to get notify node id! notify_node:%s", _notify_node.c_str());
            return QCONF_ERR_GRAY_GET_NOTIFY_NODE;
    }
    return QCONF_OK;
}

static int read_notify_content(zhandle_t *zh, const string &notify_id, const string &idc, vector< pair<string, string> > &nodes)
{
    if (notify_id.empty()) return QCONF_ERR_PARAM;

    string content_node_pfx(QCONF_NOTIFY_CONTENT_PREFIX), content;
    content_node_pfx += "/" + notify_id;

    // Get the serialized content
    int index = 0, ret = QCONF_ERR_OTHER;
    do 
    {
        stringstream ss;
        ss << index++;
        string content_node = content_node_pfx + "_" + ss.str();

        string tmp_content;
        switch (ret = zk_get_node(zh, content_node, tmp_content, 0))
        {
            case QCONF_OK:
                content += tmp_content;
            case QCONF_NODE_NOT_EXIST:
                break;
            default:
                LOG_ERR("Failed to get notify content! content_node:%s", content_node.c_str());
                return QCONF_ERR_OTHER;
        }
    } while(QCONF_OK == ret);

    // Parse notify content into map
    set<string> vnodes;
    if (QCONF_OK != tblval_to_graynodeval(content, vnodes)) return QCONF_ERR_OTHER;

    for (set<string>::iterator i = vnodes.begin(); i != vnodes.end(); ++i)
    {
        string gray_path, gray_idc, gray_val, tblkey, tblval;
        if (QCONF_OK != tblval_to_nodeval((*i), gray_val, gray_idc, gray_path))
        {
            LOG_ERR("Illegal nodfiy content! content_node_pfx:%s", content_node_pfx.c_str());
            return QCONF_ERR_OTHER;
        }

        tblkey.clear();
        serialize_to_tblkey(QCONF_DATA_TYPE_NODE, idc, gray_path, tblkey); 

        tblval.clear();
        nodeval_to_tblval(tblkey, gray_val, tblval);
        nodes.push_back(pair<string, string>(tblkey, tblval));
    }

    return QCONF_OK;
}

static void gray_nodes_set(const vector< pair<string, string> > &nodes)
{
    _gray_nodes_mutex.Lock();
    for (vector< pair<string, string> >::const_iterator it = nodes.begin(); it != nodes.end(); ++it)
    {
        _gray_nodes.insert(pair<string, string>((*it).first, (*it).second));
    }
    _gray_nodes_mutex.Unlock();
}

static void gray_nodes_get(vector< pair<string, string> > &nodes)
{
    _gray_nodes_mutex.Lock();
    for (map<string, string>::const_iterator it = _gray_nodes.begin(); it != _gray_nodes.end(); ++it)
    {
        nodes.push_back(pair<string, string>(it->first, it->second));
    }
    _gray_nodes_mutex.Unlock();
}

static void gray_nodes_clear()
{
    _gray_nodes_mutex.Lock();
    _gray_nodes.clear();
    _gray_nodes_mutex.Unlock();
}
