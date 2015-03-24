#include <sys/shm.h>

#include <string>

#include "gtest/gtest.h"
#include "qconf_shm.h"
#include "qconf_format.h"
#include "qlibc.h"


#define MAX_SLOT_COUNT (800000)

using namespace std;

// unit test case for qconf_shm.cc

// Related test environment set up and tear down

class Test_qconf_shm : public ::testing::Test
{
protected:
    static qhasharr_t *tbl;
    static const key_t shmkey = 0x1010ac02;
    static void SetUpTestCase()
    {
        create_hash_tbl(tbl, shmkey, 0666);
    }
    virtual void SetUp()
    {
    }

    virtual void TearDown()
    {
        qhasharr_clear(tbl);
    }

    static void TearDownTestCase()
    {
        qconf_destroy_qhasharr_lock();
        int shmid = shmget(shmkey, 0, 0666);
        EXPECT_EQ(0, shmctl(shmid, IPC_RMID, NULL));
        tbl = NULL;
    }
};

qhasharr_t *Test_qconf_shm::tbl = NULL;

/**
  *======================================================================================================================
  * Begin_Test_for function: int init_hash_tbl(qhasharr_t *&tbl, key_t shmkey, mode_t mode, int flags)
  */
// Test for init_hash_tbl: share memory not exist
TEST_F(Test_qconf_shm, init_hash_tbl_shm_not_exist)
{
    int retCode = 0;
    key_t shmkey = 0x1010ac03;
    qhasharr_t *tbl = NULL;

    retCode = init_hash_tbl(tbl, shmkey, 0666, 0);
    EXPECT_EQ(QCONF_ERR_SHMGET, retCode);
}

// Test for init_hash_tbl: init success
TEST_F(Test_qconf_shm, init_hash_tbl_shm_success)
{
    int shmid = -1;
    size_t memsize = 0;
    int retCode = 0;
    key_t shmkey = 0x1010ac03;
    qhasharr_t *tbl = NULL;
    
    memsize = qhasharr_calculate_memsize(QCONF_MAX_SLOTS_NUM);
    shmid = shmget(shmkey, memsize, IPC_CREAT | IPC_EXCL | 0666);
    EXPECT_LT(0, shmid);

    retCode = init_hash_tbl(tbl, shmkey, 0666, 0);
    EXPECT_EQ(QCONF_OK, retCode);
    
    EXPECT_EQ(0, shmctl(shmid, IPC_RMID, NULL));
}

// Test for init_hash_tbl: share memory exist but mode larger
TEST_F(Test_qconf_shm, init_hash_tbl_shm_mode_larger)
{
    int shmid = -1;
    size_t memsize = 0;
    int retCode = 0;
    key_t shmkey = 0x1010ac03;
    qhasharr_t *tbl = NULL;
    
    memsize = qhasharr_calculate_memsize(QCONF_MAX_SLOTS_NUM);
    shmid = shmget(shmkey, memsize, IPC_CREAT | IPC_EXCL | 0444);
    EXPECT_LT(0, shmid);

    retCode = init_hash_tbl(tbl, shmkey, 0666, 0);
    EXPECT_EQ(QCONF_ERR_SHMGET, retCode);
    
    EXPECT_EQ(0, shmctl(shmid, IPC_RMID, NULL));
}

// Test for init_hash_tbl: share memory exist but mode smaller
TEST_F(Test_qconf_shm, init_hash_tbl_shm_mode_smaller)
{
    int shmid = -1;
    size_t memsize = 0;
    int retCode = 0;
    key_t shmkey = 0x1010ac03;
    qhasharr_t *tbl = NULL;
    
    memsize = qhasharr_calculate_memsize(QCONF_MAX_SLOTS_NUM);
    shmid = shmget(shmkey, memsize, IPC_CREAT | IPC_EXCL | 0666);
    EXPECT_LT(0, shmid);

    retCode = init_hash_tbl(tbl, shmkey, 0444, 0);
    EXPECT_EQ(QCONF_OK, retCode);
    
    EXPECT_EQ(0, shmctl(shmid, IPC_RMID, NULL));
}

// Test for init_hash_tbl: share memory exist but no authentication
TEST_F(Test_qconf_shm, init_hash_tbl_shm_mode_no_authentication)
{
    int shmid = -1;
    size_t memsize = 0;
    int retCode = 0;
    key_t shmkey = 0x1010ac03;
    qhasharr_t *tbl = NULL;
    
    memsize = qhasharr_calculate_memsize(QCONF_MAX_SLOTS_NUM);
    shmid = shmget(shmkey, memsize, IPC_CREAT | IPC_EXCL | 0444);
    EXPECT_LT(0, shmid);

    retCode = init_hash_tbl(tbl, shmkey, 0444, 0);
    EXPECT_EQ(QCONF_ERR_SHMAT, retCode);
    
    EXPECT_EQ(0, shmctl(shmid, IPC_RMID, NULL));
}

// Test for init_hash_tbl: share memory exist but only read authentication
TEST_F(Test_qconf_shm, init_hash_tbl_shm_mode_only_read_authentication)
{
    int shmid = -1;
    size_t memsize = 0;
    int retCode = 0;
    key_t shmkey = 0x1010ac03;
    qhasharr_t *tbl = NULL;
    
    memsize = qhasharr_calculate_memsize(QCONF_MAX_SLOTS_NUM);
    shmid = shmget(shmkey, memsize, IPC_CREAT | IPC_EXCL | 0444);
    EXPECT_LT(0, shmid);

    retCode = init_hash_tbl(tbl, shmkey, 0444, SHM_RDONLY);
    EXPECT_EQ(QCONF_OK, retCode);
    
    EXPECT_EQ(0, shmctl(shmid, IPC_RMID, NULL));
}

// Test for init_hash_tbl: share memory exist but no read authentication
TEST_F(Test_qconf_shm, init_hash_tbl_shm_mode_no_read_authentication)
{
    int shmid = -1;
    size_t memsize = 0;
    int retCode = 0;
    key_t shmkey = 0x1010ac03;
    qhasharr_t *tbl = NULL;
    
    memsize = qhasharr_calculate_memsize(QCONF_MAX_SLOTS_NUM);
    shmid = shmget(shmkey, memsize, IPC_CREAT | IPC_EXCL | 0222);
    EXPECT_LT(0, shmid);

    retCode = init_hash_tbl(tbl, shmkey, 0222, SHM_RDONLY);
    EXPECT_EQ(QCONF_ERR_SHMAT, retCode);
    
    EXPECT_EQ(0, shmctl(shmid, IPC_RMID, NULL));
}

/**
  * End_Test_for function: int init_hash_tbl(qhasharr_t *&tbl, key_t shmkey, mode_t mode, int flags)
  *==============================================================================================================
  */



/**
  *======================================================================================================================
  * Begin_Test_for function: int create_hash_tbl(qhasharr_t *&tbl, key_t shmkey, mode_t mode)
  */

