#include <zookeeper.h>
#include <string>
#include "gtest/gtest.h"
#include "qconf_zk.h"

using namespace std;

//unit test case for qconf_zk.c

class qconf_zkTestF : public :: testing::Test
{
protected:
    virtual void SetUp()
    {
        int ret =  qconfZk.zk_init("127.0.0.1:2181");
        EXPECT_EQ(QCONF_OK, ret);
    }

    virtual void TearDown()
    {
        qconfZk.zk_close();
    }
    QConfZK qconfZk;
};




/**
 * Test for qconfZk.zk_path
 */
TEST_F(qconf_zkTestF, zk_qconf_empty)
{
    int retCode = 0;
    string path;
    string new_path;

    retCode = qconfZk.zk_path(path, new_path);

    EXPECT_EQ(QCONF_ERR_PARAM, retCode);
}

TEST_F(qconf_zkTestF, zk_qconf_right1)
{
    int retCode = 0;
    string path = "qconf";
    string new_path;

    retCode = qconfZk.zk_path(path, new_path);

    EXPECT_EQ(QCONF_OK, retCode);
    EXPECT_STREQ(new_path.c_str(), "/qconf");
}

TEST_F(qconf_zkTestF, zk_qconf_slash_begin)
{
    int retCode = 0;
    string path = "/qconf";
    string new_path;

    retCode = qconfZk.zk_path(path, new_path);

    EXPECT_EQ(QCONF_OK, retCode);
    EXPECT_STREQ(new_path.c_str(), "/qconf");
}

TEST_F(qconf_zkTestF, zk_qconf_slash)
{
    int retCode = 0;
    string path = "/qconf/demo/";
    string new_path;

    retCode = qconfZk.zk_path(path, new_path);

    EXPECT_EQ(QCONF_OK, retCode);
    EXPECT_STREQ(new_path.c_str(), "/qconf/demo");
}

TEST_F(qconf_zkTestF, zk_qconf_all_char)
{
    int retCode = 0;
    string path = "q_c-o.n:f/h213ello";
    string new_path;

    retCode = qconfZk.zk_path(path, new_path);

    EXPECT_EQ(QCONF_OK, retCode);
    EXPECT_STREQ(new_path.c_str(), "/q_c-o.n:f/h213ello");
}


TEST_F(qconf_zkTestF, zk_qconf_multi_slash)
{
    int retCode = 0;
    string path = "///qconf/demo///";
    string new_path;

    retCode = qconfZk.zk_path(path, new_path);

    EXPECT_EQ(QCONF_OK, retCode);
    EXPECT_STREQ(new_path.c_str(), "/qconf/demo");
}

TEST_F(qconf_zkTestF, zk_qconf_multi_slash_inner)
{
    int retCode = 0;
    string path = "///qconf//demo///";
    string new_path;

    retCode = qconfZk.zk_path(path, new_path);

    EXPECT_EQ(QCONF_OK, retCode);
    EXPECT_STREQ(new_path.c_str(), "/qconf//demo");
}

TEST_F(qconf_zkTestF, zk_qconf_err)
{
    int retCode = 0;
    string path = "qconf/dem&$o";
    string new_path;

    retCode = qconfZk.zk_path(path, new_path);

    EXPECT_EQ(QCONF_ERR_OTHER, retCode);
}

/**
 * Test for qconfZk.zk_node_set
 */
//int qconfZk.zk_node_set(const std::string &node, const std::string &value);
TEST_F(qconf_zkTestF, zk_node_set_modify)
{
    int retCode = 0;
    string path = "qconf/demo/1";
    string val("hello"), out_val;
    
    retCode = qconfZk.zk_node_set(path, val);
    EXPECT_EQ(QCONF_OK, retCode);
    
    retCode = qconfZk.zk_node_get(path, out_val);
    EXPECT_EQ(QCONF_OK, retCode);
    EXPECT_EQ(val, out_val);
}

TEST_F(qconf_zkTestF, zk_node_set_add)
{
    int retCode = 0;
    string path = "qconf/demo/5";
    string val("hello"), out_val;
    
    retCode = qconfZk.zk_node_set(path, val);
    EXPECT_EQ(QCONF_OK, retCode);
    
    retCode = qconfZk.zk_node_get(path, out_val);
    EXPECT_EQ(QCONF_OK, retCode);
    EXPECT_EQ(val, out_val);

    retCode = qconfZk.zk_node_delete(path);
    EXPECT_EQ(QCONF_OK, retCode);
}

