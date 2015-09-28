#include <zookeeper.h>
#include <sys/fcntl.h>

#include <string>

#include "gtest/gtest.h"
#include "qconf_zoo.h"
#include "qconf_const.h"
#include "qconf_format.h"

using namespace std;

#define MAX_NODE_NUM 20

//unit test case for qconf_zk.cc

static int children_node_cmp(const void* p1, const void* p2);
static void zoo_create_or_set(zhandle_t *zh, const string path, const string value);
static void create_string_vector(string_vector_t *nodes, size_t count, bool serv_flg = false);
static void global_watcher(zhandle_t* zh, int type, int state, const char* path, void* context);

//Related test environment set up 1:
class Test_qconf_zk : public :: testing::Test
{
protected:
    virtual void SetUp()
    {
        // init zookeeper handler
        const char* host = "127.0.0.1:2181";
        zh = zookeeper_init(host, global_watcher, QCONF_ZK_DEFAULT_RECV_TIMEOUT, NULL, NULL, 0);
        zoo_create_or_set(zh, "/qconf", "qconf");
        zoo_create_or_set(zh, "/qconf/__unit_test", "unit test");
    }

    virtual void TearDown()
    {
        zookeeper_close(zh);
    }

    zhandle_t *zh;
};

static void global_watcher(zhandle_t* zh, int type, int state, const char* path, void* context)
{
    //to do something
}

/**
 * ========================================================================================================
 * Begin_Test_for function: int zk_get_chdnodes(zhandle_t *zh, const string &path, string_vector_t &nodes)
 */

// Test for zk_get_chdnodes : zh is null
TEST_F(Test_qconf_zk, zk_get_chdnodes_null_zh)
{
    int ret = 0;

    string path;
    string_vector_t nodes;

    ret = zk_get_chdnodes(NULL, path, nodes);
    EXPECT_EQ(QCONF_ERR_PARAM, ret);
}

// Test for zk_get_chdnodes : path is empty
TEST_F(Test_qconf_zk, zk_get_chdnodes_empty_path)
{
    int ret = 0;

    string path;
    string_vector_t nodes;

    ret = zk_get_chdnodes(zh, path, nodes);
    EXPECT_EQ(QCONF_ERR_PARAM, ret);
}

// Test for zk_get_chdnodes : path does not exist
TEST_F(Test_qconf_zk, zk_get_chdnodes_not_exist_path)
{
    int ret = 0;

    string path = "/qconf/__unit_test/not_exist_path";
    string_vector_t nodes;

    ret = zk_get_chdnodes(zh, path, nodes);
    EXPECT_EQ(QCONF_NODE_NOT_EXIST, ret);
}

// Test for zk_get_chdnodes : path is illegal value
TEST_F(Test_qconf_zk, zk_get_chdnodes_illegal_path)
{
    int ret = 0;

    string path = "qconf/__unit_test/not_exist_path";
    string_vector_t nodes;

    ret = zk_get_chdnodes(zh, path, nodes);
    EXPECT_EQ(QCONF_ERR_ZOO_FAILED, ret);
}

// Test for zk_get_chdnodes : zero child
TEST_F(Test_qconf_zk, zk_get_chdnodes_zero_child)
{
    int ret = 0;

    string path = "/qconf/__unit_test/zero_child";
    string value("zero child");
    string_vector_t nodes;

    zoo_create(zh, path.c_str(), value.c_str(), value.size(), &ZOO_OPEN_ACL_UNSAFE, 0, NULL, 0);
    ret = zk_get_chdnodes(zh, path, nodes);
    EXPECT_EQ(QCONF_OK, ret);
    EXPECT_EQ(0, nodes.count);
}

