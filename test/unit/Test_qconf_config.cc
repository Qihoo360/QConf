#include <zookeeper.h>
#include <sys/fcntl.h>

#include <string>

#include "gtest/gtest.h"
#include "qconf_const.h"
#include "qconf_format.h"
#include "qconf_config.h"

using namespace std;

class Test_qconf_config : public ::testing::Test
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
 * int get_integer(const string &cnt, long &integer)
 */
// Test for get_integer: cnt empty
TEST_F(Test_qconf_config, get_integer_empty_cnt)
{
    string cnt;
    long n;

    int ret = get_integer(cnt, n);

    EXPECT_EQ(QCONF_ERR_PARAM, ret);
}

// Test for get_integer: not number, cnt: " "
TEST_F(Test_qconf_config, get_integer_with_space)
{
    string cnt = " ";
    long n = 1;

    int ret = get_integer(cnt, n);

    EXPECT_EQ(QCONF_ERR_NOT_NUMBER, ret);
}

// Test for get_integer: cnt: "123"
TEST_F(Test_qconf_config, get_integer_digit)
{
    string val = "123";
    long n;

    int ret = get_integer(val, n);

    EXPECT_EQ(QCONF_OK, ret);
    EXPECT_EQ(123, n);
}

// Test for get_integer: cnt: "00123456"
TEST_F(Test_qconf_config,get_integer_zero_start)
{
	string val = "00123456";
	long n;

	int ret = get_integer(val, n);

	EXPECT_EQ(QCONF_OK, ret);
	EXPECT_EQ(0123456, n);
}

// Test for get_integer: cnt: "+12345"
TEST_F(Test_qconf_config, get_integer_not_digit_start)
{
    string val = "+12345";
    long n;

    int ret = get_integer(val, n);

    EXPECT_EQ(QCONF_OK, ret);
    EXPECT_EQ(12345, n);
}

// Test for get_integer: cnt: "-0123"
TEST_F(Test_qconf_config,get_integer_not_digit_start2)
{
	string val="-0123";
	long n;

	int ret = get_integer(val, n);

	EXPECT_EQ(QCONF_OK, ret);
	EXPECT_EQ(-0123, n);
}

// Test for get_integer: cnt: "s1234"
TEST_F(Test_qconf_config, get_integer_illegal_start)
{
    string val = "s1234";
    long n;

    int ret = get_integer(val, n);

    EXPECT_EQ(QCONF_ERR_NOT_NUMBER, ret);
}

// Test for get_integer: cnt: " 123"
TEST_F(Test_qconf_config, get_integer_starts_with_space)
{
    string val = " 123";
    long n;

    int ret = get_integer(val, n);

    EXPECT_EQ(QCONF_OK, ret);
    EXPECT_EQ(123, n);
}

// Test for get_integer: cnt: "123a"
TEST_F(Test_qconf_config, get_integer_contains_alpha)
{
    string val = "123a";
    long n;

    int ret = get_integer(val, n);

    EXPECT_EQ(QCONF_ERR_OTHRE_CHARACTER, ret);
}

// Test for get_integer: cnt: "1a2"
TEST_F(Test_qconf_config, get_integer_contains_alpah2)
{
    string val = "1a2";
    long n;

    int ret = get_integer(val, n);

    EXPECT_EQ(QCONF_ERR_OTHRE_CHARACTER, ret);
}

// Test for get_integer: cnt: "1 2"
TEST_F(Test_qconf_config, get_integer_contais_other_character)
{
    string val = "1 2";
    long n;

    int ret = get_integer(val, n);

    EXPECT_EQ(QCONF_ERR_OTHRE_CHARACTER, ret);
}
/**
 * End_Test_for function:
 * int get_integer(const string &cnt, long &integer)
 * =========================================================================================================
 */

/**
 * ========================================================================================================
 * Begin_Test_for function:
 * int qconf_load_conf(const string &agent_dir)
 */
// Test for qconf_load_conf : illegal agent dir
TEST_F(Test_qconf_config, qconf_load_conf_illegal_dir)
{
    string agent_dir("/illegal_dir");

    int ret = qconf_load_conf(agent_dir);

    EXPECT_EQ(QCONF_ERR_OPEN, ret);
}

// Test for qconf_load_conf : not exist agent conf
TEST_F(Test_qconf_config, qconf_load_conf_not_exist_agent_conf)
{
    string agent_dir(".");
    string conf_dir("./conf");
    string agent_conf_path("./conf/agent.conf");
    mode_t mode = 0755;

    mkdir(conf_dir.c_str(), mode);
    unlink(agent_conf_path.c_str());
    int ret = qconf_load_conf(agent_dir);

    EXPECT_EQ(QCONF_ERR_OPEN, ret);
}

