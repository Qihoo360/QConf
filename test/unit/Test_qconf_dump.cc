#include <zookeeper.h>
#include <sys/fcntl.h>

#include <string>

#include "gtest/gtest.h"
#include "qconf_const.h"
#include "qconf_format.h"
#include "qconf_dump.h"
#include "qconf_shm.h"

using namespace std;

class Test_qconf_dump : public ::testing::Test
{
    public:
        virtual void SetUp()
        {
        }

        virtual void TearDown()
        {
        }
};

static void create_qhashtbl(qhasharr_t *&tbl, size_t max_slots);

/**
 * ========================================================================================================
 * Begin_Test_for function:
 * int qconf_init_dump_file(const string &agent_dir)
 */
// Test for qconf_init_dump_file: illegal agent_dir
TEST_F(Test_qconf_dump, qconf_init_dump_file_illegal_agent_dir)
{
    qconf_destroy_dbf();
    string agent_dir("/illegal_agent_dir");

    int ret = qconf_init_dump_file(agent_dir);
    EXPECT_EQ(QCONF_ERR_MKDIR, ret);
}

// Test for qconf_init_dump_file: dump directory not exist
TEST_F(Test_qconf_dump, qconf_init_dump_file_not_exist_dump_dir)
{
    qconf_destroy_dbf();
    string agent_dir(".");
    string dump_dir("./dumps");
    string dump_file("./dumps/_dump.gdbm");
    unlink(dump_file.c_str());
    unlink(dump_dir.c_str());

    int ret = qconf_init_dump_file(agent_dir);
    EXPECT_EQ(QCONF_OK, ret);
    qconf_destroy_dbf();
}

// Test for qconf_init_dump_file: dump directory exist
TEST_F(Test_qconf_dump, qconf_init_dump_file_exist_dump_dir)
{
    qconf_destroy_dbf();
    string agent_dir(".");
    string dump_dir("./dumps");
    mode_t mode = 0755;
    mkdir(dump_dir.c_str(), mode);

    int ret = qconf_init_dump_file(agent_dir);
    EXPECT_EQ(QCONF_OK, ret);
}
/**
 * End_Test_for function:
 * int qconf_init_dump_file(const string &agent_dir)
 * =========================================================================================================
 */

/**
 * ========================================================================================================
 * Begin_Test_for function:
 * int qconf_dump_get(const string &tblkey, string &tblval)
 */
// Test for qconf_dump_get : empty key
TEST_F(Test_qconf_dump, qconf_dump_get_empty_key)
{
    string key;
    string value;

    int ret = qconf_dump_get(key, value);
    EXPECT_EQ(QCONF_ERR_PARAM, ret);
}

// Test for qconf_dump_get : not exist key
TEST_F(Test_qconf_dump, qconf_dump_get_not_exist_key)
{
    string key("not exist key");
    string value;

    int ret = qconf_dump_get(key, value);
    EXPECT_EQ(QCONF_ERR_NOT_FOUND, ret);
}

// Test for qconf_dump_get : exist key
TEST_F(Test_qconf_dump, qconf_dump_get_exist_key)
{
    string key("test_key");
    string value("test_value");
    string ret_val;

    int ret = qconf_dump_set(key, value);
    EXPECT_EQ(QCONF_OK, ret);

    ret = qconf_dump_get(key, ret_val);
    EXPECT_EQ(QCONF_OK, ret);
    EXPECT_STREQ(value.c_str(), ret_val.c_str());
}

// Test for qconf_dump_get : exist key and value empty
TEST_F(Test_qconf_dump, qconf_dump_get_exist_key_with_empty_value)
{
    string key("test_key_with_empty_value");
    string value;
    string ret_val;

    int ret = qconf_dump_set(key, value);
    EXPECT_EQ(QCONF_OK, ret);

    ret = qconf_dump_get(key, ret_val);
    EXPECT_EQ(QCONF_OK, ret);
    EXPECT_STREQ(value.c_str(), ret_val.c_str());
}

// Test for qconf_dump_get : exist key and len of value is 1k
TEST_F(Test_qconf_dump, qconf_dump_get_exist_key_with_1k_value)
{
    string key("test_key_with_1k_value");
    string value;
    string ret_val(1024, 'a');

    int ret = qconf_dump_set(key, value);
    EXPECT_EQ(QCONF_OK, ret);

    ret = qconf_dump_get(key, ret_val);
    EXPECT_EQ(QCONF_OK, ret);
    EXPECT_STREQ(value.c_str(), ret_val.c_str());
}

