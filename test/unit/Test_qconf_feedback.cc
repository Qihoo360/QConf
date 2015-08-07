#include <zookeeper.h>
#include <sys/fcntl.h>

#include <string>

#include "gtest/gtest.h"
#include "qconf_const.h"
#include "qconf_format.h"
#include "qconf_feedback.h"

using namespace std;

static void create_string_vector(string_vector_t *nodes, size_t count, bool serv_flg = false);

class Test_qconf_feedback : public ::testing::Test
{
    public:
        virtual void SetUp()
        {
            host_ = "127.0.0.1:2181";
            zh_ = zookeeper_init(host_.c_str(), NULL, QCONF_ZK_DEFAULT_RECV_TIMEOUT, NULL, NULL, 0);
        }

        virtual void TearDown()
        {
            zookeeper_close(zh_);
        }

    public:
        zhandle_t *zh_;
        string host_;
};

/**
 * ========================================================================================================
 * Begin_Test_for function:
 * int get_feedback_ip(const zhandle_t *zh, string &ip_str)
 */
// Test for get_feedback_ip: zh is null
TEST_F(Test_qconf_feedback, get_feedback_ip_null_zh)
{
    zhandle_t *zh = NULL;
    string ip;

    int ret = get_feedback_ip(zh, ip);

    EXPECT_EQ(QCONF_ERR_PARAM, ret);
}

// Test for get_feedback_ip: connect zh
TEST_F(Test_qconf_feedback, get_feedback_ip_connect_zh)
{
    string ip;

    zoo_exists(zh_, "/qconf", 0, NULL);
    int ret = get_feedback_ip(zh_, ip);

    EXPECT_EQ(QCONF_OK, ret);
    EXPECT_EQ(0, strncmp(host_.c_str(), ip.c_str(), ip.size()));
}

// Test for get_feedback_ip: close zh
TEST_F(Test_qconf_feedback, get_feedback_ip_close_zh)
{
    string ip;

    zookeeper_close(zh_);
    int ret = get_feedback_ip(zh_, ip);
    zh_ = NULL;

    EXPECT_EQ(QCONF_ERR_OTHER, ret);
}

/**
 * End_Test_for function:
 * int get_feedback_ip(const zhandle_t *zh, string &ip_str)
 * void feedback_generate_chdval(const string_vector_t &chdnodes, const vector<char> &status, string &value)
 * =========================================================================================================
 */


/**
 * ========================================================================================================
 * Begin_Test_for function:
 * void feedback_generate_chdval(const string_vector_t &chdnodes, const vector<char> &status, string &value)
 */
// Test for feedback_generate_chdval : chdnodes is empty
TEST_F(Test_qconf_feedback, feedback_generate_chdval_empty_chdnodes)
{
    string_vector_t chdnodes;
    memset(&chdnodes, 0, sizeof(string_vector_t));
    vector<char> status;
    string value;

    feedback_generate_chdval(chdnodes, status, value); 
    EXPECT_STREQ("", value.c_str());
}

// Test for feedback_generate_chdval : number of chdnodes is one and up
TEST_F(Test_qconf_feedback, feedback_generate_chdval_one_up_chdnodes)
{
    string_vector_t chdnodes;
    memset(&chdnodes, 0, sizeof(string_vector_t));
    vector<char> status;
    string value;

    create_string_vector(&chdnodes, 1, true);
    status.push_back(STATUS_UP);

    feedback_generate_chdval(chdnodes, status, value); 
    EXPECT_STREQ("10.10.10.10:10#0", value.c_str());

    free_string_vector(chdnodes, chdnodes.count);
}

// Test for feedback_generate_chdval : number of chdnodes is one and down
TEST_F(Test_qconf_feedback, feedback_generate_chdval_one_down_chdnodes)
{
    string_vector_t chdnodes;
    memset(&chdnodes, 0, sizeof(string_vector_t));
    vector<char> status;
    string value;

    create_string_vector(&chdnodes, 1, true);
    status.push_back(STATUS_DOWN);

    feedback_generate_chdval(chdnodes, status, value); 
    EXPECT_STREQ("10.10.10.10:10#2", value.c_str());
    free_string_vector(chdnodes, chdnodes.count);
}

