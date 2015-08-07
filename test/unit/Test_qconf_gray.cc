#include <zookeeper.h>
#include <sys/fcntl.h>

#include <string>
#include <vector>
#include <map>

#include "gtest/gtest.h"
#include "qconf_gray.h"
#include "qconf_zk.h"
#include "qconf_format.h"
using namespace std;

#define MAX_NODE_NUM 20

//unit test case for qconf_zk.cc

static void global_watcher(zhandle_t* zh, int type, int state, const char* path, void* context);

//Related test environment set up 1:
class Test_qconf_gray : public :: testing::Test
{
protected:
    virtual void SetUp()
    {
        // init zookeeper handler
        const char* host = "127.0.0.1:2181";
        zh = zookeeper_init(host, global_watcher, 30000, NULL, NULL, 0);
        qzk.zk_init(host);
        
        // generte hostname
        char hostname[256] = {0};
        gethostname(hostname, sizeof(hostname));
        host_name.assign(hostname);
    }

    virtual void TearDown()
    {
        zookeeper_close(zh);
        qzk.zk_close();
    }

    zhandle_t *zh;
    QConfZK qzk;
    string host_name;
};

static void global_watcher(zhandle_t* zh, int type, int state, const char* path, void* context)
{
    //to do something
}

/**
 * ========================================================================================================
 * Begin_Test_for function: int gray_process(zhandle_t *zh, const std::string &idc, std::map<std::string, std::string> &nodes);
 */

TEST_F(Test_qconf_gray, zk_gray_process_error_zk_null)
{
    string idc = "test";
    vector< pair<string, string> > nodes;
    int ret = -1;
    ret = gray_process(NULL, idc, nodes);
    EXPECT_EQ(QCONF_ERR_PARAM, ret);
    
    //ret = qzk.zk_gray_rollback("GRAYID1434692493-519639");
    //EXPECT_EQ(QCONF_OK, ret);
}

TEST_F(Test_qconf_gray, zk_gray_process_error_idc_empty)
{
    string idc;
    vector< pair<string, string> > nodes;
    int ret = -1;
    ret = gray_process(zh, idc, nodes);
    EXPECT_EQ(QCONF_ERR_PARAM, ret);
}

