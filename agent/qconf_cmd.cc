#include <errno.h>
#include <fcntl.h>
#include <fstream>
#include <sstream>
#include <stdio.h>
#include <unistd.h>
#include <dirent.h>
#include <stdlib.h>
#include <sys/stat.h>

#include <vector>

#include "qconf_cmd.h"
#include "qconf_log.h"
#include "qconf_shm.h"
#include "qconf_dump.h"
#include "qconf_common.h"
#include "qconf_const.h"
#include "qconf_watcher.h"

using namespace std;

const string _cmd_pre("cmd");
const string _result_pre("result");

static string _cmd_dir("cmd");
static string _result_dir("result");

//delimiter
const char _cmd_delim = '#';

//command type
const string _cmd_stop_listen("stop_listen");
const string _cmd_restart_listen("restart_listen");
const string _cmd_list_all("list-all");
const string _cmd_clear_all("clear-all");
const string _cmd_ls("ls");
const string _cmd_del("delete");
const string _cmd_get("get");
const string _cmd_create("create");
const string _cmd_set("set");
const string _cmd_server_add("serve_add");
const string _cmd_server_del("serve_delete");

int qconf_write_file(const string &file_str, const string &content, int append);
static int operate_list_all(string &result);
static int operate_clear_all(string &result);

/**
 * initial method
 */
int qconf_init_cmd_env(const string &qconf_dir)
{
    _cmd_dir = qconf_dir + "/cmd";
    _result_dir = qconf_dir + "/result";

    mode_t mode = 0755;
    if (-1 == access(_cmd_dir.c_str(), F_OK))
    {
        if (-1 == mkdir(_cmd_dir.c_str(), mode))
        {
            LOG_ERR("Failed to create cmd directory! errno:%d", errno);
            return QCONF_ERR_MKDIR;
        }
    }
    if (-1 == access(_result_dir.c_str(), F_OK))
    {
        if (-1 == mkdir(_result_dir.c_str(), mode))
        {
            LOG_ERR("Failed to create result directory! errno:%d", errno);
            return QCONF_ERR_MKDIR;
        }
    }
    return QCONF_OK;
}

/**
 * write content into file, append mode if the third paremeter is non_zero
 */
int qconf_write_file(const string &file_str, const string &content, bool append)
{
    ofstream out;
    if (!append)
    {
        out.open(file_str.c_str(), ofstream::out);
        if (!out)
        {
            LOG_ERR("Failed to open file:%s!", file_str.c_str());
            return QCONF_ERR_OPEN;
        }
    }
    else
    {
        //append write
        out.open(file_str.c_str(), ofstream::out | ofstream::app);
        if (!out)
        {
            LOG_ERR("Failed to open file:%s!", file_str.c_str());
            return QCONF_ERR_OPEN;
        }
    }

    out << content << endl;
    out.close();
    return QCONF_OK;
}

static int operate_stop_listen(const string &host, string &result)
{
    return QCONF_OK;
}

static int operate_restart_listen(const string &host, string &result)
{
    return QCONF_OK;
}

static int operate_list_all(string &result)
{
    return QCONF_OK;
}

static int operate_clear_all(string &result)
{
    qconf_clear_shm_tbl();

    qconf_dump_clear();

    result = "clear share memory success!";

    return QCONF_OK;
}

static int operate_list(const string &host, const string &path, string &result)
{
    return QCONF_OK;
}

static int operate_delete(const string &host, const string &path, string &result)
{
    return QCONF_OK;
}

static int operate_get(const string &host, const string &path, string &result)
{
    return QCONF_OK;
}

static int operate_create(const string &host, const string &path, const string &values, string &result)
{
    return QCONF_OK;
}

static int operate_set(const string &host, const string &path, const string &values, string &result)
{
    return QCONF_OK;
}

static int operate_serve_add(const string &host, const string &path, const string &values, string &result)
{
    return QCONF_OK;
}

static int operate_serve_delete(const string &host, const string &path, const string &values, string &result)
{
    return QCONF_OK;
}

/**
 * Parse one command.
 * command format like this : set#host1#path1;path2;path3;#value1;value2;value3#
 * return -1 if error happened,otherwise return 0
 */
