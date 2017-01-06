#ifndef LOG_H
#define LOG_H

#include <cstdio>
#include <string>

#include <unistd.h>
#include <pthread.h>
#include <stdarg.h>

//log level
#define LOG_FATAL_ERROR 0
#define LOG_ERROR       1
#define LOG_WARNING     2
#define LOG_NOTICE      3
#define LOG_INFO        4
#define LOG_TRACE       5
#define LOG_DEBUG       6

#define LOG(level, format, ...) Log::printLog(__FILE__, __LINE__, level, format, ## __VA_ARGS__)
#define dp() printf("func %s, line %d\n", __func__, __LINE__)
#define LOG_FUNC_IN LOG(LOG_TRACE, "func %s...in, line %d", __func__, __LINE__);
#define LOG_FUNC_OUT LOG(LOG_TRACE, "func %s...out, line %d", __func__, __LINE__);

class Log{
 private:
  Log();
  ~Log();
  static int logLevel;
  static FILE* fp;
  static pthread_mutex_t mutex;
  static char curLogFileName[128];
  static std::string logLevelitos[7];
  static int checkFile(const int year, const int mon, const int day);

  //log file
  static std::string kLogPath;
  static std::string kLogFileNamePrefix;

 public:
  static int printLog(const char* fileName, const int line, const int level, const char* format, ...);
  //load the loglevel from config file
  static int init(const int ll);
  static std::string getLogLevelStr(int n);
  static void closeLogFile();
};
#endif