TEST_F(qconf_zkTestF, zk_node_set_add_parent_not_exist)
{
    int retCode = 0;
    string path = "qconf/demo/5/1/1/1";
    string val("hello"), out_val;
    
    retCode = qconfZk.zk_node_set(path, val);
    EXPECT_EQ(QCONF_OK, retCode);
    
    retCode = qconfZk.zk_node_get(path, out_val);
    EXPECT_EQ(QCONF_OK, retCode);
    EXPECT_EQ(val, out_val);

    retCode = qconfZk.zk_node_delete(path);
    EXPECT_EQ(QCONF_OK, retCode);
    retCode = qconfZk.zk_node_delete("qconf/demo/5/1/1");
    EXPECT_EQ(QCONF_OK, retCode);
    retCode = qconfZk.zk_node_delete("qconf/demo/5/1");
    EXPECT_EQ(QCONF_OK, retCode);
}

TEST_F(qconf_zkTestF, zk_node_set_twice)
{
    int retCode = 0;
    string path = "qconf/demo/5";
    string val("hello"), out_val;
    
    retCode = qconfZk.zk_node_set(path, val);
    EXPECT_EQ(QCONF_OK, retCode);
    
    retCode = qconfZk.zk_node_set(path, "hello1");
    EXPECT_EQ(QCONF_OK, retCode);
    
    retCode = qconfZk.zk_node_get(path, out_val);
    EXPECT_EQ(QCONF_OK, retCode);
    EXPECT_STREQ("hello1", out_val.c_str());

    retCode = qconfZk.zk_node_delete(path);
    EXPECT_EQ(QCONF_OK, retCode);
}

TEST_F(qconf_zkTestF, zk_node_set_empty)
{
    int retCode = 0;
    string path = "qconf/demo/1";
    string val, out_val;

    retCode = qconfZk.zk_node_set(path, val);
    EXPECT_EQ(QCONF_OK, retCode);
    
    retCode = qconfZk.zk_node_get(path, out_val);
    EXPECT_EQ(QCONF_OK, retCode);
    EXPECT_EQ(val, out_val);
}

TEST_F(qconf_zkTestF, zk_node_set_path_err)
{
    int retCode = 0;
    string path = "qconf/demo&&%#@";
    
    retCode = qconfZk.zk_node_set(path, "hello");
    EXPECT_EQ(QCONF_ERR_PARAM, retCode);
}

TEST_F(qconf_zkTestF, zk_node_set_path_empty)
{
    int retCode = 0;
    string path;
    
    retCode = qconfZk.zk_node_set(path, "hello");
    EXPECT_EQ(QCONF_ERR_PARAM, retCode);
}

/**
 * Test for qconfZk.zk_list_with_values
 */
//int qconfZk.zk_list_with_values(const std::string &node, std::map<std::string, std::string> &children);
TEST_F(qconf_zkTestF, zk_list_with_values_path_empty)
{
    int retCode = 0;
    string path;
    map<string, string> children;

    retCode = qconfZk.zk_list_with_values(path, children);
    EXPECT_EQ(QCONF_ERR_PARAM, retCode);
}

TEST_F(qconf_zkTestF, zk_list_with_values_children_empty)
{
    int retCode = 0;
    string path = "qconf/demo/6";
    map<string, string> children;

    retCode = qconfZk.zk_node_set(path, "6");
    EXPECT_EQ(QCONF_OK, retCode);
    
    retCode = qconfZk.zk_list_with_values(path, children);
    EXPECT_EQ(QCONF_OK, retCode);
    EXPECT_TRUE(children.empty());

    retCode = qconfZk.zk_node_delete(path);
    EXPECT_EQ(QCONF_OK, retCode);
}