// Test for feedback_generate_chdval : number of chdnodes is one and offline
TEST_F(Test_qconf_feedback, feedback_generate_chdval_one_offline_chdnodes)
{
    string_vector_t chdnodes;
    memset(&chdnodes, 0, sizeof(string_vector_t));
    vector<char> status;
    string value;

    create_string_vector(&chdnodes, 1, true);
    status.push_back(STATUS_OFFLINE);

    feedback_generate_chdval(chdnodes, status, value); 
    EXPECT_STREQ("10.10.10.10:10#1", value.c_str());
    free_string_vector(chdnodes, chdnodes.count);
}

// Test for feedback_generate_chdval : number of chdnodes is ten and up
TEST_F(Test_qconf_feedback, feedback_generate_chdval_ten_up_chdnodes)
{
    string_vector_t chdnodes;
    memset(&chdnodes, 0, sizeof(string_vector_t));
    vector<char> status;
    string value;

    create_string_vector(&chdnodes, 10, true);
    for (int i = 0; i < 10; ++i)
        status.push_back(STATUS_UP);

    feedback_generate_chdval(chdnodes, status, value); 
    EXPECT_STREQ("10.10.10.10:10#0,10.10.10.10:11#0,10.10.10.10:12#0,10.10.10.10:13#0,10.10.10.10:14#0,10.10.10.10:15#0,10.10.10.10:16#0,10.10.10.10:17#0,10.10.10.10:18#0,10.10.10.10:19#0", value.c_str());
    free_string_vector(chdnodes, chdnodes.count);
}

// Test for feedback_generate_chdval : number of chdnodes is ten and down
TEST_F(Test_qconf_feedback, feedback_generate_chdval_ten_down_chdnodes)
{
    string_vector_t chdnodes;
    memset(&chdnodes, 0, sizeof(string_vector_t));
    vector<char> status;
    string value;

    create_string_vector(&chdnodes, 10, true);
    for (int i = 0; i < 10; ++i)
        status.push_back(STATUS_DOWN);

    feedback_generate_chdval(chdnodes, status, value); 
    EXPECT_STREQ("10.10.10.10:10#2,10.10.10.10:11#2,10.10.10.10:12#2,10.10.10.10:13#2,10.10.10.10:14#2,10.10.10.10:15#2,10.10.10.10:16#2,10.10.10.10:17#2,10.10.10.10:18#2,10.10.10.10:19#2", value.c_str());
    free_string_vector(chdnodes, chdnodes.count);
}

// Test for feedback_generate_chdval : number of chdnodes is ten and offline
TEST_F(Test_qconf_feedback, feedback_generate_chdval_ten_offline_chdnodes)
{
    string_vector_t chdnodes;
    memset(&chdnodes, 0, sizeof(string_vector_t));
    vector<char> status;
    string value;

    create_string_vector(&chdnodes, 10, true);
    for (int i = 0; i < 10; ++i)
        status.push_back(STATUS_OFFLINE);

    feedback_generate_chdval(chdnodes, status, value); 
    EXPECT_STREQ("10.10.10.10:10#1,10.10.10.10:11#1,10.10.10.10:12#1,10.10.10.10:13#1,10.10.10.10:14#1,10.10.10.10:15#1,10.10.10.10:16#1,10.10.10.10:17#1,10.10.10.10:18#1,10.10.10.10:19#1", value.c_str());
    free_string_vector(chdnodes, chdnodes.count);
}

// Test for feedback_generate_chdval : number of chdnodes is ten and half up and half down
TEST_F(Test_qconf_feedback, feedback_generate_chdval_five_up_five_down_chdnodes)
{
    string_vector_t chdnodes;
    memset(&chdnodes, 0, sizeof(string_vector_t));
    vector<char> status;
    string value;

    create_string_vector(&chdnodes, 10, true);
    for (int i = 0; i < 10; ++i)
    {
        status.push_back(i % 2 == 0 ? STATUS_UP : STATUS_DOWN);
    }

    feedback_generate_chdval(chdnodes, status, value); 
    EXPECT_STREQ("10.10.10.10:10#0,10.10.10.10:11#2,10.10.10.10:12#0,10.10.10.10:13#2,10.10.10.10:14#0,10.10.10.10:15#2,10.10.10.10:16#0,10.10.10.10:17#2,10.10.10.10:18#0,10.10.10.10:19#2", value.c_str());
    free_string_vector(chdnodes, chdnodes.count);
}

