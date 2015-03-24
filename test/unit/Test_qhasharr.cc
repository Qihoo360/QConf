#include <iostream>
#include "gtest/gtest.h"
#include "qlibc.h"
#include "qconf_common.h"
#include <iostream>
#define MAX_SLOT_NUM 20

using namespace std;
// unit test case for qhasharr.c

// Related test environment set up
class Test_qhasharr : public ::testing::Test
{
protected:
    virtual void SetUp()
    {
        int memsize = qhasharr_calculate_memsize(MAX_SLOT_NUM);
        char* memory = (char*)malloc(sizeof(char) * memsize);
        tbl = qhasharr(memory, memsize);
    }

    virtual void TearDown()
    {
        free(tbl);
        tbl = NULL;
    }

    qhasharr_t* tbl;
};

static int _get_idx(qhasharr_t *tbl, const char *key, size_t key_size, unsigned int hash);

static int _get_idx(qhasharr_t *tbl, const char *key, size_t key_size, unsigned int hash)
{
    if (NULL == tbl || NULL == key)
        return -1;
    qhasharr_slot_t *_tbl_slots = NULL;
    qhasharr_init(tbl, &_tbl_slots);

    if (_tbl_slots[hash].count > 0)
    {
        unsigned int idx;
        int count = 0;
        for (count = 0, idx = hash; count < _tbl_slots[hash].count; )
        {
            if (_tbl_slots[idx].hash == hash
                    && (_tbl_slots[idx].count > 0 || _tbl_slots[idx].count == -1))
            {
                // same hash
                count++;

                // first check key length
                if (key_size == _tbl_slots[idx].data.pair.keylen)
                {
                    if (key_size <= _Q_HASHARR_KEYSIZE)
                    {
                        // original key is stored
                        if (!memcmp(key, _tbl_slots[idx].data.pair.key, key_size))
                        {
                            return idx;
                        }
                    }
                    else
                    {
                        // key is truncated, compare MD5 also.
                        unsigned char keymd5[16];
                        qhashmd5(key, key_size, keymd5);
                        if (!memcmp(key, _tbl_slots[idx].data.pair.key,
                                    _Q_HASHARR_KEYSIZE) &&
                                !memcmp(keymd5, _tbl_slots[idx].data.pair.keymd5,
                                        16))
                        {
                            return idx;
                        }
                    }
                }
            }

            // increase idx
            idx++;
            if (idx >= (unsigned int)tbl->maxslots) idx = 0;

            // check loop
            if (idx == hash) break;

            continue;
        }
    }

    return -1;
}

// find empty slot : return empty slow number, otherwise returns -1.
static int _find_empty(qhasharr_t *tbl, int startidx)
{
    if (tbl->maxslots <= 0) return -1;

    qhasharr_slot_t *_tbl_slots = NULL;
    qhasharr_init(tbl, &_tbl_slots);

    if (startidx >= tbl->maxslots) startidx = 0;

    int idx = startidx;
    //qhasharr_init(tbl);
    while (true)
    {
        if (_tbl_slots[idx].count == 0) return idx;

        idx++;
        if (idx >= tbl->maxslots) idx = 0;
        if (idx == startidx) break;
    }

    return -1;
}
/**
  *================================================================================================
  * Begin_Test_for function: bool qhasharr_put(qhasharr_t *tbl, const char *key, size_t key_size, const void *value, size_t val_size)
  */

// Test for qhasharr_put: tbl->maxslots=0
TEST_F(Test_qhasharr, qhasharr_put_zero_maxslots)
{
    bool retCode = true;
    qhasharr_t* tbl = NULL;
    char* memory = NULL;
    const char* key = "hello";
    const void* value = (const void*)"world";

    int memsize = qhasharr_calculate_memsize(0);
    memory = (char*)malloc(sizeof(char) * memsize);
    tbl = (qhasharr_t*)memory;
    tbl->maxslots = 0;

    retCode = qhasharr_put(tbl, key, strlen(key), value, 6);

    EXPECT_FALSE(retCode);
    free(tbl);
    tbl = NULL;
}

// Test for qhasharr_put: tbl already full, tbl->usedslots=tbl->maxslots
TEST_F(Test_qhasharr, qhasharr_put_tbl_already_full)
{
    bool retCode = true;
    qhasharr_t* tbl = NULL;
    char* memory = NULL;
    const char* key = "hello";
    const void* value = (const void*)"world";

    int memsize = qhasharr_calculate_memsize(1);
    memory = (char*)malloc(sizeof(char) * memsize);
    tbl = qhasharr(memory, memsize);

    retCode = qhasharr_put(tbl, key, strlen(key), value, 6);
    retCode &= qhasharr_put(tbl, key, strlen(key), value, 6);

    EXPECT_FALSE(retCode);
    free(tbl);
    tbl = NULL;
}

// Test for qhasharr_put: tbl almost full, tbl->usedslots>=tbl->maxslots*0.8
TEST_F(Test_qhasharr, qhasharr_put_tbl_almost_full)
{
    bool retCode = true;
    qhasharr_t* tbl = NULL;
    char* memory = NULL;
    const char* keys[] = {"abc", "def", "ghi", "jklmn"};
    const char* vals[] = {"123", "456", "789", "0000"};

    int memsize = qhasharr_calculate_memsize(5);
    memory = (char*)malloc(sizeof(char) * memsize);
    tbl = qhasharr(memory, memsize);

    for(int i = 0; i < 4; i++)
    {
        retCode &= qhasharr_put(tbl, keys[i], strlen(keys[i]), (const void*)vals[i], strlen(vals[i]) + 1);
    }

    EXPECT_TRUE(retCode);
    EXPECT_EQ(4, tbl->usedslots);
    EXPECT_EQ(5, tbl->maxslots);

    retCode &= qhasharr_put(tbl, "hello", 6, (const void*)"hi", 3);

    EXPECT_TRUE(retCode);
    EXPECT_EQ(5, tbl->usedslots);
    EXPECT_EQ(5, tbl->maxslots);

    free(tbl);
    tbl = NULL;
}

// Test for qhasharr_put: key not be truncated strlen(key)= _Q_HASHARR_KEYSIZE-1
TEST_F(Test_qhasharr, qhasharr_put_tbl_key_not_be_truncated)
{
    bool retCode = true;
    const char* key = "abcdefghijklmnopqrstuvwxyz12345";
    const void* val = (const void*)"1234";
    qhasharr_slot_t* _tbl_slots = NULL;
    char md5_int[QCONF_MD5_INT_LEN] = {0};

    qhasharr_init(tbl, &_tbl_slots);

    retCode = qhasharr_put(tbl, key, strlen(key), val, 5);
    EXPECT_TRUE(retCode);
    unsigned int hash = qhashmurmur3_32(key, strlen(key) ) % tbl->maxslots;
    int index = _get_idx(tbl, key, strlen(key), hash);
    qhashmd5(key, strlen(key), md5_int);

    EXPECT_TRUE(retCode);
    EXPECT_GE(index, 0);
    EXPECT_EQ(MAX_SLOT_NUM, tbl->maxslots);
    EXPECT_EQ(1, tbl->usedslots);
    EXPECT_EQ(1, tbl->num);
    EXPECT_EQ(0, strncmp(key, _tbl_slots[index].data.pair.key, _Q_HASHARR_KEYSIZE));
    EXPECT_EQ(0, memcmp("1234", _tbl_slots[index].data.pair.value, 5));
    EXPECT_EQ(31, _tbl_slots[index].data.pair.keylen);
    EXPECT_EQ(0, memcmp(md5_int, _tbl_slots[index].data.pair.keymd5, QCONF_MD5_INT_LEN));
    EXPECT_EQ(1, _tbl_slots[index].count);
    EXPECT_EQ(hash, _tbl_slots[index].hash);
    EXPECT_EQ(5, _tbl_slots[index].size);
    EXPECT_EQ(-1, _tbl_slots[index].link);
}

