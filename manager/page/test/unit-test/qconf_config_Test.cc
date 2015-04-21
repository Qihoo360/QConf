#include <string>
#include "gtest/gtest.h"
#include "qconf_config.h"

using namespace std;

//unit test case for qconf_zk.c

class qconf_configTestF : public :: testing::Test
{
protected:
    virtual void SetUp()
    {
        int ret = qconf_load_conf();
        EXPECT_EQ(QCONF_OK, ret);
    }

    virtual void TearDown()
    {
        qconf_destroy_conf_map();
    }
};

/**
 * Test for get_idc_conf
 */
//int get_idc_conf(const std::string &idc, std::string &value);
TEST_F(qconf_configTestF, get_idc_conf_exist)
{
    int retCode = 0;
    string val;

    retCode = get_idc_conf("test", val);

    EXPECT_EQ(QCONF_OK, retCode);
    EXPECT_STREQ(val.c_str(), "127.0.0.1:2181");
}

TEST_F(qconf_configTestF, get_idc_conf_not_exist)
{
    int retCode = 0;
    string val;

    retCode = get_idc_conf("test1", val);

    EXPECT_EQ(QCONF_ERR_NOT_FOUND, retCode);
}

/**
 * Test for get_idc_conf
 */
//const std::map<std::string, std::string> get_idc_map();
TEST_F(qconf_configTestF, get_idc_map_common)
{
    int retCode = 0;
    string val;

    const  map<string, string> idcs = get_idc_map();

    EXPECT_EQ(QCONF_OK, retCode);
    EXPECT_EQ(3, idcs.size());
    EXPECT_STREQ((idcs.at("test")).c_str(), "127.0.0.1:2181");
    EXPECT_STREQ((idcs.at("corp")).c_str(), "127.0.0.2:2181");
}