// Test for feedback_generate_chdval : number of chdnodes is ten and half up and half offline
TEST_F(Test_qconf_feedback, feedback_generate_chdval_five_up_five_offline_chdnodes)
{
    string_vector_t chdnodes;
    memset(&chdnodes, 0, sizeof(string_vector_t));
    vector<char> status;
    string value;

    create_string_vector(&chdnodes, 10, true);
    for (int i = 0; i < 10; ++i)
    {
        status.push_back(i % 2 == 0 ? STATUS_UP : STATUS_OFFLINE);
    }

    feedback_generate_chdval(chdnodes, status, value); 
    EXPECT_STREQ("10.10.10.10:10#0,10.10.10.10:11#1,10.10.10.10:12#0,10.10.10.10:13#1,10.10.10.10:14#0,10.10.10.10:15#1,10.10.10.10:16#0,10.10.10.10:17#1,10.10.10.10:18#0,10.10.10.10:19#1", value.c_str());
    free_string_vector(chdnodes, chdnodes.count);
}

// Test for feedback_generate_chdval : number of chdnodes is ten and half down and half offline
TEST_F(Test_qconf_feedback, feedback_generate_chdval_five_down_five_offline_chdnodes)
{
    string_vector_t chdnodes;
    memset(&chdnodes, 0, sizeof(string_vector_t));
    vector<char> status;
    string value;

    create_string_vector(&chdnodes, 10, true);
    for (int i = 0; i < 10; ++i)
    {
        status.push_back(i % 2 == 0 ? STATUS_DOWN : STATUS_OFFLINE);
    }

    feedback_generate_chdval(chdnodes, status, value); 
    EXPECT_STREQ("10.10.10.10:10#2,10.10.10.10:11#1,10.10.10.10:12#2,10.10.10.10:13#1,10.10.10.10:14#2,10.10.10.10:15#1,10.10.10.10:16#2,10.10.10.10:17#1,10.10.10.10:18#2,10.10.10.10:19#1", value.c_str());
    free_string_vector(chdnodes, chdnodes.count);
}

static void create_string_vector(string_vector_t *nodes, size_t count, bool serv_flg)
{
    char tmp[1024] = {0};
    memset(nodes, 0, sizeof(string_vector_t));
    nodes->data = (char**)malloc(sizeof(char*) * count);
    assert(NULL != nodes->data);
    nodes->count = count;
    for (int i = 0; i < nodes->count; ++i)
    {
        ssize_t len = 0;
        if (serv_flg)
        {
            len = snprintf(tmp, sizeof(tmp), "10.10.10.10:1%d", i);
        }
        else
        {
            len = snprintf(tmp, sizeof(tmp), "child_num_%d", i);
        }
        nodes->data[i] = (char*) calloc(len + 1, sizeof(char));
        assert(NULL != nodes->data[i]);
        memcpy(nodes->data[i], tmp, len);
    }
}
/**
 * End_Test_for function:
 * void feedback_generate_chdval(const string_vector_t &chdnodes, const vector<char> &status, string &value)
 * =========================================================================================================
 */

/**
 * ========================================================================================================
 * Begin_Test_for function:
 * void feedback_generate_batchval(const string_vector_t &batchnodes, string &value)
 */
// Test for feedback_generate_batchval : batchnodes is empty
TEST_F(Test_qconf_feedback, feedback_generate_batchval_empty_batchnodes)
{
    string_vector_t batchnodes;
    memset(&batchnodes, 0, sizeof(string_vector_t));
    vector<char> status;
    string value;

    feedback_generate_batchval(batchnodes, value); 
    EXPECT_STREQ("", value.c_str());
}

// Test for feedback_generate_batchval : number of batchnodes is one
TEST_F(Test_qconf_feedback, feedback_generate_batchval_one_batchnodes)
{
    string_vector_t batchnodes;
    memset(&batchnodes, 0, sizeof(string_vector_t));
    string value;

    create_string_vector(&batchnodes, 1);

    feedback_generate_batchval(batchnodes, value); 
    EXPECT_STREQ("child_num_0", value.c_str());
    free_string_vector(batchnodes, batchnodes.count);
}

