#include <errno.h>
#include <regex.h>
#include <stdio.h>
#include <limits.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <assert.h>
#include <netdb.h>
#include <arpa/inet.h>
#include <sys/types.h>
#include <sys/socket.h>

#include <map>
#include <string>
#include <iostream>
#include <sstream>

#include "qconf_log.h"
#include "qconf_const.h"
#include "qconf_config.h"

using namespace std;

//length of one line in config file and path
#define QCONF_LINE_MAX_LEN			        1024

// configure file name
#define QCONF_IDC_CONF_PATH     		    "/conf/idc.conf"
#define QCONF_AGENT_CONF_PATH               "/conf/agent.conf"
#define QCONF_LOCAL_IDC_PATH                "/conf/localidc"

static string strtrim(const string &cnt);
static int is_ip_port(const string &item);
static int get_first_host_by_name(const string &domain, string &ip_address);
static int is_domain_port(const string &item);
static int is_valid_idc(const string &idc, const string &value);
static int is_valid_conf(const string &key, const string &value);
static int load_conf_(const string &conf_path);
static int load_localidc_conf_(const string &localidc_path);
static void printf_map(const map<string, string> &map);

//data structure
static map<string, string> _agent_conf_map;
static map<string, string> _idc_conf_map;

typedef map<string, string>::iterator map_iterator;
typedef map<string, string>::const_iterator const_map_iterator;

static void printf_map(const map<string, string> &map)
{
    LOG_INFO("Map Content : pid: %d", getpid());

    for (const_map_iterator it = map.begin(); it != map.end(); ++it) 
    {
        LOG_INFO("%s=%s", it->first.c_str(), it->second.c_str());
    }
}

/**
 * Judge whether the content can be converted to integer
 */
int get_integer(const string &cnt, long &integer)
{
    if(cnt.empty()) return QCONF_ERR_PARAM;
    errno = 0;
    char *endptr = NULL;
    long value = strtol(cnt.c_str(), &endptr, 0);

    //over range
	if ((LONG_MAX == value || LONG_MIN == value) && ERANGE == errno)
        return QCONF_ERR_OUT_OF_RANGE;
    //Illegal number
    if (cnt == endptr || (0 == value && EINVAL == errno))
        return QCONF_ERR_NOT_NUMBER;
    //further characters exists
    if ('\0' != *endptr) return QCONF_ERR_OTHRE_CHARACTER;

    integer = value;
    return QCONF_OK;
}

/**
 * Trim blank at the begin and end of content
 */
static string strtrim(const string &cnt)
{
    if (cnt.empty()) return cnt;

    string tmp_str;
    size_t cnt_len = cnt.size();
    const char *begin = cnt.data();
    const char *end = cnt.data() + cnt_len - 1;

    while (' ' == *begin && begin <= end) ++begin;
    while (' ' == *end && begin <= end) --end;

    if (begin > end) return tmp_str;

    tmp_str.assign(begin, end + 1 - begin);

    return tmp_str; 
}

/** 
 * Judge whether the content is ip_port
 */
static int is_ip_port(const string &item)
{
    if (item.empty()) return QCONF_ERR_PARAM;
 
    int status = 0;
    regex_t reg = {0};
    regmatch_t pmatch[1];
    const size_t nmatch = 1;
    char errbuf[1024] = {0};
    int cflags = REG_EXTENDED | REG_NOSUB;
    const char * pattern = "^([0-9]{1,3}\\.){3}([0-9]{1,3}){1}:[0-9]{1,5}$";

    status = regcomp(&reg, pattern, cflags);
    if (0 != status)
    {
        regerror(status, &reg, errbuf, sizeof(errbuf));
        LOG_ERR("error happened when compile regex error: %s", errbuf);
        return QCONF_ERR_OTHER;
    }
    
    status = regexec(&reg, item.c_str(), nmatch, pmatch, 0);
    if (0 == status)
    {
        regfree(&reg);
        return QCONF_OK;
    }
    else
    {
        regerror(status, &reg, errbuf, sizeof(errbuf));
        LOG_ERR("error happened when execute regex error: %s", errbuf);
        regfree(&reg);
        return QCONF_ERR_OTHER;
    }
}