// Test for qconf_dump_get : exist key and len of value is 1m
TEST_F(Test_qconf_dump, qconf_dump_get_exist_key_with_1m_value)
{
    string key("test_key_with_1m_value");
    string value;
    string ret_val(1024000, 'a');

    int ret = qconf_dump_set(key, value);
    EXPECT_EQ(QCONF_OK, ret);

    ret = qconf_dump_get(key, ret_val);
    EXPECT_EQ(QCONF_OK, ret);
    EXPECT_STREQ(value.c_str(), ret_val.c_str());
}

/**
 * End_Test_for function:
 * int qconf_dump_set(const string &tblkey, const string &tblval)
 * =========================================================================================================
 */


/**
 * ========================================================================================================
 * Begin_Test_for function:
 * int qconf_dump_set(const string &tblkey, const string &tblval)
 */
// Test for qconf_dump_set : empty key
TEST_F(Test_qconf_dump, qconf_dump_set_empty_key)
{
    string key;
    string value;

    int ret = qconf_dump_set(key, value);
    EXPECT_EQ(QCONF_ERR_PARAM, ret);
}

// Test for qconf_dump_set : duplicate keys
TEST_F(Test_qconf_dump, qconf_dump_set_duplicate_keys)
{
    string key("duplicate_keys");
    string value_one("value first");
    string value_two("value second");
    string ret_val;

    int ret = qconf_dump_set(key, value_one);
    EXPECT_EQ(QCONF_OK, ret);
    ret = qconf_dump_get(key, ret_val);
    EXPECT_EQ(QCONF_OK, ret);
    EXPECT_STREQ(value_one.c_str(), ret_val.c_str());

    ret = qconf_dump_set(key, value_two);
    EXPECT_EQ(QCONF_OK, ret);
    ret = qconf_dump_get(key, ret_val);
    EXPECT_EQ(QCONF_OK, ret);
    EXPECT_STREQ(value_two.c_str(), ret_val.c_str());
}

/**
 * End_Test_for function:
 * int qconf_dump_get(const string &tblkey, string &tblval)
 * =========================================================================================================
 */


/**
 * ========================================================================================================
 * Begin_Test_for function:
 * int qconf_dump_delete(const string &tblkey)
 */
// Test for qconf_dump_delete : empty key
TEST_F(Test_qconf_dump, qconf_dump_delete_empty_key)
{
    string key;

    int ret = qconf_dump_delete(key);
    EXPECT_EQ(QCONF_ERR_PARAM, ret);
}

// Test for qconf_dump_delete : not exist key
TEST_F(Test_qconf_dump, qconf_dump_delete_not_exist_key)
{
    string key("not exist key");

    int ret = qconf_dump_delete(key);
    EXPECT_EQ(QCONF_OK, ret);
}

// Test for qconf_dump_delete : exist key
TEST_F(Test_qconf_dump, qconf_dump_delete_exist_key)
{
    string key("exist_key");
    string value("exist value");
    string ret_val;

    int ret = qconf_dump_set(key, value);
    EXPECT_EQ(QCONF_OK, ret);
    ret = qconf_dump_get(key, ret_val);
    EXPECT_EQ(QCONF_OK, ret);
    EXPECT_STREQ(value.c_str(), ret_val.c_str());

    ret = qconf_dump_delete(key);
    EXPECT_EQ(QCONF_OK, ret);
}

// Test for qconf_dump_delete : duplicate keys
TEST_F(Test_qconf_dump, qconf_dump_delete_duplicate_key)
{
    string key("exist_key");
    string value("exist value");
    string ret_val;

    int ret = qconf_dump_set(key, value);
    EXPECT_EQ(QCONF_OK, ret);
    ret = qconf_dump_get(key, ret_val);
    EXPECT_EQ(QCONF_OK, ret);
    EXPECT_STREQ(value.c_str(), ret_val.c_str());

    ret = qconf_dump_delete(key);
    EXPECT_EQ(QCONF_OK, ret);
    ret = qconf_dump_delete(key);
    EXPECT_EQ(QCONF_OK, ret);
}

/**
 * End_Test_for function:
 * int qconf_dump_delete(const string &tblkey)
 * =========================================================================================================
 */


/**
 * ========================================================================================================
 * Begin_Test_for function:
 * int qconf_dump_clear()
 */
