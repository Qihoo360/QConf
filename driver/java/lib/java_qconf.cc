#include "net_qihoo_qconf_Qconf.h"
#include <stdio.h>
#include "qconf.h"

static int print_error_message(JNIEnv *env, jclass jc, const int error_code);
static jobject convert_to_java_list(JNIEnv *env, const string_vector_t *nodes);
static jobject convert_to_java_map(JNIEnv *env, const qconf_batch_nodes *bnodes);

    JNIEXPORT jint JNICALL Java_net_qihoo_qconf_Qconf_init
(JNIEnv *env, jclass jc)
{
    int ret = 0;
    ret = qconf_init();
    if (QCONF_OK != ret)
    {
        print_error_message(env, jc, ret);
        return QCONF_ERR_OTHER;
    }
    return QCONF_OK;
}

    JNIEXPORT jint JNICALL Java_net_qihoo_qconf_Qconf_destroy
(JNIEnv * env, jclass jc)
{
   int ret = qconf_destroy();
   if (QCONF_OK != ret)
   {
        return QCONF_ERR_OTHER;
   }
   return QCONF_OK;
}

/*
 * Class:     net_qihoo_qconf_Qconf
 * Method:    getConf
 * Signature: (Ljava/lang/String;Ljava/lang/String;)Ljava/lang/String;
 */
    JNIEXPORT jstring JNICALL Java_net_qihoo_qconf_Qconf_getConf
(JNIEnv *env, jclass jc, jstring key, jstring idc)
{
    if (NULL == key)
    {
        print_error_message(env, jc, QCONF_ERR_PARAM);
        return NULL;
    }
    const char *key_c = env->GetStringUTFChars(key, NULL);
    if (NULL == key_c)
    {
        print_error_message(env, jc, QCONF_ERR_PARAM);
        return NULL;
    }
    char value_c[QCONF_JAVA_CONF_BUF_MAX_LEN];
    int ret = QCONF_ERR_OTHER;
    if (NULL == idc)
    {
        ret = qconf_get_conf(key_c, value_c, sizeof(value_c), NULL);
    }
    else
    {
        const char *idc_c = env->GetStringUTFChars(idc, NULL);
        if (NULL == idc_c)
        {
            print_error_message(env, jc, QCONF_ERR_PARAM);
            env->ReleaseStringUTFChars(key, key_c);
            return NULL;
        }
        ret = qconf_get_conf(key_c, value_c, sizeof(value_c), idc_c);
        env->ReleaseStringUTFChars(idc, idc_c);
    }
    env->ReleaseStringUTFChars(key, key_c);

    if (QCONF_OK != ret)
    {   
        print_error_message(env, jc, ret);
        return NULL;
    }
    jstring value = env->NewStringUTF(value_c);
    return value;
}

/*
 * Class:     net_qihoo_qconf_Qconf
 * Method:    getHost
 * Signature: (Ljava/lang/String;Ljava/lang/String;)Ljava/lang/String;
 */
    JNIEXPORT jstring JNICALL Java_net_qihoo_qconf_Qconf_getHost
(JNIEnv *env, jclass jc, jstring key, jstring idc)
{
    if (NULL == key)
    {
        print_error_message(env, jc, QCONF_ERR_PARAM);
        return NULL;
    }
    const char *key_c = env->GetStringUTFChars(key, NULL);
    if (NULL == key_c)
    {
        print_error_message(env, jc, QCONF_ERR_PARAM);
        return NULL;
    }
    char host_c[QCONF_HOST_BUF_MAX_LEN] = {0};
    int ret = QCONF_ERR_OTHER;
    if (NULL == idc)
    {
        ret = qconf_get_host(key_c, host_c, sizeof(host_c), NULL);
    }
    else
    {
        const char *idc_c = env->GetStringUTFChars(idc, NULL);
        if (NULL == idc_c)
        {
            print_error_message(env, jc, QCONF_ERR_PARAM);
            env->ReleaseStringUTFChars(key, key_c);
            return NULL;
        }
        ret = qconf_get_host(key_c, host_c, sizeof(host_c), idc_c);
        env->ReleaseStringUTFChars(idc, idc_c);
    }
    env->ReleaseStringUTFChars(key, key_c);

    if (QCONF_OK != ret)
    {   
        print_error_message(env, jc, ret);
        return NULL;
    }
    jstring host = env->NewStringUTF(host_c);
    return host;
}

