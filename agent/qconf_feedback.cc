#include <errno.h>
#include <unistd.h>
#include <string.h>
#include <arpa/inet.h>
#include <netinet/in.h>
#include <sys/socket.h>
#include <zk_adaptor.h>

#include <sstream>

#include "qconf_const.h"
#include "qconf_log.h"
#include "qconf_feedback.h"
#include "qconf_format.h"
#include "qlibc.h"

using namespace std;

#ifdef QCONF_CURL_ENABLE
#include <curl/curl.h>
static CURL *_curl;
static string _feedback_buf;

static int write_callback(void *buffer, size_t size, size_t nmemb, void *userp);

int qconf_init_feedback(const string &url)
{
    if (CURLE_OK != curl_global_init(CURL_GLOBAL_ALL)) return QCONF_ERR_OTHER;
    if (NULL == (_curl = curl_easy_init())) return QCONF_ERR_OTHER;
    curl_easy_setopt(_curl, CURLOPT_URL, url.c_str()); 
    curl_easy_setopt(_curl, CURLOPT_POST, 1);
    return QCONF_OK;
}

void qconf_destroy_feedback()
{
    curl_easy_cleanup(_curl);
    curl_global_cleanup();
}

int feedback_process(const string &content)
{
    if (NULL == _curl) return QCONF_ERR_OTHER;

    _feedback_buf.clear();
    curl_easy_setopt(_curl, CURLOPT_WRITEFUNCTION, write_callback);
    curl_easy_setopt(_curl, CURLOPT_POSTFIELDS, content.c_str());
    CURLcode curlcode = (CURLcode)0;
    int i = 0;
    for (i = 0; i < QCONF_FB_RETRIES; ++i)
    {
        curlcode = curl_easy_perform(_curl);
        if (CURLE_OK == curlcode)
        {
            // Check whether the feedback return value is OK
            if (QCONF_FB_RESULT != _feedback_buf)
            {
                LOG_FATAL_ERR("Fail to curl the feedback url!"
                        "return from curl server:%s, feedback content:%s",
                        _feedback_buf.c_str(), content.c_str());
                return QCONF_ERR_OTHER;
            }
            break;
        }
        else
        {
            LOG_ERR("Failed to call curl_easy_perform! curlcode:%u", curlcode);
            usleep(50000);
            continue;
        }
    }

    if (QCONF_FB_RETRIES == i)
    {
        LOG_FATAL_ERR("Failed to call curl_easy_perform! Already try times:%d",
                QCONF_FB_RETRIES);
        return QCONF_ERR_FB_TIMEOUT;
    }
    return QCONF_OK;
}

int feedback_generate_content(const string &ip, char data_type, const string &idc, const string &path, const fb_val &fbval, string &content)
{
    if (ip.empty() || idc.empty() || path.empty()) return QCONF_ERR_PARAM;

    // generte hostname
    char hostname[256] = {0};
    if (-1 == gethostname(hostname, sizeof(hostname)))
    {
        LOG_ERR("Failed to get hostname! errno:%d", errno);
        return QCONF_ERR_OTHER;
    }

    // generate send value
    string value;
    switch (data_type)
    {
        case QCONF_DATA_TYPE_NODE:
            tblval_to_nodeval(fbval.tblval, value);
            break;
        case QCONF_DATA_TYPE_SERVICE:
        case QCONF_DATA_TYPE_BATCH_NODE:
            value = fbval.fb_chds;
            break;
        default:
            return QCONF_ERR_PARAM;
    }
    
    // generate value md5
    unsigned char value_md5_int[QCONF_MD5_INT_LEN];
    qhashmd5(value.c_str(), value.size(), value_md5_int);
    char value_md5_str[QCONF_MD5_STR_LEN + 1];
    qhashmd5_bin_to_hex(value_md5_str, value_md5_int, QCONF_MD5_INT_LEN);
    value_md5_str[QCONF_MD5_STR_LEN] = '\0';

    stringstream ss;
    ss << "hostname=" << hostname << "&ip=" << ip << "&node_whole=" << path;
    ss << "&value_md5=" << value_md5_str << "&idc=" << idc;
    ss << "&update_time=" << time(NULL) << "&data_type=" << data_type;
    ss >> content;

    return QCONF_OK;
}

void feedback_generate_chdval(const string_vector_t &chdnodes, const vector<char> &status, string &value)
{
    char s;
    for (int i = 0; i < chdnodes.count; ++i)
    {
        s = status[i] + '0';
        value = value + chdnodes.data[i] + "#" + s + ",";
    }
    if (value.size() != 0) value.resize(value.size() - 1);
}

void feedback_generate_batchval(const string_vector_t &batchnodes, string &value)
{
    for (int i = 0; i < batchnodes.count; ++i)
    {
        value = value + batchnodes.data[i] + ",";
    }
    if (value.size() != 0) value.resize(value.size() - 1);
}

/**
 * get ip by zookeeper zhandle_t
 */
int get_feedback_ip(const zhandle_t *zh, string &ip_str)
{
    if (NULL == zh) return QCONF_ERR_PARAM;

    char ip[16] = {0};
    struct sockaddr_in sin;
    memset(&sin, 0, sizeof(struct sockaddr_in));
    socklen_t sin_len = sizeof(struct sockaddr_in);
    if (0 == getsockname(zh->fd, (struct sockaddr*)&sin, &sin_len))
    {
        if (NULL == inet_ntop(AF_INET, &(sin.sin_addr), ip, sizeof(ip)))
        {
            LOG_ERR("Failed to call inet_ntop! errno:%d", errno);
            return QCONF_ERR_OTHER;
        }
        ip_str.assign(ip);
    }
    else
    {
        LOG_ERR("Failt to getsockname! errno:%d", errno);
        return QCONF_ERR_OTHER;
    }
    return QCONF_OK;
}

/**
 * feedback curl callback function
 */
static int write_callback(void *buffer, size_t size, size_t nmemb, void *userp)
{
    if (NULL == buffer) return QCONF_ERR_OTHER;
    size_t realsize = size * nmemb;
    _feedback_buf.append((char *)buffer, realsize);
    return realsize;
}
#endif
