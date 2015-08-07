#ifndef QCONF_LOG_H
#define QCONF_LOG_H

#include <string>

#define QCONF_LOG_LVL_MAX           10
#define QCONF_LOG_FATAL_ERR         5
#define QCONF_LOG_ERR               4
#define QCONF_LOG_WARN              3
#define QCONF_LOG_INFO              2
#define QCONF_LOG_TRACE             1
#define QCONF_LOG_DEBUG             0

#define LOG_FATAL_ERR(format, ...) qconf_print_log(__FILE__, __LINE__, QCONF_LOG_FATAL_ERR, format, ## __VA_ARGS__)
#define LOG_ERR(format, ...)       qconf_print_log(__FILE__, __LINE__, QCONF_LOG_ERR, format, ## __VA_ARGS__)
#define LOG_WARN(format, ...)      qconf_print_log(__FILE__, __LINE__, QCONF_LOG_WARN, format, ## __VA_ARGS__)
#define LOG_INFO(format, ...)      qconf_print_log(__FILE__, __LINE__, QCONF_LOG_INFO, format, ## __VA_ARGS__)
#define LOG_TRACE(format, ...)     qconf_print_log(__FILE__, __LINE__, QCONF_LOG_TRACE, format, ## __VA_ARGS__)
#define LOG_DEBUG(format, ...)     qconf_print_log(__FILE__, __LINE__, QCONF_LOG_DEBUG, format, ## __VA_ARGS__)

#define QCONF_FUNC_IN() LOG_INFO("[ %s ] in ...", __FUNCTION__)
#define QCONF_FUNC_OUT() LOG_INFO("[ %s ] out ...", __FUNCTION__)


void qconf_log_init(const std::string &log_path_fmt, int level);
void qconf_destroy_log();
void qconf_set_log_fmt(const std::string &log_path_fmt);
void qconf_set_log_level(int level);
void qconf_close_log_stream();
void qconf_print_log(const char* file_path, int line_no, int log_level, const char* format, ...);

/**
 * print tblkey information
 */
void qconf_print_key_info(const char* file_path, int line_no, const std::string &tblkey, const char *format, ...);

#endif