static int get_first_host_by_name(const string &domain, string &ip_address)
{
    char **pptr;
    const char *ip_res;
    struct hostent *hptr;
    char str[32];

    if(NULL == (hptr = gethostbyname(domain.c_str())))
        return QCONF_ERR_OTHER;

    switch(hptr->h_addrtype)
    {
        case AF_INET:
        case AF_INET6:
            pptr=hptr->h_addr_list;
            if (pptr)
                ip_res = inet_ntop(hptr->h_addrtype, *pptr, str, sizeof(str));
            if (ip_res)
            {
                ip_address.assign(ip_res);
                return QCONF_OK;
            }
            break;
        default:
            break;
    }

    LOG_ERR("Invalid domain name: %s", domain.c_str());
    return QCONF_ERR_OTHER;
}

/**
 * Judge whether the content is ip_port
 */
static int is_domain_port(const string &item)
{
    if (item.empty()) return QCONF_ERR_PARAM;

    string::size_type spos;
    string domain, _port, ip_address;

    spos = item.find(':');
    if (spos == string::npos)
        return QCONF_ERR_OTHER;
    domain = item.substr(0, spos);
    _port = item.substr(spos);

    if (QCONF_OK != get_first_host_by_name(domain, ip_address))
        return QCONF_ERR_OTHER;
    if (QCONF_OK != is_ip_port(ip_address + _port))
        return QCONF_ERR_OTHER;

    return QCONF_OK;
}

/**
 * Validation for idc map
 */
static int is_valid_idc(const string &idc, const string &value)
{
    if (idc.empty() || value.empty()) return QCONF_ERR_PARAM;

    string idc_address;
    stringstream ss(value);

    while (getline(ss, idc_address, ','))
    {
        if ((QCONF_OK != is_domain_port(idc_address)) && (QCONF_OK != is_ip_port(idc_address)))
        {
            LOG_ERR("Invalid of ip_port:%s\n", idc_address.c_str());
            return QCONF_ERR_INVALID_IP;
        }
    }

    return QCONF_OK;
}

/**
 * Validation for conf map
 */
static int is_valid_conf(const string &key, const string &value)
{
    if (key.empty() || value.empty()) return QCONF_ERR_PARAM;

    return QCONF_OK;
}

int qconf_load_conf(const string &agent_dir)
{
    string agent_conf_path = agent_dir + QCONF_AGENT_CONF_PATH;
    string idc_conf_path = agent_dir + QCONF_IDC_CONF_PATH;
    string localidc_path = agent_dir + QCONF_LOCAL_IDC_PATH;

    // init conf map
    int ret = load_conf_(agent_conf_path);
    if (QCONF_OK != ret) return ret;
    ret = load_conf_(idc_conf_path);
    if (QCONF_OK != ret) return ret;
    ret = load_localidc_conf_(localidc_path);
    if (QCONF_OK != ret) return ret;

    printf_map(_agent_conf_map);
    printf_map(_idc_conf_map);

    return ret;
}

static int load_localidc_conf_(const string &localidc_path)
{
    if (localidc_path.empty()) return QCONF_ERR_PARAM;

    char idc_buf[QCONF_MAX_BUF_LEN] = {0};
    FILE *fp = fopen(localidc_path.c_str(), "r");
    if (NULL == fp)
    {
        LOG_FATAL_ERR("Failed to open local idc file! path:%s, errno:%d",
                localidc_path.c_str(), errno);
        return QCONF_ERR_OPEN;
    }
    if (NULL == fgets(idc_buf, sizeof(idc_buf), fp))
    {
        LOG_FATAL_ERR("Failed to read local idc file! path:%s",
                localidc_path.c_str());
        return QCONF_ERR_READ;
    }
   
    size_t idc_len = strlen(idc_buf);
    if (0 == idc_len)
    {
        LOG_FATAL_ERR("local idc is null!");
        return QCONF_ERR_NULL_LOCAL_IDC;
    }

    if ('\n' == idc_buf[idc_len - 1]) idc_buf[idc_len - 1] = '\0';
    
    _agent_conf_map.insert(make_pair<string, string>(QCONF_KEY_LOCAL_IDC, idc_buf));

    fclose(fp);
    fp = NULL;

    return QCONF_OK;
}

