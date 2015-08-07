#include <sys/ipc.h>
#include <sys/msg.h>
#include <sys/types.h>
#include <string>
#include <iostream>
#include <unistd.h>
#include "gtest/gtest.h"
#include "qconf_msg.h"
using namespace std;


#define STATIC_MSG_QUEUE_KEY 0xaa1122ee

// unit test case for qconf_msg.c

// Related test environment set up and tear down
class Test_qconf_msg : public ::testing::Test
{
protected:
    virtual void SetUp()
    {
    }

    virtual void TearDown()
    {
        msgctl(msgid, IPC_RMID, NULL);
    }

    int msgid;
};

/**
  *============================================================================
  *  Begin_Test_for function: int create_msg_queue(key_t key, int *msgid)
  */
// Test for create_msg_queue: msg queue related to the key:STATIC_MSG_QUEUE_KEY  not exists yet
TEST_F(Test_qconf_msg, create_msg_queue_not_exists)
{
    int retCode = 0;
    key_t msg_queue_key = STATIC_MSG_QUEUE_KEY;

    retCode = create_msg_queue(msg_queue_key, msgid);
    
    EXPECT_EQ(QCONF_OK, retCode);
}
// Test for create_msg_queue: msg queue related to the key: STATIC_MSG_QUEUE_KEY already exists
TEST_F(Test_qconf_msg, create_msg_queue_already_exists)
{
    int retCode = 0;
    key_t msg_queue_key = STATIC_MSG_QUEUE_KEY;

    retCode = create_msg_queue(msg_queue_key, msgid);
    EXPECT_EQ(QCONF_OK, retCode);
    
    retCode = create_msg_queue(msg_queue_key, msgid);
    EXPECT_EQ(QCONF_OK, retCode);
}
// Test for create_msg_queue: create msg queue by a new key
TEST_F(Test_qconf_msg, create_msg_queue_by_a_new_key)
{
    int retCode = 0;
    key_t key = 0xd10c5610;

    retCode = create_msg_queue(key, msgid);
    
    EXPECT_EQ(QCONF_OK, retCode);
}
/**
  *  End_Test_for function: int create_msg_queue(key_t key, int *msgid)
  *===============================================================================
  */



/**
  *============================================================================
  *  Begin_Test_for function: int init_msg_queue(key_t key, int &msqid);
  */
// msg queue not exist yet
TEST_F(Test_qconf_msg,  init_msg_queue_no_exists)
{
    int retCode = 0;

    retCode = init_msg_queue(STATIC_MSG_QUEUE_KEY, msgid);
    
    EXPECT_EQ(QCONF_ERR_MSGGET, retCode);
}

// msg queue exist
TEST_F(Test_qconf_msg,  init_msg_queue_exists)
{
    int retCode = 0, tmp_msgid=0;

    retCode = create_msg_queue(STATIC_MSG_QUEUE_KEY, msgid);
    EXPECT_EQ(QCONF_OK, retCode);
    retCode = init_msg_queue(STATIC_MSG_QUEUE_KEY, tmp_msgid);
    EXPECT_EQ(QCONF_OK, retCode);
    EXPECT_EQ(msgid, tmp_msgid);
}
/**
  *  End_Test_for function: int init_msg_queue(key_t key, int &msqid);
  *===============================================================================
  */


/**
  *============================================================================
  *  Begin_Test_for function: int send_msg(int msqid, const string &msg)
  */
// Test for send_msg: send msg to an unexisted msgid
TEST_F(Test_qconf_msg, send_msg_unexisted_msgid)
{
    int retCode = 0;
    int msgids = -1;
    string message("hello");

    retCode = send_msg(msgids, message);
    EXPECT_EQ(QCONF_ERR_MSGSND, retCode);
}

// Test for send_msg: send empty message
TEST_F(Test_qconf_msg, send_msg_empty_message)
{
    int retCode = 0;
    key_t key = STATIC_MSG_QUEUE_KEY;
    string message;

    create_msg_queue(key, msgid);
    retCode = send_msg(msgid, message);
    EXPECT_EQ(QCONF_ERR_PARAM, retCode);
}