// Test for qhasharr_put: key is truncated: strlen(key)=_Q_HASHARR_KEYSIZE
TEST_F(Test_qhasharr, qhasharr_put_key_is_truncated)
{
    bool retCode = true;
    const char* key = "abcdefghijklmnopqrstuvwxyz123456";
    const void* val = (const void*)"1234";
    qhasharr_slot_t* _tbl_slots = NULL;
    char md5_int[QCONF_MD5_INT_LEN] = {0};

    qhasharr_init(tbl, &_tbl_slots);

    retCode = qhasharr_put(tbl, key, strlen(key), val, 5);
    unsigned int hash = qhashmurmur3_32(key, strlen(key)) % tbl->maxslots;
    int index = _get_idx(tbl, key, strlen(key), hash);
    qhashmd5(key, strlen(key), md5_int);

    EXPECT_TRUE(retCode);
    EXPECT_GE(index, 0);
    EXPECT_EQ(MAX_SLOT_NUM, tbl->maxslots);
    EXPECT_EQ(1, tbl->usedslots);
    EXPECT_EQ(1, tbl->num);
    EXPECT_EQ(0, strncmp(key, _tbl_slots[index].data.pair.key, _Q_HASHARR_KEYSIZE));
    EXPECT_EQ(0, memcmp("1234", _tbl_slots[index].data.pair.value, 5));
    EXPECT_EQ(32, _tbl_slots[index].data.pair.keylen);
    EXPECT_EQ(0, memcmp(md5_int, _tbl_slots[index].data.pair.keymd5, QCONF_MD5_INT_LEN));
    EXPECT_EQ(1, _tbl_slots[index].count);
    EXPECT_EQ(hash, _tbl_slots[index].hash);
    EXPECT_EQ(5, _tbl_slots[index].size);
    EXPECT_EQ(-1, _tbl_slots[index].link);
}

// Test for qhasharr_put: key is truncated: strlen(key)=_Q_HASHARR_KEYSIZE+1
TEST_F(Test_qhasharr, qhasharr_put_key_is_truncated2)
{
    bool retCode = true;
    const char* key = "abcdefghijklmnopqrstuvwxyz1234567";
    const void* val = (const void*)"1234";
    qhasharr_slot_t* _tbl_slots = NULL;
    char md5_int[QCONF_MD5_INT_LEN] = {0};

    qhasharr_init(tbl, &_tbl_slots);

    retCode = qhasharr_put(tbl, key, strlen(key), val, 5);
    unsigned int hash = qhashmurmur3_32(key, strlen(key)) % tbl->maxslots;
    int index = _get_idx(tbl, key, strlen(key), hash);
    qhashmd5(key, strlen(key), md5_int);

    EXPECT_TRUE(retCode);
    EXPECT_GE(index, 0);
    EXPECT_EQ(MAX_SLOT_NUM, tbl->maxslots);
    EXPECT_EQ(1, tbl->usedslots);
    EXPECT_EQ(1, tbl->num);
    EXPECT_EQ(0, strncmp(key, _tbl_slots[index].data.pair.key, _Q_HASHARR_KEYSIZE));
    EXPECT_EQ(0, memcmp("1234", _tbl_slots[index].data.pair.value, 5));
    EXPECT_EQ(33, _tbl_slots[index].data.pair.keylen);
    EXPECT_EQ(0, memcmp(md5_int, _tbl_slots[index].data.pair.keymd5, QCONF_MD5_INT_LEN));
    EXPECT_EQ(1, _tbl_slots[index].count);
    EXPECT_EQ(hash, _tbl_slots[index].hash);
    EXPECT_EQ(5, _tbl_slots[index].size);
    EXPECT_EQ(-1, _tbl_slots[index].link);
}

// Test for qhasharr_put: value size < _Q_HASHARR_VALUESIZE: size of value = _Q_HASHARR_VALUESIZE-1
TEST_F(Test_qhasharr, qhasharr_put_size_of_value_less_than_max_size)
{
    bool retCode = true;
    const char* key = "value size is small enough";
    const void* val = (const void*)"abcdefghijklmnopqrstuvwxyzabcdefghijklmnopqrstuvwxyzabcdefghijklmnopqrstuvwxyz1234567890123456";
    qhasharr_slot_t* _tbl_slots = NULL;
    char md5_int[QCONF_MD5_INT_LEN] = {0};

    qhasharr_init(tbl, &_tbl_slots);

    retCode = qhasharr_put(tbl, key, strlen(key), val, 95);
    unsigned int hash = qhashmurmur3_32(key, strlen(key)) % tbl->maxslots;
    int index = _get_idx(tbl, key, strlen(key), hash);
    qhashmd5(key, strlen(key), md5_int);

    EXPECT_TRUE(retCode);
    EXPECT_GE(index, 0);
    EXPECT_EQ(MAX_SLOT_NUM, tbl->maxslots);
    EXPECT_EQ(1, tbl->usedslots);
    EXPECT_EQ(1, tbl->num);
    EXPECT_EQ(0, strncmp(key, _tbl_slots[index].data.pair.key, _Q_HASHARR_KEYSIZE));
    EXPECT_EQ(0, strncmp((const char*)val, (const char*)_tbl_slots[index].data.pair.value, _Q_HASHARR_VALUESIZE));
    EXPECT_EQ(26, _tbl_slots[index].data.pair.keylen);
    EXPECT_EQ(0, memcmp(md5_int, _tbl_slots[index].data.pair.keymd5, QCONF_MD5_INT_LEN));
    EXPECT_EQ(1, _tbl_slots[index].count);
    EXPECT_EQ(hash, _tbl_slots[index].hash);
    EXPECT_EQ(95, _tbl_slots[index].size);
    EXPECT_EQ(-1, _tbl_slots[index].link);
}

// Test for qhasharr_put: value size = _Q_HASHARR_VALUESIZE: size of value = _Q_HASHARR_VALUESIZE
TEST_F(Test_qhasharr, qhasharr_put_size_of_value_equals_with_max_size)
{
    bool retCode = true;
    const char* key = "value size is small enough";
    const void* val = (const void*)"abcdefghijklmnopqrstuvwxyzabcdefghijklmnopqrstuvwxyzabcdefghijklmnopqrstuvwxyz12345678901234567";
    qhasharr_slot_t* _tbl_slots = NULL;
    char md5_int[QCONF_MD5_INT_LEN] = {0};
    qhasharr_init(tbl, &_tbl_slots);

    retCode = qhasharr_put(tbl, key, strlen(key), val, 96);
    unsigned int hash = qhashmurmur3_32(key, strlen(key)) % tbl->maxslots;
    int index = _get_idx(tbl, key, strlen(key), hash);
    qhashmd5(key, strlen(key), md5_int);

    EXPECT_TRUE(retCode);
    EXPECT_GE(index, 0);
    EXPECT_EQ(MAX_SLOT_NUM, tbl->maxslots);
    EXPECT_EQ(1, tbl->usedslots);
    EXPECT_EQ(1, tbl->num);
    EXPECT_EQ(0, strncmp(key, _tbl_slots[index].data.pair.key, _Q_HASHARR_KEYSIZE));
    EXPECT_EQ(0, strncmp((const char*)val, (const char*)_tbl_slots[index].data.pair.value, _Q_HASHARR_VALUESIZE));
    EXPECT_EQ(26, _tbl_slots[index].data.pair.keylen);
    EXPECT_EQ(0, memcmp(md5_int, _tbl_slots[index].data.pair.keymd5, QCONF_MD5_INT_LEN));
    EXPECT_EQ(1, _tbl_slots[index].count);
    EXPECT_EQ(hash, _tbl_slots[index].hash);
    EXPECT_EQ(96, _tbl_slots[index].size);
    EXPECT_EQ(-1, _tbl_slots[index].link);
}