// Test for create_hash_tbl: init success
TEST_F(Test_qconf_shm, create_hash_tbl_common)
{
    int shmid = -1;
    int retCode = 0;
    key_t shmkey = 0x1010ac03;
    qhasharr_t *tbl = NULL;
    retCode = create_hash_tbl(tbl, shmkey, 0666);
    EXPECT_EQ(QCONF_OK, retCode);
    
    shmid = shmget(shmkey, 0, 0666);
    EXPECT_LT(0, shmid);
    
    EXPECT_EQ(0, shmctl(shmid, IPC_RMID, NULL));
}

// Test for create_hash_tbl: already_exist
TEST_F(Test_qconf_shm, create_hash_tbl_already_exist)
{
    int shmid = -1;
    size_t memsize = 0;
    int retCode = 0;
    key_t shmkey = 0x1010ac03;
    qhasharr_t *tbl = NULL;
    
    memsize = qhasharr_calculate_memsize(QCONF_MAX_SLOTS_NUM);
    shmid = shmget(shmkey, memsize, IPC_CREAT | IPC_EXCL | 0666);
    EXPECT_LT(0, shmid);
    
    retCode = create_hash_tbl(tbl, shmkey, 0666);
    EXPECT_EQ(QCONF_OK, retCode);
    
    EXPECT_EQ(0, shmctl(shmid, IPC_RMID, NULL));
}

// Test for create_hash_tbl: already_exist but mode larger
TEST_F(Test_qconf_shm, create_hash_tbl_already_exist_mode_larger)
{
    int shmid = -1;
    size_t memsize = 0;
    int retCode = 0;
    key_t shmkey = 0x1010ac03;
    qhasharr_t *tbl = NULL;
    
    memsize = qhasharr_calculate_memsize(QCONF_MAX_SLOTS_NUM);
    shmid = shmget(shmkey, memsize, IPC_CREAT | IPC_EXCL | 0444);
    EXPECT_LT(0, shmid);
    
    retCode = create_hash_tbl(tbl, shmkey, 0666);
    EXPECT_EQ(QCONF_ERR_SHMGET, retCode);
    
    EXPECT_EQ(0, shmctl(shmid, IPC_RMID, NULL));
}

// Test for create_hash_tbl: already_exist but mode smaller
TEST_F(Test_qconf_shm, create_hash_tbl_already_exist_mode_smaller)
{
    int shmid = -1;
    size_t memsize = 0;
    int retCode = 0;
    key_t shmkey = 0x1010ac03;
    qhasharr_t *tbl = NULL;
    
    memsize = qhasharr_calculate_memsize(QCONF_MAX_SLOTS_NUM);
    shmid = shmget(shmkey, memsize, IPC_CREAT | IPC_EXCL | 0666);
    EXPECT_LT(0, shmid);
    
    retCode = create_hash_tbl(tbl, shmkey, 0444);
    EXPECT_EQ(QCONF_OK, retCode);
    
    EXPECT_EQ(0, shmctl(shmid, IPC_RMID, NULL));
}

// Test for create_hash_tbl: already_exist but no write authority
TEST_F(Test_qconf_shm, create_hash_tbl_already_exist_no_write_authority)
{
    int shmid = -1;
    size_t memsize = 0;
    int retCode = 0;
    key_t shmkey = 0x1010ac03;
    qhasharr_t *tbl = NULL;
    
    memsize = qhasharr_calculate_memsize(QCONF_MAX_SLOTS_NUM);
    shmid = shmget(shmkey, memsize, IPC_CREAT | IPC_EXCL | 0444);
    EXPECT_LT(0, shmid);
    
    retCode = create_hash_tbl(tbl, shmkey, 0444);
    EXPECT_EQ(QCONF_ERR_SHMAT, retCode);
    
    EXPECT_EQ(0, shmctl(shmid, IPC_RMID, NULL));
}
/**
  * End_Test_for function: int create_hash_tbl(qhasharr_t *&tbl, key_t shmkey, mode_t mode)
  *==============================================================================================================
  */




/**
  *======================================================================================================================
  * Begin_Test_for function: int qconf_get_localidc(qhasharr_t *tbl, string &local_idc)
  */

// Test for get_localidc: tbl NULL
TEST_F(Test_qconf_shm, get_localidc_from_tbl_null)
{
    string localidc;

    int retCode = qconf_get_localidc(NULL, localidc);

    EXPECT_EQ(QCONF_ERR_PARAM, retCode);
}

// Test for get_localidc: tbl->maxslots=0
TEST_F(Test_qconf_shm, get_localidc_from_tbl_zero_maxslots)
{
    int retCode = 0;
    qhasharr_t* tmp_tbl = NULL;
    int memsize = 0;
    char* memory = NULL;
    string localidc;
    memsize = qhasharr_calculate_memsize(0);
    memory = (char*)malloc(sizeof(char) * memsize);
    memset(memory, 0, memsize);
    tmp_tbl = (qhasharr_t*)memory;
    tmp_tbl->maxslots = 0;

    retCode = qconf_get_localidc(tmp_tbl, localidc);

    EXPECT_EQ(QCONF_ERR_NOT_FOUND, retCode);

    free(memory);
    memory = NULL;
}

// Test for get_localidc: key not exists in tbl
TEST_F(Test_qconf_shm, get_localidc_from_tbl_key_not_exists)
{
    int retCode = 0;
    string localidc;

    retCode = qconf_get_localidc(tbl, localidc);

    EXPECT_EQ(QCONF_ERR_NOT_FOUND, retCode);
}

// Test for get_localidc: key exists in val
TEST_F(Test_qconf_shm, get_localidc_from_tbl_key_exists_in_val)
{
    int retCode = 0;
    string tblkey, tblval, localidc;
    serialize_to_tblkey(QCONF_DATA_TYPE_LOCAL_IDC, "", "", tblkey);

    retCode = localidc_to_tblval(tblkey, "Test", tblval);
    EXPECT_EQ(QCONF_OK, retCode);
   
    retCode = hash_tbl_set(tbl, tblkey, tblval);
    EXPECT_EQ(QCONF_OK, retCode);
    
    retCode = qconf_get_localidc(tbl, localidc);
    EXPECT_EQ(QCONF_OK, retCode);
    EXPECT_STREQ("Test", localidc.data());
}

/**
  * End_Test_for function: int qconf_get_localidc(qhasharr_t *tbl, string &local_idc)
  *==============================================================================================================
  */


/**
  *======================================================================================================
  * Begin_Test_for function: int qconf_update_localidc(qhasharr_t *tbl, const string &local_idc)
  */

// Test for qconf_update_localidc: tbl null
TEST_F(Test_qconf_shm, qconf_update_localidc_tbl_null)
{
    int retCode = 0;

    retCode = qconf_update_localidc(NULL, "test");

    EXPECT_EQ(QCONF_ERR_PARAM, retCode);
}

