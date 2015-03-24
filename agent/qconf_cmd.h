#ifndef __QCONF_CMD_H__
#define __QCONF_CMD_H__

#include <string>

size_t qconf_cmd_proc();
int qconf_init_cmd_env(const std::string &qconf_dir);
int qconf_write_file(const std::string &file_str, const std::string &content, int append);

#endif