// Test for feedback_generate_batchval : number of batchnodes is ten
TEST_F(Test_qconf_feedback, feedback_generate_batchval_ten_batchnodes)
{
    string_vector_t batchnodes;
    memset(&batchnodes, 0, sizeof(string_vector_t));
    string value;

    create_string_vector(&batchnodes, 10);

    feedback_generate_batchval(batchnodes, value); 
    EXPECT_STREQ("child_num_0,child_num_1,child_num_2,child_num_3,child_num_4,child_num_5,child_num_6,child_num_7,child_num_8,child_num_9", value.c_str());
    free_string_vector(batchnodes, batchnodes.count);
}

/**
 * End_Test_for function:
 * void feedback_generate_batchval(const string_vector_t &batchnodes, string &value)
 * =========================================================================================================
 */


/**
 * ========================================================================================================
 * Begin_Test_for function:
 * int feedback_generate_content(const string &ip, char data_type, const string &idc, const string &path, const fb_val &fbval, string &content)
 */
// Test for feedback_generate_content : ip is empty
TEST_F(Test_qconf_feedback, feedback_generate_content_empty_ip)
{
    string ip;
    string idc;
    string path;
    fb_val fbval;
    string content;
    char data_type = QCONF_DATA_TYPE_NODE;
    
    int ret = feedback_generate_content(ip, data_type, idc, path, fbval, content);
    EXPECT_EQ(QCONF_ERR_PARAM, ret);
}

// Test for feedback_generate_content : idc is empty
TEST_F(Test_qconf_feedback, feedback_generate_content_empty_idc)
{
    string ip("10.10.10.10");
    string idc;
    string path;
    fb_val fbval;
    string content;
    char data_type = QCONF_DATA_TYPE_NODE;
    
    int ret = feedback_generate_content(ip, data_type, idc, path, fbval, content);
    EXPECT_EQ(QCONF_ERR_PARAM, ret);
}

// Test for feedback_generate_content : path is empty
TEST_F(Test_qconf_feedback, feedback_generate_content_empty_path)
{
    string ip("10.10.10.10");
    string idc("test");
    string path;
    fb_val fbval;
    string content;
    char data_type = QCONF_DATA_TYPE_NODE;
    
    int ret = feedback_generate_content(ip, data_type, idc, path, fbval, content);
    EXPECT_EQ(QCONF_ERR_PARAM, ret);
}

// Test for feedback_generate_content : illegal data type
TEST_F(Test_qconf_feedback, feedback_generate_content_illegal_data_type)
{
    string ip("10.10.10.10");
    string idc("test");
    string path("/demo/test/confs/conf1/conf11");
    fb_val fbval;
    string content;
    char data_type = QCONF_DATA_TYPE_UNKNOWN;
    
    int ret = feedback_generate_content(ip, data_type, idc, path, fbval, content);
    EXPECT_EQ(QCONF_ERR_PARAM, ret);
}

// Test for feedback_generate_content : data type is node with empty value
TEST_F(Test_qconf_feedback, feedback_generate_content_node_data_type_empty_value)
{
    string ip("10.10.10.10");
    string idc("test");
    string path("/demo/test/confs/conf1/conf11");
    string value;
    fb_val fbval;
    string content;
    char data_type = QCONF_DATA_TYPE_NODE;
    string tblkey;
    serialize_to_tblkey(data_type, idc, path, tblkey);
    string tblval;
    nodeval_to_tblval(tblkey, value, tblval); 
    fbval.tblval = tblval;
    
    int ret = feedback_generate_content(ip, data_type, idc, path, fbval, content);
    EXPECT_EQ(QCONF_OK, ret);
    cout << content << endl;
}

// Test for feedback_generate_content : data type is node
TEST_F(Test_qconf_feedback, feedback_generate_content_node_data_type)
{
    string ip("10.10.10.10");
    string idc("test");
    string path("/demo/test/confs/conf1/conf11");
    string value("name=test;age=10");
    fb_val fbval;
    string content;
    char data_type = QCONF_DATA_TYPE_NODE;
    string tblkey;
    serialize_to_tblkey(data_type, idc, path, tblkey);
    string tblval;
    nodeval_to_tblval(tblkey, value, tblval); 
    fbval.tblval = tblval;
    
    int ret = feedback_generate_content(ip, data_type, idc, path, fbval, content);
    EXPECT_EQ(QCONF_OK, ret);
    cout << content << endl;
}