// Test for qconf_update_localidc: tbl->maxslots=0
TEST_F(Test_qconf_shm, qconf_update_localidc_zero_maxslots)
{
    int retCode = 0;
    qhasharr_t* tmp_tbl = NULL;
    int memsize = 0;
    char* memory = NULL;
    memsize = qhasharr_calculate_memsize(0);
    memory = (char*)malloc(sizeof(char) * memsize);
    memset(memory, 0, memsize);
    tmp_tbl = (qhasharr_t*)memory;
    tmp_tbl->maxslots = 0;

    retCode = qconf_update_localidc(tmp_tbl, "test");

    EXPECT_EQ(QCONF_ERR_TBL_SET, retCode);

    free(memory);
    memory = NULL;
}

// Test for qconf_update_localidc: tbl->maxslots > 0
TEST_F(Test_qconf_shm, qconf_update_localidc_no_zero_maxslots)
{
    int retCode = 0;
    qhasharr_t* tmp_tbl = NULL;
    int memsize = 0;
    char* memory = NULL;
    memsize = qhasharr_calculate_memsize(10);
    memory = (char*)malloc(sizeof(char) * memsize);
    memset(memory, 0, memsize);
    tmp_tbl = (qhasharr_t*)memory;
    tmp_tbl->maxslots = 10;

    retCode = qconf_update_localidc(tmp_tbl, "test");

    EXPECT_EQ(QCONF_OK, retCode);

    free(memory);
    memory = NULL;
}
// Test for qconf_update_localidc: already exists and update by the same value
TEST_F(Test_qconf_shm, qconf_update_localidc_update_by_the_same_value)
{
    int retCode = 0;
    retCode = qconf_update_localidc(tbl, "test");
    EXPECT_EQ(QCONF_OK, retCode);

    retCode = qconf_update_localidc(tbl, "test");
    EXPECT_EQ(QCONF_OK, retCode);
}

// Test for qconf_update_localidc: already exists and update by different value
TEST_F(Test_qconf_shm, qconf_update_localidc_update_by_different_value)
{
    int retCode = 0;
    retCode = qconf_update_localidc(tbl, "test");
    EXPECT_EQ(QCONF_OK, retCode);

    retCode = qconf_update_localidc(tbl, "Test");
    EXPECT_EQ(QCONF_OK, retCode);
}

// Test for qconf_update_localidc: empty localidc
TEST_F(Test_qconf_shm, qconf_update_localidc_update_empty)
{
    int retCode = 0;
    string localidc;
    retCode = qconf_update_localidc(tbl, "");
    EXPECT_EQ(QCONF_ERR_PARAM, retCode);
}
/**
  * End_Test_for function: int qconf_update_localidc(qhasharr_t *tbl, const string &local_idc)
  *================================================================================================
  */

/**
  * Begin_Test_for function: int qconf_exist_tblkey(qhasharr_t *tbl, const string &key, bool &status)
  *================================================================================================
  */

// Test for qconf_exist_tblkey: tbl null
TEST_F(Test_qconf_shm, qconf_exist_tblkey_tbl_null)
{
    int retCode = 0;
    string tblkey, tblval;
    bool status = false;
    
    serialize_to_tblkey(QCONF_DATA_TYPE_LOCAL_IDC, "", "", tblkey);

    retCode = localidc_to_tblval(tblkey, "Test", tblval);
    EXPECT_EQ(QCONF_OK, retCode);
   
    retCode = hash_tbl_set(tbl, tblkey, tblval);
    EXPECT_EQ(QCONF_OK, retCode);
    
    retCode = qconf_exist_tblkey(NULL, tblkey, status);
    EXPECT_EQ(QCONF_ERR_PARAM, retCode);
}

// Test for qconf_exist_tblkey: tblkey empty
TEST_F(Test_qconf_shm, qconf_exist_tblkey_tblkey_empty)
{
    int retCode = 0;
    bool status = false;

    retCode = qconf_exist_tblkey(tbl, "", status);

    EXPECT_EQ(QCONF_ERR_PARAM, retCode);
}

// Test for qconf_update_localidc: tbl->maxslots=0
TEST_F(Test_qconf_shm, qconf_exist_tblkey_err_tbl)
{
    int retCode = 0;
    qhasharr_t* tmp_tbl = NULL;
    int memsize = 0;
    char* memory = NULL;
    string tblkey;
    bool status = false;

    memsize = qhasharr_calculate_memsize(0);
    memory = (char*)malloc(sizeof(char) * memsize);
    memset(memory, 0, memsize);
    tmp_tbl = (qhasharr_t*)memory;
    tmp_tbl->maxslots = 0;

    serialize_to_tblkey(QCONF_DATA_TYPE_LOCAL_IDC, "", "", tblkey);

    retCode = qconf_exist_tblkey(tmp_tbl, tblkey, status);

    EXPECT_EQ(QCONF_OK, retCode);
    EXPECT_EQ(false, status);

    free(memory);
    memory = NULL;
}

// Test for qconf_exist_tblkey: not exist
TEST_F(Test_qconf_shm, qconf_exist_tblkey_not_exist)
{
    int retCode = 0;
    string tblkey, tblval;
    bool status = false;
    
    serialize_to_tblkey(QCONF_DATA_TYPE_LOCAL_IDC, "", "", tblkey);

    retCode = qconf_exist_tblkey(tbl, tblkey, status);
    EXPECT_EQ(QCONF_OK, retCode);
    EXPECT_EQ(false, status);
}

// Test for qconf_exist_tblkey: exist
TEST_F(Test_qconf_shm, qconf_exist_tblkey_exist)
{
    int retCode = 0;
    string tblkey, tblval;
    bool status = false;
    
    serialize_to_tblkey(QCONF_DATA_TYPE_LOCAL_IDC, "", "", tblkey);

    retCode = localidc_to_tblval(tblkey, "Test", tblval);
    EXPECT_EQ(QCONF_OK, retCode);
   
    retCode = hash_tbl_set(tbl, tblkey, tblval);
    EXPECT_EQ(QCONF_OK, retCode);
    
    retCode = qconf_exist_tblkey(tbl, tblkey, status);
    EXPECT_EQ(QCONF_OK, retCode);
    EXPECT_EQ(true, status);
}

/**
  * End_Test_for function: int qconf_exist_tblkey(qhasharr_t *tbl, const string &key, bool &status)
  *================================================================================================
  */


/**
  *======================================================================================================================
  * Begin_Test_for function: int hash_tbl_get_count(qhasharr_t *tbl, int &max_slots, int &used_slots)
  */
// Test for hash_get_count: tbl null
TEST_F(Test_qconf_shm, hash_tbl_get_count_tbl_null)
{
    int max_slots = -1, used_slots = -1, retCode = -1;
    
    retCode = hash_tbl_get_count(NULL, max_slots, used_slots);
    EXPECT_EQ(-1, retCode);
}

