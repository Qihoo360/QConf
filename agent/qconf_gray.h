#ifndef QCONF_GRAY_H
#define QCONF_GRAY_H

#include <string>
#include <vector>

/**
 * Check if the node_path is a notify_node
 */
bool is_notify_node(const std::string &node_path);

/**
 * Check if the mkey is a gray node, and set mval to be its tblval if the result is true
 */
bool is_gray_node(const std::string &mkey, std::string &mval);


/**
 * get notify node path for current machine
 */
int watch_notify_node(zhandle_t *zh);

/**
 * do the gray realse process
 */
int gray_process(zhandle_t *zh, const std::string &idc, std::vector< std::pair<std::string, std::string> > &nodes);


#endif