/*
 * Class:     net_qihoo_qconf_Qconf
 * Method:    getAllHost
 * Signature: (Ljava/lang/String;Ljava/lang/String;)Ljava/util/ArrayList;
 */
    JNIEXPORT jobject JNICALL Java_net_qihoo_qconf_Qconf_getAllHost
(JNIEnv *env, jclass jc, jstring key, jstring idc)
{
    if (NULL == key)
    {
        print_error_message(env, jc, QCONF_ERR_PARAM);
        return NULL;
    }
    const char *key_c = env->GetStringUTFChars(key, NULL);
    if (NULL == key_c)
    {
        print_error_message(env, jc, QCONF_ERR_PARAM);
        return NULL;
    }
    string_vector_t nodes;
    int ret = init_string_vector(&nodes);
    if (QCONF_OK != ret)
    {   
        print_error_message(env, jc, ret);
        env->ReleaseStringUTFChars(key, key_c);
        return NULL;
    }   
    if (NULL == idc)
    {
        ret = qconf_get_allhost(key_c, &nodes, NULL);
    }
    else
    {
        const char *idc_c = env->GetStringUTFChars(idc, NULL);
        if (NULL == idc_c)
        {
            print_error_message(env, jc, QCONF_ERR_PARAM);
            env->ReleaseStringUTFChars(key, key_c);
            destroy_string_vector(&nodes);
            return NULL;
        }
        ret = qconf_get_allhost(key_c, &nodes, idc_c);
        env->ReleaseStringUTFChars(idc, idc_c);
    }
    env->ReleaseStringUTFChars(key, key_c);

    if (QCONF_OK != ret)
    {   
        print_error_message(env, jc, ret);
        destroy_string_vector(&nodes);
        return NULL;
    }

    jobject node_list = convert_to_java_list(env, &nodes);
    destroy_string_vector(&nodes);
    if (NULL == node_list)
    {   
        print_error_message(env, jc, QCONF_ERR_JAVA_CONVERT_LIST);
        return NULL;
    }   
    return node_list;
}

JNIEXPORT jobject JNICALL Java_net_qihoo_qconf_Qconf_getBatchConf
  (JNIEnv *env, jclass jc, jstring key, jstring idc)
{
    if (NULL == key)
    {
        print_error_message(env, jc, QCONF_ERR_PARAM);
        return NULL;
    }
    const char *key_c = env->GetStringUTFChars(key, NULL);
    if (NULL == key_c)
    {
        print_error_message(env, jc, QCONF_ERR_PARAM);
        return NULL;
    }
    qconf_batch_nodes bnodes;
    int ret = init_qconf_batch_nodes(&bnodes);
    if (QCONF_OK != ret)
    {   
        print_error_message(env, jc, ret);
        env->ReleaseStringUTFChars(key, key_c);
        return NULL;
    }   
    if (NULL == idc)
    {
        ret = qconf_get_batch_conf(key_c, &bnodes, NULL);
    }
    else
    {
        const char *idc_c = env->GetStringUTFChars(idc, NULL);
        if (NULL == idc_c)
        {
            print_error_message(env, jc, QCONF_ERR_PARAM);
            env->ReleaseStringUTFChars(key, key_c);
            destroy_qconf_batch_nodes(&bnodes);
            return NULL;
        }
        ret = qconf_get_batch_conf(key_c, &bnodes, idc_c);
        env->ReleaseStringUTFChars(idc, idc_c);
    }
    env->ReleaseStringUTFChars(key, key_c);

    if (QCONF_OK != ret)
    {   
        print_error_message(env, jc, ret);
        destroy_qconf_batch_nodes(&bnodes);
        return NULL;
    }

    jobject node_list = convert_to_java_map(env, &bnodes);
    destroy_qconf_batch_nodes(&bnodes);
    if (NULL == node_list)
    {   
        print_error_message(env, jc, QCONF_ERR_JAVA_CONVERT_MAP);
        return NULL;
    }   
    return node_list;
}

