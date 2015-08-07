#include <time.h>
#include <stdio.h>
#include <unistd.h>
#include <stdarg.h>
#include <string.h>
#include <pthread.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <sys/syscall.h>

#include <string>

#include "qconf_log.h"
#include "qconf_common.h"
#include "qconf_format.h"

using namespace std;

static const char *qconf_get_log_level(int level);
static int qconf_check_log(const struct tm *cur_tm);

static string           _qconf_log_path;
static string           _qconf_log_path_fmt;
static int              _qconf_log_level = QCONF_LOG_LVL_MAX; 
static FILE             *_qconf_log_fp = NULL;
static pthread_mutex_t  _qconf_log_mutex = PTHREAD_MUTEX_INITIALIZER;


void qconf_set_log_level(int level)
{
    _qconf_log_level = level;
}

void qconf_set_log_fmt(const string &log_path_fmt)
{
    if (!log_path_fmt.empty()) _qconf_log_path_fmt = log_path_fmt;
}

void qconf_log_init(const string &log_path_fmt, int level)
{
    qconf_set_log_fmt(log_path_fmt);
    qconf_set_log_level(level);
}

void qconf_close_log_stream()
{
    if (NULL != _qconf_log_fp && stderr != _qconf_log_fp)
    {
        fclose(_qconf_log_fp);
        _qconf_log_fp = NULL;
    }
}

void qconf_destroy_log()
{
    qconf_close_log_stream();
    pthread_mutex_destroy(&_qconf_log_mutex);
}

void qconf_print_log(const char* file_path, int line_no, int log_level, const char* format, ...)
{
    if (log_level < _qconf_log_level || NULL == file_path || NULL == format)
        return;

    int n = 0;
    time_t cur;
    int ret = QCONF_OK;
    struct tm cur_tm = {0};
    const char* file_name = NULL;
    char log_buf[QCONF_MAX_BUF_LEN] = {0};

    file_name = strrchr(file_path, '/');
    file_name = (file_name == NULL) ? file_path : (file_name + 1);

    pthread_mutex_lock(&_qconf_log_mutex);

    time(&cur);
    localtime_r(&cur, &cur_tm);

    ret = qconf_check_log(&cur_tm);
    if (QCONF_OK == ret)
    {
        n = snprintf(log_buf, sizeof(log_buf),
                     "%4d/%02d/%02d %02d:%02d:%02d - [pid: %d][%s][%s %d] - ",
                     cur_tm.tm_year + 1900,
                     cur_tm.tm_mon + 1,
                     cur_tm.tm_mday,
                     cur_tm.tm_hour,
                     cur_tm.tm_min,
                     cur_tm.tm_sec,
                     getpid(),
                     qconf_get_log_level(log_level),
                     file_name,
                     line_no);

        va_list arg_ptr;
        va_start(arg_ptr, format);
        vsnprintf(log_buf + n, sizeof(log_buf) - n - 2, format, arg_ptr);
        va_end(arg_ptr);
        strcat(log_buf, "\n");

        fwrite(log_buf, sizeof(char), strlen(log_buf), _qconf_log_fp);
        fflush(_qconf_log_fp);
    }

    pthread_mutex_unlock(&_qconf_log_mutex);
}

static int qconf_check_log(const struct tm *cur_tm)
{
    int ret = QCONF_OK;
    size_t len = 0;
    char log_path[QCONF_FILE_PATH_LEN] = {0};

    if (_qconf_log_path_fmt.empty())
    {
        _qconf_log_fp = _qconf_log_fp == NULL ? stderr : _qconf_log_fp;
        return QCONF_OK;
    }

    len = strftime(log_path, sizeof(log_path), _qconf_log_path_fmt.c_str(), cur_tm); 

    if (strncmp(log_path, _qconf_log_path.c_str(), len) != 0)
    {
        if (_qconf_log_fp != NULL && _qconf_log_fp != stderr)
        {
            fclose(_qconf_log_fp);
            _qconf_log_fp = NULL;
        }

        _qconf_log_path.assign(log_path);
        _qconf_log_fp = fopen(_qconf_log_path.c_str(), "a");
        ret = (_qconf_log_fp == NULL) ? QCONF_ERR_OPEN : ret;
    }
    else
    {
        if (NULL == _qconf_log_fp)
        {
            _qconf_log_fp = fopen(_qconf_log_path.c_str(), "a");
            ret = (_qconf_log_fp == NULL) ? QCONF_ERR_OPEN : ret;
        }
    }

    return ret;
}

static const char *qconf_get_log_level(int level)
{
    switch (level)
    {
    case QCONF_LOG_FATAL_ERR:
        return "FATAL ERROR";
    case QCONF_LOG_ERR:
        return "ERROR";
    case QCONF_LOG_WARN:
        return "WARNING";
    case QCONF_LOG_INFO:
        return "INFO";
    case QCONF_LOG_TRACE:
        return "TRACE";
    case QCONF_LOG_DEBUG:
        return "DEBUG";
    default:
        return "UNKNOWN";
    }
}

void qconf_print_key_info(const char* file_path, int line_no, const string &tblkey, const char *format, ...)
{
    string idc;
    string path;
    char data_type;
    char buf[QCONF_MAX_BUF_LEN] = {0};

    va_list arg_ptr;
    va_start(arg_ptr, format);
    int n = vsnprintf(buf, sizeof(buf), format, arg_ptr);
    va_end(arg_ptr);

    if (n >= (int)sizeof(buf)) return;

    deserialize_from_tblkey(tblkey, data_type, idc, path);
    snprintf(buf + n, sizeof(buf) - n, "; data type:%c, idc:%s, path:%s",
            data_type, idc.c_str(), path.c_str());
    qconf_print_log(file_path, line_no, QCONF_LOG_ERR, "%s", buf);
}