// Test for qconf_load_conf : not exist idc conf
TEST_F(Test_qconf_config, qconf_load_conf_not_exist_idc_conf)
{
    string agent_dir(".");
    string conf_dir("./conf");
    string agent_conf_path("./conf/agent.conf");
    string idc_conf_path("./conf/idc.conf");
    mode_t mode = 0755;

    mkdir(conf_dir.c_str(), mode);
    mode = 0644;
    int fd = open(agent_conf_path.c_str(), O_RDWR | O_CREAT, mode);
    close(fd);
    unlink(idc_conf_path.c_str());
    int ret = qconf_load_conf(agent_dir);

    EXPECT_EQ(QCONF_ERR_OPEN, ret);
}

// Test for qconf_load_conf : not exist local idc conf
TEST_F(Test_qconf_config, qconf_load_conf_not_exist_local_idc_conf)
{
    string agent_dir(".");
    string conf_dir("./conf");
    string agent_conf_path("./conf/agent.conf");
    string idc_conf_path("./conf/idc.conf");
    string local_idc_conf_path("./conf/localidc");
    mode_t mode = 0755;

    mkdir(conf_dir.c_str(), mode);
    mode = 0644;
    int fd = open(agent_conf_path.c_str(), O_RDWR | O_CREAT, mode);
    close(fd);
    fd = open(idc_conf_path.c_str(), O_RDWR | O_CREAT, mode);
    close(fd);
    unlink(local_idc_conf_path.c_str());
    int ret = qconf_load_conf(agent_dir);

    EXPECT_EQ(QCONF_ERR_OPEN, ret);
}

// Test for qconf_load_conf :  exist all conf
TEST_F(Test_qconf_config, qconf_load_conf_exist_all_conf)
{
    string agent_dir(".");
    string conf_dir("./conf");
    string agent_conf_path("./conf/agent.conf");
    string idc_conf_path("./conf/idc.conf");
    string local_idc_conf_path("./conf/localidc");
    mode_t mode = 0755;

    mkdir(conf_dir.c_str(), mode);
    mode = 0644;
    int fd = open(agent_conf_path.c_str(), O_RDWR | O_CREAT, mode);
    string agent_conf = string("")
+ "############################################################################\n"
+ "#                             QCONF config                                 #\n"
+ "############################################################################\n"
+ "\n"
+ "#[common]\n"
+ "# 0 => console mode; 1 => background mode.\n"
+ "daemon_mode=1\n"
+ "\n"
+ "# log directory and log format which is same format of the \"strftime\"\n"
+ "log_fmt=/data/logs/qconf.log.%Y-%m-%d-%H\n"
+ "\n"
+ "# debug => 0; trace => 1; info => 2; warning => 3; error => 4; fatal_error => 5\n"
+ "log_level=4\n"
+ "\n"
+ "# Set the zookeeper timeout\n"
+ "zookeeper_recv_timeout=30000\n"
+ "\n"
+ "# Register the node on zookeeper server\n"
+ "register_node_prefix=/qconf/__qconf_register_hosts\n"
+ "\n"
+ "# zookeeper log\n"
+ "zk_log=zoo.err.log\n"
+ "\n"
+ "# try max times of reading from share memory\n"
+ "max_repeat_read_times=100\n"
+ "\n"
+ "# feedback enable flags;  1: enable;  0: unable\n"
+ "feedback_enable=1\n"
+ "\n"
+ "# feedback url\n"
+ "feedback_url=localhost:8080/feedback.php\n";
    int ret = write(fd, agent_conf.c_str(), agent_conf.size());
    EXPECT_EQ(ret, agent_conf.size());
    close(fd);

    fd = open(idc_conf_path.c_str(), O_RDWR | O_CREAT, mode);
    string idc_conf = string("")
+ "#[zookeeper]\n"
+ "zookeeper.test=127.0.0.1:2181\n";
    ret = write(fd, idc_conf.c_str(), idc_conf.size());
    EXPECT_EQ(ret, idc_conf.size());
    close(fd);

    fd = open(local_idc_conf_path.c_str(), O_RDWR | O_CREAT, mode);
    string local_idc = "test";
    ret = write(fd, local_idc.c_str(), local_idc.size());
    EXPECT_EQ(ret, local_idc.size());
    close(fd);

    ret = qconf_load_conf(agent_dir);

    EXPECT_EQ(QCONF_OK, ret);

    string value;
    ret = get_agent_conf(QCONF_KEY_DAEMON_MODE, value);
    EXPECT_STREQ(value.c_str(), "1");
    ret = get_agent_conf(QCONF_KEY_LOG_FMT, value);
    EXPECT_STREQ(value.c_str(), "/data/logs/qconf.log.%Y-%m-%d-%H");
    ret = get_agent_conf(QCONF_KEY_LOG_LEVEL, value);
    EXPECT_STREQ(value.c_str(), "4");
    ret = get_agent_conf(QCONF_KEY_ZKRECVTIMEOUT, value);
    EXPECT_STREQ(value.c_str(), "30000");
    ret = get_agent_conf(QCONF_KEY_REGISTER_NODE_PREFIX, value);
    EXPECT_STREQ(value.c_str(), "/qconf/__qconf_register_hosts");
    ret = get_agent_conf(QCONF_KEY_ZKLOG_PATH, value);
    EXPECT_STREQ(value.c_str(), "zoo.err.log");
    ret = get_agent_conf(QCONF_KEY_MAX_REPEAT_READ_TIMES, value);
    EXPECT_STREQ(value.c_str(), "100");
    ret = get_agent_conf(QCONF_KEY_FEEDBACK_ENABLE, value);
    EXPECT_STREQ(value.c_str(), "1");
    ret = get_agent_conf(QCONF_KEY_FEEDBACK_URL, value);
    EXPECT_STREQ(value.c_str(), "localhost:8080/feedback.php");
    
    ret = get_idc_conf("test", value);
    EXPECT_STREQ(value.c_str(), "127.0.0.1:2181");

    ret = get_agent_conf(QCONF_KEY_LOCAL_IDC, value);
    EXPECT_STREQ(value.c_str(), "test");

    qconf_destroy_conf_map();
}