// Test for feedback_generate_content : data type is service with empty value
TEST_F(Test_qconf_feedback, feedback_generate_content_service_data_type_empty_value)
{
    string ip("10.10.10.10");
    string idc("test");
    string path("/demo/test/hosts/host1");
    string value;
    fb_val fbval;
    string content;
    char data_type = QCONF_DATA_TYPE_SERVICE;
    string tblkey;
    serialize_to_tblkey(data_type, idc, path, tblkey);
    string tblval;
    string_vector_t chdnodes;
    vector<char> status;
    string fb_chd;
    
    memset(&chdnodes, 0, sizeof(string_vector_t));

    feedback_generate_chdval(chdnodes, status, fb_chd);
    EXPECT_STREQ("", fb_chd.c_str());

    chdnodeval_to_tblval(tblkey, chdnodes, tblval, status);

    fbval.tblval = tblval;
    fbval.fb_chds = fb_chd;
    
    int ret = feedback_generate_content(ip, data_type, idc, path, fbval, content);
    EXPECT_EQ(QCONF_OK, ret);
    cout << content << endl;
}

// Test for feedback_generate_content : data type is service with one up service
TEST_F(Test_qconf_feedback, feedback_generate_content_service_data_type_one_up)
{
    string ip("10.10.10.10");
    string idc("test");
    string path("/demo/test/hosts/host1");
    string value;
    fb_val fbval;
    string content;
    char data_type = QCONF_DATA_TYPE_SERVICE;
    string tblkey;
    serialize_to_tblkey(data_type, idc, path, tblkey);
    string tblval;
    string_vector_t chdnodes;
    vector<char> status;
    string fb_chd;
    
    memset(&chdnodes, 0, sizeof(string_vector_t));

    create_string_vector(&chdnodes, 1, true);
    status.push_back(STATUS_UP);

    feedback_generate_chdval(chdnodes, status, fb_chd);

    chdnodeval_to_tblval(tblkey, chdnodes, tblval, status);

    fbval.tblval = tblval;
    fbval.fb_chds = fb_chd;
    
    int ret = feedback_generate_content(ip, data_type, idc, path, fbval, content);
    EXPECT_EQ(QCONF_OK, ret);
    cout << content << endl;
    free_string_vector(chdnodes, chdnodes.count);
}

// Test for feedback_generate_content : data type is service with one down service
TEST_F(Test_qconf_feedback, feedback_generate_content_service_data_type_one_down)
{
    string ip("10.10.10.10");
    string idc("test");
    string path("/demo/test/hosts/host1");
    string value;
    fb_val fbval;
    string content;
    char data_type = QCONF_DATA_TYPE_SERVICE;
    string tblkey;
    serialize_to_tblkey(data_type, idc, path, tblkey);
    string tblval;
    string_vector_t chdnodes;
    vector<char> status;
    string fb_chd;
    
    memset(&chdnodes, 0, sizeof(string_vector_t));

    create_string_vector(&chdnodes, 1, true);
    status.push_back(STATUS_DOWN);

    feedback_generate_chdval(chdnodes, status, fb_chd);

    chdnodeval_to_tblval(tblkey, chdnodes, tblval, status);

    fbval.tblval = tblval;
    fbval.fb_chds = fb_chd;
    
    int ret = feedback_generate_content(ip, data_type, idc, path, fbval, content);
    EXPECT_EQ(QCONF_OK, ret);
    cout << content << endl;
    free_string_vector(chdnodes, chdnodes.count);
}

// Test for feedback_generate_content : data type is service with one offline service
TEST_F(Test_qconf_feedback, feedback_generate_content_service_data_type_one_offline)
{
    string ip("10.10.10.10");
    string idc("test");
    string path("/demo/test/hosts/host1");
    string value;
    fb_val fbval;
    string content;
    char data_type = QCONF_DATA_TYPE_SERVICE;
    string tblkey;
    serialize_to_tblkey(data_type, idc, path, tblkey);
    string tblval;
    string_vector_t chdnodes;
    vector<char> status;
    string fb_chd;
    
    memset(&chdnodes, 0, sizeof(string_vector_t));

    create_string_vector(&chdnodes, 1, true);
    status.push_back(STATUS_OFFLINE);

    feedback_generate_chdval(chdnodes, status, fb_chd);

    chdnodeval_to_tblval(tblkey, chdnodes, tblval, status);

    fbval.tblval = tblval;
    fbval.fb_chds = fb_chd;
    
    int ret = feedback_generate_content(ip, data_type, idc, path, fbval, content);
    EXPECT_EQ(QCONF_OK, ret);
    cout << content << endl;
    free_string_vector(chdnodes, chdnodes.count);
}

