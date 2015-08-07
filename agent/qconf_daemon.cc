#include <errno.h>
#include <fcntl.h>
#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <unistd.h>

#include "qconf_log.h"
#include "qconf_const.h"
#include "qconf_daemon.h"

using namespace std;

static void daemonize();
static void signal_forward(int sig);

/**
 * Start daemon dread to process the console request
 */
static void daemonize()
{
    pid_t pid = fork();
    if (pid < 0)
    {
        LOG_ERR("Failed to fork when try to daemonize!");
        exit(EXIT_FAILURE);
    }
    if (pid > 0) exit(EXIT_SUCCESS);

    //let child thread be session leader
    if (setsid() < 0)
    {
        LOG_ERR("Failed to make child process be session leader");
        exit(EXIT_FAILURE);
    }

    signal(SIGHUP, SIG_IGN);

    //fork the second time
    pid = fork();

    if (pid < 0)
    {
        LOG_ERR("Failed to second fork!");
        exit(EXIT_FAILURE);
    }
    if (pid > 0) exit(EXIT_SUCCESS);

    umask(0);

    int fd = open("/dev/null", O_RDWR);
    if (fd == -1)
    {
        LOG_ERR("open /dev/null failed! errno:%d", errno);
    }
    else
    {
        dup2(fd, 0);
        dup2(fd, 1);
        dup2(fd, 2);

        if (fd >= 3) close(fd);
    }

    qconf_close_log_stream();

    for (fd = sysconf(_SC_OPEN_MAX); fd >= 3; fd--)
    {
        close(fd);
    }

    LOG_INFO("success daemonize:%d", getpid());
}

/**
 * Signal handler for parent of current daemon process
 */
static void signal_forward(int sig)
{
    signal(sig, SIG_IGN);
    kill(0, sig);  //send the signal to all processes in same thread group
}

int check_proc_exist(const string &pid_file, int &pid_fd)
{
    pid_fd = open(pid_file.c_str(), O_RDWR | O_TRUNC | O_CREAT, 0644);
    if (-1 == pid_fd) return QCONF_ERR_OPEN;

    int ret = lockf(pid_fd, F_TLOCK, 0);
    if (-1 == ret) return QCONF_ERR_LOCK;

    return QCONF_OK;
}

void write_pid(int fd, pid_t chd_pid)
{
    char buf[32] = {0};

    snprintf(buf, sizeof(buf), "%d", chd_pid);
    lseek(fd, 0, SEEK_SET);
    write(fd, buf, strlen(buf));
}

int qconf_agent_daemon_keepalive(const string &pid_file)
{
    daemonize();

    int pid_fd = 0;
    int ret = check_proc_exist(pid_file, pid_fd);
    if (QCONF_OK != ret) return ret;

    int nprocs = 0;
    pid_t child_pid = -1;

    while (true)
    {
        while (nprocs < 1)
        {
            pid_t pid = fork();
            if (0 == pid)
            {
                LOG_INFO("current child process id is %d", getpid());
                return QCONF_OK;
            }
            else if (pid < 0)
            {
                LOG_ERR("error happened when fork child. errno:%d", errno);
                return QCONF_ERR_OTHER;
            }

            LOG_INFO("we try to keep alive for daemon process:%d", pid);
            //forward some signal
            signal(SIGPIPE, signal_forward);
            signal(SIGINT, signal_forward);
            signal(SIGTERM, signal_forward);
            signal(SIGHUP, signal_forward);
            signal(SIGUSR1, signal_forward);
            signal(SIGUSR2, signal_forward);
            child_pid = pid;
            nprocs++;
        }

        // Check child thread
        if (-1 != child_pid)
        {
            LOG_INFO("waiting for pid:%d", child_pid);
            //parent process keep alive for the current daemon process
            int exit_status;
            pid_t exit_pid;

            write_pid(pid_fd, child_pid);

            exit_pid = waitpid(child_pid, &exit_status, 0);

            if (exit_pid == child_pid)
            {
                if (WIFEXITED(exit_status))
                {
                    // exited normal
                    LOG_INFO("PID:%d exited with exit_code:%d", child_pid, WEXITSTATUS(exit_status));
                    if (0 == WEXITSTATUS(exit_status))
                    {
                        // if child process exist success, then return, or restart
                        LOG_INFO("Child PID:%d exited normally, now current PID:%d exited!", child_pid, getpid());
                        unlink(pid_file.c_str());
                        return QCONF_EXIT_NORMAL;
                    }

                    // wait and then restart the daemon
                    int time_to_wait = 1;
                    signal(SIGINT, SIG_DFL);
                    signal(SIGTERM, SIG_DFL);
                    signal(SIGHUP, SIG_DFL);
                    sleep(time_to_wait);
                    child_pid = -1;
                    nprocs--;
                }
                else if (WIFSIGNALED(exit_status))
                {
                    //exited because signal
                    LOG_INFO("PID:%d died on signal:%d", child_pid, WTERMSIG(exit_status));

                    //wait and then restart the daemon
                    int time_to_wait = 1;
                    signal(SIGINT, SIG_DFL);
                    signal(SIGTERM, SIG_DFL);
                    signal(SIGHUP, SIG_DFL);
                    sleep(time_to_wait);
                    child_pid = -1;
                    nprocs--;
                }
                else if (WIFSTOPPED(exit_status))
                {}
                else
                {
                    LOG_ERR("should never be here!");
                }
            }
            else if (exit_pid == -1)
            {
                if (errno != EINTR)
                    LOG_ERR("waitpid failed for :%d, errno:%d", child_pid, errno);
            }
            else
            {
                LOG_ERR("should never be here!");
            }
        }
    }
    return QCONF_OK;
}