// Test for qhasharr_put: value size > _Q_HASHARR_VALUESIZE: size of value = _Q_HASHARR_VALUESIZE+1
TEST_F(Test_qhasharr, qhasharr_put_size_of_value_more_thane_max_size_and_need_two_slots)
{
    bool retCode = true;
    const char* key = "value size is too big to fit it into a slot";
    const void* val = (const void*)"abcdefghijklmnopqrstuvwxyzabcdefghijklmnopqrstuvwxyzabcdefghijklmnopqrstuvwxyz123456789012345678";
    qhasharr_slot_t* _tbl_slots = NULL;
    char md5_int[QCONF_MD5_INT_LEN] = {0};
    qhasharr_init(tbl, &_tbl_slots);

    retCode = qhasharr_put(tbl, key, strlen(key), val, 97);
    unsigned int hash = qhashmurmur3_32(key, strlen(key)) % tbl->maxslots;
    int index = _get_idx(tbl, key, strlen(key), hash);
    qhashmd5(key, strlen(key), md5_int);
    int nextidx = _tbl_slots[index].link;

    EXPECT_TRUE(retCode);
    EXPECT_GE(index, 0);
    EXPECT_NE(nextidx, -1);
    EXPECT_EQ(MAX_SLOT_NUM, tbl->maxslots);
    EXPECT_EQ(2, tbl->usedslots);
    EXPECT_EQ(1, tbl->num);
    EXPECT_EQ(0, strncmp(key, _tbl_slots[index].data.pair.key, _Q_HASHARR_KEYSIZE));
    EXPECT_EQ(0, memcmp((const char*)val, (const char*)_tbl_slots[index].data.pair.value, _Q_HASHARR_VALUESIZE));
    EXPECT_EQ(43, _tbl_slots[index].data.pair.keylen);
    EXPECT_EQ(0, memcmp(md5_int, _tbl_slots[index].data.pair.keymd5, QCONF_MD5_INT_LEN));
    EXPECT_EQ(1, _tbl_slots[index].count);
    EXPECT_EQ(hash, _tbl_slots[index].hash);
    EXPECT_EQ(96, _tbl_slots[index].size);
    EXPECT_EQ(1, _tbl_slots[nextidx].size);
    EXPECT_EQ(0, strncmp("", (const char*)_tbl_slots[nextidx].data.ext.value, 1));
}

// Test for qhasharr_put: value size > _Q_HASHARR_VALUESIZE: size of value = _Q_HASHARR_VALUESIZE+ sizeof(struct _Q_HASHARR_SLOT_KEYVAL) + 1
TEST_F(Test_qhasharr, qhasharr_put_size_of_value_more_thane_max_size_and_need_three_slots)
{
    bool retCode = true;
    const char* key = "value size is too big to fit it into a slot";
    //qhasharr_slot_t slot;
    //size_t ext_size = sizeof(slot.data.pair);
    //size_t total_size=_Q_HASHARR_VALUESIZE+ext_size+1;
    char val[243] = {0};
    qhasharr_slot_t* _tbl_slots = NULL;
    char md5_int[QCONF_MD5_INT_LEN] = {0};
    qhasharr_init(tbl, &_tbl_slots);
    size_t i = 0;

    for(i = 0; i < 242; i++)
    {
        val[i] = 'a';
    }

    val[i] = '\0';
    retCode = qhasharr_put(tbl, key, strlen(key), val, 243);
    unsigned int hash = qhashmurmur3_32(key, strlen(key)) % tbl->maxslots;
    int index = _get_idx(tbl, key, strlen(key), hash);
    qhashmd5(key, strlen(key), md5_int);

    EXPECT_TRUE(retCode);
    EXPECT_GE(index, 0);
    EXPECT_EQ(MAX_SLOT_NUM, tbl->maxslots);
    EXPECT_EQ(3, tbl->usedslots);
    EXPECT_EQ(1, tbl->num);
    EXPECT_EQ(0, strncmp(key, _tbl_slots[index].data.pair.key, _Q_HASHARR_KEYSIZE));
    EXPECT_EQ(43, _tbl_slots[index].data.pair.keylen);
    EXPECT_EQ(0, memcmp(md5_int, _tbl_slots[index].data.pair.keymd5, QCONF_MD5_INT_LEN));
    EXPECT_EQ(1, _tbl_slots[index].count);
    EXPECT_EQ(hash, _tbl_slots[index].hash);
    EXPECT_EQ(96, _tbl_slots[index].size);
    EXPECT_EQ(0, memcmp((const char*)val, (const char*)_tbl_slots[index].data.pair.value, _Q_HASHARR_VALUESIZE));

    // to verify the second slot
    int nextidx = _tbl_slots[index].link;
    char tmp[147] = {0};

    for(i = 0; i < 146; i++)
    {
        tmp[i] = 'a';
    }

    tmp[i] = '\0';
    EXPECT_NE(nextidx, -1);
    EXPECT_EQ(-2, _tbl_slots[nextidx].count);
    EXPECT_NE(-1, _tbl_slots[nextidx].link);
    EXPECT_EQ(146, _tbl_slots[nextidx].size);
    EXPECT_EQ((unsigned int)index, _tbl_slots[nextidx].hash);
    EXPECT_EQ(0, memcmp((const char*)tmp, (const char*)_tbl_slots[nextidx].data.ext.value, 146));

    //to verify the third slot
    index = nextidx;
    nextidx = _tbl_slots[index].link;
    EXPECT_NE(nextidx, -1);
    EXPECT_EQ(-2, _tbl_slots[nextidx].count);
    EXPECT_EQ(-1, _tbl_slots[nextidx].link);
    EXPECT_EQ(1, _tbl_slots[nextidx].size);
    EXPECT_EQ((unsigned int)index, _tbl_slots[nextidx].hash);
    EXPECT_EQ(0, strncmp("", (const char*)_tbl_slots[nextidx].data.ext.value, 1));
}