// Test for zk_get_chdnodes : 10 children
TEST_F(Test_qconf_zk, zk_get_chdnodes_10_children)
{
    int ret = 0;

    string path = "/qconf/__unit_test/10_child";
    string value("10 child");
    string_vector_t nodes;
    string_vector_t ret_nodes;
    size_t chd_count = 10;

    create_string_vector(&nodes, chd_count);

    zoo_create(zh, path.c_str(), value.c_str(), value.size(), &ZOO_OPEN_ACL_UNSAFE, 0, NULL, 0);
    for (int i = 0; i < nodes.count; ++i)
    {
        string child_path = path + "/" + nodes.data[i];
        zoo_create(zh, child_path.c_str(), value.c_str(), value.size(), &ZOO_OPEN_ACL_UNSAFE, 0, NULL, 0);
    }

    ret = zk_get_chdnodes(zh, path, ret_nodes);
    EXPECT_EQ(QCONF_OK, ret);
    EXPECT_EQ(nodes.count, ret_nodes.count);
    for (int i = 0; i < ret_nodes.count; ++i)
    {
        EXPECT_STREQ(nodes.data[i], ret_nodes.data[i]);
    }

    free_string_vector(nodes, nodes.count);
    free_string_vector(ret_nodes, ret_nodes.count);
}

// Test for zk_get_chdnodes : 100 children
TEST_F(Test_qconf_zk, zk_get_chdnodes_100_children)
{
    int ret = 0;

    string path = "/qconf/__unit_test/100_child";
    string value("100 child");
    string_vector_t nodes;
    string_vector_t ret_nodes;
    size_t chd_count = 100;

    create_string_vector(&nodes, chd_count);

    zoo_create(zh, path.c_str(), value.c_str(), value.size(), &ZOO_OPEN_ACL_UNSAFE, 0, NULL, 0);
    for (int i = 0; i < nodes.count; ++i)
    {
        string child_path = path + "/" + nodes.data[i];
        zoo_create(zh, child_path.c_str(), value.c_str(), value.size(), &ZOO_OPEN_ACL_UNSAFE, 0, NULL, 0);
    }

    ret = zk_get_chdnodes(zh, path, ret_nodes);
    EXPECT_EQ(QCONF_OK, ret);
    EXPECT_EQ(nodes.count, ret_nodes.count);

    qsort(nodes.data, nodes.count, sizeof(char*), children_node_cmp);

    for (int i = 0; i < ret_nodes.count; ++i)
    {
        EXPECT_STREQ(nodes.data[i], ret_nodes.data[i]);
    }

    free_string_vector(nodes, nodes.count);
    free_string_vector(ret_nodes, ret_nodes.count);
}

