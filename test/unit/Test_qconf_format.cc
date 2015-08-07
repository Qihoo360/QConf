#include <string>
#include <vector>
#include "gtest/gtest.h"
#include "qconf_format.h"

using namespace std;


// Unit test case for qconf_format.cc

// Related test environment set up:
class Test_qconf_format : public ::testing::Test
{
protected:
    virtual void SetUp()
    {
        char tmp[128] = {0};
        memset((void*)&nodes, 0, sizeof(string_vector_t));
        nodes.count = 20;
        nodes.data = (char**)calloc(nodes.count, sizeof(char*));

        for(int i = 0; i < nodes.count; i++)
        {
            snprintf(tmp, 128, "10.15.16.17:%d", i);
            nodes.data[i] = (char*)calloc((strlen(tmp) + 1), sizeof(char) );
            memcpy(nodes.data[i], tmp, strlen(tmp));
            nodes.data[i][strlen(tmp)] = '\0';
            status.push_back((i % 3 == 0)?STATUS_UP:STATUS_OFFLINE);
        }
    }

    virtual void TearDown()
    {
        free_string_vector(nodes, nodes.count);
    }

    string_vector_t nodes;
    vector<char> status;
};

/**
  *===================================================================================================================================
  * Begin_Test_for function: int serialize_to_tblkey(char data_type, const string &idc, const string &path, string &tblkey)
  */

// Test for serialize_to_tblkey: invalid data_type
TEST_F(Test_qconf_format, serialize_to_tblkey_invalid_data_type)
{
    int retCode = 0;
    string idc("test"), path("/qconf/demo"), tblkey;

    retCode = serialize_to_tblkey('0', idc, path, tblkey);

    EXPECT_EQ(QCONF_ERR_DATA_TYPE, retCode);
}

// Test for serialize_to_tblkey: data_type=QCONF_DATA_TYPE_ZK_HOST
TEST_F(Test_qconf_format, serialize_to_tblkey_data_type_zk_host)
{
    int retCode = 0;
    string idc("test"), tblkey;

    retCode = serialize_to_tblkey(QCONF_DATA_TYPE_ZK_HOST, idc, "", tblkey);

    EXPECT_EQ(QCONF_OK, retCode);
    //cout << tblkey << endl;
}

// Test for serialize_to_tblkey: data_type=QCONF_DATA_TYPE_NODE
TEST_F(Test_qconf_format, serialize_to_tblkey_data_type_node)
{
    int retCode = 0;
    string idc("test"), path("/qconf/demo"), tblkey;

    retCode = serialize_to_tblkey(QCONF_DATA_TYPE_NODE, idc, path, tblkey);

    EXPECT_EQ(QCONF_OK, retCode);
    //cout << tblkey << endl;
}

// Test for serialize_to_tblkey: data_type=QCONF_DATA_TYPE_SERVICE
TEST_F(Test_qconf_format, serialize_to_tblkey_data_type_service)
{
    int retCode = 0;
    string idc("test"), path("/qconf/demo"), tblkey;

    retCode = serialize_to_tblkey(QCONF_DATA_TYPE_SERVICE, idc, path, tblkey);

    EXPECT_EQ(QCONF_OK, retCode);
    //cout << tblkey << endl;
}

// Test for serialize_to_tblkey: data_type=QCONF_DATA_TYPE_LOCAL_IDC
TEST_F(Test_qconf_format, serialize_to_tblkey_data_type_local_idc)
{
    int retCode = 0;
    string tblkey;

    retCode = serialize_to_tblkey(QCONF_DATA_TYPE_LOCAL_IDC, "", "", tblkey);

    EXPECT_EQ(QCONF_OK, retCode);
    //cout << tblkey << endl;
}
/**
  * End_Test_for function: int serialize_to_tblkey(char data_type, const string &idc, const string &path, string &tblkey)
  *================================================================================================================================
  */

/**
  *================================================================================================================================
  * Begin_Test_for function: int deserialize_from_tblkey(const string &tblkey, char &data_type, string &idc, string &path)
  */

// Test for deserialize_from_tblkey: invalid data_type
TEST_F(Test_qconf_format, deserialize_from_tblkey_invalid_data_type)
{
    int retCode = 0;
    string idc("test"), path("/qconf/demo"), tblkey, out_idc, out_path;
    char data_type = 0;

    retCode = serialize_to_tblkey(QCONF_DATA_TYPE_NODE, idc, path, tblkey);
    tblkey[0] = '0';

    retCode = deserialize_from_tblkey(tblkey, data_type, out_idc, out_path);

    EXPECT_EQ(QCONF_ERR_DATA_TYPE, retCode);
}