// Test for qconf_dump_clear
TEST_F(Test_qconf_dump, qconf_dump_clear)
{
    int ret = qconf_dump_clear();
    EXPECT_EQ(QCONF_OK, ret);
}
/**
 * End_Test_for function:
 * int qconf_dump_clear()
 * =========================================================================================================
 */


/**
 * ========================================================================================================
 * Begin_Test_for function:
 * int qconf_dump_tbl(qhasharr_t *tbl)
 */
// Test for qconf_dump_tbl : NULL table
TEST_F(Test_qconf_dump, qconf_dump_tbl_null_table)
{
    qhasharr_t *tbl = NULL;
    int ret = qconf_dump_tbl(tbl);
    EXPECT_EQ(QCONF_ERR_PARAM, ret);
}

// Test for qconf_dump_tbl : empty table
TEST_F(Test_qconf_dump, qconf_dump_tbl_empty_table)
{
    qhasharr_t *tbl = NULL;
    size_t max_slots = 100;

    create_qhashtbl(tbl, max_slots);

    int ret = qconf_dump_tbl(tbl);
    EXPECT_EQ(QCONF_OK, ret);

    free(tbl);
}

// Test for qconf_dump_tbl : number of keys in table is 1
TEST_F(Test_qconf_dump, qconf_dump_tbl_1_key)
{
    qhasharr_t *tbl = NULL;
    size_t max_slots = 100;
    string ret_val;
    char data_type = QCONF_DATA_TYPE_NODE;
    string idc("test");
    string path("/demo/mytest/confs/conf1/conf11");
    string tblkey;
    string value("table value 00000");
    string tblval;

    create_qhashtbl(tbl, max_slots);

    serialize_to_tblkey(data_type, idc, path, tblkey);
    nodeval_to_tblval(tblkey, value, tblval);
    int ret = hash_tbl_set(tbl, tblkey, tblval);
    EXPECT_EQ(QCONF_OK, ret);

    ret = qconf_dump_tbl(tbl);
    EXPECT_EQ(QCONF_OK, ret);

    ret = qconf_dump_get(tblkey, ret_val);
    EXPECT_EQ(QCONF_OK, ret);
    EXPECT_STREQ(tblval.c_str(), ret_val.c_str());

    free(tbl);
}

// Test for qconf_dump_tbl : number of keys in table is 1000
TEST_F(Test_qconf_dump, qconf_dump_tbl_1000_key)
{
    qhasharr_t *tbl = NULL;
    size_t max_slots = 10000;
    string ret_val;

    char key[1024] = {0};
    char value[65535] = {0};
    string tblkey;
    string tblval;
    string idc("test");
    char data_type = QCONF_DATA_TYPE_NODE;
    create_qhashtbl(tbl, max_slots);
    for (int i = 0; i < 1000; ++i)
    {
        snprintf(key, sizeof(key), "/qconf/test/node/confs/conf%d", i);
        serialize_to_tblkey(data_type, idc, key, tblkey);
        snprintf(value, sizeof(value), "table value: %d", i);
        nodeval_to_tblval(tblkey, value, tblval);
        int ret = hash_tbl_set(tbl, tblkey, tblval);
        EXPECT_EQ(QCONF_OK, ret);
    }

    int ret = qconf_dump_tbl(tbl);
    EXPECT_EQ(QCONF_OK, ret);

    for (int i = 0; i < 1000; ++i)
    {
        snprintf(key, sizeof(key), "/qconf/test/node/confs/conf%d", i);
        serialize_to_tblkey(data_type, idc, key, tblkey);
        snprintf(value, sizeof(value), "table value: %d", i);
        nodeval_to_tblval(tblkey, value, tblval);
        ret = qconf_dump_get(tblkey, ret_val);
        EXPECT_EQ(QCONF_OK, ret);
        EXPECT_STREQ(tblval.c_str(), ret_val.c_str());
    }

    qconf_destroy_dbf();
    free(tbl);
}

static void create_qhashtbl(qhasharr_t *&tbl, size_t max_slots)
{
    size_t memsize = qhasharr_calculate_memsize(max_slots);

    void *ptr = malloc(memsize * sizeof(char));
    tbl = qhasharr(ptr, memsize);
    EXPECT_EQ(true, NULL != tbl);
}

/**
 * End_Test_for function:
 * int qconf_dump_tbl(qhasharr_t *tbl)
 * =========================================================================================================
 */