TEST_F(Test_qconf_gray, zk_gray_process_ok_set_rollback)
{
    qzk.zk_node_set("/demo/key1", "old_value1");
    qzk.zk_node_set("/demo/key2", "old_value2");
    qzk.zk_node_set("/demo/key3", "old_value3");
    qzk.zk_node_set("/demo/key4", "old_value4");
    
    int ret = -1;
    string idc="test";
    map<string, string> nodes;
    vector< pair<string, string> > onodes;

    nodes.insert(pair<string, string>("/demo/key1", "value1"));
    nodes.insert(pair<string, string>("/demo/key2", "value2"));
    nodes.insert(pair<string, string>("/demo/key3", "value3"));
    nodes.insert(pair<string, string>("/demo/key4", "value4"));

    vector<string> machines;
    machines.push_back(host_name);
    
    // Begin gray 
    string gray_id("GRAYID1434523437-573294");
    ret = qzk.zk_gray_begin(nodes, machines, gray_id);
    EXPECT_EQ(QCONF_OK, ret);
    
    cout << "Gray ID : " << gray_id << endl; 
    ret = gray_process(zh, idc, onodes);
    EXPECT_EQ(QCONF_OK, ret);
    EXPECT_EQ(nodes.size(), onodes.size());
    string tblkey, tblval;
    for (map<string, string>::iterator it = nodes.begin(); it != nodes.end(); ++it)
    {
        tblkey.clear();
        serialize_to_tblkey(QCONF_DATA_TYPE_NODE, idc, (*it).first, tblkey); 
        
        tblval.clear();
        nodeval_to_tblval(tblkey, (*it).second, tblval);
        EXPECT_NE(std::find(onodes.begin(), onodes.end(), pair<string, string>(tblkey, tblval)), onodes.end());
    }
    
    // gray rollback
    ret = qzk.zk_gray_rollback(gray_id);
    EXPECT_EQ(QCONF_OK, ret);

    string val;
    ret = qzk.zk_node_get("/demo/key1", val);
    EXPECT_EQ(QCONF_OK, ret);
    EXPECT_EQ("old_value1", val);

    ret = qzk.zk_node_get("/demo/key2", val);
    EXPECT_EQ(QCONF_OK, ret);
    EXPECT_EQ("old_value2", val);
    
    ret = qzk.zk_node_get("/demo/key3", val);
    EXPECT_EQ(QCONF_OK, ret);
    EXPECT_EQ("old_value3", val);
    
    ret = qzk.zk_node_get("/demo/key4", val);
    EXPECT_EQ(QCONF_OK, ret);
    EXPECT_EQ("old_value4", val);

}
TEST_F(Test_qconf_gray, zk_gray_process_ok_set_commit)
{
    qzk.zk_node_set("/demo/key1", "old_value1");
    qzk.zk_node_set("/demo/key2", "old_value2");
    qzk.zk_node_set("/demo/key3", "old_value3");
    qzk.zk_node_set("/demo/key4", "old_value4");
    
    int ret = -1;
    string idc="test";
    map<string, string> nodes;
    vector< pair<string, string> > onodes;
    nodes.insert(pair<string, string>("/demo/key1", "value1"));
    nodes.insert(pair<string, string>("/demo/key2", "value2"));
    nodes.insert(pair<string, string>("/demo/key3", "value3"));
    nodes.insert(pair<string, string>("/demo/key4", "value4"));

    vector<string> machines;
    machines.push_back(host_name);
    
    // Begin gray 
    string gray_id("GRAYID1434523437-573294");
    ret = qzk.zk_gray_begin(nodes, machines, gray_id);
    EXPECT_EQ(QCONF_OK, ret);
    
    cout << "Gray ID : " << gray_id << endl; 

    ret = gray_process(zh, idc, onodes);
    EXPECT_EQ(QCONF_OK, ret);
    EXPECT_EQ(nodes.size(), onodes.size());
    string tblkey, tblval;
    for (map<string, string>::iterator it = nodes.begin(); it != nodes.end(); ++it)
    {
        tblkey.clear();
        serialize_to_tblkey(QCONF_DATA_TYPE_NODE, idc, (*it).first, tblkey); 
        
        tblval.clear();
        nodeval_to_tblval(tblkey, (*it).second, tblval);
        EXPECT_NE(std::find(onodes.begin(), onodes.end(), pair<string, string>(tblkey, tblval)), onodes.end());
    }
    
    // gray rollback
    ret = qzk.zk_gray_commit(gray_id);
    EXPECT_EQ(QCONF_OK, ret);

    string val;
    ret = qzk.zk_node_get("/demo/key1", val);
    EXPECT_EQ(QCONF_OK, ret);
    EXPECT_EQ("value1", val);

    ret = qzk.zk_node_get("/demo/key2", val);
    EXPECT_EQ(QCONF_OK, ret);
    EXPECT_EQ("value2", val);
    
    ret = qzk.zk_node_get("/demo/key3", val);
    EXPECT_EQ(QCONF_OK, ret);
    EXPECT_EQ("value3", val);
    
    ret = qzk.zk_node_get("/demo/key4", val);
    EXPECT_EQ(QCONF_OK, ret);
    EXPECT_EQ("value4", val);
}

TEST_F(Test_qconf_gray, zk_gray_process_ok_multiset_rollback)
{
    qzk.zk_node_set("/demo/key1", "old_value1");
    qzk.zk_node_set("/demo/key2", "old_value2");
    qzk.zk_node_set("/demo/key3", "old_value3");
    qzk.zk_node_set("/demo/key4", "old_value4");
    
    int ret = -1;
    string idc="test";
    map<string, string> nodes;
    vector < pair<string, string> >onodes;
    string bval(1024* 1023, '1');
    nodes.insert(pair<string, string>("/demo/key1", bval));
    nodes.insert(pair<string, string>("/demo/key2", bval));
    nodes.insert(pair<string, string>("/demo/key3", bval));
    nodes.insert(pair<string, string>("/demo/key4", bval));

    vector<string> machines;
    machines.push_back(host_name);
    
    // Begin gray 
    string gray_id("GRAYID1434523437-573294");
    ret = qzk.zk_gray_begin(nodes, machines, gray_id);
    EXPECT_EQ(QCONF_OK, ret);
    
    cout << "Gray ID : " << gray_id << endl; 

    ret = gray_process(zh, idc, onodes);
    EXPECT_EQ(QCONF_OK, ret);
    EXPECT_EQ(nodes.size(), onodes.size());
    string tblkey, tblval;
    for (map<string, string>::iterator it = nodes.begin(); it != nodes.end(); ++it)
    {
        tblkey.clear();
        serialize_to_tblkey(QCONF_DATA_TYPE_NODE, idc, (*it).first, tblkey); 
        
        tblval.clear();
        nodeval_to_tblval(tblkey, (*it).second, tblval);
        EXPECT_NE(std::find(onodes.begin(), onodes.end(), pair<string, string>(tblkey, tblval)), onodes.end());
    }
    
    // gray rollback
    ret = qzk.zk_gray_rollback(gray_id);
    EXPECT_EQ(QCONF_OK, ret);

   string val;
   ret = qzk.zk_node_get("/demo/key1", val);
   EXPECT_EQ(QCONF_OK, ret);
   EXPECT_EQ("old_value1", val);

   ret = qzk.zk_node_get("/demo/key2", val);
   EXPECT_EQ(QCONF_OK, ret);
   EXPECT_EQ("old_value2", val);
   
   ret = qzk.zk_node_get("/demo/key3", val);
   EXPECT_EQ(QCONF_OK, ret);
   EXPECT_EQ("old_value3", val);
   
   ret = qzk.zk_node_get("/demo/key4", val);
   EXPECT_EQ(QCONF_OK, ret);
   EXPECT_EQ("old_value4", val);
}

