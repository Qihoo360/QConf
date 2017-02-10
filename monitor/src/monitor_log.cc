#include "monitor_log.h"

namespace mlog {

std::string kLogPath = "logs/";
std::string kLogFileNamePrefix = "qconf_monitor.log";

int log_level = LOG_WARNING;
FILE* fp = nullptr;
pthread_mutex_t f_mutex = PTHREAD_MUTEX_INITIALIZER;
char cur_log_file_name[128] = {0};

std::string log_levelitos[7] = {"FATAL_ERROR", "ERROR", "WARNING", "NOTICE", "INFO", "TRACE", "DEBUG"};

int Init(const int ll) {
  log_level = ll;
  return 0;
}

int CheckFile(const int year, const int mon, const int day) {
  char new_name[128] = {0};
  snprintf(new_name, sizeof(new_name), "%s%s.%4d%02d%02d", kLogPath.c_str(),
           kLogFileNamePrefix.c_str(), year, mon, day);
  if (strcmp(cur_log_file_name, new_name) == 0) {
    return 0;
  } else {
    // We need to touch a new log file
    if (fp) {
      pthread_mutex_lock(&f_mutex);
      //because multithread, so we need to compare again.
      if (fp){
        fclose(fp);
        fp = NULL;
      }
      pthread_mutex_unlock(&f_mutex);
    }
    if (strcmp(cur_log_file_name, new_name) != 0) {
      pthread_mutex_lock(&f_mutex);
      if (strcmp(cur_log_file_name, new_name) != 0) {
        strcpy(cur_log_file_name, new_name);
        fp = fopen(cur_log_file_name, "a");
        if (!fp) {
          fprintf(stderr, "Log file open failed. Path %s.  Error No:%s\n", cur_log_file_name, strerror(errno));
          pthread_mutex_unlock(&f_mutex);
          return -1;
        }
      }
      pthread_mutex_unlock(&f_mutex);
    }
  }
  return 0;
}

int PrintLog(const char* file_name, const int line, const int level, const char* format, ...) {
  //we only log when the level input is lower than the loglevel in the config file
  if (level > log_level) {
    return 0;
  }
  std::string log_info = log_levelitos[level];
  //get the time
  time_t cur_time;
  time(&cur_time);
  struct tm* real_time = localtime(&cur_time);

  char log_buf[1024] = {0};
  int char_count = snprintf(log_buf, sizeof(log_buf),
                            "%4d/%02d/%02d %02d:%02d:%02d - [%s][%s %d] - ",
                           real_time->tm_year + 1900,
                           real_time->tm_mon + 1,
                           real_time->tm_mday,
                           real_time->tm_hour,
                           real_time->tm_min,
                           real_time->tm_sec,
                           log_info.c_str(), file_name, line);
  //check weather we need change a log file
  if (CheckFile(real_time->tm_year + 1900, real_time->tm_mon + 1, real_time->tm_mday) != 0) {
    return -1;
  }
  if (!fp) {
    return -1;
  }
  va_list arg_ptr;
  va_start(arg_ptr, format);
  vsnprintf(log_buf + char_count, sizeof(log_buf) - char_count, format, arg_ptr);
  va_end(arg_ptr);
  strcat(log_buf, "\n");
  pthread_mutex_lock(&f_mutex);
  fwrite(log_buf, sizeof(char), strlen(log_buf), fp);
  fflush(fp);
  pthread_mutex_unlock(&f_mutex);
  return 0;
}

void CloseLogFile() {
  if (fp && fp != stderr) {
    fclose(fp);
    fp = NULL;
    memset(cur_log_file_name, 0, 128);
  }
  return;
}

}  // namespace log