// Test for hash_get_count: success
TEST_F(Test_qconf_shm, hash_tbl_get_count_tmp_tbl_common)
{
    int max_slots = -1, used_slots = -1, retCode = -1;
    qhasharr_t* tmp_tbl = NULL;
    int memsize = 0;
    char* memory = NULL;
    memsize = qhasharr_calculate_memsize(10);
    memory = (char*)malloc(sizeof(char) * memsize);
    memset(memory, 0, memsize);
    tmp_tbl = (qhasharr_t*)memory;
    tmp_tbl->maxslots = 10;
    tmp_tbl->usedslots = 2;
    
    retCode = hash_tbl_get_count(tmp_tbl, max_slots, used_slots);
    EXPECT_EQ(0, retCode);
    EXPECT_EQ(10, max_slots);
    EXPECT_EQ(2, used_slots);

    free(memory);
    memory = NULL;
}

// Test for hash_get_count: success
TEST_F(Test_qconf_shm, hash_tbl_get_count_tbl_common)
{
    int max_slots = -1, used_slots = -1, retCode = -1;
    
    retCode = hash_tbl_get_count(tbl, max_slots, used_slots);
    
    EXPECT_EQ(0, retCode);
    EXPECT_EQ(tbl->maxslots, max_slots);
    EXPECT_EQ(0, used_slots);
}

// Test for hash_get_count: has item
TEST_F(Test_qconf_shm, hash_tbl_get_count_tbl_has_item)
{
    int max_slots = -1, used_slots = -1, retCode = -1;
    
    retCode = hash_tbl_set(tbl, "hello", "world");
    EXPECT_EQ(QCONF_OK, retCode);
    
    retCode = hash_tbl_get_count(tbl, max_slots, used_slots);
    
    EXPECT_EQ(1, retCode);
    EXPECT_EQ(tbl->maxslots, max_slots);
    EXPECT_EQ(1, used_slots);
}

/**
  * End_Test_for function: int hash_tbl_get_count(qhasharr_t *tbl, int &max_slots, int &used_slots)
  *==============================================================================================================
  */


/**
  * Begin_Test_for function: int hash_tbl_set(qhasharr_t *tbl, const string &key, const string &val)
  *================================================================================================
  */

// Test for hash_tbl_set: tbl->maxslots=0
TEST_F(Test_qconf_shm, hash_tbl_set_zero_maxslots)
{
    int retCode = 0;
    string key("hello"), val("world");
    qhasharr_t* tbl = NULL;
    int memsize = 0;
    char* memory = NULL;

    memsize = qhasharr_calculate_memsize(0);
    memory = (char*)malloc(sizeof(char) * memsize);
    memset(memory, 0, memsize);
    tbl = (qhasharr_t*)memory;
    tbl->maxslots = 0;

    retCode = hash_tbl_set(tbl, key, val);

    EXPECT_EQ(QCONF_ERR_TBL_SET, retCode);
    free(tbl);
    tbl = NULL;
}

// Test for hash_tbl_set: tbl null
TEST_F(Test_qconf_shm, hash_tbl_set_tbl_null)
{
    int retCode = 0;
    string key("hello"), val("world");
    
    retCode = hash_tbl_set(NULL, key, val);

    EXPECT_EQ(QCONF_ERR_PARAM, retCode);
}

// Test for hash_tbl_set: key empty
TEST_F(Test_qconf_shm, hash_tbl_set_key_empty)
{
    int retCode = 0;
    string key, val("world");
    
    retCode = hash_tbl_set(tbl, key, val);

    EXPECT_EQ(QCONF_ERR_PARAM, retCode);
}

// Test for hash_tbl_set: key not exists in tbl
TEST_F(Test_qconf_shm, hash_tbl_set_key_not_exists_in_tbl)
{
    int retCode = 0;
    string key("hello"), val("world"), val_out;

    retCode = hash_tbl_set(tbl, key, val);
    EXPECT_EQ(QCONF_OK, retCode);
    
    retCode = hash_tbl_get(tbl, key, val_out);
    EXPECT_EQ(QCONF_OK, retCode);
    EXPECT_EQ(1, tbl->usedslots);

    EXPECT_STREQ(val.data(), val_out.data());
}

// Test for hash_tbl_set: set with same value
TEST_F(Test_qconf_shm, hash_tbl_set_with_same_value)
{
    int retCode = 0;
    string key("hello"), val("world"), val_out;

    retCode = hash_tbl_set(tbl, key, val);
    EXPECT_EQ(QCONF_OK, retCode);
    
    retCode = hash_tbl_set(tbl, key, val);
    EXPECT_EQ(QCONF_ERR_SAME_VALUE, retCode);
    
    retCode = hash_tbl_get(tbl, key, val_out);
    EXPECT_EQ(QCONF_OK, retCode);
    EXPECT_EQ(1, tbl->usedslots);

    EXPECT_STREQ(val.data(), val_out.data());
}

// Test for hash_tbl_set: set with new value
TEST_F(Test_qconf_shm, hash_tbl_set_key_set_with_new_value)
{
    int retCode = 0;
    string key("hello"), val("world"), val_new("World"), val_out;

    retCode = hash_tbl_set(tbl, key, val);
    EXPECT_EQ(QCONF_OK, retCode);
    
    retCode = hash_tbl_set(tbl, key, val_new);
    EXPECT_EQ(QCONF_OK, retCode);
    
    retCode = hash_tbl_get(tbl, key, val_out);
    EXPECT_EQ(QCONF_OK, retCode);
    EXPECT_EQ(1, tbl->usedslots);

    EXPECT_STREQ(val_new.data(), val_out.data());
}

// Test for hash_tbl_set: old value needs 2 slots but new value need 1 slot
TEST_F(Test_qconf_shm, hash_tbl_set_slots_needed_of_new_value_less_than_old_value)
{
    int retCode = 0;
    string key("hello"), val(81, 'a');
    string val_new("111111"), val_out;
    retCode = hash_tbl_set(tbl, key, val);
    EXPECT_EQ(QCONF_OK, retCode);
    EXPECT_EQ(2, tbl->usedslots);
    
    retCode = hash_tbl_set(tbl, key, val_new);
    EXPECT_EQ(QCONF_OK, retCode);
    EXPECT_EQ(1, tbl->usedslots);
    
    retCode = hash_tbl_get(tbl, key, val_out);
    EXPECT_EQ(QCONF_OK, retCode);

    EXPECT_STREQ(val_new.data(), val_out.data());
}

// Test for hash_tbl_set: old value needs 2 slots but new value need 1 slot
TEST_F(Test_qconf_shm, hash_tbl_set_slots_needed_of_new_value_more_than_old_value)
{
    int retCode = 0;
    string key("hello"), val("11111"), val_out;
    string val_new(81, 'a');
    retCode = hash_tbl_set(tbl, key, val);
    EXPECT_EQ(QCONF_OK, retCode);
    EXPECT_EQ(1, tbl->usedslots);
    
    retCode = hash_tbl_set(tbl, key, val_new);
    EXPECT_EQ(QCONF_OK, retCode);
    EXPECT_EQ(2, tbl->usedslots);
    
    retCode = hash_tbl_get(tbl, key, val_out);
    EXPECT_EQ(QCONF_OK, retCode);

    EXPECT_STREQ(val_new.data(), val_out.data());
}