// Test for qhasharr_put: put key-value and key has already exists and both oldvalue and newvale is small enough to be fitted into 1 slot
TEST_F(Test_qhasharr, qhasharr_put_key_already_exists)
{
    bool retCode = true;
    qhasharr_slot_t* _tbl_slots = NULL;
    const char* key = "hello";
    const void* data = (const void*)"1234";
    char md5_int[QCONF_MD5_INT_LEN] = {0};

    qhasharr_init(tbl, &_tbl_slots);

    unsigned int hash = qhashmurmur3_32(key, strlen(key)) % tbl->maxslots;
    qhashmd5(key, strlen(key), md5_int);
    retCode &= qhasharr_put(tbl, key, strlen(key), data, 5);
    int index1 = _get_idx(tbl, key, strlen(key), hash);
    retCode &= qhasharr_put(tbl, key, strlen(key), (const void*)"aaaaa", 6);
    int index = _get_idx(tbl, key, strlen(key), hash);

    EXPECT_TRUE(retCode);
    EXPECT_EQ(index1, index);
    EXPECT_EQ(1, tbl->usedslots);
    EXPECT_EQ(1, tbl->num);
    EXPECT_EQ(1, _tbl_slots[index].count);
    EXPECT_EQ(hash, _tbl_slots[index].hash);
    EXPECT_EQ(-1, _tbl_slots[index].link);
    EXPECT_STREQ(key, _tbl_slots[index].data.pair.key);
    EXPECT_EQ(5, _tbl_slots[index].data.pair.keylen);
    EXPECT_EQ(0, memcmp(md5_int, _tbl_slots[index].data.pair.keymd5, QCONF_MD5_INT_LEN));
    EXPECT_EQ(0, strncmp("aaaaa", (const char*)_tbl_slots[index].data.pair.value, _Q_HASHARR_VALUESIZE));
}

/*Test for qhasharr_put: key has already exists,
* oldvalue: need 1 slot
* newvalue: need 2 slots
*/
TEST_F(Test_qhasharr, qhasharr_put_key_already_exists_and_new_value_needs_two_slots)
{
    bool retCode = true;
    qhasharr_slot_t* _tbl_slots = NULL;
    const char* key = "hello";
    const void* data = (const void*)"1234";
    const void* newdata = (const void*)"abcdefghijklmnopqrstuvwxyzabcdefghijklmnopqrstuvwxyzabcdefghijklmnopqrstuvwxyz123456789012345678901";
    char md5_int[QCONF_MD5_INT_LEN] = {0};

    qhasharr_init(tbl, &_tbl_slots);

    unsigned int hash = qhashmurmur3_32(key, strlen(key)) % tbl->maxslots;
    qhashmd5(key, strlen(key), md5_int);
    retCode &= qhasharr_put(tbl, key, strlen(key), data, 5);
    int index1 = _get_idx(tbl, key, strlen(key), hash);
    retCode &= qhasharr_put(tbl, key, strlen(key), newdata, 100);
    int index = _get_idx(tbl, key, strlen(key), hash);
    int nextidx = _tbl_slots[index].link;

    EXPECT_TRUE(retCode);
    EXPECT_EQ(index1, index);
    EXPECT_NE(-1, nextidx);
    EXPECT_EQ(2, tbl->usedslots);
    EXPECT_EQ(1, tbl->num);
    EXPECT_EQ(1, _tbl_slots[index].count);
    EXPECT_EQ(hash, _tbl_slots[index].hash);
    EXPECT_EQ(nextidx, _tbl_slots[index].link);
    EXPECT_EQ(_Q_HASHARR_VALUESIZE, _tbl_slots[index].size);
    EXPECT_STREQ(key, _tbl_slots[index].data.pair.key);
    EXPECT_EQ(5, _tbl_slots[index].data.pair.keylen);
    EXPECT_EQ(0, memcmp(md5_int, _tbl_slots[index].data.pair.keymd5, QCONF_MD5_INT_LEN));
    EXPECT_EQ(0, memcmp(newdata, (const char*)_tbl_slots[index].data.pair.value, _Q_HASHARR_VALUESIZE));

    //to verify the second slot
    EXPECT_EQ(-2, _tbl_slots[nextidx].count);
    EXPECT_EQ((unsigned int)index, _tbl_slots[nextidx].hash);
    EXPECT_EQ(-1, _tbl_slots[nextidx].link);
    EXPECT_EQ(4, _tbl_slots[nextidx].size);
    EXPECT_EQ(0, strncmp("901", (const char*)_tbl_slots[nextidx].data.ext.value, 4));
}

/*Test for qhasharr_put: key has already exists,
* oldvalue: need 1 slot
* newvalue: need 2 slots
*/
TEST_F(Test_qhasharr, qhasharr_put_key_already_exists_and_new_value_needs_one_slots)
{
    bool retCode = true;
    qhasharr_slot_t* _tbl_slots = NULL;
    const char* key = "hello";
    const void* data = (const void*)"abcdefghijklmnopqrstuvwxyzabcdefghijklmnopqrstuvwxyzabcdefghijklmnopqrstuvwxyz123456789012345678901";
    const void* newdata = (const void*)"1234";
    char md5_int[QCONF_MD5_INT_LEN] = {0};

    qhasharr_init(tbl, &_tbl_slots);

    unsigned int hash = qhashmurmur3_32(key, strlen(key)) % tbl->maxslots;
    qhashmd5(key, strlen(key), md5_int);
    retCode &= qhasharr_put(tbl, key, strlen(key), data, 100);
    int index1 = _get_idx(tbl, key, strlen(key), hash);
    retCode &= qhasharr_put(tbl, key, strlen(key), newdata, 5);
    int index = _get_idx(tbl, key, strlen(key), hash);

    EXPECT_TRUE(retCode);
    EXPECT_EQ(index, index1);
    EXPECT_EQ(1, tbl->usedslots);
    EXPECT_EQ(1, tbl->num);
    EXPECT_EQ(1, _tbl_slots[index].count);
    EXPECT_EQ(hash, _tbl_slots[index].hash);
    EXPECT_EQ(5, _tbl_slots[index].size);
    EXPECT_EQ(-1, _tbl_slots[index].link);
    EXPECT_STREQ(key, _tbl_slots[index].data.pair.key);
    EXPECT_EQ(5, _tbl_slots[index].data.pair.keylen);
    EXPECT_EQ(0, memcmp(md5_int, _tbl_slots[index].data.pair.keymd5, QCONF_MD5_INT_LEN));
    EXPECT_EQ(0, memcmp(newdata, (const char*)_tbl_slots[index].data.pair.value, 5));
}