JNIEXPORT jobject JNICALL Java_net_qihoo_qconf_Qconf_getBatchKeys
  (JNIEnv *env, jclass jc, jstring key, jstring idc)
{
    if (NULL == key)
    {
        print_error_message(env, jc, QCONF_ERR_PARAM);
        return NULL;
    }
    const char *key_c = env->GetStringUTFChars(key, NULL);
    if (NULL == key_c)
    {
        print_error_message(env, jc, QCONF_ERR_PARAM);
        return NULL;
    }
    string_vector_t nodes;
    int ret = init_string_vector(&nodes);
    if (QCONF_OK != ret)
    {   
        print_error_message(env, jc, ret);
        env->ReleaseStringUTFChars(key, key_c);
        return NULL;
    }   
    if (NULL == idc)
    {
        ret = qconf_get_batch_keys(key_c, &nodes, NULL);
    }
    else
    {
        const char *idc_c = env->GetStringUTFChars(idc, NULL);
        if (NULL == idc_c)
        {
            print_error_message(env, jc, QCONF_ERR_PARAM);
            env->ReleaseStringUTFChars(key, key_c);
            destroy_string_vector(&nodes);
            return NULL;
        }
        ret = qconf_get_batch_keys(key_c, &nodes, idc_c);
        env->ReleaseStringUTFChars(idc, idc_c);
    }
    env->ReleaseStringUTFChars(key, key_c);

    if (QCONF_OK != ret)
    {   
        print_error_message(env, jc, ret);
        destroy_string_vector(&nodes);
        return NULL;
    }

    jobject keys_list = convert_to_java_list(env, &nodes);
    destroy_string_vector(&nodes);
    if (NULL == keys_list)
    {   
        print_error_message(env, jc, QCONF_ERR_JAVA_CONVERT_LIST);
        return NULL;
    }   
    return keys_list;
}
static int print_error_message(JNIEnv *env, jclass jc, const int error_code)
{
    const char* message = NULL;
    switch (error_code)
    {   
        case QCONF_ERR_OTHER :
            message = "Execute failure!";
            break;
        case QCONF_ERR_PARAM :
            message = "Error parameter!";
            break;
        case QCONF_ERR_MEM :
            message = "Failed to malloc memory!";
            break;
        case QCONF_ERR_TBL_SET :
            message = "Failed to set share memory";
            break;
        case QCONF_ERR_GET_HOST :
            message = "Failed to get zookeeper host!";
            break;
        case QCONF_ERR_GET_IDC :
            message = "Failed to get idc!";
            break;
        case QCONF_ERR_BUF_NOT_ENOUGH :
            message = "Buffer not enough!";
            break;
        case QCONF_ERR_DATA_TYPE :
            message = "Illegal data type!";
            break;
        case QCONF_ERR_DATA_FORMAT :
            message = "Illegal data format!";
            break;
        case QCONF_ERR_NOT_FOUND :
            message = "Failed to find the key on given idc!";
            break;
        case QCONF_ERR_OPEN_DUMP :
            message = "Failed to open dump file!";
            break;
        case QCONF_ERR_OPEN_TMP_DUMP :
            message = "Failed to open tmp dump file!";
            break;
        case QCONF_ERR_NOT_IN_DUMP :
            message = "Failed to find key in dump!";
            break;
        case QCONF_ERR_WRITE_DUMP :
            message = "Failed to write dump!";
            break;
        case QCONF_ERR_SAME_VALUE :
            message = "Same with the value in share memory!";
            break;
        case QCONF_ERR_OUT_OF_RANGE :
            message = "Configure item error : out of range!";
            break;
        case QCONF_ERR_NOT_NUMBER :
            message = "Configure item error : not number!";
            break;
        case QCONF_ERR_OTHRE_CHARACTER :
            message = "Configure item error : further characters exists!";
            break;
        case QCONF_ERR_INVALID_IP :
            message = "Configure item error : invalid ip!";
            break;
        case QCONF_ERR_INVALID_PORT :
            message = "Configure item error : invalid port!";
            break;
        case QCONF_ERR_NO_MESSAGE :
            message = "No message exist in message queue!";
            break;
        case QCONF_ERR_E2BIG :
            message = "Length of message in the queue is too large!";
            break;     
        case QCONF_ERR_HOSTNAME :
            message = "Error hostname!";
            break;
        case QCONF_ERR_JAVA_CONVERT_LIST:
            message = "Failed to covert string vector to arraylist";
            break;
        case QCONF_ERR_JAVA_CONVERT_MAP:
            message = "Failed to covert qconf_batch_nodes to treeMap";
            break;
        default :
            message = "Unknown error detected!";
            break;
    }
    jmethodID mid = env->GetStaticMethodID(jc, "exceptionCallback", "(Ljava/lang/String;)V");
    if (NULL == mid)
    {
        return QCONF_ERR_OTHER;
    }
    jstring msg = env->NewStringUTF(message);
    env->CallStaticVoidMethod(jc, mid, msg);
    env->DeleteLocalRef(msg);
    return QCONF_OK;
}

