#ifndef QCONF_FEEDBACK_H
#define QCONF_FEEDBACK_H

#include <string>
#include <vector>
#include <qconf_common.h>

/**
 * struct for keeping tblval and feedback values 
 */
typedef struct
{
    std::string tblval;
    std::string fb_chds;
} fb_val;

#ifdef QCONF_CURL_ENABLE
/**
 * Init environment of feedback
 */
int qconf_init_feedback(const std::string &url);

/**
 * Destroy environment of feedback
 */
void qconf_destroy_feedback();

/**
 * Do feedback
 */
int feedback_process(const std::string &content);

/**
 * Generate content will be send as feedback message
 */
int feedback_generate_content(const std::string &ip, char data_type, const std::string &idc, const std::string &path, const fb_val &fbval, std::string &content);

/**
 * Generate ip by zhandle_t
 */
int get_feedback_ip(const zhandle_t *zh, std::string &ip_str);

/**
 * Generate string containing all chdnodes together with their status
 */
void feedback_generate_chdval(const string_vector_t &chdnodes, const std::vector<char> &status, std::string &value);

/**
 * Generate string containing all batch nodes
 */
void feedback_generate_batchval(const string_vector_t &batchnodes, std::string &value);
#endif

#endif // __QCONF_FEEDBACK_H__
