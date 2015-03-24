#ifndef __QCONF_SCRIPT_H__
#define __QCONF_SCRIPT_H__

#include <string>

/**
 * Init qconf directory of script
 */
void qconf_init_script_dir(const std::string &agent_dir);

/**
 * Get script content by given node path
 */
int find_script(const std::string &path, std::string &script);

/**
 * Execute script content within given timeout
 */
int execute_script(const std::string &script, const long mtimeout);

#endif