// Test for hash_tbl_set: key not exists and usedslots = maxslots
TEST_F(Test_qconf_shm, hash_tbl_set_usedslots_equals_with_maxslots)
{
    int retCode = 0;
    string keys[] = {"abc", "def", "ghi", "jkl", "mno"};
    string vals[]= {"123", "456", "789", "123", "456"};
    
    string key("hello"), val(80, 'a');
    qhasharr_t* tmp_tbl = NULL;
    int memsize = 0;
    char* memory = NULL;

    memsize = qhasharr_calculate_memsize(5);
    memory = (char*)malloc(sizeof(char) * memsize);
    tmp_tbl = qhasharr(memory, memsize);

    for(int i = 0; i < 5; i++)
    {
        retCode = hash_tbl_set(tmp_tbl, keys[i], vals[i]);
        EXPECT_EQ(QCONF_OK, retCode);
    }
    EXPECT_EQ(5, tmp_tbl->usedslots);
    EXPECT_EQ(5, tmp_tbl->maxslots);

    retCode = hash_tbl_set(tmp_tbl, key, val);

    EXPECT_EQ(QCONF_ERR_TBL_SET, retCode);
    free(tmp_tbl);
    tmp_tbl = NULL;
}

// Test for hash_tbl_set: key not exists and slots remain is not enough for new item
TEST_F(Test_qconf_shm, hash_tbl_set_remain_slots_not_enough)
{
    int retCode = 0;
    string keys[] = {"abc", "def", "ghi", "jkl"};
    string vals[]= {"123", "456", "789", "123"};
    
    string key("hello"), val(81, 'a');
    qhasharr_t* tmp_tbl = NULL;
    int memsize = 0;
    char* memory = NULL;

    memsize = qhasharr_calculate_memsize(5);
    memory = (char*)malloc(sizeof(char) * memsize);
    tmp_tbl = qhasharr(memory, memsize);

    for(int i = 0; i < 4; i++)
    {
        retCode = hash_tbl_set(tmp_tbl, keys[i], vals[i]);
        EXPECT_EQ(QCONF_OK, retCode);
    }
    EXPECT_EQ(4, tmp_tbl->usedslots);
    EXPECT_EQ(5, tmp_tbl->maxslots);

    retCode = hash_tbl_set(tmp_tbl, key, val);

    EXPECT_EQ(QCONF_ERR_TBL_SET, retCode);
    free(tmp_tbl);
    tmp_tbl = NULL;
}

// Test for hash_tbl_set: key exists and slots remain is not enough for new item
TEST_F(Test_qconf_shm, hash_tbl_set_replace_remain_slots_not_enough)
{
    int retCode = 0;
    string keys[] = {"abc", "def", "ghi", "jkl"};
    string vals[]= {"123", "456", "789", "123"};
    
    string key("abc"), val(272, 'a');
    qhasharr_t* tmp_tbl = NULL;
    int memsize = 0;
    char* memory = NULL;

    memsize = qhasharr_calculate_memsize(5);
    memory = (char*)malloc(sizeof(char) * memsize);
    tmp_tbl = qhasharr(memory, memsize);

    for(int i = 0; i < 4; i++)
    {
        retCode = hash_tbl_set(tmp_tbl, keys[i], vals[i]);
        EXPECT_EQ(QCONF_OK, retCode);
    }
    EXPECT_EQ(4, tmp_tbl->usedslots);
    EXPECT_EQ(5, tmp_tbl->maxslots);

    retCode = hash_tbl_set(tmp_tbl, key, val);

    EXPECT_EQ(QCONF_ERR_TBL_SET, retCode);
    free(tmp_tbl);
    tmp_tbl = NULL;
}

// Test for hash_tbl_set: key exists and slots remain is enough for new item
TEST_F(Test_qconf_shm, hash_tbl_set_replace_remain_slots_enough)
{
    int retCode = 0;
    string keys[] = {"abc", "def", "ghi"};
    string vals[]= {"123", "456", "789"};
    
    string key("abc"), val(272, 'a');
    qhasharr_t* tmp_tbl = NULL;
    int memsize = 0;
    char* memory = NULL;

    memsize = qhasharr_calculate_memsize(5);
    memory = (char*)malloc(sizeof(char) * memsize);
    tmp_tbl = qhasharr(memory, memsize);

    for(int i = 0; i < 3; i++)
    {
        retCode = hash_tbl_set(tmp_tbl, keys[i], vals[i]);
        EXPECT_EQ(QCONF_OK, retCode);
    }
    EXPECT_EQ(3, tmp_tbl->usedslots);
    EXPECT_EQ(5, tmp_tbl->maxslots);

    retCode = hash_tbl_set(tmp_tbl, key, val);

    EXPECT_EQ(QCONF_OK, retCode);
    free(tmp_tbl);
    tmp_tbl = NULL;
}

/**
  * End_Test_for function: int hash_tbl_set(qhasharr_t *tbl, const string &key, const string &val)
  *==============================================================================================================
  */


/**
  * Begin_Test_for function: int hash_tbl_get(qhasharr_t *tbl, const string &key, string &val)
  *================================================================================================
  */

// Test for hash_tbl_get: tbl->maxslots=0
TEST_F(Test_qconf_shm, hash_tbl_get_zero_maxslots)
{
    int retCode = 0;
    string val, key("hello");
    qhasharr_t* tmp_tbl = NULL;
    int memsize = 0;
    char* memory = NULL;

    memsize = qhasharr_calculate_memsize(0);
    memory = (char*)malloc(sizeof(char) * memsize);
    tmp_tbl = (qhasharr_t*)memory;
    tmp_tbl->maxslots = 0;

    retCode = hash_tbl_get(tmp_tbl, key, val);

    EXPECT_EQ(QCONF_ERR_NOT_FOUND, retCode);
    free(tmp_tbl);
    tmp_tbl = NULL;
}

// Test for hash_tbl_get: tbl null
TEST_F(Test_qconf_shm, hash_tbl_get_tbl_null)
{
    int retCode = 0;
    string key("hello"), val;
    
    retCode = hash_tbl_set(NULL, key, val);

    EXPECT_EQ(QCONF_ERR_PARAM, retCode);
}

// Test for hash_tbl_get: key empty
TEST_F(Test_qconf_shm, hash_tbl_get_tbl_empty_key)
{
    int retCode = 0;
    string key, val;
    
    retCode = hash_tbl_get(tbl, key, val);

    EXPECT_EQ(QCONF_ERR_PARAM, retCode);
}