// Test for deserialize_from_tblkey: invalid tblkey format
TEST_F(Test_qconf_format, deserialize_from_tblkey_invalid_tblkey_format)
{
    //int retCode = 0;
    string tblkey("2,4,corp11,/qconf/demo33,2,4.corp11./qconf/demo9.demo-test"), idc, path;
    //char data_type;

    //assert abort
    //retCode = deserialize_from_tblkey(tblkey, data_type, idc, path);
}

// Test for deserialize_from_tblkey: data_type=QCONF_DATA_TYPE_ZK_HOST
TEST_F(Test_qconf_format, deserialize_from_tblkey_data_type_zk_host)
{
    int retCode = 0;
    string idc, path, tblkey;
    char data_type = 0;

    retCode = serialize_to_tblkey(QCONF_DATA_TYPE_ZK_HOST, "test", "", tblkey);
    EXPECT_EQ(QCONF_OK, retCode);

    retCode = deserialize_from_tblkey(tblkey, data_type, idc, path);
    EXPECT_EQ(QCONF_OK, retCode);
    EXPECT_EQ(QCONF_DATA_TYPE_ZK_HOST, data_type);
    EXPECT_STREQ("test", idc.data());
}

// Test for deserialize_from_tblkey: data_type=QCONF_DATA_TYPE_NODE
TEST_F(Test_qconf_format, deseralize_from_tblkey_data_type_node)
{
    int retCode = 0;
    string idc, path, tblkey;
    char data_type = 0;

    retCode = serialize_to_tblkey(QCONF_DATA_TYPE_NODE, "test", "/qconf/demo", tblkey);
    EXPECT_EQ(QCONF_OK, retCode);

    retCode = deserialize_from_tblkey(tblkey, data_type, idc, path);
    EXPECT_EQ(QCONF_OK, retCode);
    EXPECT_EQ(QCONF_DATA_TYPE_NODE, data_type);
    EXPECT_STREQ("test", idc.data());
    EXPECT_STREQ("/qconf/demo", path.data());
}

// Test for deserialize_from_tblkey: data_type=QCONF_DATA_TYPE_SERVICE
TEST_F(Test_qconf_format, deserialize_from_tblkey_data_type_service)
{
    int retCode = 0;
    string idc, path, tblkey;
    char data_type = 0;

    retCode = serialize_to_tblkey(QCONF_DATA_TYPE_SERVICE, "test", "/qconf/demo", tblkey);
    EXPECT_EQ(QCONF_OK, retCode);

    retCode = deserialize_from_tblkey(tblkey, data_type, idc, path);
    EXPECT_EQ(QCONF_OK, retCode);
    EXPECT_EQ(QCONF_DATA_TYPE_SERVICE, data_type);
    EXPECT_STREQ("test", idc.data());
    EXPECT_STREQ("/qconf/demo", path.data());
}

// Test for deserialize_from_tblkey: data_type=QCONF_DATA_TYPE_LOCAL_IDC
TEST_F(Test_qconf_format, deserialize_from_tblkey_data_type_local_idc)
{
    int retCode = 0;
    string idc, path, tblkey;
    char data_type = 0;

    retCode = serialize_to_tblkey(QCONF_DATA_TYPE_LOCAL_IDC, "", "", tblkey);
    EXPECT_EQ(QCONF_OK, retCode);

    retCode = deserialize_from_tblkey(tblkey, data_type, idc, path);
    EXPECT_EQ(QCONF_OK, retCode);
    EXPECT_EQ(QCONF_DATA_TYPE_LOCAL_IDC, data_type);
}

// Test for char get_data_type(const string &value)
TEST_F(Test_qconf_format, get_data_type_common)
{
    int retCode = 0;
    string tblkey;
    char data_type = 0;

    retCode = serialize_to_tblkey(QCONF_DATA_TYPE_LOCAL_IDC, "", "", tblkey);
    EXPECT_EQ(QCONF_OK, retCode);

    data_type = get_data_type(tblkey);
    EXPECT_EQ(QCONF_DATA_TYPE_LOCAL_IDC, data_type);
}

/**
  * End_Test_for function: int deserialize_from_tblkey(const char *tblkey, char *idc, int idc_size, char *path, int path_size, char *data_type)
  *============================================================================================================================================
  */

/**
  *============================================================================================================================================
  * Begin_Test_for convert between tblval and localidc, idcval, nodeval, chdnodeval or batchnodeval
  */