TEST_F(qconf_zkTestF, zk_list_with_values_multi)
{
    int retCode = 0;
    string path = "qconf/demo/6";
    map<string, string> children, out_children;
    children.insert(map<string, string>::value_type("1", "1"));
    children.insert(map<string, string>::value_type("2", "2"));
    children.insert(map<string, string>::value_type("3", "3"));
    children.insert(map<string, string>::value_type("4", "4"));
    
    map<string, string>::iterator it = children.begin();
    for (; it != children.end(); ++it)
    {
        retCode = qconfZk.zk_node_set(path + "/" + it->first, it->second);
        EXPECT_EQ(QCONF_OK, retCode);
    }
    
    retCode = qconfZk.zk_list_with_values(path, out_children);
    EXPECT_EQ(QCONF_OK, retCode);
    EXPECT_EQ(children.size(), out_children.size());
    EXPECT_EQ(children, out_children);
    
    for (it = children.begin(); it != children.end(); ++it)
    {
        retCode = qconfZk.zk_node_delete(path + "/" + it->first);
        EXPECT_EQ(QCONF_OK, retCode);
    }

    retCode = qconfZk.zk_node_delete(path);
    EXPECT_EQ(QCONF_OK, retCode);
}

/**
 * Test for qconfZk.zk_services_set
 */
//int qconfZk.zk_services_set(const std::string &node, const std::map<std::string, char> &servs);
TEST_F(qconf_zkTestF, zk_services_set_common)
{
    int retCode = 0;
    string path = "qconf/demo/1";
    map<string, char> servs, out_servs;
    
    servs.insert(map<string, char>::value_type("1.1.1.1:80", STATUS_UP));
    servs.insert(map<string, char>::value_type("1.1.1.2:80", STATUS_UP));
    servs.insert(map<string, char>::value_type("1.1.1.3:80", STATUS_DOWN));
    servs.insert(map<string, char>::value_type("1.1.1.4:80", STATUS_UP));
    retCode = qconfZk.zk_services_set(path, servs);

    EXPECT_EQ(QCONF_OK, retCode);
    
    retCode = qconfZk.zk_services_get_with_status(path, out_servs);
    EXPECT_EQ(QCONF_OK, retCode);
    EXPECT_EQ(servs.size(), out_servs.size());
    EXPECT_EQ(servs, out_servs);
}

TEST_F(qconf_zkTestF, zk_services_set_twice)
{
    int retCode = 0;
    string path = "qconf/demo/1";
    map<string, char> servs, out_servs;
    
    servs.insert(map<string, char>::value_type("1.1.1.1:80", STATUS_UP));
    servs.insert(map<string, char>::value_type("1.1.1.2:80", STATUS_UP));
    servs.insert(map<string, char>::value_type("1.1.1.3:80", STATUS_DOWN));
    servs.insert(map<string, char>::value_type("1.1.1.4:80", STATUS_UP));
    
    retCode = qconfZk.zk_services_set(path, servs);
    EXPECT_EQ(QCONF_OK, retCode);
    
    retCode = qconfZk.zk_services_set(path, servs);
    EXPECT_EQ(QCONF_OK, retCode);
    
    retCode = qconfZk.zk_services_get_with_status(path, out_servs);
    EXPECT_EQ(QCONF_OK, retCode);
    EXPECT_EQ(servs.size(), out_servs.size());
    EXPECT_EQ(servs, out_servs);
}

TEST_F(qconf_zkTestF, zk_services_set_empty)
{
    int retCode = 0;
    string path = "qconf/demo/1";
    map<string, char> servs, out_servs;
    
    retCode = qconfZk.zk_services_set(path, servs);
    EXPECT_EQ(QCONF_ERR_PARAM, retCode);
}

TEST_F(qconf_zkTestF, zk_services_set_path_err)
{
    int retCode = 0;
    string path = "qconf/demo&&%#@";
    map<string, char> servs, out_servs;
    
    retCode = qconfZk.zk_services_set(path, servs);
    EXPECT_EQ(QCONF_ERR_PARAM, retCode);
}

TEST_F(qconf_zkTestF, zk_services_set_key_empty)
{
    int retCode = 0;
    string path = "qconf/demo/2";
    map<string, char> servs;
    servs.insert(map<string, char>::value_type("1.1.1.1:80", STATUS_UP));
    servs.insert(map<string, char>::value_type("", STATUS_UP));
    servs.insert(map<string, char>::value_type("1.1.1.3:80", STATUS_DOWN));
    servs.insert(map<string, char>::value_type("1.1.1.4:80", STATUS_UP));
    
    retCode = qconfZk.zk_services_set(path, servs);
    EXPECT_EQ(QCONF_ERR_PARAM, retCode);
}