// Test for hash_tbl_get: key not exists
TEST_F(Test_qconf_shm, hash_tbl_get_key_not_exists)
{
    int retCode = 0;
    string key("hello"), val;

    retCode = hash_tbl_get(tbl, key, val);

    EXPECT_EQ(QCONF_ERR_NOT_FOUND, retCode);
}

// Test for hash_tbl_get: key exists
TEST_F(Test_qconf_shm, hash_tbl_get)
{
    int retCode = 0;
    string key("hello"), value, value_out;

    retCode = hash_tbl_set(tbl, key, value);
    EXPECT_EQ(QCONF_OK, retCode);
    
    retCode = hash_tbl_get(tbl, key, value_out);
    EXPECT_EQ(QCONF_OK, retCode);
    EXPECT_STREQ(value.data(), value_out.data());
}

// Test for hash_tbl_get: key exists and key is truncated
TEST_F(Test_qconf_shm, hash_tbl_get_key_truncated)
{
    int retCode = 0;
    string key(33, 'a'), value("data"), value_out;

    EXPECT_EQ(0, tbl->usedslots);
    retCode = hash_tbl_set(tbl, key, value);
    EXPECT_EQ(QCONF_OK, retCode);
    EXPECT_EQ(1, tbl->usedslots);
    
    retCode = hash_tbl_get(tbl, key, value_out);
    EXPECT_EQ(QCONF_OK, retCode);
    EXPECT_STREQ(value.data(), value_out.data());
}

// Test for hash_tbl_get: key exists and key is truncated, get by the truncated key
TEST_F(Test_qconf_shm, hash_tbl_get_exists_and_key_is_truncated_get_by_truncated_key)
{
    int retCode = 0;
    string key(33, 'a'), gkey(32, 'a'), value("data"), value_out;

    EXPECT_EQ(0, tbl->usedslots);
    retCode = hash_tbl_set(tbl, key, value);
    EXPECT_EQ(QCONF_OK, retCode);
    EXPECT_EQ(1, tbl->usedslots);
    
    retCode = hash_tbl_get(tbl, gkey, value_out);
    EXPECT_EQ(QCONF_ERR_NOT_FOUND, retCode);
}

// Test for hash_tbl_get: key exists and value needs 2 slots
TEST_F(Test_qconf_shm, hash_tbl_get_key_exists_and_value_needs_two_slots)
{
    int retCode = 0;
    string key(33, 'a'), value(97, 'a'), value_out;

    EXPECT_EQ(0, tbl->usedslots);
    retCode = hash_tbl_set(tbl, key, value);
    EXPECT_EQ(QCONF_OK, retCode);
    EXPECT_EQ(2, tbl->usedslots);
    
    retCode = hash_tbl_get(tbl, key, value_out);
    EXPECT_EQ(QCONF_OK, retCode);
    EXPECT_STREQ(value.data(), value_out.data());
}

/**
  * End_Test_for function: int hash_tbl_get(qhasharr_t *tbl, const string &key, string &val)
  *==============================================================================================================
  */


/**
  * Begin_Test_for function: int qconf_check_md5(string &val)
  *==============================================================================================================
  */
// Test for qconf_check_md5: success
TEST_F(Test_qconf_shm, qconf_check_md5_common)
{
    int retCode = 0;
    char val_md5[QCONF_MD5_INT_LEN] = {0};
    string val("hello");
    qhashmd5(val.data(), val.size(), val_md5);
    val.append(val_md5, QCONF_MD5_INT_LEN);

    retCode = qconf_check_md5(val);
    EXPECT_EQ(QCONF_OK, retCode);
    EXPECT_STREQ("hello", val.data());
}

// Test for qconf_check_md5: md5 error
TEST_F(Test_qconf_shm, qconf_check_md5_error_md5)
{
    int retCode = 0;
    char val_md5[QCONF_MD5_INT_LEN] = {0};
    string val("hello");
    qhashmd5(val.data(), val.size(), val_md5);
    val_md5[2] = '1';
    val.append(val_md5, QCONF_MD5_INT_LEN);

    retCode = qconf_check_md5(val);
    EXPECT_EQ(QCONF_ERR_TBL_DATA_MESS, retCode);
}

// Test for qconf_check_md5: empty
TEST_F(Test_qconf_shm, qconf_check_md5_val_empty)
{
    int retCode = 0;
    string val;

    retCode = qconf_check_md5(val);
    EXPECT_EQ(QCONF_ERR_TBL_DATA_MESS, retCode);
}

/**
  * End_Test_for function: int qconf_check_md5(string &val)
  *==============================================================================================================
  */


/**
  * Begin_Test_for function: bool hash_tbl_exist(qhasharr_t *tbl, const string &key)
  *==============================================================================================================
  */
// Test for hash_tbl_exist: tbl null
TEST_F(Test_qconf_shm, hash_tbl_exist_tbl_null)
{
    int retCode = 0;
    string tblkey, tblval;
    bool status = false;
    
    serialize_to_tblkey(QCONF_DATA_TYPE_LOCAL_IDC, "", "", tblkey);

    retCode = localidc_to_tblval(tblkey, "Test", tblval);
    EXPECT_EQ(QCONF_OK, retCode);
   
    retCode = hash_tbl_set(tbl, tblkey, tblval);
    EXPECT_EQ(QCONF_OK, retCode);
    
    status = hash_tbl_exist(NULL, tblkey);
    EXPECT_EQ(false, status);
}

// Test for hash_tbl_exist: tblkey empty
TEST_F(Test_qconf_shm, hash_tbl_exist_tblkey_empty)
{
    bool status = false;

    status = hash_tbl_exist(tbl, "");
    EXPECT_EQ(false, status);
}

// Test for hash_tbl_exist: tbl->maxslots=0
TEST_F(Test_qconf_shm, hash_tbl_exist_err_tbl)
{
    qhasharr_t* tmp_tbl = NULL;
    int memsize = 0;
    char* memory = NULL;
    string tblkey;
    bool status = false;

    memsize = qhasharr_calculate_memsize(0);
    memory = (char*)malloc(sizeof(char) * memsize);
    memset(memory, 0, memsize);
    tmp_tbl = (qhasharr_t*)memory;
    tmp_tbl->maxslots = 0;

    serialize_to_tblkey(QCONF_DATA_TYPE_LOCAL_IDC, "", "", tblkey);

    status = hash_tbl_exist(tmp_tbl, tblkey);
    EXPECT_EQ(false, status);

    free(memory);
    memory = NULL;
}

// Test for hash_tbl_exist: not exist
TEST_F(Test_qconf_shm, hash_tbl_exist_not_exist)
{
    string tblkey, tblval;
    bool status = false;
    
    serialize_to_tblkey(QCONF_DATA_TYPE_LOCAL_IDC, "", "", tblkey);

    status = hash_tbl_exist(tbl, tblkey);
    EXPECT_EQ(false, status);
}

