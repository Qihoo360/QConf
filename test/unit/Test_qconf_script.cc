#include <zookeeper.h>
#include <sys/fcntl.h>

#include <string>

#include "gtest/gtest.h"
#include "qconf_const.h"
#include "qconf_format.h"
#include "qconf_script.h"

using namespace std;

class Test_qconf_script : public ::testing::Test
{
    public:
        virtual void SetUp()
        {
        }

        virtual void TearDown()
        {
        }
};

/**
 * ========================================================================================================
 * Begin_Test_for function:
 * int find_script(const string &path,  string &script)
 */

// Test for find_script : empty path
TEST_F(Test_qconf_script, find_script_empty_path)
{
    string node;
    string script;

    int ret = find_script(node, script);
    EXPECT_EQ(QCONF_ERR_PARAM, ret);
}

// Test for find_script : not init _qconf_script_dir
TEST_F(Test_qconf_script, find_script_not_init_script_dir)
{
    string node("/demo/mytest");
    string script;

    int ret = find_script(node, script);
    EXPECT_EQ(QCONF_ERR_SCRIPT_DIR_NOT_INIT, ret);
}

// Test for find_script : not exist _qconf_script_dir
TEST_F(Test_qconf_script, find_script_not_exist_script_dir)
{
    string node("/demo/mytest");
    string script;

    string agent_dir("/not_exist_agent_dir");
    qconf_init_script_dir(agent_dir);

    int ret = find_script(node, script);
    EXPECT_EQ(QCONF_ERR_SCRIPT_NOT_EXIST, ret);
}

// Test for find_script : not exist script
TEST_F(Test_qconf_script, find_script_not_exist_script)
{
    string node("/not_exist_demo");
    string script;

    mode_t mode = 0755;
    string agent_dir(".");
    string path("./script/");
    mkdir(path.c_str(), mode);
    qconf_init_script_dir(agent_dir);

    int ret = find_script(node, script);
    EXPECT_EQ(QCONF_ERR_SCRIPT_NOT_EXIST, ret);
}

// Test for find_script : not exist script with more node path
TEST_F(Test_qconf_script, find_script_not_exist_script_with_more_node_path)
{
    string node("/not_exist_demo/mytest/confs/conf1/conf11");
    string script;

    mode_t mode = 0755;
    string agent_dir(".");
    string path("./script/");
    mkdir(path.c_str(), mode);
    qconf_init_script_dir(agent_dir);

    int ret = find_script(node, script);
    EXPECT_EQ(QCONF_ERR_SCRIPT_NOT_EXIST, ret);
}

// Test for find_script : exist script for root node
TEST_F(Test_qconf_script, find_script_exist_script_root_node)
{
    string node("/demo/mytest/confs/conf1/conf11");
    string script;

    mode_t mode = 0755;
    string agent_dir(".");
    string path("./script/");
    mkdir(path.c_str(), mode);
    qconf_init_script_dir(agent_dir);

    mode = 0644;
    string root_file(path + "demo#mytest#confs#conf1#conf11.sh");
    string script_cnt("sub node file");
    int fd = open(root_file.c_str(), O_RDWR | O_CREAT, mode);
    assert(fd > 0);
    ssize_t ret = write(fd, script_cnt.c_str(), script_cnt.size());
    EXPECT_EQ(ret, script_cnt.size());

    ret = find_script(node, script);
    EXPECT_EQ(QCONF_OK, ret);
    EXPECT_STREQ(script_cnt.c_str(), script.c_str());
    close(fd);
    unlink(root_file.c_str());
}

// Test for find_script : exist script for sub node
TEST_F(Test_qconf_script, find_script_exist_script_sub_node)
{
    string node("/demo/mytest/confs/conf1/conf11");
    string script;

    mode_t mode = 0755;
    string agent_dir(".");
    string path("./script/");
    mkdir(path.c_str(), mode);
    qconf_init_script_dir(agent_dir);

    mode = 0644;
    string node_file(path + "demo#mytest#confs.sh");
    string script_cnt("parent node file");
    int fd = open(node_file.c_str(), O_RDWR | O_CREAT, mode);
    assert(fd > 0);
    ssize_t ret = write(fd, script_cnt.c_str(), script_cnt.size());
    EXPECT_EQ(ret, script_cnt.size());
    close(fd);

    ret = find_script(node, script);
    EXPECT_EQ(QCONF_OK, ret);
    EXPECT_STREQ(script_cnt.c_str(), script.c_str());
    unlink(node_file.c_str());
}

// Test for find_script : exist script for leaf node
TEST_F(Test_qconf_script, find_script_exist_script_leaf_node)
{
    string node("/demo/mytest/confs/conf1/conf11");
    string script;

    mode_t mode = 0755;
    string agent_dir(".");
    string path("./script/");
    mkdir(path.c_str(), mode);
    qconf_init_script_dir(agent_dir);

    mode = 0644;
    string node_file(path + "demo#mytest#confs#conf1#conf11.sh");
    string script_cnt("echo $qconf_path > c; echo $qconf_idc > c; echo $qconf_type > c");
    int fd = open(node_file.c_str(), O_RDWR | O_CREAT, mode);
    assert(fd > 0);
    ssize_t ret = write(fd, script_cnt.c_str(), script_cnt.size());
    EXPECT_EQ(ret, script_cnt.size());
    close(fd);

    ret = find_script(node, script);
    EXPECT_EQ(QCONF_OK, ret);
    EXPECT_STREQ(script_cnt.c_str(), script.c_str());
    unlink(node_file.c_str());
}

/**
 * End_Test_for function:
 * int find_script(const string &path,  string &script)
 * =========================================================================================================
 */

/**
 * ========================================================================================================
 * Begin_Test_for function:
 * int execute_script(const string &script, const long mtimeout)
 */
// Test for execute script : script is empty
TEST_F(Test_qconf_script, execute_script_empty_script)
{
    string script_cnt;

    int ret = execute_script(script_cnt, 1000);
    EXPECT_EQ(QCONF_ERR_PARAM, ret);
}

// Test for execute script : script is not right
TEST_F(Test_qconf_script, execute_script_illegal_script)
{
    string script_cnt("unright shell content");

    int ret = execute_script(script_cnt, 1000);
    EXPECT_EQ(QCONF_OK, ret);
}

// Test for execute script : script is right
TEST_F(Test_qconf_script, execute_script_good_script)
{
    string script_cnt("echo yes > a;");

    int ret = execute_script(script_cnt, 1000);
    EXPECT_EQ(QCONF_OK, ret);
}

// Test for execute script : script is timeout
TEST_F(Test_qconf_script, execute_script_timeout)
{
    string script_cnt("sleep 2");

    int ret = execute_script(script_cnt, 1000);
    EXPECT_EQ(QCONF_ERR_SCRIPT_TIMEOUT, ret);
}

/**
 * End_Test_for function:
 * int execute_script(const string &script, const long mtimeout)
 * =========================================================================================================
 */