TEST_F(qconf_zkTestF, zk_services_set_status_err)
{
    int retCode = 0;
    string path = "qconf/demo/2";
    map<string, char> servs;
    servs.insert(map<string, char>::value_type("1.1.1.1:80", STATUS_UP));
    servs.insert(map<string, char>::value_type("1.1.1.2:80", 10));
    servs.insert(map<string, char>::value_type("1.1.1.3:80", STATUS_DOWN));
    servs.insert(map<string, char>::value_type("1.1.1.4:80", STATUS_UP));
    
    retCode = qconfZk.zk_services_set(path, servs);
    EXPECT_EQ(QCONF_ERR_PARAM, retCode);
}

TEST_F(qconf_zkTestF, zk_services_set_modify_more)
{
    int retCode = 0;
    string path = "qconf/demo/1";
    map<string, char> servs, out_servs;
    
    servs.insert(map<string, char>::value_type("1.1.1.1:80", STATUS_UP));
    servs.insert(map<string, char>::value_type("1.1.1.2:80", STATUS_UP));
    servs.insert(map<string, char>::value_type("1.1.1.3:80", STATUS_DOWN));
    servs.insert(map<string, char>::value_type("1.1.1.4:80", STATUS_UP));
    
    retCode = qconfZk.zk_services_set(path, servs);
    EXPECT_EQ(QCONF_OK, retCode);
    
    servs.insert(map<string, char>::value_type("1.1.1.5:80", STATUS_UP));

    retCode = qconfZk.zk_services_set(path, servs);
    EXPECT_EQ(QCONF_OK, retCode);
    
    retCode = qconfZk.zk_services_get_with_status(path, out_servs);
    EXPECT_EQ(QCONF_OK, retCode);
    EXPECT_EQ(servs, out_servs);
}

TEST_F(qconf_zkTestF, zk_services_set_modify_less)
{
    int retCode = 0;
    string path = "qconf/demo/1";
    map<string, char> servs, out_servs;
    
    servs.insert(map<string, char>::value_type("1.1.1.1:80", STATUS_UP));
    servs.insert(map<string, char>::value_type("1.1.1.2:80", STATUS_UP));
    servs.insert(map<string, char>::value_type("1.1.1.3:80", STATUS_DOWN));
    servs.insert(map<string, char>::value_type("1.1.1.4:80", STATUS_UP));
    
    retCode = qconfZk.zk_services_set(path, servs);
    EXPECT_EQ(QCONF_OK, retCode);
    
    servs.erase("1.1.1.2:80");

    retCode = qconfZk.zk_services_set(path, servs);
    EXPECT_EQ(QCONF_OK, retCode);
    
    retCode = qconfZk.zk_services_get_with_status(path, out_servs);
    EXPECT_EQ(QCONF_OK, retCode);
    EXPECT_EQ(servs, out_servs);
}

TEST_F(qconf_zkTestF, zk_services_set_modify_change_has_same)
{
    int retCode = 0;
    string path = "qconf/demo/1";
    map<string, char> servs, out_servs;
    
    servs.insert(map<string, char>::value_type("1.1.1.1:80", STATUS_UP));
    servs.insert(map<string, char>::value_type("1.1.1.2:80", STATUS_UP));
    servs.insert(map<string, char>::value_type("1.1.1.3:80", STATUS_DOWN));
    servs.insert(map<string, char>::value_type("1.1.1.4:80", STATUS_UP));
    
    retCode = qconfZk.zk_services_set(path, servs);
    EXPECT_EQ(QCONF_OK, retCode);
    
    servs.clear();
    servs.insert(map<string, char>::value_type("1.1.1.2:80", STATUS_UP));
    servs.insert(map<string, char>::value_type("1.1.1.3:80", STATUS_DOWN));
    servs.insert(map<string, char>::value_type("1.1.1.5:80", STATUS_UP));

    retCode = qconfZk.zk_services_set(path, servs);
    EXPECT_EQ(QCONF_OK, retCode);
    
    retCode = qconfZk.zk_services_get_with_status(path, out_servs);
    EXPECT_EQ(QCONF_OK, retCode);
    EXPECT_EQ(servs, out_servs);
}