TEST_F(Test_qconf_gray, zk_gray_process_ok_multiset_commit)
{
    qzk.zk_node_set("/demo/key1", "old_value1");
    qzk.zk_node_set("/demo/key2", "old_value2");
    qzk.zk_node_set("/demo/key3", "old_value3");
    qzk.zk_node_set("/demo/key4", "old_value4");
    
    int ret = -1;
    string idc="test";
    map<string, string> nodes;
    vector< pair<string, string> > onodes;
    string bval(1024* 1023, '1');
    nodes.insert(pair<string, string>("/demo/key1", bval));
    nodes.insert(pair<string, string>("/demo/key2", bval));
    nodes.insert(pair<string, string>("/demo/key3", bval));
    nodes.insert(pair<string, string>("/demo/key4", bval));

    vector<string> machines;
    machines.push_back(host_name);
    
    // Begin gray 
    string gray_id("GRAYID1434523437-573294");
    ret = qzk.zk_gray_begin(nodes, machines, gray_id);
    EXPECT_EQ(QCONF_OK, ret);
    
    cout << "Gray ID : " << gray_id << endl; 

    ret = gray_process(zh, idc, onodes);
    EXPECT_EQ(QCONF_OK, ret);
    EXPECT_EQ(nodes.size(), onodes.size());
    string tblkey, tblval;
    for (map<string, string>::iterator it = nodes.begin(); it != nodes.end(); ++it)
    {
        tblkey.clear();
        serialize_to_tblkey(QCONF_DATA_TYPE_NODE, idc, (*it).first, tblkey); 
        
        tblval.clear();
        nodeval_to_tblval(tblkey, (*it).second, tblval);
        EXPECT_NE(std::find(onodes.begin(), onodes.end(), pair<string, string>(tblkey, tblval)), onodes.end());
    }
    
    // gray rollback
    ret = qzk.zk_gray_commit(gray_id);
    EXPECT_EQ(QCONF_OK, ret);

   string val;
   ret = qzk.zk_node_get("/demo/key1", val);
   EXPECT_EQ(QCONF_OK, ret);
   EXPECT_EQ(bval, val);

   ret = qzk.zk_node_get("/demo/key2", val);
   EXPECT_EQ(QCONF_OK, ret);
   EXPECT_EQ(bval, val);

   ret = qzk.zk_node_get("/demo/key3", val);
   EXPECT_EQ(QCONF_OK, ret);
   EXPECT_EQ(bval, val);

   ret = qzk.zk_node_get("/demo/key4", val);
   EXPECT_EQ(QCONF_OK, ret);
   EXPECT_EQ(bval, val);
}

TEST_F(Test_qconf_gray, zk_gray_process_too_large)
{
    qzk.zk_node_set("/demo/key1", "old_value1");
    qzk.zk_node_set("/demo/key2", "old_value2");
    qzk.zk_node_set("/demo/key3", "old_value3");
    qzk.zk_node_set("/demo/key4", "old_value4");
    
    int ret = -1;
    string idc="test";
    map<string, string> nodes, onodes;
    string bval(1024 * 1024, '1');
    nodes.insert(pair<string, string>("/demo/key1", bval));
    nodes.insert(pair<string, string>("/demo/key2", bval));
    nodes.insert(pair<string, string>("/demo/key3", bval));
    nodes.insert(pair<string, string>("/demo/key4", bval));

    vector<string> machines;
    machines.push_back(host_name);
    
    // Begin gray 
    string gray_id("GRAYID1434523437-573294");
    ret = qzk.zk_gray_begin(nodes, machines, gray_id);
    EXPECT_EQ(QCONF_ERR_PARAM, ret);
    
}

