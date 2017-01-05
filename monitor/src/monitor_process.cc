#include "slash_string.h"

#include <string>
#include <iostream>
#include <fstream>
#include <iomanip>
#include <cstdio>
#include <cstring>
#include <cstdlib>

#include <unistd.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/wait.h>
#include <sys/resource.h>
#include <signal.h>
#include <fcntl.h>

#include "monitor_log.h"
#include "monitor_const.h"
#include "monitor_service_item.h"
#include "monitor_options.h"
#include "monitor_listener.h"
#include "monitor_process.h"

namespace process {

MonitorOptions *options;
bool need_restart;

bool IsProcessRunning(const std::string& processName) {
  FILE* ptr = NULL;
  char ps[128] = {0};
  char resBuf[128] = {0};
  snprintf(ps, sizeof(ps), "ps -e | grep -c %s", processName.c_str());
  strcpy(resBuf, "ABNORMAL");
  if ((ptr = popen(ps, "r")) != NULL) {
    while(fgets(resBuf, sizeof(resBuf), ptr)) {
      if (atoi(resBuf) >= 2) {
        pclose(ptr);
        return true;
      }
    }
  }
  //excute ps failed or fgets() failed
  if (strcmp(resBuf, "ABNORMAL") == 0) {
    LOG(LOG_ERROR, "excute command failed");
    return true;
  }
  pclose(ptr);
  return false;
}

int Daemonize() {
  signal(SIGTTOU, SIG_IGN);
  signal(SIGTTIN, SIG_IGN);
  signal(SIGTSTP, SIG_IGN);
  signal(SIGHUP, SIG_IGN);

  int fd;
  pid_t pid;
  //already a daemon
  if (getppid() == 1) {
    return 1;
  }
  //fork off the parent process
  pid = fork();
  if (pid < 0) {
    exit(1);
  }
  else if (pid > 0) {
    exit(0);
  }
  //create a new session ID
  if (setsid() < 0) {
    exit(1);
  }

  pid = fork();
  if (pid < 0) {
    exit(1);
  }
  else if (pid > 0) {
    exit(0);
  }
  //change directory
  /*
     if (chdir("/") < 0) {
     exit(EXIT_FAILURE);
     }*/
  // here close all the file description and redirect stand IO
  fd = open("/dev/null", O_RDWR, 0);
  dup2(fd, STDIN_FILENO);
  dup2(fd, STDOUT_FILENO);
  dup2(fd, STDERR_FILENO);
  if (fd >= 3) {
    close(fd);
  }

  Log::closeLogFile();

  for (fd = sysconf(_SC_OPEN_MAX); fd >= 3; --fd) {
    close(fd);
  }

  umask(0);
  LOG(LOG_INFO, "daemonize success");
  return 0;
}

void SigForward(const int sig) {
  signal(sig, SIG_IGN);
  kill(0, sig);
}

void ProcessParam(const std::string& op) {
  ofstream fout;
  fout.open(STATUS_LIST_FILE, ofstream::out);
  if (!fout.good()) {
    LOG(LOG_ERROR, "file open failed, path: %s", STATUS_LIST_FILE.c_str());
    return;
  }
  int status;
  int upCount = 0;
  int offlineCount = 0;
  int downCount = 0;
  int unknownCount = 0;
  int allCount = 0;
  std::string service;
  std::string stat;
  ServiceItem item;
  std::string node;

  //list node
  if (op != UP && op != DOWN && op != OFFLINE && op != ALL) {
    std::set<std::string> ips = options->service_father_to_ip[op];
    if (ips.empty()) {
      LOG(LOG_ERROR, "node: %s doesn't exist.", op.c_str());
      return;
    }
    //find and output
    allCount = ips.size();
    for (int i = 0; i < LINE_LENGTH; ++i) {
      fout << "-";
    }
    fout << endl;
    fout << setiosflags(ios::left) << setw(10) << "status" << setiosflags(ios::left) \
      << setw(30) << "service" << setiosflags(ios::left) << "node" << endl;
    for (auto it = ips.begin(); it != ips.end(); ++it) {
      std::string ip_port;
      if (op.back() == '/') {
        ip_port = op + (*it);
      }
      else {
        ip_port = op + "/" + (*it);
      }
      item = options->service_map[ip_port];
      status = item.status;
      if (status == STATUS_UP) {
        ++upCount;
        stat = "up";
      }
      else if (status == STATUS_DOWN) {
        ++downCount;
        stat = "down";
      }
      else if (status == STATUS_OFFLINE) {
        ++offlineCount;
        stat = "offline";
      }
      else {
        ++unknownCount;
        stat = "unknown";
      }
      for (int i = 0; i < LINE_LENGTH; ++i) {
        fout << "-";
      }
      fout << endl;
      fout << setw(10) << stat << setw(30) << (*it) << op <<  endl;
    }
    for (int i = 0; i < LINE_LENGTH; ++i) {
      fout << "-";
    }
    fout << endl;
    fout <<"Up:" << upCount << "    Offline:" << offlineCount << "    Down:" \
      << downCount << "    Unknown:" << unknownCount << "    Total:" << allCount << endl;
    return;
  }
  for (int i = 0; i < LINE_LENGTH; ++i) {
    fout << "-";
  }
  fout << endl;
  fout << setiosflags(ios::left) << setw(10) << "status" << setiosflags(ios::left) \
    << setw(30) << "service" << setiosflags(ios::left) << "node" << endl;
  allCount = options->service_map.size();
  for (auto it = options->service_map.begin(); it != options->service_map.end(); ++it) {
    item = it->second;
    status = item.status;
    node = item.service_father;
    service = item.host + ":" + to_string(item.port);
    if (status == STATUS_UP) {
      stat = "up";
      if (op == UP || op == ALL) {
        for (int i = 0; i < LINE_LENGTH; ++i) {
          fout << "-";
        }
        fout << endl;
        fout << setw(10) << stat << setw(30) << service << node << endl;
      }
      ++upCount;
    }
    else if (status == STATUS_DOWN) {
      stat = "down";
      if (op == DOWN || op == ALL) {
        for (int i = 0; i < LINE_LENGTH; ++i) {
          fout << "-";
        }
        fout << endl;
        fout << setw(10) << stat << setw(30) << service << node << endl;
      }
      ++downCount;
    }
    else if (status == STATUS_OFFLINE) {
      stat = "offline";
      if (op == OFFLINE || op == ALL) {
        for (int i = 0; i < LINE_LENGTH; ++i) {
          fout << "-";
        }
        fout << endl;
        fout << setw(10) << stat << setw(30) << service << node << endl;
      }
      ++offlineCount;
    }
    else {
      stat = "unknown";
      if (op == ALL) {
        for (int i = 0; i < LINE_LENGTH; ++i) {
          fout << "-";
        }
        fout << endl;
        fout << setw(10) << stat << setw(30) << service << node << endl;
      }
      ++unknownCount;
    }
  }
  if (op == UP) {
    for (int i = 0; i < LINE_LENGTH; ++i) {
      fout << "-";
    }
    fout << endl;
    fout <<"Up Service:" << upCount << endl;
  }
  if (op == DOWN) {
    for (int i = 0; i < LINE_LENGTH; ++i) {
      fout << "-";
    }
    fout << endl;
    fout <<"Down Service:" << downCount << endl;
  }
  if (op == OFFLINE) {
    for (int i = 0; i < LINE_LENGTH; ++i) {
      fout << "-";
    }
    fout << endl;
    fout <<"Offline Service:" << offlineCount << endl;
  }
  if (op == ALL) {
    for (int i = 0; i < LINE_LENGTH; ++i) {
      fout << "-";
    }
    fout << endl;
    fout <<"Up:" << upCount << "    Offline:" << offlineCount << "    Down:" \
      << downCount << "    Unknown:" << unknownCount << "    Total:" << allCount << endl;
  }
  fout.close();
  return;
}

void HandleCmd(std::vector<std::string>& cmd) {
  if (cmd[0] == CMD_RELOAD) {
    //handle reload
  }
  else if (cmd[0] == CMD_LIST) {
    if (cmd.size() == 1) {
      cmd.push_back("all");
    }
    ProcessParam(cmd[1]);
  }
  else {
    LOG(LOG_ERROR, "handleCmd error: unknown cmd(%s)", cmd[0].c_str());
  }
}

void trim(std::string &str) {
    size_t left = 0;
    while (left < str.size()) {
        char c = str[left];
        if (c == ' ' || c == '\t' || c == '\n' || c == '\r') {
            ++left;
        } else {
            break;
        }
    }
    size_t right = str.size() - 1;
    while (right >= left) {
        char c = str[right];
        if (c == ' ' || c == '\t' || c == '\n' || c == '\r') {
            --right;
        } else {
            break;
        }
    }
    str = str.substr(left, right - left + 1);
}

int ProcessFileMsg(const std::string cmdFile) {
  LOG(LOG_TRACE, "processFileMsg...in...");
  ifstream file;
  file.open(cmdFile);
  if (file.good()) {
    std::string line;
    while (!file.eof()) {
      getline(file, line);
      trim(line);
      std::vector<std::string> cmd_explain;
      slash::StringSplit(line, ':', cmd_explain);
      if (cmd_explain.size() <= 0 || cmd_explain.size() > 2) {
        continue;
      }
      HandleCmd(cmd_explain);
    }
  }
  else {
    LOG(LOG_ERROR, "processFileMsg open file failed. path:%s errno:%d", cmdFile.c_str(), errno);
  }
  file.close();
  LOG(LOG_TRACE, "processFileMsg...out...");
  return 0;
}

void SigHandler(const int sig) {
  switch (sig) {
    case SIGUSR1:
      ProcessFileMsg(CMDFILE);
      break;
    default:
      need_restart = true;
      break;
  }
}

int ProcessKeepalive(int& childExitStatus, const std::string pidFile) {

  int processNum = 0;
  pid_t childPid = -1;

  while (1) {
    while (processNum < 1) {
      childPid = fork();
      if (childPid < 0) {
        LOG(LOG_FATAL_ERROR, "fork excute failed");
        return -1;
      }
      //child process
      else if (childPid == 0) {
        LOG(LOG_INFO, "child process ID %d", getpid());
        //child process has It's own signal handler
        signal(SIGTERM, SigHandler);
        signal(SIGKILL, SigHandler);
        signal(SIGUSR1, SigHandler);
        signal(SIGUSR2, SigHandler);
        return 0;
      }
      //parent process
      else {
        ++processNum;
        LOG(LOG_INFO, "try to keep PID = %d alive", childPid);
        //parent process forward the signal to child process
        signal(SIGINT, SigForward);
        signal(SIGTERM, SigForward);
        signal(SIGHUP, SigForward);
        signal(SIGUSR1, SigForward);
        signal(SIGUSR2, SigForward);
      }
    }
    //parent process
    LOG(LOG_INFO, "waiting for PID = %d", childPid);

    struct rusage resourceUsage;
    int exitPid = -1;
    int exitStatus = -1;
#ifdef HAVE_WAIT4
    exitPid = wait4(childPid, &exitStatus, 0, &resourceUsage);
#else
    memset(&resourceUsage, 0, sizeof(resourceUsage));
    exitPid = waitpid(childPid, &exitStatus, 0);
#endif
    LOG(LOG_INFO, "child process %d returned %d", childPid, exitPid);

    if (childPid == exitPid) {
      //delete pid file
      if (pidFile.c_str()) {
        unlink(pidFile.c_str());
      }

      //exit noemally
      if (WIFEXITED(exitStatus)) {
        LOG(LOG_INFO, "worker process PID = %d exited normally with exit-code = %d (it used %ld kBytes max",
            childPid, WEXITSTATUS(exitStatus), resourceUsage.ru_maxrss / 1024);
        childExitStatus = WEXITSTATUS(exitStatus);
        return 1;
      }
      //exit because of signal. parent process will fork child process again
      else if (WIFSIGNALED(exitStatus)) {
        LOG(LOG_INFO, "worker process PID = %d died on signal = %d (it used %ld kBytes max) ",
            childPid, WTERMSIG(exitStatus), resourceUsage.ru_maxrss / 1024);
        int timeToWait = 16;
        while (timeToWait > 0) {
          timeToWait = sleep(timeToWait);
        }
        --processNum;
        childPid = -1;
        signal(SIGINT, SIG_DFL);
        signal(SIGTERM, SIG_DFL);
        signal(SIGHUP, SIG_DFL);
      }
      else if (WIFSTOPPED(exitStatus)) {
        LOG(LOG_INFO, "child process is stopped and should restart later");
        --processNum;
        childPid = -1;
        sleep(16);
      }
      else {
        LOG(LOG_ERROR, "Can't get here!");
        --processNum;
        childPid = -1;
        sleep(16);
      }
    }
    else if (exitPid == -1) {
      if (errno != EINTR) {
        /* how can this happen ? */
        LOG(LOG_INFO, "wait4(%d, ...) failed. errno: %d", childPid, errno);
        return -1;
      }
    }
    else {
      LOG(LOG_ERROR, "Can't get here");
    }
  }
}

}  // namespace process
