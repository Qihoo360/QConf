#ifndef __QCONF_CONFIG_H__
#define __QCONF_CONFIG_H__

#include <map>
#include <string>

/**
 * Read configuration from file
 */
int qconf_load_conf(const std::string &agent_dir);

/**
 * Get config value
 */
int get_agent_conf(const std::string &key, std::string &value);

/**
 * Get host value of idc
 */
int get_idc_conf(const std::string &idc, std::string &value);

/**
 * Destory environment
 */
void qconf_destroy_conf_map();

/**
 * Judge whether can be parse to integer
 */
int get_integer(const std::string &cnt, long &integer);

#endif // __QCONF_CONFIG_H__
