#include <stdio.h>
#include <errno.h>
#include <signal.h>
#include <string.h>
#include <unistd.h>
#include <pthread.h>

#include <iostream>

#include "qconf_zoo.h"
#include "qconf_log.h"
#include "qconf_shm.h"
#include "qconf_cmd.h"
#include "qconf_dump.h"
#include "qconf_const.h"
#include "qconf_config.h"
#include "qconf_daemon.h"
#include "qconf_script.h"
#include "qconf_watcher.h"
#include "qconf_feedback.h"

using namespace std;

extern int maxSlotsNum;
const string QCONF_PID_FILE("/pid");
const string QCONF_LOG_FMT("/logs/qconf.log.%Y-%m-%d-%H");

static void sig_handler(int sig);
static int qconf_agent_init(const string &agent_dir, const string &log_dir);
static void qconf_agent_destroy();

#define STRING_(str) #str
#define STRING(str) STRING_(str)

int main(int argc, char **arg)
{
    string agent_dir("..");
#ifdef QCONF_AGENT_DIR
    agent_dir = STRING(QCONF_AGENT_DIR);
#endif

    qconf_set_log_level(QCONF_LOG_ERR);
    LOG_INFO("agent_dir:%s", agent_dir.c_str());

    // check whether agent is running
    int pid_fd = 0;
    string pid_file = agent_dir + QCONF_PID_FILE;
    int ret = check_proc_exist(pid_file, pid_fd);
    if (QCONF_OK != ret) return ret;

    // load configure
    ret = qconf_load_conf(agent_dir);
    if (QCONF_OK != ret)
    {
        LOG_FATAL_ERR("Failed to load configure!");
        return ret;
    }

    // daemonize
    string value;
    long daemon_mode = 1;
    ret = get_agent_conf(QCONF_KEY_DAEMON_MODE, value); 
    if (QCONF_OK == ret) get_integer(value, daemon_mode);

    string log_fmt = agent_dir + QCONF_LOG_FMT;
    ret = get_agent_conf(QCONF_KEY_LOG_FMT, value);
    if (QCONF_OK == ret) log_fmt = value;
    size_t pos = log_fmt.find_last_of('/');
    string log_dir = (string::npos == pos) ? "." : string(log_fmt.c_str(), pos);

    long log_level = QCONF_LOG_ERR;
    ret = get_agent_conf(QCONF_KEY_LOG_LEVEL, value);
    if (QCONF_OK == ret) get_integer(value, log_level);
    qconf_log_init(log_fmt, log_level);

    if (daemon_mode != 0)
    {
        close(pid_fd);
        ret = qconf_agent_daemon_keepalive(pid_file);
        if (QCONF_OK != ret)
        {
            qconf_destroy_log();
            return ret;
        }
    }
    else
    {
        write_pid(pid_fd, getpid());
    }

    // TODO: using socket
    // Set the signal process handler
    signal(SIGPIPE, SIG_IGN);
    signal(SIGINT,  sig_handler);
    signal(SIGHUP,  sig_handler);
    signal(SIGTERM, sig_handler);
    signal(SIGUSR1, sig_handler);
    signal(SIGUSR2, sig_handler);

    // Environment initialize
    ret = get_agent_conf(SHARED_MEMORY_SIZE, value);
    if (ret == QCONF_OK) {
        maxSlotsNum = atoi(value.c_str());
    }
    else {
        maxSlotsNum = QCONF_MAX_SLOTS_NUM;
    }

    ret = qconf_agent_init(agent_dir, log_dir);
    if (QCONF_OK != ret)
    {
        LOG_ERR("Failed to init qconf agent!");
        return ret;
    }
    // main 
    watcher_setting_start();
   
    qconf_agent_destroy();

    return QCONF_OK;
}