// Test for send_msg: send msg by msgid which is created by a key != STATIC_MSG_QUEUE_KEY
TEST_F(Test_qconf_msg, send_msg_msg_to_msg_queue_by_msgid_created_by_another_key)
{
    int retCode = 0;
    key_t key = 0x101023ab;
    string message("hello");

    create_msg_queue(key, msgid);
    retCode = send_msg(msgid, message);
    EXPECT_EQ(QCONF_OK, retCode);
}

// Test for send_msg: length of msg equals with QCONF_MAX_MSG_LEN
TEST_F(Test_qconf_msg, send_msg_length_of_msg_exceed)
{
    int retCode = 0;
    key_t key = STATIC_MSG_QUEUE_KEY;
    create_msg_queue(key, msgid);
    
    string message(QCONF_MAX_MSG_LEN, 'a');
    retCode = send_msg(msgid, message);
    EXPECT_EQ(QCONF_ERR_E2BIG, retCode);
}

// Test for send_msg: length of msg exceed QCONF_MAX_MSG_LEN
TEST_F(Test_qconf_msg, send_msg_length_exceed2)
{
    int retCode = 0;
    key_t key = STATIC_MSG_QUEUE_KEY;
    create_msg_queue(key, msgid);
    
    string message(QCONF_MAX_MSG_LEN + 1, 'a');
    retCode = send_msg(msgid, message);
    EXPECT_EQ(QCONF_ERR_E2BIG, retCode);
}

// Test for send_msg: send msg failed since has no access to write the msg queue
TEST_F(Test_qconf_msg, send_msg_failed_since_has_no_access_to_write)
{
    int retCode = 0;
    key_t key = 0x25251000;
    struct msqid_ds buf;
    memset(&buf, 0, sizeof(buf));

    create_msg_queue(key, msgid);
    msgctl(msgid, IPC_STAT, &buf);
    buf.msg_perm.mode = 0444;
    msgctl(msgid, IPC_SET, &buf);

    retCode = send_msg(msgid, "has no access to write");
    EXPECT_EQ(QCONF_ERR_MSGSND, retCode);
}

// Test for send_msg: send msg successfully
TEST_F(Test_qconf_msg, send_msg_successfully)
{
    int retCode = 0;
    key_t key = STATIC_MSG_QUEUE_KEY;
    string message("hello");

    create_msg_queue(key, msgid);
    retCode = send_msg(msgid, message);
    EXPECT_EQ(QCONF_OK, retCode);
}
/**
  *  End_Test_for function: int send_msg(int msgid, string &msg)
  *===============================================================================
  */

/**
  *============================================================================
  *  Begin_Test_for function: int receive_msg(int msqid, string &msg)
  */
// Test for receive_msg: recv msg from an unexisted msgid
TEST_F(Test_qconf_msg, receive_msg_from_unexisted_msgid)
{
    int retCode = 0;
    string message;

    retCode = receive_msg(-1, message);
    EXPECT_EQ(QCONF_ERR_MSGRCV, retCode);
}

// Test for receive_msg: no message in msg queue
TEST_F(Test_qconf_msg, msg_recv_from_msgid_has_no_msg)
{
    int retCode = 0;
    key_t key = 0x12340710;
    string message;
    struct msqid_ds buf;
    memset(&buf, 0, sizeof(buf));

    create_msg_queue(key, msgid);
    retCode = msgctl(msgid, IPC_STAT, &buf);
    EXPECT_EQ(0, retCode);
    EXPECT_EQ(0ul, buf.msg_qnum);
    //EXPECT_EQ(0ul, buf.__msg_cbytes);
    //retCode = receive_msg(msgid, message);
}

// Test for receive_msg: receive a new message 
TEST_F(Test_qconf_msg, msg_recv_from_msg_queue_by_msgid_created_by_a_new_key)
{
    int retCode = 0;
    key_t key = 0x25252510;
    string message;

    create_msg_queue(key, msgid);
    send_msg(msgid, "hihihi");
    retCode = receive_msg(msgid, message);

    EXPECT_EQ(0, retCode);
    EXPECT_EQ("hihihi", message);
}

