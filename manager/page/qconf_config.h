#ifndef QCONF_CONFIG_H
#define QCONF_CONFIG_H

#include <string>
#include <map>

#define QCONF_OK                            0
#define QCONF_ERR_OTHER                     -1
#define QCONF_ERR_PARAM                     1
#define QCONF_ERR_NOT_FOUND                 10  
#define QCONF_ERR_INVALID_IP                30
#define QCONF_ERR_OPEN                      51

/**
 * Read configuration from file
 */
int qconf_load_conf();

/**
 * Get host value of idc
 */
int get_host(const std::string &idc, std::string &value);

/**
 * get all idcs
 */
const std::map<std::string, std::string> get_idc_map();

/**
 * Destory environment
 */
void qconf_destroy_conf_map();

#endif