// Test for convert between localidc and tblval
// int localidc_to_tblval(const string &key, const string &local_idc, string &tblval)
// int tblval_to_localidc(const string &tblval, string &idc)
TEST_F(Test_qconf_format, convert_from_localidc_and_tblval)
{
    int retCode = 0;
    string tblkey, local_idc("test"), tblval, idc_out;
    
    retCode = serialize_to_tblkey(QCONF_DATA_TYPE_LOCAL_IDC, "", "", tblkey);
    EXPECT_EQ(QCONF_OK, retCode);
    
    retCode = localidc_to_tblval(tblkey, local_idc, tblval);
    EXPECT_EQ(QCONF_OK, retCode);

    retCode = tblval_to_localidc(tblval, idc_out);
    EXPECT_EQ(QCONF_OK, retCode);
    EXPECT_STREQ("test", idc_out.data());
}

// Test for convert between localidc and tblval
TEST_F(Test_qconf_format, convert_from_localidc_and_tblval_invalid_tblkey)
{
    //int retCode = 0;
    string tblkey, local_idc("test"), tblval, idc_out;
    
    localidc_to_tblval("2,test.kjlk", local_idc, tblval);

    //assert abort
    //tblval_to_localidc(tblval, idc_out);
}

// Test for convert between idcval and tblval
// int idcval_to_tblval(const string &key, const string &host, string &tblval)
// int tblval_to_idcval(const string &tblval, string &host)
TEST_F(Test_qconf_format, convert_from_idcval_and_tblval)
{
    int retCode = 0;
    string tblkey, local_idc("test"), tblval,  host_out;
    
    retCode = serialize_to_tblkey(QCONF_DATA_TYPE_ZK_HOST, "test", "", tblkey);
    EXPECT_EQ(QCONF_OK, retCode);
    
    retCode = idcval_to_tblval(tblkey, "1.1.1.1:80", tblval);
    EXPECT_EQ(QCONF_OK, retCode);

    retCode = tblval_to_idcval(tblval, host_out);
    EXPECT_EQ(QCONF_OK, retCode);
    EXPECT_STREQ("1.1.1.1:80", host_out.data());
}

// Test for convert between idcval and tblval
// int idcval_to_tblval(const string &key, const string &host, string &tblval)
// int tblval_to_idcval(const string &tblval, string &host, string &idc)
TEST_F(Test_qconf_format, convert_from_idcval_and_tblval1)
{
    int retCode = 0;
    string tblkey, local_idc("test"), tblval, idc_out, host_out;
    
    retCode = serialize_to_tblkey(QCONF_DATA_TYPE_ZK_HOST, "test", "", tblkey);
    EXPECT_EQ(QCONF_OK, retCode);
    
    retCode = idcval_to_tblval(tblkey, "1.1.1.1:80", tblval);
    EXPECT_EQ(QCONF_OK, retCode);

    retCode = tblval_to_idcval(tblval, host_out, idc_out);
    EXPECT_EQ(QCONF_OK, retCode);
    EXPECT_STREQ("1.1.1.1:80", host_out.data());
    EXPECT_STREQ("test", idc_out.data());
}

// Test for convert between nodeval and tbleval
// int nodeval_to_tblval(const string &key, const string &nodeval, string &tblval)
// int tblval_to_nodeval(const string &tblval, string &nodeval)
TEST_F(Test_qconf_format, convert_from_nodeval_and_tblval)
{
    int retCode = 0;
    string tblkey, value("demo_value"), tblval, value_out;
    
    retCode = serialize_to_tblkey(QCONF_DATA_TYPE_NODE, "test", "/qconf/demo", tblkey);
    EXPECT_EQ(QCONF_OK, retCode);
    
    retCode = nodeval_to_tblval(tblkey, value, tblval);
    EXPECT_EQ(QCONF_OK, retCode);

    retCode = tblval_to_nodeval(tblval, value_out);
    EXPECT_EQ(QCONF_OK, retCode);
    EXPECT_STREQ("demo_value", value_out.data());
}

// Test for convert between nodeval and tbleval
// int tblval_to_nodeval(const string &tblval, string &nodeval, string &idc, string &path)
TEST_F(Test_qconf_format, convert_from_nodeval_and_tblval1)
{
    int retCode = 0;
    string tblkey, value("demo_value"), tblval, value_out, idc_out, path_out;
    
    retCode = serialize_to_tblkey(QCONF_DATA_TYPE_NODE, "test", "/qconf/demo", tblkey);
    EXPECT_EQ(QCONF_OK, retCode);
    
    retCode = nodeval_to_tblval(tblkey, value, tblval);
    EXPECT_EQ(QCONF_OK, retCode);

    retCode = tblval_to_nodeval(tblval, value_out, idc_out, path_out);
    
    EXPECT_EQ(QCONF_OK, retCode);
    EXPECT_STREQ("/qconf/demo", path_out.data());
    EXPECT_STREQ("test", idc_out.data());
    EXPECT_EQ(QCONF_OK, retCode);
    EXPECT_STREQ("demo_value", value_out.data());
}

