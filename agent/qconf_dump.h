#ifndef QCONF_DUMP_H
#define QCONF_DUMP_H

#include <string>

#include "qlibc.h"
#include "qconf_common.h"

int qconf_init_dump_file(const std::string &agent_dir);
int qconf_init_dbf();
void qconf_destroy_dbf();
void qconf_destroy_dump_lock();
int qconf_dump_get(const std::string &key, std::string &value);
int qconf_dump_set(const std::string &key, const std::string &value);
int qconf_dump_delete(const std::string &tblkey);
int qconf_dump_clear();
int qconf_dump_tbl(qhasharr_t *tbl);

#endif