static int load_conf_(const string &conf_path)
{
    if (conf_path.empty()) return QCONF_ERR_PARAM;

    //open config file
    FILE *conf_file = fopen(conf_path.c_str(), "r"); 
    if (NULL == conf_file)
    {
        LOG_FATAL_ERR("Failed to open conf file! path:%s", conf_path.c_str());
        return QCONF_ERR_OPEN;
    }

    //read config file
    size_t line_num = 0;
    pair<map_iterator, bool> ret;
    char line[QCONF_LINE_MAX_LEN] = {0};
    while (NULL != fgets(line, QCONF_LINE_MAX_LEN, conf_file))
    {
        line_num++;

        //empty line or comment line
        if ('\0' == line[0] || '#' == line[0] || '\n' == line[0])
            continue;

        //remove the LF at the end of line
        line[strlen(line) - 1] = '\0';

        //format error line
        char *delimiter = NULL;
        if (NULL == (delimiter = strchr(line, '=')))
        {
            LOG_ERR("Error format of conf file! line:%zd", line_num);
            continue;
        }

        string key(line, delimiter - line);
        string value(delimiter + 1);

        key = strtrim(key);
        value = strtrim(value);

        //idc map
        if (0 == key.find("zookeeper."))
        {
            string idc(key, key.find(".") + 1);
            if (QCONF_OK != is_valid_idc(idc, value))
            {
                LOG_ERR("Invalid ips of idc! line_num:%zd, idc:%s, value:%s",
                        line_num, idc.c_str(), value.c_str());
                continue;
            }

            ret = _idc_conf_map.insert(make_pair<string, string>(idc, value));
            if (!ret.second)
            {
                LOG_ERR("Failed to put idc map item:<%s, %s>",
                        idc.c_str(), value.c_str());
                continue;
            }
        }
        else
        {
            if (QCONF_OK != is_valid_conf(key, value))
            {
                LOG_ERR("Invalid conf of key! line_num:%zd, key:%s, value:%s",
                        line_num, key.c_str(), value.c_str());
                continue;
            }

            ret = _agent_conf_map.insert(make_pair<string, string>(key, value));
            if (!ret.second)
            {
                LOG_ERR("Failed to put conf_map item:<%s, %s>", 
                        key.c_str(), value.c_str());
                continue;
            }
        }
    }
    fclose(conf_file);
    conf_file = NULL;

    return QCONF_OK;
}


/**
 * Get config value
 */
int get_agent_conf(const string &key, string &value)
{
    if (key.empty()) return QCONF_ERR_PARAM;

    map_iterator it = _agent_conf_map.find(key);
    if (it == _agent_conf_map.end())
    {
        LOG_WARN("Failed to get conf! key:%s", key.c_str());
        return QCONF_ERR_NOT_FOUND;
    }
    
    value.assign(it->second);

    return QCONF_OK;
}

/**
 * Get host value of idc
 */
int get_idc_conf(const string &idc, string &value)
{
    if (idc.empty()) return QCONF_ERR_PARAM;

    map_iterator it = _idc_conf_map.find(idc);
    if (it == _idc_conf_map.end())
    {
        LOG_ERR("Failed to get conf! idc:%s", idc.c_str());
        return QCONF_ERR_NOT_FOUND;
    }
    
    value.assign(it->second);

    return QCONF_OK;
}

/**
 * Destroy the configuration environment
 */
void qconf_destroy_conf_map()
{
    _agent_conf_map.clear();
    _idc_conf_map.clear();
}