static jobject convert_to_java_list(JNIEnv *env, const string_vector_t *nodes)
{
    if (NULL == nodes)
    {
        return NULL;
    }
    jclass cls_ArrayList = env->FindClass("java/util/ArrayList");  
    jmethodID construct = env->GetMethodID(cls_ArrayList, "<init>", "()V");  
    if (NULL == construct)
    {
        env->DeleteLocalRef(cls_ArrayList);
        return NULL;
    }

    jobject obj_ArrayList = env->NewObject(cls_ArrayList, construct, "");  
    jmethodID arrayList_add = env->GetMethodID(cls_ArrayList, "add", "(Ljava/lang/Object;)Z");  
    env->DeleteLocalRef(cls_ArrayList);
    if (NULL == construct)
    {
        return NULL;
    }
    int i;
    int count = (*nodes).count;
    for (i = 0; i < count; i++){
        char *data_c = (*nodes).data[i];
        jstring node_data = env->NewStringUTF(data_c);
        env->CallObjectMethod(obj_ArrayList, arrayList_add, node_data); 
        env->DeleteLocalRef(node_data);
    }
    return obj_ArrayList;
}


static jobject convert_to_java_map(JNIEnv *env, const qconf_batch_nodes *bnodes)
{
    if (NULL == bnodes)
    {
        return NULL;
    }
    jclass cls_TreeMap = env->FindClass("java/util/TreeMap");  
    jmethodID construct = env->GetMethodID(cls_TreeMap, "<init>", "()V");  
    if (NULL == construct)
    {
        env->DeleteLocalRef(cls_TreeMap);
        return NULL;
    }

    int count = (*bnodes).count;
    jobject obj_TreeMap = env->NewObject(cls_TreeMap, construct, "");  
    jmethodID treeMap_put = env->GetMethodID(cls_TreeMap, "put", "(Ljava/lang/Object;Ljava/lang/Object;)Ljava/lang/Object;");  
    env->DeleteLocalRef(cls_TreeMap);
    if (NULL == construct)
    {
        return NULL;
    }
    int i;
    for (i = 0; i < count; i++){
        char *key_c = (*bnodes).nodes[i].key;
        char *value_c = (*bnodes).nodes[i].value;
        jstring key_j = env->NewStringUTF(key_c);
        jstring value_j = env->NewStringUTF(value_c);
        env->CallObjectMethod(obj_TreeMap, treeMap_put, key_j, value_j); 
        env->DeleteLocalRef(key_j);
        env->DeleteLocalRef(value_j);
    }
    return obj_TreeMap;
}
