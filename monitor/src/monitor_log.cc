#include <string>
#include <cstring>

#include <stdarg.h>
#include <errno.h>
#include <time.h>

#include "monitor_log.h"
#include "monitor_const.h"
#include "monitor_config.h"

using namespace std;

int Log::logLevel = LOG_WARNING;
FILE* Log::fp = NULL;
pthread_mutex_t Log::mutex;
char Log::curLogFileName[128] = {0};
string Log::logLevelitos[7] = {"FATAL_ERROR", "ERROR", "WARNING", "NOTICE", "INFO", "TRACE", "DEBUG"};

int Log::init(const int ll) {
    logLevel = ll;
    return 0;
}

string Log::getLogLevelStr(const int level) {
    return logLevelitos[level];
}

int Log::checkFile(const int year, const int mon, const int day){
    char newName[128] = {0};
    snprintf(newName, sizeof(newName), "%s%s.%4d%02d%02d", logPath.c_str(),
        logFileNamePrefix.c_str(), year, mon, day);
    if (strcmp(curLogFileName, newName) == 0) {
        return 0;
    }
    //we need to touch a new log file
    else {
        if (fp) {
            pthread_mutex_lock(&mutex);
            //because multithread, so we need to compare again.
            if (fp){
                fclose(fp);
                fp = NULL;
            }
            pthread_mutex_unlock(&mutex);
        }
        if (strcmp(curLogFileName, newName) != 0) {
            pthread_mutex_lock(&mutex);
            if (strcmp(curLogFileName, newName) != 0) {
                strcpy(curLogFileName, newName);
                fp = fopen(curLogFileName, "a");
                if (!fp) {
                    fprintf(stderr, "Log file open failed. Path %s.  Error No:%s\n", curLogFileName, strerror(errno));
                    pthread_mutex_unlock(&mutex);
                    return -1;
                }
            }
            pthread_mutex_unlock(&mutex);
        }
    }
    return 0;
}

int Log::printLog(const char* fileName, const int line, const int level, const char* format, ...) {
    //we only log when the level input is lower than the loglevel in the config file
    if (level > logLevel) {
        return 0;
    }
    string logInfo = getLogLevelStr(level);
    //get the time
    time_t curTime;
    time(&curTime);
    struct tm* realTime = localtime(&curTime);

    char logBuf[1024] = {0};
    int charCount = snprintf(logBuf, sizeof(logBuf), "%4d/%02d/%02d %02d:%02d:%02d - [%s][%s %d] - ",
        realTime->tm_year + 1900, realTime->tm_mon + 1, realTime->tm_mday, realTime->tm_hour,
        realTime->tm_min, realTime->tm_sec, logInfo.c_str(), fileName, line);
    //check weather we need change a log file
    if (checkFile(realTime->tm_year + 1900, realTime->tm_mon + 1, realTime->tm_mday) != 0) {
        return -1;
    }
    if (!fp) {
        return -1;
    }
    va_list argPtr;
    va_start(argPtr, format);
    vsnprintf(logBuf + charCount, sizeof(logBuf) - charCount, format, argPtr);
    va_end(argPtr);
    strcat(logBuf, "\n");
    pthread_mutex_lock(&mutex);
    fwrite(logBuf, sizeof(char), strlen(logBuf), fp);
    fflush(fp);
    pthread_mutex_unlock(&mutex);
    return 0;
}

void Log::closeLogFile() {
    if (fp && fp != stderr) {
        fclose(fp);
        fp = NULL;
        memset(curLogFileName, 0, 128);
    }
    return;
}