// Test for feedback_generate_content : data type is service with ten up services
TEST_F(Test_qconf_feedback, feedback_generate_content_service_data_type_ten_up)
{
    string ip("10.10.10.10");
    string idc("test");
    string path("/demo/test/hosts/host1");
    string value;
    fb_val fbval;
    string content;
    char data_type = QCONF_DATA_TYPE_SERVICE;
    string tblkey;
    serialize_to_tblkey(data_type, idc, path, tblkey);
    string tblval;
    string_vector_t chdnodes;
    vector<char> status;
    string fb_chd;
    
    memset(&chdnodes, 0, sizeof(string_vector_t));

    create_string_vector(&chdnodes, 10, true);
    for (int i = 0; i < 10; ++i)
        status.push_back(STATUS_UP);

    feedback_generate_chdval(chdnodes, status, fb_chd);

    chdnodeval_to_tblval(tblkey, chdnodes, tblval, status);

    fbval.tblval = tblval;
    fbval.fb_chds = fb_chd;
    
    int ret = feedback_generate_content(ip, data_type, idc, path, fbval, content);
    EXPECT_EQ(QCONF_OK, ret);
    cout << content << endl;
    free_string_vector(chdnodes, chdnodes.count);
}

// Test for feedback_generate_content : data type is service with ten down services
TEST_F(Test_qconf_feedback, feedback_generate_content_service_data_type_ten_down)
{
    string ip("10.10.10.10");
    string idc("test");
    string path("/demo/test/hosts/host1");
    string value;
    fb_val fbval;
    string content;
    char data_type = QCONF_DATA_TYPE_SERVICE;
    string tblkey;
    serialize_to_tblkey(data_type, idc, path, tblkey);
    string tblval;
    string_vector_t chdnodes;
    vector<char> status;
    string fb_chd;
    
    memset(&chdnodes, 0, sizeof(string_vector_t));

    create_string_vector(&chdnodes, 10, true);
    for (int i = 0; i < 10; ++i)
        status.push_back(STATUS_DOWN);

    feedback_generate_chdval(chdnodes, status, fb_chd);

    chdnodeval_to_tblval(tblkey, chdnodes, tblval, status);

    fbval.tblval = tblval;
    fbval.fb_chds = fb_chd;
    
    int ret = feedback_generate_content(ip, data_type, idc, path, fbval, content);
    EXPECT_EQ(QCONF_OK, ret);
    cout << content << endl;
    free_string_vector(chdnodes, chdnodes.count);
}

// Test for feedback_generate_content : data type is service with ten offline services
TEST_F(Test_qconf_feedback, feedback_generate_content_service_data_type_ten_offline)
{
    string ip("10.10.10.10");
    string idc("test");
    string path("/demo/test/hosts/host1");
    string value;
    fb_val fbval;
    string content;
    char data_type = QCONF_DATA_TYPE_SERVICE;
    string tblkey;
    serialize_to_tblkey(data_type, idc, path, tblkey);
    string tblval;
    string_vector_t chdnodes;
    vector<char> status;
    string fb_chd;
    
    memset(&chdnodes, 0, sizeof(string_vector_t));

    create_string_vector(&chdnodes, 10, true);
    for (int i = 0; i < 10; ++i)
        status.push_back(STATUS_OFFLINE);

    feedback_generate_chdval(chdnodes, status, fb_chd);

    chdnodeval_to_tblval(tblkey, chdnodes, tblval, status);

    fbval.tblval = tblval;
    fbval.fb_chds = fb_chd;
    
    int ret = feedback_generate_content(ip, data_type, idc, path, fbval, content);
    EXPECT_EQ(QCONF_OK, ret);
    cout << content << endl;
    free_string_vector(chdnodes, chdnodes.count);
}

