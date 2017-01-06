#ifndef LOG_H
#define LOG_H

#include <cstdio>
#include <string>
#include <cstring>

#include <unistd.h>
#include <pthread.h>
#include <stdarg.h>
#include <errno.h>
#include <time.h>

//log level
#define LOG_FATAL_ERROR 0
#define LOG_ERROR       1
#define LOG_WARNING     2
#define LOG_NOTICE      3
#define LOG_INFO        4
#define LOG_TRACE       5
#define LOG_DEBUG       6

namespace mlog {

int CheckFile(const int year, const int mon, const int day);
int PrintLog(const char* file_name, const int line,
             const int level, const char* format, ...);
int Init(const int ll);
void CloseLogFile();

}  // namespace log

#define LOG(level, format, ...) mlog::PrintLog(__FILE__, __LINE__, level, format, ## __VA_ARGS__)
#define dp() printf("func %s, line %d\n", __func__, __LINE__)
#define LOG_FUNC_IN LOG(LOG_TRACE, "func %s...in, line %d", __func__, __LINE__);
#define LOG_FUNC_OUT LOG(LOG_TRACE, "func %s...out, line %d", __func__, __LINE__);

#endif  // LOG_H
