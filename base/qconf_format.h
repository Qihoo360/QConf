#ifndef QCONF_FORMAT_H
#define QCONF_FORMAT_H

#include <vector>
#include <string>
#include <set>

#include "qconf_common.h"

/**
 * free the string vector according to the free_size
 */
static inline void free_string_vector(string_vector_t &vector, size_t free_size)
{
    for (size_t i = 0; i < free_size; ++i)
    {
        free(vector.data[i]);
        vector.data[i] = NULL;
    }
    free(vector.data);
    vector.data = NULL;
    vector.count = 0;
}

/**
 * Format the data_type, idc and path to tblkey
 */
int serialize_to_tblkey(char data_type, const std::string &idc, const std::string &path, std::string &tblkey);

/**
 * Get data_type, idc and path from tblkey
 */
int deserialize_from_tblkey(const std::string &tblkey, char &data_type, std::string &idc, std::string &path);


/**
 * Format local idc to tblval
 */
int localidc_to_tblval(const std::string &key, const std::string &local_idc, std::string &tblval);

/**
 * Format key and nodeval to tblval
 */
int nodeval_to_tblval(const std::string &key, const std::string &nodeval, std::string &tblval_buf);

/**
 * Format key and children nodes to tblval
 */
int chdnodeval_to_tblval(const std::string &key, const string_vector_t &nodes, std::string &tblval_buf, const std::vector<char> &valid_flg);

/**
 * Format key and batch nodes to tblval
 */
int batchnodeval_to_tblval(const std::string &key, const string_vector_t &nodes, std::string &tblval_buf);

/**
 * Format key and host to tblval
 */
int idcval_to_tblval(const std::string &key, const std::string &host, std::string &tblval_buf);

/**
 * Get current data type
 */
char get_data_type(const std::string &value);

/**
 * Get the local idc from tblval
 */
int tblval_to_localidc(const std::string &tblval, std::string &idc);

/**
 * Get the host from tblval
 */
int tblval_to_idcval(const std::string &tblval, std::string &host);

/**
 * Get the idc and host from tblval
 */
int tblval_to_idcval(const std::string &tblval, std::string &host, std::string &idc);

/**
 * Get node value from tblval
 */
int tblval_to_nodeval(const std::string &tblval, std::string &nodeval);

/**
 * Get idc, path and node value from tblval
 */
int tblval_to_nodeval(const std::string &tblval, std::string &nodeval, std::string &idc, std::string &path);

/**
 * Get nodes from tblval
 */
int tblval_to_chdnodeval(const std::string &tblval, string_vector_t &nodes);

/**
 * Get idc, path and nodes from tblval
 */
int tblval_to_chdnodeval(const std::string &tblval, string_vector_t &nodes, std::string &idc, std::string &path);

/**
 * Get batch nodes from tblval
 */
int tblval_to_batchnodeval(const std::string &tblval, string_vector_t &nodes);

/**
 * Get idc, path and batch nodes from tblval
 */
int tblval_to_batchnodeval(const std::string &tblval, string_vector_t &nodes, std::string &idc, std::string &path);

/**
 * format the idc and host
 */
void serialize_to_idc_host(const std::string &idc, const std::string &host, std::string &dest);

/**
 * get idc and host from idc host
 */
int deserialize_from_idc_host(const std::string &idc_host, std::string &idc, std::string &host);

#define LOG_ERR_KEY_INFO(format, ...)  qconf_print_key_info(__FILE__, __LINE__, format, ## __VA_ARGS__)

/**
 * Get gray nodes from serialized string
 */
int tblval_to_graynodeval(const std::string &tblval, std::set<std::string> &nodes);

/**
 * Serialize the gray nodes into string
 */
int graynodeval_to_tblval(const std::set<std::string> &nodes, std::string &tblval);

#endif