// Test for feedback_generate_content : data type is batchnode with empty value
TEST_F(Test_qconf_feedback, feedback_generate_content_batchnode_data_type_empty_value)
{
    string ip("10.10.10.10");
    string idc("test");
    string path("/demo/test/confs/conf1");
    string value;
    fb_val fbval;
    string content;
    char data_type = QCONF_DATA_TYPE_BATCH_NODE;
    string tblkey;
    serialize_to_tblkey(data_type, idc, path, tblkey);
    string tblval;
    string_vector_t batchnodes;
    string fb_chd;
    
    memset(&batchnodes, 0, sizeof(string_vector_t));

    feedback_generate_batchval(batchnodes, fb_chd);
    EXPECT_STREQ("", fb_chd.c_str());

    batchnodeval_to_tblval(tblkey, batchnodes, tblval);

    fbval.tblval = tblval;
    fbval.fb_chds = fb_chd;
    
    int ret = feedback_generate_content(ip, data_type, idc, path, fbval, content);
    EXPECT_EQ(QCONF_OK, ret);
    cout << content << endl;
}

// Test for feedback_generate_content : data type is batchnode with one node
TEST_F(Test_qconf_feedback, feedback_generate_content_batchnode_data_type_one)
{
    string ip("10.10.10.10");
    string idc("test");
    string path("/demo/test/confs/conf1");
    string value;
    fb_val fbval;
    string content;
    char data_type = QCONF_DATA_TYPE_BATCH_NODE;
    string tblkey;
    serialize_to_tblkey(data_type, idc, path, tblkey);
    string tblval;
    string_vector_t batchnodes;
    string fb_chd;
    
    memset(&batchnodes, 0, sizeof(string_vector_t));
    create_string_vector(&batchnodes, 1);

    feedback_generate_batchval(batchnodes, fb_chd);

    batchnodeval_to_tblval(tblkey, batchnodes, tblval);

    fbval.tblval = tblval;
    fbval.fb_chds = fb_chd;
    
    int ret = feedback_generate_content(ip, data_type, idc, path, fbval, content);
    EXPECT_EQ(QCONF_OK, ret);
    cout << content << endl;
    free_string_vector(batchnodes, batchnodes.count);
}

// Test for feedback_generate_content : data type is batchnode with ten node
TEST_F(Test_qconf_feedback, feedback_generate_content_batchnode_data_type_ten)
{
    string ip("10.10.10.10");
    string idc("test");
    string path("/demo/test/confs/conf1");
    string value;
    fb_val fbval;
    string content;
    char data_type = QCONF_DATA_TYPE_BATCH_NODE;
    string tblkey;
    serialize_to_tblkey(data_type, idc, path, tblkey);
    string tblval;
    string_vector_t batchnodes;
    string fb_chd;
    
    memset(&batchnodes, 0, sizeof(string_vector_t));
    create_string_vector(&batchnodes, 10);

    feedback_generate_batchval(batchnodes, fb_chd);

    batchnodeval_to_tblval(tblkey, batchnodes, tblval);

    fbval.tblval = tblval;
    fbval.fb_chds = fb_chd;
    
    int ret = feedback_generate_content(ip, data_type, idc, path, fbval, content);
    EXPECT_EQ(QCONF_OK, ret);
    cout << content << endl;
    free_string_vector(batchnodes, batchnodes.count);
}

// Test for feedback_generate_content : data type is batchnode with hundred node
TEST_F(Test_qconf_feedback, feedback_generate_content_batchnode_data_type_hundred)
{
    string ip("10.10.10.10");
    string idc("test");
    string path("/demo/test/confs/conf1");
    string value;
    fb_val fbval;
    string content;
    char data_type = QCONF_DATA_TYPE_BATCH_NODE;
    string tblkey;
    serialize_to_tblkey(data_type, idc, path, tblkey);
    string tblval;
    string_vector_t batchnodes;
    string fb_chd;
    
    memset(&batchnodes, 0, sizeof(string_vector_t));
    create_string_vector(&batchnodes, 100);

    feedback_generate_batchval(batchnodes, fb_chd);
    cout << fb_chd << endl;

    batchnodeval_to_tblval(tblkey, batchnodes, tblval);

    fbval.tblval = tblval;
    fbval.fb_chds = fb_chd;
    
    int ret = feedback_generate_content(ip, data_type, idc, path, fbval, content);
    EXPECT_EQ(QCONF_OK, ret);
    cout << content << endl;
    free_string_vector(batchnodes, batchnodes.count);
}

/**
 * End_Test_for function:
 * int feedback_generate_content(const string &ip, char data_type, const string &idc, const string &path, const fb_val &fbval, string &content)
 * =========================================================================================================
 */