// Test for convert between chdnodes and tbleval
// int chdnodeval_to_tblval(const string &key, const string_vector_t &nodes, string &tblval, const vector<char> &valid_flg)
// int tblval_to_chdnodeval(const string &tblval, string_vector_t &nodes)
TEST_F(Test_qconf_format, convert_from_chdnodeval_and_tblval)
{
    int retCode = 0;
    string tblkey, value("demo_value"), tblval, value_out;
    string_vector_t nodes_out;
    memset((void*)&nodes_out, 0, sizeof(string_vector_t));
    
    retCode = serialize_to_tblkey(QCONF_DATA_TYPE_SERVICE, "test", "/qconf/demo", tblkey);
    EXPECT_EQ(QCONF_OK, retCode);
    
    chdnodeval_to_tblval(tblkey, nodes, tblval, status);
    retCode = tblval_to_chdnodeval(tblval, nodes_out);
    EXPECT_EQ(QCONF_OK, retCode);

    int up_pos = 0;
    for (int i = 0; i < nodes.count; ++i)
    {
        if (STATUS_UP == status[i])
        {
            EXPECT_STREQ(nodes.data[i], nodes_out.data[up_pos]);
            ++up_pos;
        }
    }

    EXPECT_EQ(up_pos, nodes_out.count);
        
    free_string_vector(nodes_out, nodes_out.count);
}

// Test for convert between chdnodes and tbleval
// int tblval_to_chdnodeval(const string &tblval, string_vector_t &nodes, string &idc, string &path)
TEST_F(Test_qconf_format, convert_from_chdnodeval_and_tblval1)
{
    int retCode = 0;
    string tblkey, value("demo_value"), tblval, value_out, idc_out, path_out;
    string_vector_t nodes_out;
    memset((void*)&nodes_out, 0, sizeof(string_vector_t));
    
    retCode = serialize_to_tblkey(QCONF_DATA_TYPE_SERVICE, "test", "/qconf/demo", tblkey);
    EXPECT_EQ(QCONF_OK, retCode);
    
    chdnodeval_to_tblval(tblkey, nodes, tblval, status);
    retCode = tblval_to_chdnodeval(tblval, nodes_out, idc_out, path_out);
    EXPECT_EQ(QCONF_OK, retCode);

    int up_pos = 0;
    for (int i = 0; i < nodes.count; ++i)
    {
        if (STATUS_UP == status[i])
        {
            EXPECT_STREQ(nodes.data[i], nodes_out.data[up_pos]);
            ++up_pos;
        }
    }

    EXPECT_EQ(up_pos, nodes_out.count);
    EXPECT_STREQ("test", idc_out.data());
    EXPECT_STREQ("/qconf/demo", path_out.data());
        
    free_string_vector(nodes_out, nodes_out.count);
}

// Test for convert between batchnodeval and tbleval
// int batchnodeval_to_tblval(const string &key, const string_vector_t &nodes, string &tblval)
// int tblval_to_batchnodeval(const string &tblval, string_vector_t &nodes)
TEST_F(Test_qconf_format, convert_from_batchnodeval_and_tblval)
{
    int retCode = 0;
    string tblkey, value("demo_value"), tblval, value_out, idc_out, path_out;
    string_vector_t nodes_out;
    memset((void*)&nodes_out, 0, sizeof(string_vector_t));
    
    retCode = serialize_to_tblkey(QCONF_DATA_TYPE_BATCH_NODE, "test", "/qconf/demo", tblkey);
    EXPECT_EQ(QCONF_OK, retCode);
    
    batchnodeval_to_tblval(tblkey, nodes, tblval);
    retCode = tblval_to_batchnodeval(tblval, nodes_out);
    EXPECT_EQ(QCONF_OK, retCode);

    for (int i = 0, up_pos = 0; i < nodes.count; ++i, ++up_pos)
    {
        EXPECT_STREQ(nodes.data[i], nodes_out.data[up_pos]);
    }

    free_string_vector(nodes_out, nodes_out.count);
}