// Test for qhasharr_put: key id truncated because exceeding _Q_HASHARR_KEYSIZE, put a new key which equals with the truncated key
TEST_F(Test_qhasharr, qhasharr_put_key_be_truncated)
{
    bool retCode = true;
    qhasharr_slot_t* _tbl_slots = NULL;
    const char* key = "abcdefghijklmnopqrstuvwxyz012345";
    const char* newkey = "abcdefghijklmnopqrstuvwxyz01234";
    const void* data = (const void*)"hello";
    char md5_int[QCONF_MD5_INT_LEN] = {0};
    char md5_int1[QCONF_MD5_INT_LEN] = {0};
    qhasharr_init(tbl, &_tbl_slots);

    unsigned int hash = qhashmurmur3_32(key, strlen(key)) % tbl->maxslots;
    unsigned int hash1 = qhashmurmur3_32(newkey, strlen(newkey)) % tbl->maxslots;
    qhashmd5(key, strlen(key), md5_int);
    qhashmd5(newkey, strlen(newkey), md5_int1);
    retCode &= qhasharr_put(tbl, key, strlen(key), data, 6);
    int index = _get_idx(tbl, key, strlen(key), hash);
    retCode &= qhasharr_put(tbl, newkey, strlen(newkey), data, 6);
    int index1 = _get_idx(tbl, newkey, strlen(newkey), hash1);

    EXPECT_TRUE(retCode);
    EXPECT_NE(index, index1);
    EXPECT_EQ(2, tbl->usedslots);
    EXPECT_EQ(2, tbl->num);
    EXPECT_NE(index, index1);
    EXPECT_NE(hash, hash1);
    EXPECT_NE(0, memcmp(md5_int, md5_int1, QCONF_MD5_INT_LEN));
    //to verify the slot contains key
    EXPECT_EQ(1, _tbl_slots[index].count);
    EXPECT_EQ(hash, _tbl_slots[index].hash);
    EXPECT_EQ(6, _tbl_slots[index].size);
    EXPECT_EQ(-1, _tbl_slots[index].link);
    EXPECT_EQ(32, _tbl_slots[index].data.pair.keylen);
    EXPECT_EQ(0, memcmp(key, _tbl_slots[index].data.pair.key, _Q_HASHARR_KEYSIZE));
    EXPECT_EQ(0, memcmp(md5_int, _tbl_slots[index].data.pair.keymd5, QCONF_MD5_INT_LEN));
    EXPECT_EQ(0, memcmp(data, (const char*)_tbl_slots[index].data.pair.value, 6));

    //to verify the slot contains newkey
    EXPECT_EQ(1, _tbl_slots[index1].count);
    EXPECT_EQ(hash1, _tbl_slots[index1].hash);
    EXPECT_EQ(6, _tbl_slots[index1].size);
    EXPECT_EQ(-1, _tbl_slots[index1].link);
    EXPECT_EQ(31, _tbl_slots[index1].data.pair.keylen);
    EXPECT_EQ(0, memcmp(newkey, _tbl_slots[index1].data.pair.key, _Q_HASHARR_KEYSIZE));
    EXPECT_EQ(0, memcmp(md5_int1, _tbl_slots[index1].data.pair.keymd5, QCONF_MD5_INT_LEN));
    EXPECT_EQ(0, memcmp(data, (const char*)_tbl_slots[index1].data.pair.value, 6));
}
/**
  * End_Test_for function: bool qhasharr_put(qhasharr_t *tbl, const char *key, size_t key_size, const void *value, size_t val_size)
  *===========================================================================================================
  */

/**
  ============================================================================================================
  * Begin_Test_for function: void *qhasharr_get(qhasharr_t *tbl, const char *key, size_t key_size, size_t *val_size)
  */

// Test for qhasharr_get: tbl->maxslots=0
TEST_F(Test_qhasharr, qhasharr_get_zero_maxslots)
{
    void* val = NULL;
    qhasharr_t* tbl = NULL;
    const char* key = "hello";
    size_t size = 0;
    int memsize = qhasharr_calculate_memsize(0);
    char* memory = (char*)malloc(sizeof(char) * memsize);
    tbl = (qhasharr_t*)memory;
    tbl->maxslots = 0;

    val = qhasharr_get(tbl, key, strlen(key), &size);

    EXPECT_EQ(NULL, val);
    free(tbl);
    tbl = NULL;
}

// Test for qhasharr_get: key not exists in tbl
TEST_F(Test_qhasharr, qhasharr_get_key_not_exists_in_tbl)
{
    void* val = NULL;
    const char* key = "hello";
    size_t size = 0;

    val = qhasharr_get(tbl, key, strlen(key), &size);

    EXPECT_EQ(NULL, val);
}

// Test for qhasharr_get: value size < _Q_HASHARR_VALUESIZE
TEST_F(Test_qhasharr, qhasharr_get_value_size_less_than_max_size)
{
    void* value = NULL;
    const char* key = "hello";
    size_t size = 0;
    const void* data = (const void*)"abcdefghijklmnopqrstuvwxyzabcdefghijklmnopqrstuvwxyzabcdefghijklmnopqrstuvwxuz0123456789012345";
    qhasharr_slot_t* _tbl_slots = NULL;

    qhasharr_init(tbl, &_tbl_slots);
    qhasharr_put(tbl, key, strlen(key), data, 95);
    value = qhasharr_get(tbl, key, strlen(key), &size);

    EXPECT_EQ(95ul, size);
    EXPECT_EQ(0, memcmp(data, value, 95));
    free(value);
    value = NULL;
}

// Test for qhasharr_get: value size = _Q_HASHARR_VALUESIZE
TEST_F(Test_qhasharr, qhasharr_get_value_size_equals_with_max_size)
{
    void* value = NULL;
    const char* key = "hello";
    size_t size = 0;
    const void* data = (const void*)"abcdefghijklmnopqrstuvwxyzabcdefghijklmnopqrstuvwxyzabcdefghijklmnopqrstuvwxuz01234567890123456";
    qhasharr_slot_t* _tbl_slots = NULL;

    qhasharr_init(tbl, &_tbl_slots);
    qhasharr_put(tbl, key, strlen(key), data, 96);
    value = qhasharr_get(tbl, key, strlen(key), &size);

    EXPECT_EQ(96ul, size);
    EXPECT_EQ(0, memcmp(data, value, 96));
    free(value);
    value = NULL;
}

// Test for qhasharr_get: value size > _Q_HASHARR_VALUESIZE and needs 2 slots
TEST_F(Test_qhasharr, qhasharr_get_value_size_more_than_max_size_and_needs_two_slots)
{
    void* value = NULL;
    const char* key = "hello";
    size_t size = 0;
    const void* data = (const void*)"abcdefghijklmnopqrstuvwxyzabcdefghijklmnopqrstuvwxyzabcdefghijklmnopqrstuvwxuz012345678901234567";
    qhasharr_slot_t* _tbl_slots = NULL;

    qhasharr_init(tbl, &_tbl_slots);
    qhasharr_put(tbl, key, strlen(key), data, 97);
    value = qhasharr_get(tbl, key, strlen(key), &size);

    EXPECT_EQ(97ul, size);
    EXPECT_EQ(0, memcmp(data, value, 97));
    free(value);
    value = NULL;
}

// Test for qhasharr_get: value size > _Q_HASHARR_VALUESIZE and needs 3 slots
TEST_F(Test_qhasharr, qhasharr_get_value_size_more_than_max_size_and_needs_three_slots)
{
    void* value = NULL;
    const char* key = "hello";
    size_t size = 0;
    char data[243] = {0};
    qhasharr_slot_t* _tbl_slots = NULL;

    for(int i = 0; i < 242; i++)
    {
        data[i] = 'a';
    }

    data[242] = '\0';
    qhasharr_init(tbl, &_tbl_slots);
    qhasharr_put(tbl, key, strlen(key), data, 243);
    value = qhasharr_get(tbl, key, strlen(key), &size);

    EXPECT_EQ(243ul, size);
    EXPECT_EQ(0, memcmp(data, value, 243));
    free(value);
    value = NULL;
}

// Test for qhasharr_get: key is truncated and get value by the prime key
TEST_F(Test_qhasharr, qhasharr_get_value_key_is_truncated_and_get_value_by_the_prime_key)
{
    void* value = NULL;
    const char* key = "abcdefghijklmnopqrstuvwxyz012341";
    size_t size = 0;
    const void* data = (const void*)"hello";
    qhasharr_slot_t* _tbl_slots = NULL;

    qhasharr_init(tbl, &_tbl_slots);
    qhasharr_put(tbl, key, strlen(key), data, 6);
    value = qhasharr_get(tbl, key, strlen(key), &size);

    EXPECT_EQ(6ul, size);
    EXPECT_EQ(0, memcmp(data, value, 6));
    free(value);
    value = NULL;
}