// Test for qconf_load_conf : only part of agent conf exist
TEST_F(Test_qconf_config, qconf_load_conf_part_agent_conf)
{
    string agent_dir(".");
    string conf_dir("./conf");
    string agent_conf_path("./conf/agent.conf");
    string idc_conf_path("./conf/idc.conf");
    string local_idc_conf_path("./conf/localidc");
    mode_t mode = 0755;

    mkdir(conf_dir.c_str(), mode);
    mode = 0644;
    int fd = open(agent_conf_path.c_str(), O_RDWR | O_CREAT, mode);
    string agent_conf = string("")
+ "############################################################################\n"
+ "#                             QCONF config                                 #\n"
+ "############################################################################\n"
+ "\n"
+ "#[common]\n"
+ "# 0 => console mode; 1 => background mode.\n"
+ "#daemon_mode=1\n"
+ "\n"
+ "# log directory and log format which is same format of the \"strftime\"\n"
+ "log_fmt=/data/logs/qconf.log.%Y-%m-%d-%H\n"
+ "\n"
+ "# debug => 0; trace => 1; info => 2; warning => 3; error => 4; fatal_error => 5\n"
+ "log_level=4\n"
+ "\n"
+ "# Set the zookeeper timeout\n"
+ "#zookeeper_recv_timeout=30000\n"
+ "\n"
+ "# Register the node on zookeeper server\n"
+ "register_node_prefix=/qconf/__qconf_register_hosts\n"
+ "\n"
+ "# zookeeper log\n"
+ "zk_log=zoo.err.log\n"
+ "\n"
+ "# try max times of reading from share memory\n"
+ "max_repeat_read_times=100\n"
+ "\n"
+ "# feedback enable flags;  1: enable;  0: unable\n"
+ "feedback_enable=1\n"
+ "\n"
+ "# feedback url\n"
+ "feedback_url=localhost:8080/feedback.php\n";
    int ret = write(fd, agent_conf.c_str(), agent_conf.size());
    EXPECT_EQ(ret, agent_conf.size());
    close(fd);

    fd = open(idc_conf_path.c_str(), O_RDWR | O_CREAT, mode);
    string idc_conf = string("")
+ "#[zookeeper]\n"
+ "zookeeper.gtest=127.0.0.1:2181\n";
    ret = write(fd, idc_conf.c_str(), idc_conf.size());
    EXPECT_EQ(ret, idc_conf.size());
    close(fd);

    ret = qconf_load_conf(agent_dir);

    EXPECT_EQ(QCONF_OK, ret);

    string value;
    ret = get_agent_conf(QCONF_KEY_DAEMON_MODE, value);
    EXPECT_EQ(QCONF_ERR_NOT_FOUND, ret);
    ret = get_agent_conf(QCONF_KEY_LOG_FMT, value);
    EXPECT_STREQ(value.c_str(), "/data/logs/qconf.log.%Y-%m-%d-%H");
    ret = get_agent_conf(QCONF_KEY_LOG_LEVEL, value);
    EXPECT_STREQ(value.c_str(), "4");
    ret = get_agent_conf(QCONF_KEY_ZKRECVTIMEOUT, value);
    EXPECT_EQ(QCONF_ERR_NOT_FOUND, ret);
    ret = get_agent_conf(QCONF_KEY_REGISTER_NODE_PREFIX, value);
    EXPECT_STREQ(value.c_str(), "/qconf/__qconf_register_hosts");
    ret = get_agent_conf(QCONF_KEY_ZKLOG_PATH, value);
    EXPECT_STREQ(value.c_str(), "zoo.err.log");
    ret = get_agent_conf(QCONF_KEY_MAX_REPEAT_READ_TIMES, value);
    EXPECT_STREQ(value.c_str(), "100");
    ret = get_agent_conf(QCONF_KEY_FEEDBACK_ENABLE, value);
    EXPECT_STREQ(value.c_str(), "1");
    ret = get_agent_conf(QCONF_KEY_FEEDBACK_URL, value);
    EXPECT_STREQ(value.c_str(), "localhost:8080/feedback.php");
    
    ret = get_idc_conf("test", value);
    EXPECT_EQ(QCONF_ERR_NOT_FOUND, ret);

    ret = get_agent_conf(QCONF_KEY_LOCAL_IDC, value);
    EXPECT_STREQ(value.c_str(), "test");

    qconf_destroy_conf_map();
}