// Test for hash_tbl_exist: exist
TEST_F(Test_qconf_shm, hash_tbl_exist_exist)
{
    int retCode = 0;
    string tblkey, tblval;
    bool status = false;
    
    serialize_to_tblkey(QCONF_DATA_TYPE_LOCAL_IDC, "", "", tblkey);

    retCode = localidc_to_tblval(tblkey, "Test", tblval);
    EXPECT_EQ(QCONF_OK, retCode);
   
    retCode = hash_tbl_set(tbl, tblkey, tblval);
    EXPECT_EQ(QCONF_OK, retCode);
    
    status = hash_tbl_exist(tbl, tblkey);
    EXPECT_EQ(true, status);
}

/**
  * End_Test_for function: bool hash_tbl_exist(qhasharr_t *tbl, const string &key)
  *==============================================================================================================
  */


/**
  * Begin_Test_for function: int hash_tbl_remove(qhasharr_t *tbl, const string &key)
  *==============================================================================================================
  */

// Test for hash_tbl_remove: tbl null
TEST_F(Test_qconf_shm, hash_tbl_remove_null)
{
    int retCode = 0;
    string tblkey("hello");
    retCode = hash_tbl_remove(NULL, tblkey);
    EXPECT_EQ(QCONF_ERR_PARAM, retCode);
}

// Test for hash_tbl_exist: tblkey empty
TEST_F(Test_qconf_shm, hash_tbl_remove_tblkey_empty)
{
    int retCode = 0;
    retCode = hash_tbl_remove(tbl, "");
    EXPECT_EQ(QCONF_ERR_PARAM, retCode);
}

// Test for hash_tbl_remove: tbl->maxslots=0
TEST_F(Test_qconf_shm, hash_tbl_remove_maxslots_zero)
{
    int retCode = 0;
    qhasharr_t* tmp_tbl = NULL;
    int memsize = 0;
    char* memory = NULL;
    string tblkey;

    memsize = qhasharr_calculate_memsize(0);
    memory = (char*)malloc(sizeof(char) * memsize);
    memset(memory, 0, memsize);
    tmp_tbl = (qhasharr_t*)memory;
    tmp_tbl->maxslots = 0;

    serialize_to_tblkey(QCONF_DATA_TYPE_LOCAL_IDC, "", "", tblkey);

    retCode = hash_tbl_remove(tmp_tbl, tblkey);
    
    EXPECT_EQ(QCONF_OK, retCode);

    free(memory);
    memory = NULL;
}

// Test for hash_tbl_remove: key not exists
TEST_F(Test_qconf_shm, hash_tbl_remove_key_not_exists)
{
    int retCode = 0;
    string key("hello");

    retCode = hash_tbl_remove(tbl, key);

    EXPECT_EQ(QCONF_OK, retCode);
}

// Test for hash_tbl_remove: key exists
TEST_F(Test_qconf_shm, hash_tbl_remove_key_exists)
{
    int retCode = 0;
    string key("hello"), val("123456");

    qhasharr_put(tbl, key.data(), key.size(), val.data(), val.size());
    retCode = hash_tbl_remove(tbl, key);
    bool exists = qhasharr_exist(tbl, key.data(), key.size());

    EXPECT_EQ(QCONF_OK, retCode);
    EXPECT_FALSE(exists);
    EXPECT_EQ(0, tbl->usedslots);
    EXPECT_EQ(0, tbl->num);
}

// Test for hash_tbl_remove:
TEST_F(Test_qconf_shm, hash_tbl_remove_all_key_exists)
{
    int retCode = 0;
    string keys[] = {"abc", "def", "hijk", "lmn", "opq"};
    string vals[] = {"1234", "111", "aaaa", "bbb", "1"};

    for(int i = 0; i < 5; i++)
    {
        qhasharr_put(tbl, keys[i].data(), keys[i].size(), vals[i].data(), vals[i].size());
    }

    for(int i = 0; i < 5; i++)
    {
        retCode = hash_tbl_remove(tbl, keys[i]);
        EXPECT_EQ(QCONF_OK, retCode);
        EXPECT_FALSE(qhasharr_exist(tbl, keys[i].data(), keys[i].size()));
    }

    EXPECT_EQ(0, tbl->usedslots);
    EXPECT_EQ(0, tbl->num);
}

/**
  * End_Test_for function: int hash_tbl_remove(qhasharr_t *tbl, const string &key)
  *==============================================================================================================
  */


/**
  * Begin_Test_for function: int hash_tbl_clear(qhasharr_t *tbl)
  *==============================================================================================================
  */

// Test for hash_tbl_clear: tbl null
TEST_F(Test_qconf_shm, hash_tbl_clear_null)
{
    int retCode = 0;
    retCode = hash_tbl_clear(NULL);
    EXPECT_EQ(QCONF_ERR_PARAM, retCode);
}

// Test for hash_tbl_clear:
TEST_F(Test_qconf_shm, hash_tbl_clear_all)
{
    int retCode = 0;
    string keys[] = {"abc", "def", "hijk", "lmn", "opq"};
    string vals[] = {"1234", "111", "aaaa", "bbb", "1"};

    for(int i = 0; i < 5; i++)
    {
        qhasharr_put(tbl, keys[i].data(), keys[i].size(), vals[i].data(), vals[i].size());
    }

    retCode = hash_tbl_clear(tbl);
    EXPECT_EQ(QCONF_OK, retCode);
    
    for(int i = 0; i < 5; i++)
    {
        EXPECT_FALSE(qhasharr_exist(tbl, keys[i].data(), keys[i].size()));
    }

    EXPECT_EQ(0, tbl->usedslots);
    EXPECT_EQ(0, tbl->num);
}

/**
  * End_Test_for function: int hash_tbl_clear(qhasharr_t *tbl)
  *==============================================================================================================
  */



/**
  * Begin_Test_for function: int hash_tbl_getnext(qhasharr_t *tbl, string &tblkey, string &tblval, int &idx)
  *==============================================================================================================
  */

// Test for hash_tbl_getnext: tbl_null
TEST_F(Test_qconf_shm, hash_tbl_getnext_tbl_null)
{
    int retCode = 0, idx = -1;
    string tblkey_out, tblval_out;
    retCode = hash_tbl_getnext(NULL, tblkey_out, tblval_out, idx);
    EXPECT_EQ(QCONF_ERR_PARAM, retCode);
}

// Test for hash_tbl_getnext: tbl empty
TEST_F(Test_qconf_shm, hash_tbl_getnext_tbl_empty)
{
    int retCode = 0, idx = 0;
    string tblkey_out, tblval_out;
    EXPECT_EQ(0, tbl->usedslots);
    retCode = hash_tbl_getnext(tbl, tblkey_out, tblval_out, idx);
    EXPECT_EQ(tbl->maxslots+1, idx);
    EXPECT_EQ(QCONF_ERR_TBL_END, retCode);
}

