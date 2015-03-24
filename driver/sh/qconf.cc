#include <iostream>
#include <string>

#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <qconf.h>

using namespace std;

#define CMD_GET_CONF            "get_conf"
#define CMD_GET_HOST            "get_host"
#define CMD_GET_ALLHOST         "get_allhost"
#define CMD_GET_BATCH_CONF      "get_batch_conf"
#define CMD_GET_BATCH_KEYS      "get_batch_keys"
#define CMD_VERSION             "version"

#define QCONF_SHELL_VERSION     "1.0.0"

void show_usage()
{
    printf("usage: qconf command key [idc]\n");
    printf("       command: can be one of below commands:\n");
    printf("                version         : get qconf version\n");
    printf("                get_conf        : get configure value\n");
    printf("                get_host        : get one service\n");
    printf("                get_allhost     : get all services available\n");
    printf("                get_batch_keys  : get all children keys\n");
    printf("       key    : the path of your configure items\n");
    printf("       idc    : query from current idc if be omitted\n");
    printf("example: \n");
    printf("       qconf get_conf \"demo/conf\"\n");
    printf("       qconf get_conf \"demo/conf\" \"corp\" \n");
}

int main(int argc, char *argv[])
{
    if (argc < 2)
    {
        show_usage();
        return QCONF_ERR_OTHER;
    }
    char *command = argv[1];
    if (!strcmp(command, CMD_VERSION))
    {
        printf("Version : %s\n", QCONF_SHELL_VERSION) ;
        return QCONF_OK;
    }
    if (argc == 2)
    {
        show_usage();
        return QCONF_ERR_OTHER;
    }

    int ret = qconf_init();
    if (QCONF_OK != ret)
    {
        printf("[ERROR] Failed to init qconf! ret:%d\n", ret);
        return ret;
    }
    char *path = argv[2];
    char *idc = ((argc == 4) ? argv[3] : NULL);
    
    if (!strcmp(command, CMD_GET_CONF))
    {
        // Get the conf
        char value[QCONF_CONF_BUF_MAX_LEN];
        ret = qconf_get_conf(path, value, sizeof(value), idc);
        if (QCONF_OK != ret)
        {
            printf("[ERROR]Failed to get conf! ret:%d\n", ret);
            return ret;
        }
        printf("%s\n", value);
        return ret;
    }
    else if (!strcmp(command, CMD_GET_HOST))
    {
        // Get the host
        char host[QCONF_HOST_BUF_MAX_LEN] = {0};
        ret = qconf_get_host(path, host, sizeof(host), idc);
        if (QCONF_OK != ret)
        {
            printf("[ERROR]Failed to get get host! ret:%d\n", ret);
            return ret;
        }
        printf("%s\n", host);
        return ret;
    }
    else if (!strcmp(command, CMD_GET_ALLHOST))
    {
        // Get the services
        int i = 0;
        string_vector_t nodes;
        ret = init_string_vector(&nodes);
        if (QCONF_OK != ret)
        {
            printf("[ERROR]Failed to init string vector! ret:%d\n", ret);
            return ret;
        }
        ret = qconf_get_allhost(path, &nodes, idc);
        if (QCONF_OK != ret)
        {
            printf("[ERROR]Failed to get all services! ret:%d\n", ret);
            return ret;
        }

        for (i = 0; i < nodes.count; i++)
        {
            printf("%s\n", nodes.data[i]);
        }
        destroy_string_vector(&nodes);
        return ret;
    }
    else if (!strcmp(command, CMD_GET_BATCH_KEYS))
    {
        // Get Batch keys
        int i = 0;
        string_vector_t bnodes_key;
        ret = init_string_vector(&bnodes_key);
        if (QCONF_OK != ret)
        {
            printf("[ERROR]Faield to init string vector! ret:%d\n", ret);
            return ret;
        }
        ret = qconf_get_batch_keys(path, &bnodes_key, idc);
        if (QCONF_OK != ret)
        {
            printf("[ERROR]Failed to get batch keys! ret:%d\n", ret);
            return ret;
        }
        for (i = 0; i < bnodes_key.count; i++)
        {
            printf("%s\n", bnodes_key.data[i]);
        }
        destroy_string_vector(&bnodes_key);
        return ret;
    }
    else
    {
        show_usage();
    }

    return QCONF_OK;
}