// Test for qconf_load_conf : local idc with wrap
TEST_F(Test_qconf_config, qconf_load_conf_localidc_with_wrap)
{
    string agent_dir(".");
    string conf_dir("./conf");
    string agent_conf_path("./conf/agent.conf");
    string idc_conf_path("./conf/idc.conf");
    string local_idc_conf_path("./conf/localidc");
    mode_t mode = 0755;

    mkdir(conf_dir.c_str(), mode);
    mode = 0644;
    int fd = open(local_idc_conf_path.c_str(), O_RDWR | O_CREAT, mode);
    string local_idc = "test\n";
    int ret = write(fd, local_idc.c_str(), local_idc.size());
    EXPECT_EQ(ret, local_idc.size());
    close(fd);

    ret = qconf_load_conf(agent_dir);

    EXPECT_EQ(QCONF_OK, ret);

    string value;
    ret = get_agent_conf(QCONF_KEY_LOCAL_IDC, value);
    EXPECT_STREQ(value.c_str(), "test");
}
/**
 * End_Test_for function:
 * int get_agent_conf(const string &key, string &value)
 * =========================================================================================================
 */

/**
 * ========================================================================================================
 * Begin_Test_for function:
 * int get_agent_conf(const string &key, string &value)
 */

// Test for get_agent_conf : empty key
TEST_F(Test_qconf_config, get_agent_conf_empty_key)
{
    string key;
    string value;

    int ret = get_agent_conf(key, value);
    EXPECT_EQ(QCONF_ERR_PARAM, ret);
}

// Test for get_agent_conf : illegal key
TEST_F(Test_qconf_config, get_agent_conf_illegal_key)
{
    string key("illegal_key");
    string value;

    int ret = get_agent_conf(key, value);
    EXPECT_EQ(QCONF_ERR_NOT_FOUND, ret);
}

/**
 * End_Test_for function:
 * int qconf_load_conf(const string &agent_dir)
 * =========================================================================================================
 */

/**
 * ========================================================================================================
 * Begin_Test_for function:
 * int get_idc_conf(const string &key, string &value)
 */

// Test for get_idc_conf : empty key
TEST_F(Test_qconf_config, get_idc_conf_empty_key)
{
    string key;
    string value;

    int ret = get_idc_conf(key, value);
    EXPECT_EQ(QCONF_ERR_PARAM, ret);
}

// Test for get_idc_conf : illegal key
TEST_F(Test_qconf_config, get_idc_conf_illegal_key)
{
    string key("illegal_key");
    string value;

    int ret = get_idc_conf(key, value);
    EXPECT_EQ(QCONF_ERR_NOT_FOUND, ret);
}

/**
 * End_Test_for function:
 * int qconf_load_conf(const string &agent_dir)
 * =========================================================================================================
 */