// Test for qhasharr_get: key is truncated and get value by the truncated key
TEST_F(Test_qhasharr, qhasharr_get_value_key_is_truncated_and_get_value_by_the_truncated_key)
{
    void* value = NULL;
    const char* key = "abcdefghijklmnopqrstuvwxyz012341";
    const char* key_truncated = "abcdefghijklmnopqrstuvwxyz01234";
    size_t size = 0;
    const void* data = (const void*)"hello";
    qhasharr_slot_t* _tbl_slots = NULL;

    qhasharr_init(tbl, &_tbl_slots);
    qhasharr_put(tbl, key, strlen(key), data, 6);
    value = qhasharr_get(tbl, key_truncated, strlen(key_truncated), &size);

    EXPECT_EQ(0ul, size);
    EXPECT_EQ(NULL, value);
}
/**
  * End_Test_for function: void *qhasharr_get(qhasharr_t *tbl, const char *key, size_t key_size, size_t *val_size)
  *=====================================================================================================
  */

/**
  *====================================================================================================
  * Begin_Test_for function: bool qhasharr_remove(qhasharr_t *tbl, const char *key, size_t key_size)
  */

// Test for qhasharr_remove: tbl->maxslots=0
TEST_F(Test_qhasharr, qhasharr_remove_zero_maxslots)
{
    bool retCode = true;
    qhasharr_t* tbl = NULL;
    char* memory = NULL;
    const char* key = "hello";
    int memsize = qhasharr_calculate_memsize(0);
    memory = (char*)malloc(sizeof(char) * memsize);
    tbl = (qhasharr_t*)memory;
    tbl->maxslots = 0;

    retCode = qhasharr_remove(tbl, key, strlen(key));

    EXPECT_FALSE(retCode);
    free(tbl);
    tbl = NULL;
}

// Test for qhasharr_remove: key not exists in tbl
TEST_F(Test_qhasharr, qhasharr_remove_key_not_exists_in_tbl)
{
    bool retCode = true;
    const char* key = "hello";

    retCode = qhasharr_remove(tbl, key, strlen(key));

    EXPECT_FALSE(retCode);
}

// Test for qhasharr_remove: value size < _Q_HASHARR_VALUESIZE
TEST_F(Test_qhasharr, qhasharr_remove_value_needs_one_slot)
{
    bool retCode = true;
    const char* keys[] = {"abc", "def", "ghi", "jklmn", "opq"};
    const char* vals[] = {"123", "456", "789", "111", "12345678901234567890"};
    void* val = NULL;
    size_t size = 0;

    for(int i = 0; i < 5; i++)
    {
        retCode &= qhasharr_put(tbl, keys[i], strlen(keys[i]), (const void*)vals[i], strlen(vals[i]) + 1);
    }

    retCode &= qhasharr_remove(tbl, keys[4], strlen(keys[4]));
    val = qhasharr_get(tbl, keys[4], strlen(keys[4]), &size);
    unsigned int hash = qhashmurmur3_32(keys[4], strlen(keys[4])) % tbl->maxslots;
    int idx = _get_idx(tbl, keys[4], strlen(keys[4]), hash);

    EXPECT_TRUE(retCode);
    EXPECT_EQ(4, tbl->usedslots);
    EXPECT_LT(idx, 0);
    EXPECT_EQ(4, tbl->num);
    EXPECT_EQ(NULL, val);
    EXPECT_EQ(0ul, size);
    free(val);
    val = NULL;
}

// Test for qhasharr_remove: value size > _Q_HASHARR_VALUESIZE
TEST_F(Test_qhasharr, qhasharr_remove_value_needs_two_slot)
{
    bool retCode = true;
    const char* key = "hello";
    const void* data = (const void*)"abcdefghijklmnopqrstuvwxyzabcdefghijklmnopqrstuvwxyzabcdefghijklmnopqrstuvwxyzabcdefghijklmnopqrstuvwxyz";
    void* val = NULL;
    size_t size = 0;

    retCode &= qhasharr_put(tbl, key, strlen(key), data, 105);
    retCode &= qhasharr_remove(tbl, key, strlen(key));
    val = qhasharr_get(tbl, key, strlen(key), &size);
    unsigned int hash = qhashmurmur3_32(key, strlen(key)) % tbl->maxslots;
    int idx = _get_idx(tbl, key, strlen(key), hash);

    EXPECT_TRUE(retCode);
    EXPECT_EQ(0, tbl->usedslots);
    EXPECT_LT(idx, 0);
    EXPECT_EQ(0, tbl->num);
    EXPECT_EQ(NULL, val);
    EXPECT_EQ(0ul, size);
    free(val);
    val = NULL;
}

// Test for qhasharr_remove: key is truncated and remove by the prime key
TEST_F(Test_qhasharr, qhasharr_remove_key_is_truncated_and_remove_by_the_prime_key)
{
    bool retCode = true;
    const char* key = "abcdefghijklmnopqrstuvwxuy012345";
    const void* data = (const void*)"hello";
    void* val = NULL;
    size_t size = 0;

    retCode &= qhasharr_put(tbl, key, strlen(key), data, 6);
    retCode &= qhasharr_remove(tbl, key, strlen(key));
    val = qhasharr_get(tbl, key, strlen(key), &size);
    unsigned int hash = qhashmurmur3_32(key, strlen(key)) % tbl->maxslots;
    int idx = _get_idx(tbl, key, strlen(key), hash);

    EXPECT_TRUE(retCode);
    EXPECT_EQ(0, tbl->usedslots);
    EXPECT_LT(idx, 0);
    EXPECT_EQ(0, tbl->num);
    EXPECT_EQ(NULL, val);
    EXPECT_EQ(0ul, size);
    free(val);
    val = NULL;
}

// Test for qhasharr_remove: key is truncated and remove by the truncated key
TEST_F(Test_qhasharr, qhasharr_remove_key_is_truncated_and_remove_by_the_truncated_key)
{
    bool retCode = true;
    const char* key = "abcdefghijklmnopqrstuvwxyz012345";
    const char* key1 = "abcdefghijklmnopqrstuvwxyz01234";
    const void* data = (const void*)"hello";
    void* val = NULL;
    size_t size = 0;
    qhasharr_slot_t* _tbl_slots = NULL;

    qhasharr_init(tbl, &_tbl_slots);
    retCode &= qhasharr_put(tbl, key, strlen(key), data, 6);
    retCode &= qhasharr_remove(tbl, key1, strlen(key1));
    val = qhasharr_get(tbl, key, strlen(key), &size);
    unsigned int hash = qhashmurmur3_32(key, strlen(key)) % tbl->maxslots;
    int idx = _get_idx(tbl, key, strlen(key), hash);

    EXPECT_FALSE(retCode);
    EXPECT_EQ(1, tbl->usedslots);
    EXPECT_GE(idx, 0);
    EXPECT_EQ(1, tbl->num);
    EXPECT_EQ(0, memcmp(data, _tbl_slots[idx].data.pair.value, 6));
    EXPECT_EQ(6ul, size);
    free(val);
    val = NULL;
}
/**
  * End_Test_for function: bool qhasharr_remove(qhasharr_t *tbl, const char *key, size_t key_size)
  *====================================================================================================
  */

/**
  *=====================================================================================================
  * Begin_Test_for fucntion: bool qhasharr_getnext(qhasharr_t *tbl, qnobj_t *obj, int *idx)
  */