// Test for hash_tbl_getnext: tbl->maxslots=0
TEST_F(Test_qconf_shm, hash_tbl_getnext_zero_maxslots)
{
    int retCode = 0;
    string key, val;
    qhasharr_t* tmp_tbl = NULL;
    int memsize = 0, idx = 0;
    char* memory = NULL;

    memsize = qhasharr_calculate_memsize(0);
    memory = (char*)malloc(sizeof(char) * memsize);
    memset(memory, 0, memsize);
    tmp_tbl = (qhasharr_t*)memory;
    tmp_tbl->maxslots = 0;

    retCode = hash_tbl_getnext(tmp_tbl, key, val, idx);

    EXPECT_EQ(QCONF_ERR_TBL_END, retCode);
    free(tmp_tbl);
    tmp_tbl = NULL;
}

// Test for hash_tbl_getnext: type node
TEST_F(Test_qconf_shm, hash_tbl_getnext_type_node)
{
    int retCode = 0;
    string idcs[] = {"abc", "def", "hijk", "lmn", "opq"};
    string paths[] = {"/demo/1", "/demo/2", "/demo/3", "/demo/4", "/demo/5"};
    string vals[] = {"1234", "111", "aaaa", "bbb", "1"};
    

    for(int i = 0; i < 5; i++)
    {
        string tblkey, tblval;
        serialize_to_tblkey(QCONF_DATA_TYPE_NODE, idcs[i], paths[i], tblkey);

        retCode = nodeval_to_tblval(tblkey, vals[i], tblval);
        EXPECT_EQ(QCONF_OK, retCode);

        retCode = hash_tbl_set(tbl, tblkey, tblval);
        EXPECT_EQ(QCONF_OK, retCode);
    }

    int count = 0;
    int max_slots = 0, used_slots = 0;
    count = hash_tbl_get_count(tbl, max_slots, used_slots);
    string tblkey_out, tblval_out;
    for (int idx = 0; idx < max_slots; ) 
    {
        string idc, path, nodeval;
        char data_type = 0;
        int ret = hash_tbl_getnext(tbl, tblkey_out, tblval_out, idx);
        if (QCONF_ERR_TBL_END == ret)
            break;

        EXPECT_EQ(QCONF_OK, ret);
        deserialize_from_tblkey(tblkey_out, data_type, idc, path);
        tblval_to_nodeval(tblval_out, nodeval);
        int i = 0;
        for (i = 0; i < 5; i++)
            if (idcs[i] == idc) break;
        EXPECT_STREQ(paths[i].data(), path.data());
        EXPECT_STREQ(vals[i].data(), nodeval.data());
    }
}

// Test for hash_tbl_getnext: type mix
TEST_F(Test_qconf_shm, hash_tbl_getnext_type_mix)
{
    int retCode = 0;
    string tblkey, tblval;

    //node
    serialize_to_tblkey(QCONF_DATA_TYPE_NODE, "abc", "/demo/1", tblkey);

    retCode = nodeval_to_tblval(tblkey, "123", tblval);
    EXPECT_EQ(QCONF_OK, retCode);

    retCode = hash_tbl_set(tbl, tblkey, tblval);
    EXPECT_EQ(QCONF_OK, retCode);
    
    tblkey.clear();
    tblval.clear();
    
    //service
    serialize_to_tblkey(QCONF_DATA_TYPE_SERVICE, "def", "/demo/2", tblkey);
    
    string_vector_t nodes;
    vector<char> status;
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
    retCode = chdnodeval_to_tblval(tblkey, nodes, tblval, status);
    EXPECT_EQ(QCONF_OK, retCode);

    retCode = hash_tbl_set(tbl, tblkey, tblval);
    EXPECT_EQ(QCONF_OK, retCode);
    
    tblkey.clear();
    tblval.clear();
    
    //batch node
    serialize_to_tblkey(QCONF_DATA_TYPE_BATCH_NODE, "ghi", "/demo/3", tblkey);
    
    retCode = batchnodeval_to_tblval(tblkey, nodes, tblval);
    EXPECT_EQ(QCONF_OK, retCode);

    retCode = hash_tbl_set(tbl, tblkey, tblval);
    EXPECT_EQ(QCONF_OK, retCode);
        
    free_string_vector(nodes, nodes.count);
    tblkey.clear();
    tblval.clear();
    
    //zk_host
    serialize_to_tblkey(QCONF_DATA_TYPE_ZK_HOST, "jkl", "", tblkey);
    
    retCode = idcval_to_tblval(tblkey, "1.1.1.1:80", tblval);
    EXPECT_EQ(QCONF_OK, retCode);

    retCode = hash_tbl_set(tbl, tblkey, tblval);
    EXPECT_EQ(QCONF_OK, retCode);
        
    tblkey.clear();
    tblval.clear();
    
    //local idc
    serialize_to_tblkey(QCONF_DATA_TYPE_LOCAL_IDC, "", "", tblkey);
    
    retCode = localidc_to_tblval(tblkey, "test", tblval);
    EXPECT_EQ(QCONF_OK, retCode);

    retCode = hash_tbl_set(tbl, tblkey, tblval);
    EXPECT_EQ(QCONF_OK, retCode);
        
    int count = 0;
    int max_slots = 0, used_slots = 0;
    count = hash_tbl_get_count(tbl, max_slots, used_slots);
    string tblkey_out, tblval_out;
    for (int idx = 0; idx < max_slots; ) 
    {
        string idc, path, nodeval, host, localidc;
        int ret = hash_tbl_getnext(tbl, tblkey_out, tblval_out, idx);
        if (QCONF_ERR_TBL_END == ret)
            break;
        EXPECT_EQ(QCONF_OK, ret);

        char data_type = 0;
        deserialize_from_tblkey(tblkey_out, data_type, idc, path);
        switch (data_type)
        {
            case QCONF_DATA_TYPE_NODE:
                EXPECT_STREQ("abc", idc.data());
                EXPECT_STREQ("/demo/1", path.data());
                break;
            case QCONF_DATA_TYPE_SERVICE:
                EXPECT_STREQ("def", idc.data());
                EXPECT_STREQ("/demo/2", path.data());
                break;
            case QCONF_DATA_TYPE_BATCH_NODE:
                EXPECT_STREQ("ghi", idc.data());
                EXPECT_STREQ("/demo/3", path.data());
                break;
            case QCONF_DATA_TYPE_ZK_HOST:
                tblval_to_idcval(tblval_out, host);
                EXPECT_STREQ("1.1.1.1:80", host.data());
                break;
            case QCONF_DATA_TYPE_LOCAL_IDC:
                tblval_to_localidc(tblval_out, localidc);
                EXPECT_STREQ("test", localidc.data());
                break;
            default:
                EXPECT_EQ(1, 2);
        }
    }
}

/**
  * End_Test_for function: int hash_tbl_getnext(qhasharr_t *tbl, string &tblkey, string &tblval, int &idx)
  *==============================================================================================================
  */