static int children_node_cmp(const void* p1, const void* p2)
{
    char **s1 = (char**)p1;
    char **s2 = (char**)p2;

    return strcmp(*s1, *s2);
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
 * End_Test_for function: int zk_get_chdnodes(zhandle_t *zh, const string &path, string_vector_t &nodes)
 * =========================================================================================================
 */

/**
 * ========================================================================================================
 * Begin_Test_for function:
 * int zk_get_chdnodes_with_status(zhandle_t *zh, const string &path, string_vector_t &nodes, vector<char> &status)
 */

// Test for zk_get_chdnodes_with_status : zh is null
TEST_F(Test_qconf_zk, zk_get_chdnodes_with_status_null_zh)
{
    int ret = 0;

    string path;
    string_vector_t nodes;
    vector<char> status;

    ret = zk_get_chdnodes_with_status(NULL, path, nodes, status);
    EXPECT_EQ(QCONF_ERR_PARAM, ret);
}

// Test for zk_get_chdnodes_with_status : path is empty
TEST_F(Test_qconf_zk, zk_get_chdnodes_with_status_empty_path)
{
    int ret = 0;
    string path;
    string_vector_t nodes;
    vector<char> status;

    ret = zk_get_chdnodes_with_status(zh, path, nodes, status);
    EXPECT_EQ(QCONF_ERR_PARAM, ret);
}

// Test for zk_get_chdnodes_with_status : path does not exist
TEST_F(Test_qconf_zk, zk_get_chdnodes_with_status_not_exist_path)
{
    int ret = 0;
    vector<char> status;
    string_vector_t nodes;
    string path = "/qconf/__unit_test/not_exist_path";

    ret = zk_get_chdnodes_with_status(zh, path, nodes, status);
    EXPECT_EQ(QCONF_NODE_NOT_EXIST, ret);
}

// Test for zk_get_chdnodes_with_status : path is illegal value
TEST_F(Test_qconf_zk, zk_get_chdnodes_with_status_illegal_path)
{
    int ret = 0;
    vector<char> status;
    string_vector_t nodes;
    string path = "qconf/__unit_test/not_exist_path";

    ret = zk_get_chdnodes_with_status(zh, path, nodes, status);
    EXPECT_EQ(QCONF_ERR_ZOO_FAILED, ret);
}

// Test for zk_get_chdnodes_with_status : zero servs 
TEST_F(Test_qconf_zk, zk_get_chdnodes_with_status_zero_serv)
{
    int ret = 0;
    string_vector_t nodes;
    string value("zero service");
    string path = "/qconf/__unit_test/zero_service";
    vector<char> status;

    zoo_create(zh, path.c_str(), value.c_str(), value.size(), &ZOO_OPEN_ACL_UNSAFE, 0, NULL, 0);
    ret = zk_get_chdnodes_with_status(zh, path, nodes, status);
    EXPECT_EQ(QCONF_OK, ret);
    EXPECT_EQ(0, nodes.count);
}

// Test for zk_get_chdnodes_with_status : 10 services with 10 down
TEST_F(Test_qconf_zk, zk_get_chdnodes_with_status_10_servs_all_down)
{
    int ret = 0;
    string_vector_t nodes;
    size_t chd_count = 10;
    string_vector_t ret_nodes;
    string value("10 services");
    string path = "/qconf/__unit_test/10_servs";
    vector<char> status;
    vector<char> ret_status;

    create_string_vector(&nodes, chd_count, true);

    for (int i = 0; i < nodes.count; ++i)
    {
        status.push_back(STATUS_DOWN);
    }

    zoo_create(zh, path.c_str(), value.c_str(), value.size(), &ZOO_OPEN_ACL_UNSAFE, 0, NULL, 0);
    for (int i = 0; i < nodes.count; ++i)
    {
        string child_path = path + "/" + nodes.data[i];
        zoo_create_or_set(zh, child_path, string(1, status[i] + '0'));
    }

    ret = zk_get_chdnodes_with_status(zh, path, ret_nodes, ret_status);
    EXPECT_EQ(QCONF_OK, ret);
    EXPECT_EQ(nodes.count, ret_nodes.count);

    for (int i = 0; i < ret_nodes.count; ++i)
    {
        EXPECT_STREQ(nodes.data[i], ret_nodes.data[i]);
        EXPECT_EQ(status[i], ret_status[i]);
    }

    free_string_vector(nodes, nodes.count);
    free_string_vector(ret_nodes, ret_nodes.count);
}

// Test for zk_get_chdnodes_with_status : 10 services with 5 down and 5 up
TEST_F(Test_qconf_zk, zk_get_chdnodes_with_status_10_servs_half_down)
{
    int ret = 0;
    string_vector_t nodes;
    size_t chd_count = 10;
    string_vector_t ret_nodes;
    string value("10 services");
    string path = "/qconf/__unit_test/10_servs";
    vector<char> status;
    vector<char> ret_status;

    create_string_vector(&nodes, chd_count, true);

    for (int i = 0; i < nodes.count; ++i)
    {
        status.push_back(i % 2 == 0 ? STATUS_DOWN : STATUS_UP);
    }

    zoo_create(zh, path.c_str(), value.c_str(), value.size(), &ZOO_OPEN_ACL_UNSAFE, 0, NULL, 0);
    for (int i = 0; i < nodes.count; ++i)
    {
        string child_path = path + "/" + nodes.data[i];
        zoo_create_or_set(zh, child_path, string(1, status[i] + '0'));
    }

    ret = zk_get_chdnodes_with_status(zh, path, ret_nodes, ret_status);
    EXPECT_EQ(QCONF_OK, ret);
    EXPECT_EQ(nodes.count, ret_nodes.count);

    for (int i = 0; i < ret_nodes.count; ++i)
    {
        EXPECT_STREQ(nodes.data[i], ret_nodes.data[i]);
        EXPECT_EQ(status[i], ret_status[i]);
    }

    free_string_vector(nodes, nodes.count);
    free_string_vector(ret_nodes, ret_nodes.count);
}

// Test for zk_get_chdnodes_with_status : 10 services with 5 offline and 5 up
TEST_F(Test_qconf_zk, zk_get_chdnodes_with_status_10_servs_half_offline)
{
    int ret = 0;
    string_vector_t nodes;
    size_t chd_count = 10;
    string_vector_t ret_nodes;
    string value("10 services");
    string path = "/qconf/__unit_test/10_servs";
    vector<char> status;
    vector<char> ret_status;

    create_string_vector(&nodes, chd_count, true);

    for (int i = 0; i < nodes.count; ++i)
    {
        status.push_back(i % 2 == 0 ? STATUS_UP : STATUS_OFFLINE);
    }

    zoo_create(zh, path.c_str(), value.c_str(), value.size(), &ZOO_OPEN_ACL_UNSAFE, 0, NULL, 0);
    for (int i = 0; i < nodes.count; ++i)
    {
        string child_path = path + "/" + nodes.data[i];
        zoo_create_or_set(zh, child_path, string(1, status[i] + '0'));
    }

    ret = zk_get_chdnodes_with_status(zh, path, ret_nodes, ret_status);
    EXPECT_EQ(QCONF_OK, ret);
    EXPECT_EQ(nodes.count, ret_nodes.count);

    for (int i = 0; i < ret_nodes.count; ++i)
    {
        EXPECT_STREQ(nodes.data[i], ret_nodes.data[i]);
        EXPECT_EQ(status[i], ret_status[i]);
    }

    free_string_vector(nodes, nodes.count);
    free_string_vector(ret_nodes, ret_nodes.count);
}

// Test for zk_get_chdnodes_with_status : 10 services with 10 up
TEST_F(Test_qconf_zk, zk_get_chdnodes_with_status_10_servs_all_up)
{
    int ret = 0;
    string_vector_t nodes;
    size_t chd_count = 10;
    string_vector_t ret_nodes;
    string value("10 services");
    string path = "/qconf/__unit_test/10_servs";
    vector<char> status;
    vector<char> ret_status;

    create_string_vector(&nodes, chd_count, true);

    for (int i = 0; i < nodes.count; ++i)
    {
        status.push_back(STATUS_UP);
    }

    zoo_create(zh, path.c_str(), value.c_str(), value.size(), &ZOO_OPEN_ACL_UNSAFE, 0, NULL, 0);
    for (int i = 0; i < nodes.count; ++i)
    {
        string child_path = path + "/" + nodes.data[i];
        zoo_create_or_set(zh, child_path, string(1, status[i] + '0'));
    }

    ret = zk_get_chdnodes_with_status(zh, path, ret_nodes, ret_status);
    EXPECT_EQ(QCONF_OK, ret);
    EXPECT_EQ(nodes.count, ret_nodes.count);

    for (int i = 0; i < ret_nodes.count; ++i)
    {
        EXPECT_STREQ(nodes.data[i], ret_nodes.data[i]);
        EXPECT_EQ(status[i], ret_status[i]);
    }

    free_string_vector(nodes, nodes.count);
    free_string_vector(ret_nodes, ret_nodes.count);
}

static void zoo_create_or_set(zhandle_t *zh, const string path, const string value)
{
    int ret = zoo_exists(zh, path.c_str(), 0, NULL);
    if (ZOK == ret)
    {
        zoo_set(zh, path.c_str(), value.c_str(), value.size(), -1);
    }
    else if (ZNONODE == ret)
    {
        zoo_create(zh, path.c_str(), value.c_str(), value.size(), &ZOO_OPEN_ACL_UNSAFE, 0, NULL, 0);
    }
}

/**
 * End_Test_for function:
 * int zk_get_chdnodes_with_status(zhandle_t *zh, const string &path, string_vector_t &nodes, vector<char> &status)
 * =========================================================================================================
 */

/**
 * ========================================================================================================
 * Begin_Test_for function:
 * int zk_get_node(zhandle_t *zh, const string &path, string &buf)
 */

// Test for zk_get_node: node not exists
TEST_F(Test_qconf_zk, zk_get_node_not_exists)
{
    int ret = 0;
    string path = "/qconf/__unit_test/untest";
    string buf;

    ret = zk_get_node(zh, path, buf, 0);

    EXPECT_EQ(QCONF_NODE_NOT_EXIST, ret);
}

// Test for zk_get_node: bad arguments
TEST_F(Test_qconf_zk, zk_get_node_bad_arguments)
{
    int ret = 0;
    string path = "test/bad/args";
    string buf;

    ret = zk_get_node(zh, path, buf, 0);

    EXPECT_EQ(QCONF_ERR_ZOO_FAILED, ret);
}

// Test for zk_get_node: node exists
TEST_F(Test_qconf_zk, zk_get_node_exists)
{
    int ret = 0;
    string path = "/qconf/__unit_test/test";
    string value = "hello good";
    string buf;

    zoo_create(zh, path.c_str(), value.c_str(), value.size(), &ZOO_OPEN_ACL_UNSAFE, 0, NULL, 0);

    ret = zk_get_node(zh, path, buf, 0);

    EXPECT_EQ(QCONF_OK, ret);
    EXPECT_STREQ(value.c_str(), buf.c_str());
}

// Test for zk_get_node: node val = empty
TEST_F(Test_qconf_zk, zk_get_node_with_empty_val)
{
    int ret = 0;
    string path("/qconf/__unit_test/nullval");
    string buf;

    zoo_create(zh, path.c_str(), NULL, 0, &ZOO_OPEN_ACL_UNSAFE, 0, NULL, 0);
    ret = zk_get_node(zh, path, buf, 0);

    EXPECT_EQ(QCONF_OK, ret);
    EXPECT_STREQ("", buf.c_str());
}

// Test for zk_get_node: length of node : 1K
TEST_F(Test_qconf_zk, zk_get_node_with_1k_val)
{
    string buf;
    int ret = 0;
    string path("/qconf/__unit_test/1k");
    string value(1024, 'a');

    zoo_create(zh, path.c_str(), value.c_str(), value.size(), &ZOO_OPEN_ACL_UNSAFE, 0, NULL, 0);
    ret = zk_get_node(zh, path, buf, 0);

    EXPECT_EQ(QCONF_OK, ret);
    EXPECT_STREQ(value.c_str(), buf.c_str());
}

// Test for zk_get_node: length of node : 10k
TEST_F(Test_qconf_zk, zk_get_node_with_10k_val)
{
    string buf;
    int ret = 0;
    string path("/qconf/__unit_test/10k");
    string value(10240, 'a');

    zoo_create(zh, path.c_str(), value.c_str(), value.size(), &ZOO_OPEN_ACL_UNSAFE, 0, NULL, 0);
    ret = zk_get_node(zh, path, buf, 0);

    EXPECT_EQ(QCONF_OK, ret);
    EXPECT_STREQ(value.c_str(), buf.c_str());
}

// Test for zk_get_node: length of node : 100k
TEST_F(Test_qconf_zk, zk_get_node_with_100k_val)
{
    string buf;
    int ret = 0;
    string path("/qconf/__unit_test/100k");
    string value(10240, 'a');

    zoo_create(zh, path.c_str(), value.c_str(), value.size(), &ZOO_OPEN_ACL_UNSAFE, 0, NULL, 0);
    ret = zk_get_node(zh, path, buf, 0);

    EXPECT_EQ(QCONF_OK, ret);
    EXPECT_STREQ(value.c_str(), buf.c_str());
}

// Test for zk_get_node: length of node : 1m
TEST_F(Test_qconf_zk, zk_get_node_with_1m_val)
{
    string buf;
    int ret = 0;
    string path("/qconf/__unit_test/1m");
    string value(1000000, 'a');

    zoo_create(zh, path.c_str(), value.c_str(), value.size(), &ZOO_OPEN_ACL_UNSAFE, 0, NULL, 0);
    ret = zk_get_node(zh, path, buf, 0);

    EXPECT_EQ(QCONF_OK, ret);
    EXPECT_STREQ(value.c_str(), buf.c_str());
}
/**
 * End_Test_for function:
 * int zk_get_node(zhandle_t *zh, const string &path, string &buf)
 * =========================================================================================================
 */

/**
 * ========================================================================================================
 * Begin_Test_for function:
 * int zk_create_node(zhandle_t *zh, const string &path, const string &value, int flags)
 */

// Test for zk_create_node: parent not exists
TEST_F(Test_qconf_zk, zk_create_node_parent_not_exists)
{
    int ret = 0;
    string path = "/qconf/111/tst_create_no";
    string value("hello");

    ret = zk_create_node(zh, path, value, 0);
    EXPECT_EQ(QCONF_ERR_ZOO_FAILED, ret);
}

// Test for zk_create_node: node not exists
TEST_F(Test_qconf_zk, zk_create_node_not_exists)
{
    int ret = 0;
    string path = "/qconf/__unit_test/tst_create_no";
    string buf;
    string value("hello");

    ret = zk_create_node(zh, path, value, 0);
    EXPECT_EQ(QCONF_OK, ret);

    ret = zk_get_node(zh, path, buf, 0);

    EXPECT_EQ(QCONF_OK, ret);
    EXPECT_STREQ(value.c_str(), buf.c_str());

    ret = zoo_delete(zh, path.c_str(), -1);
    EXPECT_EQ(ret, ZOK);

}

// Test for zk_create_node: node exists
TEST_F(Test_qconf_zk, zk_create_node_exists)
{
    int ret = 0;
    string path = "/qconf/__unit_test/tst_create";
    string value = "hello good";

    ret = zk_create_node(zh, path, value, 0);

    EXPECT_EQ(QCONF_NODE_EXIST, ret);
}

// Test for zk_create_node: node val = empty
TEST_F(Test_qconf_zk, zk_create_node_with_empty_val)
{
    int ret = 0;
    string path("/qconf/__unit_test/tst_create_nullval");
    string buf;

    ret = zk_create_node(zh, path, "", 0);
    EXPECT_EQ(QCONF_OK, ret);

    ret = zk_get_node(zh, path, buf, 0);
    EXPECT_EQ(QCONF_OK, ret);
    EXPECT_STREQ("", buf.c_str());

    ret = zoo_delete(zh, path.c_str(), -1);
    EXPECT_EQ(ret, ZOK);
}

// Test for zk_create_node: length of node : 1K
TEST_F(Test_qconf_zk, zk_create_node_with_1k_val)
{
    string buf;
    int ret = 0;
    string path("/qconf/__unit_test/test_create_1k");
    string value(1024, 'a');

    ret = zk_create_node(zh, path, value, 0);
    EXPECT_EQ(QCONF_OK, ret);
    
    ret = zk_get_node(zh, path, buf, 0);
    EXPECT_EQ(QCONF_OK, ret);
    EXPECT_STREQ(value.c_str(), buf.c_str());

    ret = zoo_delete(zh, path.c_str(), -1);
    EXPECT_EQ(ret, ZOK);

}

/**
 * End_Test_for function:
 * int zk_create_node(zhandle_t *zh, const string &path, string &buf)
 * =========================================================================================================
 */

/**
 * ===============================================================================================
 * Begin_Test_for function:
 * int zk_register_ephemeral(zhandle_t *zh, const string &path, const string &value)
 */

// Test for zk_register_ephemeral: zh=NULL
TEST_F(Test_qconf_zk, zk_register_ephemeral_null_zh)
{
    int ret = 0;
    string path = "/qconf/__unit_test/test";
    const char* val = "hello";

    ret = zk_register_ephemeral(NULL, path, val);

    EXPECT_EQ(QCONF_ERR_PARAM, ret);
}

// Test for zk_register_ephemeral: path=NULL
TEST_F(Test_qconf_zk, zk_register_ephemeral_empty_path)
{
    int ret = 0;
    const char* val = "hello";
    string path;

    ret = zk_register_ephemeral(zh, path, val);

    EXPECT_EQ(QCONF_ERR_PARAM, ret);
}

// Test for zk_register_ephemeral: value=NULL
TEST_F(Test_qconf_zk, zk_register_ephemeral_empty_value)
{
    int ret = 0;
    string path = "/qconf/test";
    string value;

    ret = zk_register_ephemeral(zh, path, value);

    EXPECT_EQ(QCONF_ERR_PARAM, ret);
}

// Test for zk_register_ephemeral: invalid path
TEST_F(Test_qconf_zk, zk_register_ephemeral_invalid_path)
{
    int ret = 0;
    string path = "qconf/test";
    const char* val = "hello";

    ret = zk_register_ephemeral(zh, path, val);

    EXPECT_EQ(QCONF_ERR_ZOO_FAILED, ret);
}

// Test for zk_register_ephemeral: path="/qconf"
TEST_F(Test_qconf_zk, zk_register_ephemeral_path_with_one_delim)
{
    int ret = 0;
    string path = "/qconf/__qconf_register_hosts";
    const char* val = "hello";
    string buf;
    char hostname[256] = {0};
    string path_buf;
    ret = gethostname(hostname, sizeof(hostname));
    EXPECT_EQ(0, ret);
    path_buf = path + "/" + hostname + "-" + hostname;

    ret = zk_register_ephemeral(zh, path_buf, val);
    zk_get_node(zh, path_buf, buf, 0);

    EXPECT_EQ(QCONF_OK, ret);
    EXPECT_EQ(5, buf.size());
    EXPECT_STREQ("hello", buf.c_str());
}

/**
 * End_Test_for function:
 * int zk_register_ephemeral(zhandle_t *zh, const string &path, const string &value)
 * =========================================================================================================
 */

/**
 * ========================================================================================================
 * Begin_Test_for function:
 * int qconf_init_zoo_log(const string &log_dir, const string &zoo_log)
 */

// Test for qconf_init_zoo_log: log_dir empty
TEST_F(Test_qconf_zk, qconf_init_zoo_log_empty_log_dir)
{
    string log_dir;
    string zoo_log;

    int ret = qconf_init_zoo_log(log_dir, zoo_log);
    EXPECT_EQ(QCONF_ERR_PARAM, ret);
}

// Test for qconf_init_zoo_log: zoo_log empty
TEST_F(Test_qconf_zk, qconf_init_zoo_log_empty_zoo_log)
{
    string log_dir(".");
    string zoo_log;

    int ret = qconf_init_zoo_log(log_dir, zoo_log);
    EXPECT_EQ(QCONF_ERR_PARAM, ret);
}

// Test for qconf_init_zoo_log: log dir not exist
TEST_F(Test_qconf_zk, qconf_init_zoo_log_unexist_log_dir)
{
    string log_dir("/unexist_log_dir");
    string zoo_log("zoo_err.log");

    int ret = qconf_init_zoo_log(log_dir, zoo_log);
    EXPECT_EQ(QCONF_ERR_FAILED_OPEN_FILE, ret);
}

// Test for qconf_init_zoo_log: zoo log not exist
TEST_F(Test_qconf_zk, qconf_init_zoo_log_unexist_zoo_log)
{
    string log_dir("./logs");
    string zoo_log("zoo_err.log");

    mode_t mode = 0755;
    mkdir(log_dir.c_str(), mode);
    string path = log_dir + "/" + zoo_log;
    unlink(path.c_str());

    int ret = qconf_init_zoo_log(log_dir, zoo_log);
    EXPECT_EQ(QCONF_OK, ret);
}

// Test for qconf_init_zoo_log: zoo log exist
TEST_F(Test_qconf_zk, qconf_init_zoo_log_exist_zoo_log)
{
    string log_dir("./logs");
    string zoo_log("zoo_err.log");

    mode_t mode = 0755;
    mkdir(log_dir.c_str(), mode);
    string path = log_dir + "/" + zoo_log;
    int fd = open(path.c_str(), O_RDWR | O_CREAT, mode);
    assert(fd > 0);
    close(fd);


    int ret = qconf_init_zoo_log(log_dir, zoo_log);
    EXPECT_EQ(QCONF_OK, ret);
}

/**
 * End_Test_for function:
 * int qconf_init_zoo_log(const string &log_dir, const string &zoo_log)
 * =========================================================================================================
 */

//End_Test_for qconf_zk.c