// Test for qhasharr_getnext: tbl->maxslots=0
TEST_F(Test_qhasharr, qhasharr_getnext_zero_maxslots)
{
    bool retCode = true;
    qhasharr_t* tbl = NULL;
    int memsize = qhasharr_calculate_memsize(0);
    char* memory = NULL;
    memory = (char*)malloc(sizeof(char) * memsize);
    tbl = (qhasharr_t*)memory;
    tbl->maxslots = 0;
    int idx = 0;
    qnobj_t obj;

    retCode = qhasharr_getnext(tbl, &obj, &idx);

    EXPECT_FALSE(retCode);
    free(tbl);
    tbl = NULL;
}

// Test for qhasharr_getnext: tbl->maxslots>0 but tbl->usedslots=0
TEST_F(Test_qhasharr, qhasharr_getnext_zero_usedslots)
{
    bool retCode = true;
    int idx = 0;
    qnobj_t obj;

    retCode = qhasharr_getnext(tbl, &obj, &idx);

    EXPECT_FALSE(retCode);
}

// Test for qhasharr_getnext: only one object in tbl
TEST_F(Test_qhasharr, qhasharr_getnext_one_obj)
{
    bool retCode = true;
    int idx = 0;
    const char* key = "hello";
    const void* data = (const void*)"12345";
    qnobj_t obj;
    qhasharr_slot_t* _tbl_slots;

    qhasharr_init(tbl, &_tbl_slots);
    qhasharr_put(tbl, key, strlen(key), data, 6);
    unsigned int hash = qhashmurmur3_32(key, strlen(key)) % tbl->maxslots;
    int index = _get_idx(tbl, key, strlen(key), hash);
    retCode = qhasharr_getnext(tbl, &obj, &idx);

    EXPECT_TRUE(retCode);
    EXPECT_EQ(0, memcmp(key, obj.name, strlen(key)));
    EXPECT_EQ(0, memcmp(data, obj.data, 6));
    EXPECT_EQ(idx - 1, index);
    free(obj.name);
    free(obj.data);
}

// Test for qhasharr_getnext: only one object in tbl and key is truncated
TEST_F(Test_qhasharr, qhasharr_getnext_one_obj_and_key_is_truncated)
{
    bool retCode = true;
    int idx = 0;
    const char* key = "abcdefghijklmnopqrstuvwxyz0123456789";
    const void* data = (const void*)"1234";
    qnobj_t obj;
    qhasharr_slot_t* _tbl_slots;

    qhasharr_init(tbl, &_tbl_slots);
    qhasharr_put(tbl, key, strlen(key), data, 5);
    unsigned int hash = qhashmurmur3_32(key, strlen(key)) % tbl->maxslots;
    int index = _get_idx(tbl, key, strlen(key), hash);
    retCode = qhasharr_getnext(tbl, &obj, &idx);

    EXPECT_TRUE(retCode);
    EXPECT_EQ(0, strncmp(key, obj.name, _Q_HASHARR_KEYSIZE));
    EXPECT_EQ(0, memcmp(data, obj.data, 5));
    EXPECT_EQ(idx - 1, index);
    free(obj.name);
    free(obj.data);
}

// Test for qhasharr_getnext: only one object in tbl and value size exceeds _Q_HASHARR_VALUESIZE
TEST_F(Test_qhasharr, qhasharr_getnext_one_obj_and_value_size_exceeds)
{
    bool retCode = true;
    int idx = 0;
    const char* key = "abcdefghijklmnopqrstuvwxyz0123456789";
    const void* data = (const void*)"abcdefghijklmnopqrstuvwxyzabcdefghijklmnopqrstuvwxyzabcdefghijklmnopqrstuvwxyz012345678901234567890";
    qnobj_t obj;
    qhasharr_slot_t* _tbl_slots;

    qhasharr_init(tbl, &_tbl_slots);
    qhasharr_put(tbl, key, strlen(key), data, 100);
    unsigned int hash = qhashmurmur3_32(key, strlen(key)) % tbl->maxslots;
    int index = _get_idx(tbl, key, strlen(key), hash);
    retCode = qhasharr_getnext(tbl, &obj, &idx);

    EXPECT_TRUE(retCode);
    EXPECT_EQ(0, strncmp(key, obj.name, _Q_HASHARR_KEYSIZE));
    EXPECT_EQ(0, memcmp(data, obj.data, 100));
    EXPECT_EQ(idx - 1, index);
    free(obj.name);
    free(obj.data);
}

// Test for qhasharr_getnext: several objects in tbl and get all of them
TEST_F(Test_qhasharr, qhasharr_getnext_all)
{
    bool retCode = true;
    int idx = 0;
    int num = 0;

    const char* keys[] = {"abcd", "efg", "hijk", "lmn", "opq", "rst"};
    const void* vals[] = {"123", "123456", "hihihihi", "", "ok", "abcdefghijklmnopqrstuvwxyzabcdefghijklmnopqrstuvwxyzabcdefghijklmnopqrstuvwxyz012345678901234567890"};
    qnobj_t obj;
    qhasharr_slot_t* _tbl_slots;

    qhasharr_init(tbl, &_tbl_slots);

    for(int i = 0; i < 6; i++)
    {
        qhasharr_put(tbl, keys[i], strlen(keys[i]), vals[i], strlen((const char*)vals[i]) + 1);
    }

    while(true)
    {
        if((retCode = qhasharr_getnext(tbl, &obj, &idx)) == false)
            break;

        num++;

        if(strncmp(keys[0], obj.name, strlen(keys[0]) + 1) == 0)
            EXPECT_EQ(0, memcmp(vals[0], obj.data, 4));
        else if(strncmp(keys[1], obj.name, strlen(keys[1]) + 1) == 0)
            EXPECT_EQ(0, memcmp(vals[1], obj.data, 7));
        else if(strncmp(keys[2], obj.name, strlen(keys[2]) + 1) == 0)
            EXPECT_EQ(0, memcmp(vals[2], obj.data, 9));
        else if(strncmp(keys[3], obj.name, strlen(keys[3]) + 1) == 0)
            EXPECT_EQ(0, memcmp(vals[3], obj.data, 1));
        else if(strncmp(keys[4], obj.name, strlen(keys[4]) + 1) == 0)
            EXPECT_EQ(0, memcmp(vals[4], obj.data, 3));
        else if(strncmp(keys[5], obj.name, strlen(keys[5]) + 1) == 0)
            EXPECT_EQ(0, memcmp(vals[5], obj.data, 100));
        else
            ;

        free(obj.name);
        free(obj.data);
    }

    EXPECT_EQ(6, num);
}
/**
  * End_Test_for function: bool qhasharr_getnext(qhasharr_t *tbl, qnobj_t *obj, int *idx)
  *===============================================================================================================
  */

/**
  *===============================================================================================================
  * Begin_Test_for function: int _find_empty(qhasharr_t *tbl, int startidx)
  */

// Test for _find_empty: tbl->maxslots=0
TEST_F(Test_qhasharr, _find_empty_zero_maxslots)
{
    qhasharr_t* tmptbl = NULL;
    int num = -1;
    int memsize = qhasharr_calculate_memsize(0);
    char* memory = (char*)malloc(sizeof(char) * memsize);
    tmptbl = (qhasharr_t*)memory;
    tmptbl->maxslots = 0;

    num = _find_empty(tmptbl, 0);

    EXPECT_EQ(-1, num);
    free(tmptbl);
    tmptbl = NULL;
}

// Test for _find_empty: tbl->usedslots=0
TEST_F(Test_qhasharr, _find_empty_zero_usedslots)
{
    int num = -1;
    int startidx = 0;

    num = _find_empty(tbl, startidx);

    EXPECT_EQ(0, num);
}