static int single_command_process(const string &line, const string &custom_id)
{
    if (custom_id.empty() || line.empty()) return QCONF_ERR_PARAM;

    string item;
    string result;
    int ret = QCONF_OK;
    stringstream ss(line);
    vector<string> parameters;

    while (getline(ss, item, _cmd_delim))
    {
        if (item.length() > 0) parameters.push_back(item);
    }

    if (!parameters.empty())
    {
        string command(parameters[0]);
        size_t para_length = parameters.size();
        if (command == _cmd_stop_listen)
        {
            if (2 != para_length)
            {
                LOG_ERR("Operand error for command:%s", _cmd_stop_listen.c_str());
                return QCONF_ERR_CMD;
            }
            ret = operate_stop_listen(parameters[1], result);
        }
        else if (_cmd_restart_listen == command)
        {
            if (2 != para_length)
            {
                LOG_ERR("Operand error for command:%s", _cmd_restart_listen.c_str());
                return QCONF_ERR_CMD;
            }
            ret = operate_restart_listen(parameters[1], result);
        }
        else if (_cmd_list_all == command)
        {
            if (1 != para_length)
            {
                LOG_ERR("Operand error for command:%s", _cmd_list_all.c_str());
                return QCONF_ERR_CMD;
            }
            ret = operate_list_all(result);
        }
        else if (_cmd_clear_all == command)
        {
            if (1 != para_length)
            {
                LOG_ERR("Operand error for command:%s", _cmd_clear_all.c_str());
                return QCONF_ERR_CMD;
            }
            ret = operate_clear_all(result);
        }
        else if (_cmd_ls == command)
        {
            if (3 != para_length)
            {
                LOG_ERR("Operand error for command:%s", _cmd_ls.c_str());
                return QCONF_ERR_CMD;
            }
            ret = operate_list(parameters[1], parameters[2], result);
        }
        else if (_cmd_del == command)
        {
            if (3 != para_length)
            {
                LOG_ERR("Operand error for command:%s", _cmd_del.c_str());
                return QCONF_ERR_CMD;
            }
            ret = operate_delete(parameters[1], parameters[2], result);
        }
        else if (_cmd_get == command)
        {
            if (3 != para_length)
            {
                LOG_ERR("Operand error for command:%s", _cmd_get.c_str());
                return QCONF_ERR_CMD;
            }
            ret = operate_get(parameters[1], parameters[2], result);
        }
        else if (_cmd_create == command)
        {
            if (4 != para_length)
            {
                LOG_ERR("Operand error for command:%s", _cmd_create.c_str());
                return QCONF_ERR_CMD;
            }
            ret = operate_create(parameters[1], parameters[2], parameters[3], result);
        }
        else if (_cmd_set == command)
        {
            if (4 != para_length)
            {
                LOG_ERR("Operand error for command:%s", _cmd_set.c_str());
                return QCONF_ERR_CMD;
            }
            ret = operate_set(parameters[1], parameters[2], parameters[3], result);
        }
        else if (_cmd_server_add == command)
        {
            if (4 != para_length)
            {
                LOG_ERR("Operand error for command:%s", _cmd_server_add.c_str());
                return QCONF_ERR_CMD;
            }
            ret = operate_serve_add(parameters[1], parameters[2], parameters[3], result);
        }
        else if (_cmd_server_del == command)
        {
            if (4 != para_length)
            {
                LOG_ERR("Operand error for command:%s", _cmd_server_del.c_str());
                return QCONF_ERR_CMD;
            }
            ret = operate_serve_delete(parameters[1], parameters[2], parameters[3], result);
        }
        else
        {
            LOG_ERR("Error command! command:%s", command.c_str());
            return QCONF_ERR_CMD;
        }
    }
    else
    {
        LOG_ERR("No correct delimiter in the command line!");
        return QCONF_ERR_CMD;
    }

    if (QCONF_OK == ret)
    {
        if (result.length() > 0)
        {
            string result_file(_result_dir);
            result_file = result_file + "/" + _result_pre + custom_id;
            if ((ret = qconf_write_file(result_file, result, true)) == -1)
                LOG_ERR("Failed to write the result file:%s!", result_file.c_str());
        }
    }

    return ret;
}

/**
 * process the command in cmd directory and return cmd file number
 */
size_t qconf_cmd_proc()
{
    int ret = QCONF_OK;
    DIR *cmd_dir = NULL;
    size_t file_count = 0;
    struct dirent *file = NULL;

    if (NULL == (cmd_dir = opendir(_cmd_dir.c_str())))
    {
        LOG_ERR("Faild to open cmd_dir:%s", _cmd_dir.c_str());
        return QCONF_ERR_OPEN;
    }

    while (NULL != (file = readdir(cmd_dir)))
    {
        string file_name(file->d_name);
        if (0 == file_name.find_first_of(_cmd_pre))
        {
            file_count++;

            string file_path(_cmd_dir);
            file_path += "/" + file_name;
            ifstream cmd_file(file_path.c_str());
            if (!cmd_file)
            {
                LOG_ERR("Faild to read cmd_file! file name:%s", file_name.c_str());
                continue;
            }

            //record the id of the cmd file which will be append to reslut file name
            string console_pid(file_name, _cmd_pre.length());

            //read file by line
            string line;
            while (getline(cmd_file, line))
            {
                ret = single_command_process(line, console_pid);
                if (QCONF_OK != ret)
                    LOG_ERR("command proces failed!");
            }
            cmd_file.close();
            unlink(file_path.c_str());
        }
    }
    closedir(cmd_dir);
    return file_count;
}
