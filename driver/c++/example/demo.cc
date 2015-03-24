#include <stdlib.h>
#include <string.h>
#include <iostream>
#include <qconf/qconf.h>

using namespace std;

int main()
{
    int ret = 0;
    int i = 0;

    // Init the qconf env
    ret = qconf_init();
    if (QCONF_OK != ret)
    {
        cout << "qconf init error! ret:" << ret << endl;
        return ret;
    }

    const char *path = "/demo/test/confs/conf1/conf11";
    char value[QCONF_CONF_BUF_MAX_LEN];
    char host[QCONF_HOST_BUF_MAX_LEN] = {0};

    // Get the conf
    ret = qconf_get_conf(path, value, sizeof(value), NULL);
    if (QCONF_OK != ret)
    {
        cout << "qconf get conf failed " << endl;
        qconf_destroy();
        return ret;
    }
    cout << "path: " << path << endl;
    cout << "value: " << value << endl << endl;

    /***********************************/
    // Get Batch conf with key and value
    /***********************************/
    path = "/demo/test/confs/conf1";
    qconf_batch_nodes bnodes;

    init_qconf_batch_nodes(&bnodes);
    ret = qconf_get_batch_conf(path, &bnodes, NULL);
    if (QCONF_OK != ret)
    {
        cout << "qconf get batch conf failed! " << endl;
        qconf_destroy();
        return ret;
    }
    cout << "children of parent path: " << path << endl;
    if (bnodes.count == 0)
        cout << "children's number is 0" << endl;

    for (i = 0; i < bnodes.count; i++)
    {
        cout << bnodes.nodes[i].key << " : " << bnodes.nodes[i].value;
        if (strcmp(bnodes.nodes[i].value, "") == 0)
        {
            cout << "\033[35;1mempty\033[0m" << endl;
        }
        else
        {
            cout << endl;
        }
    }
    destroy_qconf_batch_nodes(&bnodes);
    cout << endl;

    /***********************************/
    // Get Batch keys
    /***********************************/
    string_vector_t bnodes_key;

    init_string_vector(&bnodes_key);
    ret = qconf_get_batch_keys(path, &bnodes_key, NULL);
    if (QCONF_OK != ret)
    {
        cout << "qconf get batch conf's key failed! " << endl;
        qconf_destroy();
        return ret;
    }
    cout << "children of parent path: " << path << endl;
    if (bnodes_key.count == 0)
        cout << "children's number is 0" << endl;

    for (i = 0; i < bnodes_key.count; i++)
    {
        cout << bnodes_key.data[i] << endl;
    }
    destroy_string_vector(&bnodes_key);
    cout << endl;

    /***********************************/
    // Get the services
    /***********************************/
    path = "/demo/test/hosts/host1";
    string_vector_t nodes = {0};
    ret = init_string_vector(&nodes);
    if (QCONF_OK != ret)
    {
        cout << "init string vector failed" << endl;
        qconf_destroy();
        return ret;
    }
    //ret = qconf_get_allhost(path, &nodes, NULL);
    ret = qconf_get_allhost(path, &nodes, NULL);
    if (QCONF_OK != ret)
    {
        cout << "qconf get services failed! " << endl;
        qconf_destroy();
        return ret;
    }
    cout << "number of services of path: " << path << " is " << nodes.count << "; services are: " << endl;
    for (i = 0; i < nodes.count; i++)
    {
        cout << nodes.data[i] << endl;
    }
    destroy_string_vector(&nodes);
    cout << endl;
    
    /***********************************/
    // Get one service of path
    /***********************************/
    //ret = qconf_get_host(path, host, sizeof(host), NULL);
    ret = qconf_get_host(path, host, sizeof(host), NULL);
    if (QCONF_OK != ret)
    {
        cout << "qconf get service failed!" << endl;
        qconf_destroy();
        return ret;
    }
    cout << "service of path: " << path << " is below: " << endl;
    cout << host << endl << endl;

    // Get the qconf version
    const char *qv = qconf_version();
    cout << "qconf version: " << qv << endl;

    // Destroy the qconf env
    ret = qconf_destroy();
    if (QCONF_OK != ret)
    {
        cout << "qconf destroy failed" << endl;
        return -1;
    }

    return 0;
}