TEST_F(qconf_zkTestF, zk_services_set_modify_change_no_same)
{
    int retCode = 0;
    string path = "qconf/demo/1";
    map<string, char> servs, out_servs;
    
    servs.insert(map<string, char>::value_type("1.1.1.1:80", STATUS_UP));
    servs.insert(map<string, char>::value_type("1.1.1.2:80", STATUS_UP));
    servs.insert(map<string, char>::value_type("1.1.1.3:80", STATUS_DOWN));
    servs.insert(map<string, char>::value_type("1.1.1.4:80", STATUS_UP));
    
    retCode = qconfZk.zk_services_set(path, servs);
    EXPECT_EQ(QCONF_OK, retCode);
    
    servs.clear();
    servs.insert(map<string, char>::value_type("1.1.2.2:80", STATUS_UP));
    servs.insert(map<string, char>::value_type("1.1.2.3:80", STATUS_DOWN));
    servs.insert(map<string, char>::value_type("1.1.2.5:80", STATUS_UP));

    retCode = qconfZk.zk_services_set(path, servs);
    EXPECT_EQ(QCONF_OK, retCode);
    
    retCode = qconfZk.zk_services_get_with_status(path, out_servs);
    EXPECT_EQ(QCONF_OK, retCode);
    EXPECT_EQ(servs, out_servs);
}

TEST_F(qconf_zkTestF, zk_services_set_modify_change_change_status)
{
    int retCode = 0;
    string path = "qconf/demo/1";
    map<string, char> servs, out_servs;
    
    servs.insert(map<string, char>::value_type("1.1.1.1:80", STATUS_UP));
    servs.insert(map<string, char>::value_type("1.1.1.2:80", STATUS_UP));
    servs.insert(map<string, char>::value_type("1.1.1.3:80", STATUS_DOWN));
    servs.insert(map<string, char>::value_type("1.1.1.4:80", STATUS_UP));
    
    retCode = qconfZk.zk_services_set(path, servs);
    EXPECT_EQ(QCONF_OK, retCode);
    
    servs["1.1.1.2:80"] = STATUS_DOWN;

    retCode = qconfZk.zk_services_set(path, servs);
    EXPECT_EQ(QCONF_OK, retCode);
    
    retCode = qconfZk.zk_services_get_with_status(path, out_servs);
    EXPECT_EQ(QCONF_OK, retCode);
    EXPECT_EQ(servs, out_servs);
}

TEST_F(qconf_zkTestF, zk_services_set_node_not_exist)
{
    int retCode = 0;
    string path = "qconf/demo1/1";
    map<string, char> servs, out_servs;
    
    servs.insert(map<string, char>::value_type("1.1.1.1:80", STATUS_UP));
    servs.insert(map<string, char>::value_type("1.1.1.2:80", STATUS_UP));
    servs.insert(map<string, char>::value_type("1.1.1.3:80", STATUS_DOWN));
    servs.insert(map<string, char>::value_type("1.1.1.4:80", STATUS_UP));
    
    retCode = qconfZk.zk_services_set(path, servs);
    EXPECT_EQ(QCONF_OK, retCode);
    
    retCode = qconfZk.zk_services_get_with_status(path, out_servs);
    EXPECT_EQ(QCONF_OK, retCode);
    EXPECT_EQ(servs.size(), out_servs.size());
    EXPECT_EQ(servs, out_servs);

    retCode = qconfZk.zk_service_clear(path);
    EXPECT_EQ(QCONF_OK, retCode);
}

//Test for qconfZk.zk_service_add
//int qconfZk.zk_service_add(const std::string &node, const std::string &serv, const char &status);
TEST_F(qconf_zkTestF, zk_service_add_change_type)
{
    int retCode = 0;
    string path = "qconf/demo/2";
    map<string, char> out_servs;

    retCode = qconfZk.zk_service_add(path, "1.1.1.5:80", STATUS_UP);
    EXPECT_EQ(QCONF_OK, retCode);
    
    retCode = qconfZk.zk_services_get_with_status(path, out_servs);
    EXPECT_EQ(QCONF_OK, retCode);
    EXPECT_EQ(1, out_servs.size());
    EXPECT_EQ(out_servs["1.1.1.5:80"], STATUS_UP);

    retCode = qconfZk.zk_service_clear(path);
    EXPECT_EQ(QCONF_OK, retCode);
}

TEST_F(qconf_zkTestF, zk_service_add_change_empty)
{
    int retCode = 0;
    string path = "qconf/demo/2";
    map<string, char> out_servs;

    retCode = qconfZk.zk_service_add(path, "", STATUS_UP);
    EXPECT_EQ(QCONF_ERR_PARAM, retCode);
}

