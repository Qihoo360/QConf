#ifndef __QCONF_SHM_H__
#define __QCONF_SHM_H__

#include <string>

#include "qlibc/qlibc.h"

/**
 * Destroy qhasharr mutex lock
 */
void qconf_destroy_qhasharr_lock();

/**
 * Get local idc from tbl without locking
 */
int qconf_get_localidc(qhasharr_t *tbl, std::string &local_idc);

/**
 * Update the tbl using local idc
 */
int qconf_update_localidc(qhasharr_t *tbl, const std::string &local_idc);

/**
 * Check current tblkey whether exist
 */
int qconf_exist_tblkey(qhasharr_t *tbl, const std::string &key, bool &status);

/**
 * Create or init hashtable
 */
int create_hash_tbl(qhasharr_t *&tbl, key_t shmkey, mode_t mode);
int init_hash_tbl(qhasharr_t *&tbl, key_t shmkey, mode_t mode, int flags);

/**
 * Operation of hashtable
 */
int hash_tbl_get(qhasharr_t *tbl, const std::string &key, std::string &val);
int hash_tbl_set(qhasharr_t *tbl, const std::string &key, const std::string &val);
bool hash_tbl_exist(qhasharr_t *tbl, const std::string &key);
int hash_tbl_remove(qhasharr_t *tbl, const std::string &key);
int hash_tbl_getnext(qhasharr_t *tbl, std::string &tblkey, std::string &tblval, int &idx);
int hash_tbl_get_count(qhasharr_t *tbl, int &max_slots, int &used_slots);
int qconf_check_md5(std::string &val);
int hash_tbl_clear(qhasharr_t *tbl);

#endif