// Test for _find_empty: startidx = tbl->maxslots-1
TEST_F(Test_qhasharr, _find_empty_startidx_valid)
{
    int num = -1;
    int startidx = tbl->maxslots - 1;

    num = _find_empty(tbl, startidx);

    EXPECT_EQ(19, num);
}

// Test for _find_empty: startidx = tbl->maxslots
TEST_F(Test_qhasharr, _find_empty_startsidx_equals_with_maxslots)
{
    int num = -1;
    int startidx = tbl->maxslots;

    num = _find_empty(tbl, startidx);

    EXPECT_EQ(0, num);
}

// Test for _find_empty: startidx > tbl->maxslots
TEST_F(Test_qhasharr, _find_empty_startsidx_greater_than_maxslots)
{
    int num = -1;
    int startidx = tbl->maxslots + 1;

    num = _find_empty(tbl, startidx);

    EXPECT_EQ(0, num);
}

// Test for _find_empty: usedslot =1 and idx=_get_idx(tbl,key,hash)=3, startsidx=2
TEST_F(Test_qhasharr, _find_empty_one_used_slots)
{
    int num = -1;
    const char* key = "hello";
    const void* data = (const void*)"12345";

    qhasharr_put(tbl, key, strlen(key), data, 6);
    unsigned int hash = qhashmurmur3_32(key, strlen(key)) % tbl->maxslots;
    int idx = _get_idx(tbl, key, strlen(key), hash);
    int startidx = idx - 1;

    num = _find_empty(tbl, startidx);

    EXPECT_EQ(startidx, num);
}

// Test for _find_empty: usedslots =1 and idx=_get_idx(tbl,key,hash)=3, startsidx=3
TEST_F(Test_qhasharr, _find_empty_one_used_slots_and_startidx_equals_with_idx)
{
    int num = -1;
    const char* key = "hello";
    const void* data = (const void*)"12345";

    qhasharr_put(tbl, key, strlen(key), data, 6);
    unsigned int hash = qhashmurmur3_32(key, strlen(key)) % tbl->maxslots;
    int idx = _get_idx(tbl, key, strlen(key), hash);
    int startidx = idx;
    num = _find_empty(tbl, startidx);

    EXPECT_EQ(startidx + 1, num);
}

// Test for _find_empty: usedslots =1 and idx=_get_idx(tbl,key,hash)=3, startsidx=4
TEST_F(Test_qhasharr, _find_empty_one_used_slots_and_startidx_after_idx)
{
    int num = -1;
    const char* key = "hello";
    const void* data = (const void*)"12345";

    qhasharr_put(tbl, key, strlen(key), data, 6);
    unsigned int hash = qhashmurmur3_32(key, strlen(key)) % tbl->maxslots;
    int idx = _get_idx(tbl, key, strlen(key), hash);
    int startidx = idx + 1;
    num = _find_empty(tbl, startidx);

    EXPECT_EQ(startidx, num);
}

// Test for _find_empty: usedslots = maxslots-1 and startidx=0
TEST_F(Test_qhasharr, _find_empty_usedslots_less_than_maxslots)
{
    int num = -1;
    int memsize = 0;
    const char* keys[] = {"abc", "def", "hijklmn"};
    const void* vals[] = {"111", "hihihi", "abcdefghijklmnopqrstuvwxyzabcdefghijklmnopqrstuvwxyzabcdefghijklmnopqrstuvwxxyz012345678901234567890"};
    memsize = qhasharr_calculate_memsize(5);
    char* memory = (char*)malloc(sizeof(char) * memsize);
    qhasharr_t* tbl = qhasharr(memory, memsize);

    for(int i = 0; i < 3; i++)
    {
        qhasharr_put(tbl, keys[i], strlen(keys[i]), vals[i], strlen((const char*)vals[i]) + 1);
    }

    int startidx = 0;
    num = _find_empty(tbl, startidx);

    EXPECT_EQ(2, num);
    EXPECT_EQ(4, tbl->usedslots);
    free(tbl);
    tbl = NULL;
}

// Test for _find_empty: usedslots = maxslots-1 and startidx=maxslots
TEST_F(Test_qhasharr, _find_empty_usedslots_less_than_maxslots2)
{
    int num = -1;
    int memsize = 0;
    const char* keys[] = {"abc", "def", "hijklmn"};
    const void* vals[] = {"111", "hihihi", "abcdefghijklmnopqrstuvwxyzabcdefghijklmnopqrstuvwxyzabcdefghijklmnopqrstuvwxxyz012345678901234567890"};
    memsize = qhasharr_calculate_memsize(5);
    char* memory = (char*)malloc(sizeof(char) * memsize);
    qhasharr_t* tbl = qhasharr(memory, memsize);

    for(int i = 0; i < 3; i++)
    {
        qhasharr_put(tbl, keys[i], strlen(keys[i]), vals[i], strlen((const char*)vals[i]) + 1);
    }

    int startidx = tbl->maxslots;
    num = _find_empty(tbl, startidx);

    EXPECT_EQ(2, num);
    EXPECT_EQ(4, tbl->usedslots);
    free(tbl);
    tbl = NULL;
}

// Test for _find_empty: usedslots = maxslots and startidx=0
TEST_F(Test_qhasharr, _find_empty_usedslots_equals_with_maxslots)
{
    int num = -1;
    int memsize = 0;
    const char* keys[] = {"abc", "def", "hijklmn", "opq"};
    const void* vals[] = {"111", "hihihi", "abcdefghijklmnopqrstuvwxyzabcdefghijklmnopqrstuvwxyzabcdefghijklmnopqrstuvwxxyz012345678901234567890", "aaa"};
    memsize = qhasharr_calculate_memsize(5);
    char* memory = (char*)malloc(sizeof(char) * memsize);
    qhasharr_t* tbl = qhasharr(memory, memsize);

    for(int i = 0; i < 4; i++)
    {
        qhasharr_put(tbl, keys[i], strlen(keys[i]), vals[i], strlen((const char*)vals[i]) + 1);
    }

    int startidx = 0;
    num = _find_empty(tbl, startidx);

    EXPECT_EQ(-1, num);
    EXPECT_EQ(5, tbl->usedslots);
    free(tbl);
    tbl = NULL;
}

// Test for _find_empty: usedslots = maxslots and startidx=tbl->maxslots
TEST_F(Test_qhasharr, _find_empty_usedslots_equals_with_maxslots2)
{
    int num = -1;
    int memsize = 0;
    const char* keys[] = {"abc", "def", "hijklmn", "opq"};
    const void* vals[] = {"111", "hihihi", "abcdefghijklmnopqrstuvwxyzabcdefghijklmnopqrstuvwxyzabcdefghijklmnopqrstuvwxxyz012345678901234567890", "aaa"};
    memsize = qhasharr_calculate_memsize(5);
    char* memory = (char*)malloc(sizeof(char) * memsize);
    qhasharr_t* tbl = qhasharr(memory, memsize);

    for(int i = 0; i < 4; i++)
    {
        qhasharr_put(tbl, keys[i], strlen(keys[i]), vals[i], strlen((const char*)vals[i]) + 1);
    }

    int startidx = tbl->maxslots;
    num = _find_empty(tbl, startidx);

    EXPECT_EQ(-1, num);
    EXPECT_EQ(5, tbl->usedslots);
    free(tbl);
    tbl = NULL;
}
/**
  * End_Test_for function: int _find_empty(qhasharr_t *tbl, int startidx)
  *=========================================================================================================================
  */

// End Test for qhasharr.c