// Test for convert between batchnodeval and tbleval: erro data_type
// int batchnodeval_to_tblval(const string &key, const string_vector_t &nodes, string &tblval)
// int tblval_to_batchnodeval(const string &tblval, string_vector_t &nodes)
TEST_F(Test_qconf_format, convert_from_batchnodeval_and_tblval_error_data_type)
{
    int retCode = 0;
    string tblkey, value("demo_value"), tblval, value_out, idc_out, path_out;
    string_vector_t nodes_out;
    memset((void*)&nodes_out, 0, sizeof(string_vector_t));
    
    retCode = serialize_to_tblkey(QCONF_DATA_TYPE_SERVICE, "test", "/qconf/demo", tblkey);
    EXPECT_EQ(QCONF_OK, retCode);
    
    batchnodeval_to_tblval(tblkey, nodes, tblval);
    // assert abort
    //retCode = tblval_to_batchnodeval(tblval, nodes_out);
    //EXPECT_EQ(QCONF_OK, retCode);

    //for (int i = 0, up_pos = 0; i < nodes.count; ++i, ++up_pos)
    //{
    //   EXPECT_STREQ(nodes.data[i], nodes_out.data[up_pos]);
    //}

    //free_string_vector(nodes_out, nodes_out.count);
}

// Test for convert between batchnodeval and tbleval
// int batchnodeval_to_tblval(const string &key, const string_vector_t &nodes, string &tblval)
// int tblval_to_batchnodeval(const string &tblval, string_vector_t &nodes, string &idc, string &path)
TEST_F(Test_qconf_format, convert_from_batchnodeval_and_tblval1)
{
    int retCode = 0;
    string tblkey, value("demo_value"), tblval, value_out, idc_out, path_out;
    string_vector_t nodes_out;
    memset((void*)&nodes_out, 0, sizeof(string_vector_t));
    
    retCode = serialize_to_tblkey(QCONF_DATA_TYPE_BATCH_NODE, "test", "/qconf/demo", tblkey);
    EXPECT_EQ(QCONF_OK, retCode);
    
    batchnodeval_to_tblval(tblkey, nodes, tblval);
    retCode = tblval_to_batchnodeval(tblval, nodes_out, idc_out, path_out);
    EXPECT_EQ(QCONF_OK, retCode);

    for (int i = 0, up_pos = 0; i < nodes.count; ++i, ++up_pos)
    {
        EXPECT_STREQ(nodes.data[i], nodes_out.data[up_pos]);
    }

    EXPECT_STREQ("test", idc_out.data());
    EXPECT_STREQ("/qconf/demo", path_out.data());
        
    free_string_vector(nodes_out, nodes_out.count);
}

// Test for convert between batchnodeval and tbleval: error data_type
// int batchnodeval_to_tblval(const string &key, const string_vector_t &nodes, string &tblval)
// int tblval_to_batchnodeval(const string &tblval, string_vector_t &nodes, string &idc, string &path)
TEST_F(Test_qconf_format, convert_from_batchnodeval_and_tblval1_error_data_type)
{
    int retCode = 0;
    string tblkey, value("demo_value"), tblval, value_out, idc_out, path_out;
    string_vector_t nodes_out;
    memset((void*)&nodes_out, 0, sizeof(string_vector_t));
    
    retCode = serialize_to_tblkey(QCONF_DATA_TYPE_SERVICE, "test", "/qconf/demo", tblkey);
    EXPECT_EQ(QCONF_OK, retCode);
    
    batchnodeval_to_tblval(tblkey, nodes, tblval);
    //assert abort
    //retCode = tblval_to_batchnodeval(tblval, nodes_out, idc_out, path_out);
    //EXPECT_EQ(QCONF_OK, retCode);

    // for (int i = 0, up_pos = 0; i < nodes.count; ++i, ++up_pos)
    // {
    //    EXPECT_STREQ(nodes.data[i], nodes_out.data[up_pos]);
    // }

    // EXPECT_STREQ("test", idc_out.data());
    // EXPECT_STREQ("/qconf/demo", path_out.data());

    free_string_vector(nodes_out, nodes_out.count);
}

// Test for convert between idc host and idc_host
// Test for void serialize_to_idc_host(const string &idc, const string &host, string &dest)
TEST_F(Test_qconf_format, serialize_to_idc_host)
{
    string idc("test"), host("1.1.1.1:2181"), idc_host;
    serialize_to_idc_host(idc, host, idc_host);
    deserialize_from_idc_host(idc_host, idc, host);
    EXPECT_STREQ("test", idc.data());
    EXPECT_STREQ("1.1.1.1:2181", host.data());
}
/**
  * End_Test between tblval and other val
  *==================================================================================================================================
  */