TEST_F(qconf_zkTestF, zk_service_add_change_err_status)
{
    int retCode = 0;
    string path = "qconf/demo/2";
    map<string, char> out_servs;

    retCode = qconfZk.zk_service_add(path, "", 10);
    EXPECT_EQ(QCONF_ERR_PARAM, retCode);
}

TEST_F(qconf_zkTestF, zk_service_add_on_service)
{
    int retCode = 0;
    string path = "qconf/demo/2";
    map<string, char> out_servs;

    retCode = qconfZk.zk_service_add(path, "1.1.1.6:80", STATUS_UP);
    EXPECT_EQ(QCONF_OK, retCode);
    
    retCode = qconfZk.zk_services_get_with_status(path, out_servs);
    EXPECT_EQ(QCONF_OK, retCode);
    EXPECT_EQ(1, out_servs.size());
    EXPECT_EQ(out_servs["1.1.1.6:80"], STATUS_UP);
    
    retCode = qconfZk.zk_service_clear(path);
    EXPECT_EQ(QCONF_OK, retCode);
}

TEST_F(qconf_zkTestF, zk_service_add_on_repeat)
{
    int retCode = 0;
    string path = "qconf/demo/2";
    map<string, char> out_servs;

    retCode = qconfZk.zk_service_add(path, "1.1.1.7:80", STATUS_UP);
    EXPECT_EQ(QCONF_OK, retCode);
    
    retCode = qconfZk.zk_service_add(path, "1.1.1.7:80", STATUS_DOWN);
    EXPECT_EQ(QCONF_OK, retCode);
    
    retCode = qconfZk.zk_services_get_with_status(path, out_servs);
    EXPECT_EQ(QCONF_OK, retCode);
    EXPECT_EQ(1, out_servs.size());
    EXPECT_EQ(out_servs["1.1.1.7:80"], STATUS_DOWN);
    
    retCode = qconfZk.zk_service_clear(path);
    EXPECT_EQ(QCONF_OK, retCode);
}

TEST_F(qconf_zkTestF, zk_service_add_on_multi)
{
    int retCode = 0;
    string path = "qconf/demo/2";
    map<string, char> servs, out_servs;
    servs.insert(map<string, char>::value_type("1.1.1.1:80", STATUS_UP));
    servs.insert(map<string, char>::value_type("1.1.1.2:80", STATUS_UP));
    servs.insert(map<string, char>::value_type("1.1.1.3:80", STATUS_DOWN));
    servs.insert(map<string, char>::value_type("1.1.1.4:80", STATUS_UP));
    
    map<string, char>::iterator it = servs.begin();
    for (; it != servs.end(); ++it)
    {
        retCode = qconfZk.zk_service_add(path, it->first, it->second);
        EXPECT_EQ(QCONF_OK, retCode);
    }
    
    retCode = qconfZk.zk_services_get_with_status(path, out_servs);
    EXPECT_EQ(QCONF_OK, retCode);
    EXPECT_EQ(servs.size(), out_servs.size());
    EXPECT_EQ(out_servs, servs);
    
    retCode = qconfZk.zk_service_clear(path);
    EXPECT_EQ(QCONF_OK, retCode);
}


//Test for qconfZk.zk_service_delete
//int qconfZk.zk_service_delete(const std::string &node, const std::string &serv);
TEST_F(qconf_zkTestF, zk_service_delete_path_empty)
{
    int retCode = 0;
    string path;
    map<string, char> out_servs;

    retCode = qconfZk.zk_service_delete(path, "1.1.1.1:80");
    EXPECT_EQ(QCONF_ERR_PARAM, retCode);
}

TEST_F(qconf_zkTestF, zk_service_delete_path_not_exist)
{
    int retCode = 0;
    string path = "qconf/demo2/2";
    map<string, char> out_servs;

    retCode = qconfZk.zk_service_delete(path, "1.1.1.1:80");
    EXPECT_EQ(QCONF_ERR_NODE_TYPE, retCode);
}

