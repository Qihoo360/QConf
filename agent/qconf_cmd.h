#ifndef QCONF_CMD_H
#define QCONF_CMD_H

#include <string>

size_t qconf_cmd_proc();
int qconf_init_cmd_env(const std::string &qconf_dir);
int qconf_write_file(const std::string &file_str, const std::string &content, int append);

#endif
