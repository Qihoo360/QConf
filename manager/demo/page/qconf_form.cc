#include <stdio.h>
#include <string.h>
#include <stdlib.h>

#include "cgic.h"
#include "qconf_page.h"
#include "qconf_zk.h"
#include "qconf_config.h"

#include <string>
#include <vector>
using namespace std;

string path, idc, value;
vector<string> idcs, children;
QConfZK qconfZk;
const int QCONF_VALUE_BUFFER_SIZE = 1024*1024;

static void HandleDeleteNode();
static void HandleModifyNode();
static void ShowPage();
static void InitialPage();
static void InitialZk();
static int Initial();
static int GetNodeValue();
static int GetChildList();
static void PrintInfo(string err);

int cgiMain() {
	cgiHeaderContentType("text/html");
    if (0 != Initial()) return -1;
        	
    // Check path and format it
    string tmp_path;
    if (!path.empty() && QCONF_OK != qconfZk.zk_path(path, tmp_path))
    {
        PrintInfo("Incorrect path!");
    }
    path.assign(tmp_path);

    if (!path.empty())
    {
        InitialZk();
        // If a submit button has already been clicked, act on the submission of the form.
        if ((cgiFormSubmitClicked("modify_node") == cgiFormSuccess))
        {
            HandleModifyNode();
        }
        else if ((cgiFormSubmitClicked("delete_node") == cgiFormSuccess))
        {
            HandleDeleteNode();
        }

        InitialPage();
    }

    // Now show the form
    ShowPage();

    qconf_destroy_conf_map();
    qconfZk.zk_close();
	return 0;
}

static void HandleDeleteNode()
{
    // Delete node
    switch (qconfZk.zk_node_delete(path))
    {
        case QCONF_OK:
            break;
        case QCONF_ERR_ZOO_NOTEMPTY:
            PrintInfo("Failed! Children already exist under:" + path);
            return;
        default:
            PrintInfo("Failed to delete node: " + path);
            return;
    }
    // Show parent node
    path.resize(path.find_last_of("/"));
    PrintInfo("Success!");
}

static void HandleModifyNode()
{
	char *raw_buffer = (char *)malloc((QCONF_VALUE_BUFFER_SIZE + 1) * sizeof(char));
	cgiFormString("value", raw_buffer, QCONF_VALUE_BUFFER_SIZE + 1);
    string new_value(raw_buffer);
    free(raw_buffer);

    // Modify node value
    if (QCONF_OK != qconfZk.zk_node_set(path, new_value))
    {
        PrintInfo("Failed to add or modify node: " + path );
        return;
    }
    value.assign(new_value);
    PrintInfo("Success!");
}

static void PrintInfo(string err)
{
	fprintf(cgiOut, "<script>alert(\"%s\");</script>\n", err.c_str());
}

static void InitialZk()
{
	char raw[256] = {0};
    // Current idc
	cgiFormStringNoNewlines("idc", raw, sizeof(raw));
    idc.assign(raw);

    // Initial zookeeper
    string host;
    if (QCONF_OK != get_host(idc, host)) 
    {
        PrintInfo("Failed find idc :" + idc);
        return;
    }
    if(QCONF_OK != qconfZk.zk_init(host))
        PrintInfo("Failed to init qconf server environment!");
}

static void InitialPage()
{
    // Get value from ZooKeeper
    if (0 != GetNodeValue())
        return;

    // Get children list
    GetChildList();
}


static int Initial()
{
    // Load idc
    if (QCONF_OK != qconf_load_conf())
    {
        PrintInfo("Failed to load idc file!");
        return -1;
    }

    const map<string, string> idc_hosts = get_idc_map(); 
    map<string, string>::const_iterator it = idc_hosts.begin();
    for (; it != idc_hosts.end(); ++it)
    {
       idcs.push_back(it->first);
    }

    // Current path
	char raw[1024] = {0};
    cgiFormStringNoNewlines("path", raw, sizeof(raw));
    path.assign(raw);
    return 0;
}

static int GetNodeValue()
{
    string buf = "";
    int ret = 0;
    switch(ret = qconfZk.zk_node_get(path, buf))
    {
        case QCONF_OK:
            value.assign(buf);
            break;
        case QCONF_ERR_ZOO_NOT_EXIST:
            PrintInfo("Failed to get value, Node not exist: " + path);
            break;
        default:
            PrintInfo("Failed to get node value for node: " + path);
    }
    return (QCONF_OK == ret) ? 0 : -1;
}

static int GetChildList()
{
    
    set<string> list;
    set<string>::iterator it;
    int ret = 0;
    switch (ret = qconfZk.zk_list(path, list))
    {
        case QCONF_OK:
            for (it = list.begin(); it != list.end(); ++it)
            {
                children.push_back(path + "/" + (*it));
            }
            break;
        case QCONF_ERR_ZOO_NOT_EXIST:
            PrintInfo("Failed to list children Node not exist: " + path);
            break;
        default:
            PrintInfo("Failed to list children for node: " + path);
    }
    return (QCONF_OK == ret) ? 0 : -1;
}

static void ShowPage()
{
    // Page is defined in qconf_page.cc
    generate(path, value, idc, idcs, children, cgiScriptName);
}
