#include <errno.h>
#include <unistd.h>
#include <signal.h>
#include <sys/wait.h>
#include <sys/select.h>

#include <fstream>
#include <algorithm>

#include "qconf_log.h"
#include "qconf_const.h"
#include "qconf_script.h"

using namespace std;

/**
 * read all script content
 */
static int script_content_read(const string &path, string &script);
static void sig_child(int sig);

static const string QCONF_SCRIPT_DIR("/script/");
static string _qconf_script_dir;

void qconf_init_script_dir(const string &agent_dir)
{
    _qconf_script_dir = agent_dir + QCONF_SCRIPT_DIR;
}

static void sig_child(int sig)
{
    pid_t pid;
    int status;
    while ((pid = waitpid(-1, &status, WNOHANG)) > 0)
    {
        if (WIFEXITED(status))
        {
            if (WEXITSTATUS(status)) 
            {
                LOG_ERR("Script not exit with success, ret:%d", WEXITSTATUS(status));
            }
        }
        else 
        {
            LOG_ERR("Child process didnot terminate normally, pid:%d", pid);
        }
    }
}

int execute_script(const string &script, const long mtimeout)
{
    if (script.empty()) return QCONF_ERR_PARAM;

    int     rv = 0;
    int     pfd[2];
    pid_t   pid;
    fd_set  set;
    struct  timeval timeout;
    timeout.tv_sec = mtimeout / 1000;
    timeout.tv_usec = mtimeout % 1000 * 1000000;

    struct sigaction act, oact;
    sigemptyset(&act.sa_mask);
    act.sa_handler = sig_child;
    act.sa_flags = SA_RESTART;
    sigaction(SIGCHLD, &act, &oact);

    if (pipe(pfd) < 0)
    {
        LOG_ERR("Failed to create pipe! error:%d", errno);
        return QCONF_ERR_OTHER; 
    }

    if ((pid = fork()) < 0) 
    {
        LOG_ERR("Failed to fork process! error:%d", errno);
        return QCONF_ERR_OTHER; 
    } 
    else if (0 == pid) 
    {
        setpgrp();
        close(pfd[0]);
        execl("/bin/sh", "sh", "-c", script.c_str(), (char *)NULL);
        _exit(127);  // won't be here, if execl was successful
    }

    close(pfd[1]);
    do
    {
        FD_ZERO(&set); 
        FD_SET(pfd[0], &set);
        rv = select(pfd[0] + 1, &set, NULL, NULL, &timeout);
        if (-1 == rv)
        {
            if (EINTR == errno) continue;
            else
            {
                LOG_ERR("Failed to select from pipe, error:%d!", errno);
                break;
            }
        }
        else if(0 == rv)
        {
            LOG_FATAL_ERR("Script execute timeout! script:%s", script.c_str());
            break;
        }
        else
        {
            close(pfd[0]); // sub process has exit
            // sigaction(SIGCHLD, &oact, NULL);
            return QCONF_OK;
        }
    } while (true);

    close(pfd[0]);
    kill(-pid, SIGKILL);
    // sigaction(SIGCHLD, &oact, NULL);
    if (0 == rv) return QCONF_ERR_SCRIPT_TIMEOUT;
    return QCONF_ERR_OTHER;
}

int find_script(const string &path,  string &script)
{
    if (path.empty()) return QCONF_ERR_PARAM;

    if (_qconf_script_dir.empty())
    {
        LOG_ERR("Not init qconf script dir");
        return QCONF_ERR_SCRIPT_DIR_NOT_INIT;
    }
    //find exist script file
    string script_path(_qconf_script_dir), tmp_path, file_path;
    tmp_path = (path[0] == '/') ? path.substr(1) : path;
    replace(tmp_path.begin(), tmp_path.end(), '/', QCONF_SCRIPT_SEPARATOR);
    script_path.append(tmp_path);
    size_t index = script_path.size();
    do 
    {
        script_path.resize(index);
        file_path = script_path + QCONF_SCRIPT_SUFFIX;
        if (-1 != access(file_path.c_str(), R_OK )) break; //script exist
        index = script_path.find_last_of(QCONF_SCRIPT_SEPARATOR);
    }
    while (string::npos != index);

    if (string::npos == index)
    {
        LOG_INFO("No script exist for %s!", script_path.c_str());
        return QCONF_ERR_SCRIPT_NOT_EXIST;
    }

    //read script content
    int ret = script_content_read(file_path, script);
    if (QCONF_OK != ret)
    {
        LOG_ERR("Failed to read all script content, file:%s", script_path.c_str());
        return QCONF_ERR_FAILED_READ_FILE;
    }
    return QCONF_OK;
}

static int script_content_read(const string &path, string &script)
{
    if (path.empty()) return QCONF_ERR_PARAM;
    ifstream in(path.c_str(), std::ifstream::in);
    if (!in)
    {
        LOG_ERR("Failed to open script file:%s", path.c_str());
        return QCONF_ERR_OTHER;
    }
    in.seekg(0, in.end);
    script.resize(in.tellg());
    in.seekg(0, in.beg);
    in.read(&script[0], script.size());
    in.close();
    return QCONF_OK;
}