// Test for receive_msg: msg in msg queue has already been received
TEST_F(Test_qconf_msg, msg_recv_from_msg_queue_but_msg_has_been_received_already)
{
    int retCode = 0;
    key_t key = 0x2525101a;
    string message;
    struct msqid_ds buf;
    memset(&buf, 0, sizeof(buf));

    create_msg_queue(key, msgid);
    send_msg(msgid, "hello");

    receive_msg(msgid, message);
    EXPECT_EQ("hello", message);

    // check whether there is no value in the queue
    retCode = msgctl(msgid, IPC_STAT, &buf);
    EXPECT_EQ(0, retCode);
    EXPECT_EQ(0ul, buf.msg_qnum);
    //EXPECT_EQ(0ul, buf.__msg_cbytes);

    //retCode = receive_msg(msgid, message);
}

// Test for receive_msg: recv msg successfully
TEST_F(Test_qconf_msg, receive_msg_successfully)
{
    int retCode = 0;
    key_t key = 0x10102345;
    string message;

    create_msg_queue(key, msgid);
    send_msg(msgid, "hello");
    retCode = receive_msg(msgid, message);

    EXPECT_EQ(0, retCode);
    EXPECT_EQ("hello", message);
}

// Test for receive_msg: recv times = send times
TEST_F(Test_qconf_msg, receive_msg_times_equals_send_times)
{
    int retCode = 0;
    key_t key = 0x12341234;
    string msg;

    create_msg_queue(key, msgid);

    for(int i = 0; i < 10; i++)
    {
       retCode = send_msg(msgid, "hello");
       EXPECT_EQ(QCONF_OK, retCode);
    }

    for(int i = 0; i < 10; i++)
    {
        retCode = receive_msg(msgid, msg);
        EXPECT_EQ(QCONF_OK, retCode);
        EXPECT_EQ("hello", msg);
        msg.clear();
    }
}

// Test for receive_msg: recv times > send time
TEST_F(Test_qconf_msg, receive_msg_times_more_than_send_times)
{
    int retCode = 0;
    key_t key = 0x23451010;
    string msg;
    struct msqid_ds buf;
    memset(&buf, 0, sizeof(buf));

    create_msg_queue(key, msgid);

    for(int i = 0; i < 10; i++)
    {
        retCode = send_msg(msgid, "hello");
        EXPECT_EQ(QCONF_OK, retCode);
    }

    for(int i = 0; i < 10; i++)
    {
        retCode = receive_msg(msgid, msg);
        EXPECT_EQ(QCONF_OK, retCode);
        EXPECT_EQ("hello", msg);
    }

    // check whether there is no value in the queue
    retCode = msgctl(msgid, IPC_STAT, &buf);
    EXPECT_EQ(0, retCode);
    EXPECT_EQ(0ul, buf.msg_qnum);
   // EXPECT_EQ(0ul, buf.__msg_cbytes);


    // retCode = receive_msg(msgid, msg);
}

// Test for send_msg: send msg to a msg queue repeatedly
TEST_F(Test_qconf_msg, send_msg_repeatedly)
{
    int retCode = 0;
    key_t key = STATIC_MSG_QUEUE_KEY;
    string message("hello");
    string ret_mes;
    unsigned long i = 0;
    struct msqid_ds buf;
    memset(&buf, 0, sizeof(buf));

    create_msg_queue(key, msgid);

    for(i = 0; i < 65536; i++)
    {
        retCode = send_msg(msgid, message);
        EXPECT_EQ(QCONF_OK, retCode);
        retCode = msgctl(msgid, IPC_STAT, &buf);
        EXPECT_EQ(0, retCode);
        EXPECT_EQ(1ul, buf.msg_qnum);
        //EXPECT_EQ(message.size(), buf.__msg_cbytes);

        retCode = receive_msg(msgid, ret_mes);
        EXPECT_EQ(QCONF_OK, retCode);
        EXPECT_EQ(QCONF_OK, retCode);
        // check whether there is no value in the queue
        retCode = msgctl(msgid, IPC_STAT, &buf);
        EXPECT_EQ(0, retCode);
        EXPECT_EQ(0ul, buf.msg_qnum);
       // EXPECT_EQ(0ul, buf.__msg_cbytes);
    }
}

/**
  *  End_Test_for function: int receive_msg(int msqid, const string &msg)
  *===============================================================================
  */