TEST_F(qconf_zkTestF, zk_service_delete_common)
{
    int retCode = 0;
    string path = "qconf/demo/3";
    map<string, char> servs, out_servs;
    servs.insert(map<string, char>::value_type("1.1.1.1:80", STATUS_UP));
    servs.insert(map<string, char>::value_type("1.1.1.2:80", STATUS_UP));
    servs.insert(map<string, char>::value_type("1.1.1.3:80", STATUS_DOWN));
    servs.insert(map<string, char>::value_type("1.1.1.4:80", STATUS_UP));
    
    retCode = qconfZk.zk_services_set(path, servs);
    EXPECT_EQ(QCONF_OK, retCode);

    retCode = qconfZk.zk_service_delete(path, "1.1.1.3:80");
    EXPECT_EQ(QCONF_OK, retCode);
    
    retCode = qconfZk.zk_services_get_with_status(path, out_servs);
    EXPECT_EQ(QCONF_OK, retCode);

    servs.erase("1.1.1.3:80");
    EXPECT_EQ(servs.size(), out_servs.size());
    EXPECT_EQ(out_servs, servs);
    
    retCode = qconfZk.zk_service_clear(path);
    EXPECT_EQ(QCONF_OK, retCode);
}

TEST_F(qconf_zkTestF, zk_service_delete_serv_not_exist)
{
    int retCode = 0;
    string path = "qconf/demo/3";
    map<string, char> servs, out_servs;
    servs.insert(map<string, char>::value_type("1.1.1.1:80", STATUS_UP));
    servs.insert(map<string, char>::value_type("1.1.1.2:80", STATUS_UP));
    servs.insert(map<string, char>::value_type("1.1.1.3:80", STATUS_DOWN));
    servs.insert(map<string, char>::value_type("1.1.1.4:80", STATUS_UP));
    
    retCode = qconfZk.zk_services_set(path, servs);
    EXPECT_EQ(QCONF_OK, retCode);

    retCode = qconfZk.zk_service_delete(path, "2.1.1.1:80");
    EXPECT_EQ(QCONF_OK, retCode);
}

// Test for qconfZk.zk_service_up
//int qconfZk.zk_service_up(const std::string &node, const std::string &serv)
TEST_F(qconf_zkTestF, zk_service_up_path_empty)
{
    int retCode = 0;
    string path;
    map<string, char> out_servs;

    retCode = qconfZk.zk_service_up(path, "1.1.1.1:80");
    EXPECT_EQ(QCONF_ERR_PARAM, retCode);
}

TEST_F(qconf_zkTestF, zk_service_up_common)
{
    int retCode = 0;
    string path = "qconf/demo/3";
    map<string, char> servs, out_servs;
    servs.insert(map<string, char>::value_type("1.1.1.1:80", STATUS_UP));
    servs.insert(map<string, char>::value_type("1.1.1.2:80", STATUS_UP));
    servs.insert(map<string, char>::value_type("1.1.1.3:80", STATUS_DOWN));
    servs.insert(map<string, char>::value_type("1.1.1.4:80", STATUS_UP));
    
    retCode = qconfZk.zk_services_set(path, servs);
    EXPECT_EQ(QCONF_OK, retCode);

    retCode = qconfZk.zk_service_up(path, "1.1.1.3:80");
    EXPECT_EQ(QCONF_OK, retCode);
    
    retCode = qconfZk.zk_services_get_with_status(path, out_servs);
    EXPECT_EQ(QCONF_OK, retCode);

    servs["1.1.1.3:80"] = STATUS_UP;
    EXPECT_EQ(servs.size(), out_servs.size());
    EXPECT_EQ(out_servs, servs);
    
    retCode = qconfZk.zk_service_clear(path);
    EXPECT_EQ(QCONF_OK, retCode);
}

TEST_F(qconf_zkTestF, zk_service_up_serv_not_exist)
{
    int retCode = 0;
    string path = "qconf/demo/3";
    map<string, char> servs, out_servs;
    servs.insert(map<string, char>::value_type("1.1.1.1:80", STATUS_UP));
    servs.insert(map<string, char>::value_type("1.1.1.2:80", STATUS_UP));
    servs.insert(map<string, char>::value_type("1.1.1.3:80", STATUS_DOWN));
    servs.insert(map<string, char>::value_type("1.1.1.4:80", STATUS_UP));
    
    retCode = qconfZk.zk_services_set(path, servs);
    EXPECT_EQ(QCONF_OK, retCode);

    retCode = qconfZk.zk_service_up(path, "2.1.1.1:80");
    EXPECT_EQ(QCONF_ERR_ZOO_NOT_EXIST, retCode);
    
    retCode = qconfZk.zk_service_clear(path);
    EXPECT_EQ(QCONF_OK, retCode);
}