static int qconf_agent_init(const string &agent_dir, const string &log_dir)
{
    string value;
    int ret = QCONF_OK;

    // init zookeeper log
    ret = get_agent_conf(QCONF_KEY_ZKLOG_PATH, value);
    if (QCONF_OK != ret)
        qconf_init_zoo_log(log_dir);
    else
        qconf_init_zoo_log(log_dir, value);

    // init cmd env
    ret = qconf_init_cmd_env(agent_dir);
    if (QCONF_OK != ret)
    {
        LOG_FATAL_ERR("Failed to init cmd environment");
        return ret;
    }

    // init dump env
    ret = qconf_init_dump_file(agent_dir);
    if (QCONF_OK != ret)
    {
        LOG_FATAL_ERR("Failed to init dump file");
        return ret;
    }

    // init share memory table
    ret = qconf_init_shm_tbl();
    if (QCONF_OK != ret)
    {
        LOG_FATAL_ERR("Failed to init share memory!");
        return ret;
    }

    // init message queue
    ret = qconf_init_msg_key();
    if (QCONF_OK != ret)
    {
        LOG_FATAL_ERR("Failed to init message queue!");
        return ret;
    }

    // init register node prefix
    string node_prefix(QCONF_DEFAULT_REGIESTER_PREFIX);
    ret = get_agent_conf(QCONF_KEY_REGISTER_NODE_PREFIX, value);
    if (QCONF_OK == ret) node_prefix = value;
    qconf_init_rgs_node_pfx(node_prefix);

    // init zookeeper operation timeout
    long zk_timeout = 3000;
    ret = get_agent_conf(QCONF_KEY_ZKRECVTIMEOUT, value);
    if (QCONF_OK == ret) get_integer(value, zk_timeout);
    qconf_init_recv_timeout(static_cast<int>(zk_timeout));

    // init local idc
    ret = get_agent_conf(QCONF_KEY_LOCAL_IDC, value);
    if (QCONF_OK != ret)
    {
        LOG_FATAL_ERR("Failed to get local idc!");
        return ret;
    }
    ret = qconf_init_local_idc(value);
    if (QCONF_OK != ret)
    {
        LOG_FATAL_ERR("Failed to set local idc!");
        return ret;
    }

    // init script dir
    qconf_init_script_dir(agent_dir);
    
    // init script execute timeout
    long sc_timeout = 3000;
    ret = get_agent_conf(QCONF_KEY_SCEXECTIMEOUT, value);
    if (QCONF_OK == ret) get_integer(value, sc_timeout);
    qconf_init_scexec_timeout(static_cast<int>(sc_timeout));

#ifdef QCONF_CURL_ENABLE
    long fd_enable = 0;
    ret = get_agent_conf(QCONF_KEY_FEEDBACK_ENABLE, value);
    if (QCONF_OK == ret) get_integer(value, fd_enable);
    if (1 == fd_enable)
    {
        qconf_init_fb_flg(true);
       
        ret = get_agent_conf(QCONF_KEY_FEEDBACK_URL, value);
        if (QCONF_OK != ret)
        {
            LOG_FATAL_ERR("Failed to get feedback url!");
            return ret;
        }

        ret = qconf_init_feedback(value);
        if (QCONF_OK != ret)
        {
            LOG_FATAL_ERR("Failed to init feedback!");
            return ret;
        }
    }
#endif

    return QCONF_OK;
}

static void qconf_agent_destroy()
{
#ifdef QCONF_CURL_ENABLE
    qconf_destroy_feedback();
#endif
    qconf_destroy_zk();
    qconf_destroy_conf_map();
    qconf_destroy_dbf();
    qconf_destroy_dump_lock();
    qconf_destroy_qhasharr_lock();
    qconf_destroy_zoo_log();
    qconf_destroy_log();
}

static void sig_handler(int sig)
{
    switch(sig)
    {
    case SIGINT:
        break;
    case SIGTERM:
    case SIGUSR2:
        qconf_thread_exit();
        //qconf_agent_destroy();
        //exit(0);
        break;
    case SIGHUP:
        break;
    case SIGUSR1:
        qconf_cmd_proc();
        break;
    default:
        break;
    }
}