TEST_F(Test_qconf_gray, zk_gray_process_error_node_not_exist)
{
    qzk.zk_node_set("/demo/key1", "old_value1");
    qzk.zk_node_set("/demo/key2", "old_value2");
    qzk.zk_node_set("/demo/key3", "old_value3");
    qzk.zk_node_set("/demo/key4", "old_value4");
    
    int ret = -1;
    string idc="test";
    map<string, string> nodes, onodes;
    nodes.insert(pair<string, string>("/demo/key1", "value1"));
    nodes.insert(pair<string, string>("/demo/key2", "value2"));
    nodes.insert(pair<string, string>("/demo/key3", "value3"));
    nodes.insert(pair<string, string>("/demo/key4", "value4"));
    nodes.insert(pair<string, string>("/demo/key5", "value4"));

    vector<string> machines;
    machines.push_back(host_name);
    
    // Begin gray 
    string gray_id("GRAYID1434523437-573294");
    ret = qzk.zk_gray_begin(nodes, machines, gray_id);
    EXPECT_EQ(QCONF_ERR_PARAM, ret);
}

TEST_F(Test_qconf_gray, zk_gray_process_error_client_already_in_gray)
{
    qzk.zk_node_set("/demo/key1", "old_value1");
    qzk.zk_node_set("/demo/key2", "old_value2");
    qzk.zk_node_set("/demo/key3", "old_value3");
    qzk.zk_node_set("/demo/key4", "old_value4");
    
    int ret = -1;
    string idc="test";
    map<string, string> nodes;
    vector< pair<string, string> > onodes;
    nodes.insert(pair<string, string>("/demo/key1", "value1"));
    nodes.insert(pair<string, string>("/demo/key2", "value2"));
    nodes.insert(pair<string, string>("/demo/key3", "value3"));
    nodes.insert(pair<string, string>("/demo/key4", "value4"));

    vector<string> machines;
    machines.push_back(host_name);
    
    // Begin gray 
    string gray_id("GRAYID1434523437-573294");
    ret = qzk.zk_gray_begin(nodes, machines, gray_id);
    EXPECT_EQ(QCONF_OK, ret);
    
    cout << "Gray ID : " << gray_id << endl; 

    ret = gray_process(zh, idc, onodes);
    EXPECT_EQ(QCONF_OK, ret);
    EXPECT_EQ(nodes.size(), onodes.size());
    string tblkey, tblval;
    for (map<string, string>::iterator it = nodes.begin(); it != nodes.end(); ++it)
    {
        tblkey.clear();
        serialize_to_tblkey(QCONF_DATA_TYPE_NODE, idc, (*it).first, tblkey); 
        
        tblval.clear();
        nodeval_to_tblval(tblkey, (*it).second, tblval);
        EXPECT_NE(std::find(onodes.begin(), onodes.end(), pair<string, string>(tblkey, tblval)), onodes.end());
    }
    
    map<string, string> sec_nodes;
    sec_nodes.insert(pair<string, string>("/demo", "value1"));
    ret = qzk.zk_gray_begin(sec_nodes, machines, gray_id);
    EXPECT_EQ(QCONF_ERR_PARAM, ret);

    // gray rollback
    ret = qzk.zk_gray_rollback(gray_id);
    EXPECT_EQ(QCONF_OK, ret);

    string val;
    ret = qzk.zk_node_get("/demo/key1", val);
    EXPECT_EQ(QCONF_OK, ret);
    EXPECT_EQ("old_value1", val);

    ret = qzk.zk_node_get("/demo/key2", val);
    EXPECT_EQ(QCONF_OK, ret);
    EXPECT_EQ("old_value2", val);
    
    ret = qzk.zk_node_get("/demo/key3", val);
    EXPECT_EQ(QCONF_OK, ret);
    EXPECT_EQ("old_value3", val);
    
    ret = qzk.zk_node_get("/demo/key4", val);
    EXPECT_EQ(QCONF_OK, ret);
    EXPECT_EQ("old_value4", val);
}

TEST_F(Test_qconf_gray, zk_gray_process_ok_rollback)
{
    int ret = 0;
    ret = qzk.zk_gray_rollback("GRAYID1434627521-581423");
}