// Test for qconfZk.zk_service_offline
//int qconfZk.zk_service_offline(const std::string &node, const std::string &serv)
TEST_F(qconf_zkTestF, zk_service_offline_path_empty)
{
    int retCode = 0;
    string path;
    map<string, char> out_servs;

    retCode = qconfZk.zk_service_offline(path, "1.1.1.1:80");
    EXPECT_EQ(QCONF_ERR_PARAM, retCode);
}

TEST_F(qconf_zkTestF, zk_service_offline_common)
{
    int retCode = 0;
    string path = "qconf/demo/3";
    map<string, char> servs, out_servs;
    servs.insert(map<string, char>::value_type("1.1.1.1:80", STATUS_UP));
    servs.insert(map<string, char>::value_type("1.1.1.2:80", STATUS_UP));
    servs.insert(map<string, char>::value_type("1.1.1.3:80", STATUS_DOWN));
    servs.insert(map<string, char>::value_type("1.1.1.4:80", STATUS_UP));
    
    retCode = qconfZk.zk_services_set(path, servs);
    EXPECT_EQ(QCONF_OK, retCode);

    retCode = qconfZk.zk_service_offline(path, "1.1.1.2:80");
    EXPECT_EQ(QCONF_OK, retCode);
    
    retCode = qconfZk.zk_services_get_with_status(path, out_servs);
    EXPECT_EQ(QCONF_OK, retCode);

    servs["1.1.1.2:80"] = STATUS_OFFLINE;
    EXPECT_EQ(servs.size(), out_servs.size());
    EXPECT_EQ(out_servs, servs);
    
    retCode = qconfZk.zk_service_clear(path);
    EXPECT_EQ(QCONF_OK, retCode);
}

TEST_F(qconf_zkTestF, zk_service_offline_serv_not_exist)
{
    int retCode = 0;
    string path = "qconf/demo/3";
    map<string, char> servs, out_servs;
    servs.insert(map<string, char>::value_type("1.1.1.1:80", STATUS_UP));
    servs.insert(map<string, char>::value_type("1.1.1.2:80", STATUS_UP));
    servs.insert(map<string, char>::value_type("1.1.1.3:80", STATUS_DOWN));
    servs.insert(map<string, char>::value_type("1.1.1.4:80", STATUS_UP));
    
    retCode = qconfZk.zk_services_set(path, servs);
    EXPECT_EQ(QCONF_OK, retCode);

    retCode = qconfZk.zk_service_offline(path, "2.1.1.1:80");
    EXPECT_EQ(QCONF_ERR_ZOO_NOT_EXIST, retCode);
    
    retCode = qconfZk.zk_service_clear(path);
    EXPECT_EQ(QCONF_OK, retCode);
}

// Test for qconfZk.zk_service_clear
//int qconfZk.zk_service_clear(const std::string &node)
TEST_F(qconf_zkTestF, zk_service_clear_path_empty)
{
    int retCode = 0;
    string path;

    retCode = qconfZk.zk_service_clear(path);
    EXPECT_EQ(QCONF_ERR_PARAM, retCode);
}

TEST_F(qconf_zkTestF, zk_service_clear_common)
{
    int retCode = 0;
    string path = "qconf/demo/3";
    map<string, char> servs, out_servs;
    servs.insert(map<string, char>::value_type("1.1.1.1:80", STATUS_UP));
    servs.insert(map<string, char>::value_type("1.1.1.2:80", STATUS_UP));
    servs.insert(map<string, char>::value_type("1.1.1.3:80", STATUS_DOWN));
    servs.insert(map<string, char>::value_type("1.1.1.4:80", STATUS_UP));
    
    retCode = qconfZk.zk_services_set(path, servs);
    EXPECT_EQ(QCONF_OK, retCode);

    retCode = qconfZk.zk_services_get_with_status(path, out_servs);
    EXPECT_EQ(QCONF_OK, retCode);

    EXPECT_EQ(servs.size(), out_servs.size());
    EXPECT_EQ(out_servs, servs);
    
    retCode = qconfZk.zk_service_clear(path);
    EXPECT_EQ(QCONF_OK, retCode);
    out_servs.clear();

    retCode = qconfZk.zk_services_get_with_status(path, out_servs);
    EXPECT_EQ(QCONF_OK, retCode);

    EXPECT_EQ(0, out_servs.size());
}
