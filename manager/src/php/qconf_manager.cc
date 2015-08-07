/*
  +----------------------------------------------------------------------+
  | PHP Version 5                                                        |
  +----------------------------------------------------------------------+
  | Copyright (c) 1997-2015 The PHP Group                                |
  +----------------------------------------------------------------------+
  | This source file is subject to version 3.01 of the PHP license,      |
  | that is bundled with this package in the file LICENSE, and is        |
  | available through the world-wide-web at the following url:           |
  | http://www.php.net/license/3_01.txt                                  |
  | If you did not receive a copy of the PHP license and are unable to   |
  | obtain it through the world-wide-web, please send a note to          |
  | license@php.net so we can mail you a copy immediately.               |
  +----------------------------------------------------------------------+
  | Author:                                                              |
  +----------------------------------------------------------------------+
*/

/* $Id$ */

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include "php.h"
#include "php_ini.h"
#include "ext/standard/info.h"
#include "php_qconf_manager.h"
#include <ctype.h>
#include <string>
#include <set>
#include <map>
#include <vector>
#include "qconf_zk.h"

#define QCONF_PHP_ERROR -1

zend_object_handlers qconfzk_object_handlers;
struct qconfzk_object
{
    zend_object std;
    QConfZK *qconfzk;
};

void qconfzk_free_storage(void *object TSRMLS_DC)
{
    qconfzk_object *obj = (qconfzk_object *)object;
    delete obj->qconfzk; 
    zend_hash_destroy(obj->std.properties);
    FREE_HASHTABLE(obj->std.properties);
    efree(obj);
}

zend_object_value qconfzk_create_handler(zend_class_entry *type TSRMLS_DC)
{
    zval *tmp;
    zend_object_value retval;

    qconfzk_object *obj = (qconfzk_object *)emalloc(sizeof(qconfzk_object));
    memset(obj, 0, sizeof(qconfzk_object));
    obj->std.ce = type;

    ALLOC_HASHTABLE(obj->std.properties);
    zend_hash_init(obj->std.properties, 0, NULL, ZVAL_PTR_DTOR, 0);
    zend_hash_copy(obj->std.properties, &type->properties_info,
            (copy_ctor_func_t)zval_add_ref, (void *)&tmp, sizeof(zval *));

    retval.handle = zend_objects_store_put(obj, NULL,
            qconfzk_free_storage, NULL TSRMLS_CC);
    retval.handlers = &qconfzk_object_handlers;

    return retval;
}


/****************************************
  Method implementations
 ****************************************/
zend_class_entry *qconfzk_ce;

/* {{{ QConfZK::__construct ( .. )
   Creates a QConfZK object */
static PHP_METHOD(QConfZK, __construct)
{
    char *host = NULL;
    int host_len = 0;
    zval *instance = getThis();

    if (zend_parse_parameters(ZEND_NUM_ARGS() TSRMLS_CC, "s", &host, &host_len) == FAILURE) 
    {
        RETURN_NULL();
    }

    QConfZK *qconfZk = new QConfZK();
    qconfZk->zk_init(std::string(host, host_len));
    
    qconfzk_object *obj = (qconfzk_object *)zend_object_store_get_object(instance TSRMLS_CC);
    obj->qconfzk = qconfZk;
}
/* }}} */

/* {{{ QConfZK::__construct ( .. )
   Creates a QConfZK object */
static PHP_METHOD(QConfZK, __destruct)
{
    QConfZK *qconfzk = NULL;
    qconfzk_object *obj = (qconfzk_object *)zend_object_store_get_object(
            getThis() TSRMLS_CC);
    qconfzk = obj->qconfzk;
    if (qconfzk != NULL) 
    {
        qconfzk->zk_close();
    }
}
/* }}} */

/* {{{ Method implementations
*/
static PHP_METHOD(QConfZK, nodeSet)
{
    char *path = NULL, *value = NULL;
    int path_len = 0, value_len = 0;

    if (zend_parse_parameters(ZEND_NUM_ARGS() TSRMLS_CC, "ss", &path, &path_len, &value, &value_len) == FAILURE) 
    {
        RETURN_LONG(QCONF_PHP_ERROR);
    }

    QConfZK *qconfzk = NULL;
    qconfzk_object *obj = (qconfzk_object *)zend_object_store_get_object(
            getThis() TSRMLS_CC);
    qconfzk = obj->qconfzk;
    if (qconfzk != NULL) 
    {
        RETURN_LONG(qconfzk->zk_node_set(std::string(path, path_len), std::string(value, value_len)));
    }
    
    RETURN_LONG(QCONF_PHP_ERROR);
}

static PHP_METHOD(QConfZK, nodeGet)
{
    char *path = NULL;
    int path_len = 0;

    if (zend_parse_parameters(ZEND_NUM_ARGS() TSRMLS_CC, "s", &path, &path_len) == FAILURE) 
    {
        RETURN_NULL();
    }

    QConfZK *qconfzk = NULL;
    qconfzk_object *obj = (qconfzk_object *)zend_object_store_get_object(getThis() TSRMLS_CC);
    qconfzk = obj->qconfzk;
    if (qconfzk != NULL) {
        std::string value;
        if (QCONF_OK != qconfzk->zk_node_get(std::string(path, path_len), value)) RETURN_NULL();
        RETURN_STRINGL(const_cast<char*>(value.c_str()), value.size(), 1);
    }
    RETURN_NULL();
}

static PHP_METHOD(QConfZK, nodeDelete)
{
    char *path = NULL;
    int path_len = 0;

    if (zend_parse_parameters(ZEND_NUM_ARGS() TSRMLS_CC, "s", &path, &path_len) == FAILURE) 
    {
        RETURN_LONG(QCONF_PHP_ERROR);
    }

    QConfZK *qconfzk = NULL;
    qconfzk_object *obj = (qconfzk_object *)zend_object_store_get_object(
            getThis() TSRMLS_CC);
    qconfzk = obj->qconfzk;
    if (qconfzk != NULL) {
        RETURN_LONG(qconfzk->zk_node_delete(std::string(path, path_len)));
    }
    
    RETURN_LONG(QCONF_PHP_ERROR);
}

static PHP_METHOD(QConfZK, servicesSet)
{
    char *path = NULL;
    zval *array_input = NULL;
    zval **z_item = NULL;
    int path_len = 0, service_count = 0, i = 0;
    std::map<std::string, char> mserv;

    if (zend_parse_parameters(ZEND_NUM_ARGS() TSRMLS_CC, "sa", &path, &path_len, &array_input) == FAILURE) 
    {
        RETURN_LONG(QCONF_PHP_ERROR);
    }

    service_count = zend_hash_num_elements(Z_ARRVAL_P(array_input));

    zend_hash_internal_pointer_reset(Z_ARRVAL_P(array_input)); 
    for (i = 0; i < service_count; i++) 
    {
        char* key;
        unsigned long idx;
        zend_hash_get_current_data(Z_ARRVAL_P(array_input), (void**) &z_item);
        convert_to_long_ex(z_item);
        if (zend_hash_get_current_key(Z_ARRVAL_P(array_input), &key, &idx, 0) == HASH_KEY_IS_STRING) 
        {
            if (!isascii(Z_LVAL_PP(z_item))) 
                RETURN_LONG(QCONF_PHP_ERROR);
            mserv.insert(std::pair<std::string, char>(key, Z_LVAL_PP(z_item)));
        } 
        else 
        {
            RETURN_LONG(QCONF_PHP_ERROR);
        }
        zend_hash_move_forward(Z_ARRVAL_P(array_input));
    }

    QConfZK *qconfzk = NULL;
    qconfzk_object *obj = (qconfzk_object *)zend_object_store_get_object(
            getThis() TSRMLS_CC);
    qconfzk = obj->qconfzk;
    if (qconfzk != NULL) 
    {
        RETURN_LONG(qconfzk->zk_services_set(std::string(path, path_len), mserv));
    }
    
    RETURN_LONG(QCONF_PHP_ERROR);
}

static PHP_METHOD(QConfZK, servicesGet)
{
    char *path = NULL;
    int path_len = 0;

    if (zend_parse_parameters(ZEND_NUM_ARGS() TSRMLS_CC, "s", &path, &path_len) == FAILURE) 
    {
        RETURN_NULL();
    }

    // Get C++ object
    QConfZK *qconfzk = NULL;
    qconfzk_object *obj = (qconfzk_object *)zend_object_store_get_object(
            getThis() TSRMLS_CC);
    qconfzk = obj->qconfzk;
    if (qconfzk == NULL) RETURN_NULL();
    
    int ret = QCONF_PHP_ERROR;
    std::set<std::string> servs;
    ret = qconfzk->zk_services_get(std::string(path, path_len), servs);
    if (QCONF_OK != ret) RETURN_NULL();
    
    zval *return_data = NULL;                    
    MAKE_STD_ZVAL(return_data);                
    array_init(return_data); 
    std::set<std::string>::const_iterator it;
    int i = 0;
    for (it = servs.begin(); it != servs.end(); ++it, ++i)
    {
        add_index_string(return_data, i, const_cast<char*>((*it).c_str()), 1); 
    }
    *return_value = *return_data;
    return;
}

static PHP_METHOD(QConfZK, servicesGetWithStatus)
{
    char *path = NULL;
    int path_len = 0;

    if (zend_parse_parameters(ZEND_NUM_ARGS() TSRMLS_CC, "s", &path, &path_len) == FAILURE) 
    {
        RETURN_NULL();
    }

    QConfZK *qconfzk = NULL;
    qconfzk_object *obj = (qconfzk_object *)zend_object_store_get_object(
            getThis() TSRMLS_CC);
    qconfzk = obj->qconfzk;
    if (qconfzk == NULL)
        RETURN_NULL();
    
    int ret = QCONF_PHP_ERROR;
    std::map<std::string, char> servs;
    ret = qconfzk->zk_services_get_with_status(std::string(path, path_len), servs);
    if (QCONF_OK != ret) RETURN_NULL();

    zval *return_data;                    
    MAKE_STD_ZVAL(return_data);                
    array_init(return_data); 

    std::map<std::string, char>::iterator it = servs.begin();
    for (int i = 0; it != servs.end(); ++it, ++i)
    {
        add_assoc_long(return_data, const_cast<char*>((it->first).c_str()), it->second); 
    }
    *return_value = *return_data;
    return;
}

static PHP_METHOD(QConfZK, serviceAdd)
{
    char *path, *serv = NULL;
    int path_len = 0, serv_len = 0;
    long status = -1;

    if (zend_parse_parameters(ZEND_NUM_ARGS() TSRMLS_CC, "ssl", &path, &path_len, &serv, &serv_len, &status) == FAILURE) 
    {
        RETURN_LONG(QCONF_PHP_ERROR);
    }

    if (!isascii(status)) RETURN_LONG(QCONF_PHP_ERROR);

    QConfZK *qconfzk = NULL;
    qconfzk_object *obj = (qconfzk_object *)zend_object_store_get_object(
            getThis() TSRMLS_CC);
    qconfzk = obj->qconfzk;
    if (qconfzk == NULL)
        RETURN_LONG(QCONF_PHP_ERROR);
    RETURN_LONG(qconfzk->zk_service_add(std::string(path, path_len), std::string(serv, serv_len), status));
}

static PHP_METHOD(QConfZK, serviceDelete)
{
    char *path = NULL, *serv = NULL;
    int path_len = 0, serv_len = 0;

    if (zend_parse_parameters(ZEND_NUM_ARGS() TSRMLS_CC, "ss", &path, &path_len, &serv, &serv_len) == FAILURE) 
    {
        RETURN_LONG(QCONF_PHP_ERROR);
    }

    QConfZK *qconfzk = NULL;
    qconfzk_object *obj = (qconfzk_object *)zend_object_store_get_object(
            getThis() TSRMLS_CC);
    qconfzk = obj->qconfzk;
    if (qconfzk == NULL)
        RETURN_LONG(QCONF_PHP_ERROR);
    
    RETURN_LONG(qconfzk->zk_service_delete(std::string(path, path_len), std::string(serv, serv_len)));
}

static PHP_METHOD(QConfZK, serviceUp)
{
    char *path = NULL, *serv = NULL;
    int path_len = 0, serv_len = 0;

    if (zend_parse_parameters(ZEND_NUM_ARGS() TSRMLS_CC, "ss", &path, &path_len, &serv, &serv_len) == FAILURE) 
    {
        RETURN_LONG(QCONF_PHP_ERROR);
    }

    QConfZK *qconfzk = NULL;
    qconfzk_object *obj = (qconfzk_object *)zend_object_store_get_object(
            getThis() TSRMLS_CC);
    qconfzk = obj->qconfzk;
    if (qconfzk == NULL)
        RETURN_LONG(QCONF_PHP_ERROR);
    
    RETURN_LONG(qconfzk->zk_service_up(std::string(path, path_len), std::string(serv, serv_len)));
}

static PHP_METHOD(QConfZK, serviceDown)
{
    char *path = NULL, *serv = NULL;
    int path_len = 0, serv_len = 0;

    if (zend_parse_parameters(ZEND_NUM_ARGS() TSRMLS_CC, "ss", &path, &path_len, &serv, &serv_len) == FAILURE) 
    {
        RETURN_LONG(QCONF_PHP_ERROR);
    }

    QConfZK *qconfzk = NULL;
    qconfzk_object *obj = (qconfzk_object *)zend_object_store_get_object(
            getThis() TSRMLS_CC);
    qconfzk = obj->qconfzk;
    if (qconfzk == NULL)
        RETURN_LONG(QCONF_PHP_ERROR);
    
    RETURN_LONG(qconfzk->zk_service_down(std::string(path, path_len), std::string(serv, serv_len)));
}

static PHP_METHOD(QConfZK, serviceOffline)
{
    char *path = NULL, *serv = NULL;
    int path_len = 0, serv_len = 0;

    if (zend_parse_parameters(ZEND_NUM_ARGS() TSRMLS_CC, "ss", &path, &path_len, &serv, &serv_len) == FAILURE) 
    {
        RETURN_LONG(QCONF_PHP_ERROR);
    }

    QConfZK *qconfzk = NULL;
    qconfzk_object *obj = (qconfzk_object *)zend_object_store_get_object(
            getThis() TSRMLS_CC);
    qconfzk = obj->qconfzk;
    if (qconfzk == NULL)
        RETURN_LONG(QCONF_PHP_ERROR);
    
    RETURN_LONG(qconfzk->zk_service_offline(std::string(path, path_len), std::string(serv, serv_len)));
}

static PHP_METHOD(QConfZK, serviceClear)
{
    char *path = NULL;
    int path_len = 0;

    if (zend_parse_parameters(ZEND_NUM_ARGS() TSRMLS_CC, "s", &path, &path_len) == FAILURE) 
    {
        RETURN_LONG(QCONF_PHP_ERROR);
    }

    QConfZK *qconfzk = NULL;
    qconfzk_object *obj = (qconfzk_object *)zend_object_store_get_object(
            getThis() TSRMLS_CC);
    qconfzk = obj->qconfzk;
    if (qconfzk != NULL) {
        RETURN_LONG(qconfzk->zk_service_clear(std::string(path, path_len)));
    }
    
    RETURN_LONG(QCONF_PHP_ERROR);
}

static PHP_METHOD(QConfZK, list)
{
    char *path = NULL;
    int path_len = 0;

    if (zend_parse_parameters(ZEND_NUM_ARGS() TSRMLS_CC, "s", &path, &path_len) == FAILURE) 
    {
        RETURN_NULL();
    }

    // Get C++ object
    QConfZK *qconfzk = NULL;
    qconfzk_object *obj = (qconfzk_object *)zend_object_store_get_object(
            getThis() TSRMLS_CC);
    qconfzk = obj->qconfzk;
    if (qconfzk == NULL) RETURN_NULL();
    
    int ret = QCONF_PHP_ERROR;
    std::set<std::string> children;
    ret = qconfzk->zk_list(std::string(path, path_len), children);
    if (QCONF_OK != ret) RETURN_NULL();
    
    zval *return_data = NULL;                    
    MAKE_STD_ZVAL(return_data);                
    array_init(return_data); 
    std::set<std::string>::const_iterator it;
    int i = 0;
    for (it = children.begin(); it != children.end(); ++it, ++i)
    {
        add_index_string(return_data, i, const_cast<char*>((*it).c_str()), 1); 
    }
    *return_value = *return_data;
    return;
}

static PHP_METHOD(QConfZK, listWithValue)
{
    char *path = NULL;
    int path_len = 0;

    if (zend_parse_parameters(ZEND_NUM_ARGS() TSRMLS_CC, "s", &path, &path_len) == FAILURE) 
    {
        RETURN_NULL();
    }

    QConfZK *qconfzk = NULL;
    qconfzk_object *obj = (qconfzk_object *)zend_object_store_get_object(
            getThis() TSRMLS_CC);
    qconfzk = obj->qconfzk;
    if (qconfzk == NULL)
        RETURN_NULL();
    
    int ret = QCONF_PHP_ERROR;
    std::map<std::string, std::string> children;
    ret = qconfzk->zk_list_with_values(std::string(path, path_len), children);
    if (QCONF_OK != ret) RETURN_NULL();
    
    zval *return_data = NULL;                    
    MAKE_STD_ZVAL(return_data);                
    array_init(return_data); 

    std::map<std::string, std::string>::iterator it = children.begin();
    for (int i = 0; it != children.end(); ++it, ++i)
    {
        add_assoc_string(return_data, const_cast<char*>((it->first).c_str()), (char*)(it->second).c_str(), 1); 
    }
    *return_value = *return_data;
    return;
}

static PHP_METHOD(QConfZK, grayBegin)
{
    zval *apath = NULL, *amachine = NULL;
    zval **z_item = NULL;
    int paths_count = 0, machine_count = 0, i = 0;
    std::map<std::string, std::string> mpaths;
    std::vector< std::string > mmachines;

    if (zend_parse_parameters(ZEND_NUM_ARGS() TSRMLS_CC, "aa", &apath, &amachine) == FAILURE) 
    {
        RETURN_NULL();
    }

    // paths value map
    paths_count = zend_hash_num_elements(Z_ARRVAL_P(apath));
    zend_hash_internal_pointer_reset(Z_ARRVAL_P(apath)); 
    for (i = 0; i < paths_count; i++) 
    {
        char* key;
        unsigned long idx;
        zend_hash_get_current_data(Z_ARRVAL_P(apath), (void**) &z_item);
        convert_to_string_ex(z_item);
        if (zend_hash_get_current_key(Z_ARRVAL_P(apath), &key, &idx, 0) != HASH_KEY_IS_STRING) 
            RETURN_NULL();
        mpaths.insert(std::pair<std::string, std::string>(key, Z_STRVAL_PP(z_item)));
        zend_hash_move_forward(Z_ARRVAL_P(apath));
    }

    // machine vector
    machine_count = zend_hash_num_elements(Z_ARRVAL_P(amachine));
    zend_hash_internal_pointer_reset(Z_ARRVAL_P(amachine)); 
    for (i = 0; i < machine_count; i++) 
    {
        char* key;
        unsigned long idx;
        zend_hash_get_current_data(Z_ARRVAL_P(amachine), (void**) &z_item);
        convert_to_string_ex(z_item);
        if (zend_hash_get_current_key(Z_ARRVAL_P(amachine), &key, &idx, 0) == HASH_KEY_IS_STRING) 
            RETURN_NULL()
        mmachines.push_back(Z_STRVAL_PP(z_item));
        zend_hash_move_forward(Z_ARRVAL_P(amachine));
    }

    QConfZK *qconfzk = NULL;
    std::string gray_id;
    qconfzk_object *obj = (qconfzk_object *)zend_object_store_get_object(
            getThis() TSRMLS_CC);
    qconfzk = obj->qconfzk;
    if (qconfzk != NULL) {
        if (QCONF_OK == qconfzk->zk_gray_begin(mpaths, mmachines, gray_id))
            RETURN_STRINGL(const_cast<char*>(gray_id.c_str()), gray_id.size(), 1);
    }
    
    RETURN_NULL();
}

static PHP_METHOD(QConfZK, grayRollback)
{
    char *gray_id = NULL;
    int gray_id_len = 0;

    if (zend_parse_parameters(ZEND_NUM_ARGS() TSRMLS_CC, "s", &gray_id, &gray_id_len) == FAILURE) 
    {
        RETURN_LONG(QCONF_PHP_ERROR);
    }

    QConfZK *qconfzk = NULL;
    qconfzk_object *obj = (qconfzk_object *)zend_object_store_get_object(
            getThis() TSRMLS_CC);
    qconfzk = obj->qconfzk;
    if (qconfzk != NULL) {
        RETURN_LONG(qconfzk->zk_gray_rollback(std::string(gray_id, gray_id_len)));
    }
    
    RETURN_LONG(QCONF_PHP_ERROR);
}

static PHP_METHOD(QConfZK, grayCommit)
{
    char *gray_id = NULL;
    int gray_id_len = 0;

    if (zend_parse_parameters(ZEND_NUM_ARGS() TSRMLS_CC, "s", &gray_id, &gray_id_len) == FAILURE) 
    {
        RETURN_LONG(QCONF_PHP_ERROR);
    }

    QConfZK *qconfzk = NULL;
    qconfzk_object *obj = (qconfzk_object *)zend_object_store_get_object(
            getThis() TSRMLS_CC);
    qconfzk = obj->qconfzk;
    if (qconfzk != NULL) {
        RETURN_LONG(qconfzk->zk_gray_commit(std::string(gray_id, gray_id_len)));
    }
    
    RETURN_LONG(QCONF_PHP_ERROR);
}

/* }}} */



/****************************************
  Internal support code
 ****************************************/

/* {{{ PHP_MSHUTDOWN_FUNCTION
 */
PHP_MSHUTDOWN_FUNCTION(qconf_manager)
{
	/* uncomment this line if you have INI entries
	UNREGISTER_INI_ENTRIES();
	*/
	return SUCCESS;
}
/* }}} */

/* {{{ PHP_MINFO_FUNCTION
 */
PHP_MINFO_FUNCTION(qconf_manager)
{
	php_info_print_table_start();
	php_info_print_table_header(2, "qconf_manager support", "enabled");
	php_info_print_table_row(2, "qconf_manager version", PHP_QCONF_MANAGER_VERSION);
	php_info_print_table_end();

	/* Remove comments if you have entries in php.ini
	DISPLAY_INI_ENTRIES();
	*/
}
/* }}} */

/* {{{ QConfZK_functions[]
 *
 * Every user visible function must have an entry in qconf_manager_functions[].
 */
zend_function_entry qconfzk_methods[] = {
    PHP_ME(QConfZK,  __construct,           NULL, ZEND_ACC_PUBLIC | ZEND_ACC_CTOR)
    PHP_ME(QConfZK,  __destruct,            NULL, ZEND_ACC_PUBLIC | ZEND_ACC_DTOR)
    PHP_ME(QConfZK,  nodeGet,               NULL, ZEND_ACC_PUBLIC)
    PHP_ME(QConfZK,  nodeSet,               NULL, ZEND_ACC_PUBLIC)
    PHP_ME(QConfZK,  nodeDelete,            NULL, ZEND_ACC_PUBLIC)
    PHP_ME(QConfZK,  servicesGet,           NULL, ZEND_ACC_PUBLIC)
    PHP_ME(QConfZK,  servicesSet,           NULL, ZEND_ACC_PUBLIC)
    PHP_ME(QConfZK,  servicesGetWithStatus, NULL, ZEND_ACC_PUBLIC)
    PHP_ME(QConfZK,  serviceAdd,            NULL, ZEND_ACC_PUBLIC)
    PHP_ME(QConfZK,  serviceDelete,         NULL, ZEND_ACC_PUBLIC)
    PHP_ME(QConfZK,  serviceUp,             NULL, ZEND_ACC_PUBLIC)
    PHP_ME(QConfZK,  serviceDown,           NULL, ZEND_ACC_PUBLIC)
    PHP_ME(QConfZK,  serviceOffline,        NULL, ZEND_ACC_PUBLIC)
    PHP_ME(QConfZK,  serviceClear,          NULL, ZEND_ACC_PUBLIC)
    PHP_ME(QConfZK,  list,                  NULL, ZEND_ACC_PUBLIC)
    PHP_ME(QConfZK,  listWithValue,         NULL, ZEND_ACC_PUBLIC)
    PHP_ME(QConfZK,  grayBegin,             NULL, ZEND_ACC_PUBLIC)
    PHP_ME(QConfZK,  grayRollback,          NULL, ZEND_ACC_PUBLIC)
    PHP_ME(QConfZK,  grayCommit,            NULL, ZEND_ACC_PUBLIC)
    {NULL, NULL, NULL}	/* Must be the last line in qconf_manager_functions[] */
};
/* }}} */

/* {{{ PHP_MINIT_FUNCTION
 */
PHP_MINIT_FUNCTION(qconf_manager)
{
    zend_class_entry ce;
    INIT_CLASS_ENTRY(ce, "QConfZK", qconfzk_methods);
    qconfzk_ce = zend_register_internal_class(&ce TSRMLS_CC);
    qconfzk_ce->create_object = qconfzk_create_handler;
    memcpy(&qconfzk_object_handlers,
    zend_get_std_object_handlers(), sizeof(zend_object_handlers));
    qconfzk_object_handlers.clone_obj = NULL;

    //Const value
    REGISTER_LONG_CONSTANT("QCONF_STATUS_UP", 0, CONST_CS | CONST_PERSISTENT);
    REGISTER_LONG_CONSTANT("QCONF_STATUS_OFFLINE", 1, CONST_CS | CONST_PERSISTENT);
    REGISTER_LONG_CONSTANT("QCONF_STATUS_DOWN", 2, CONST_CS | CONST_PERSISTENT);
	return SUCCESS;
}

/* }}} */
/* {{{ qconf_manager_module_entry
 */
zend_module_entry qconf_manager_module_entry = {
#if ZEND_MODULE_API_NO >= 20010901
    STANDARD_MODULE_HEADER,
#endif
	"qconf_manager",
	NULL,
	PHP_MINIT(qconf_manager),
	PHP_MSHUTDOWN(qconf_manager),
	NULL,		
	NULL,
	PHP_MINFO(qconf_manager),
	PHP_QCONF_MANAGER_VERSION,
	STANDARD_MODULE_PROPERTIES
};
/* }}} */

#ifdef COMPILE_DL_QCONF_MANAGER
extern "C"{
ZEND_GET_MODULE(qconf_manager)
}
#endif

/*
 * Local variables:
 * tab-width: 4
 * c-basic-offset: 4
 * End:
 * vim600: noet sw=4 ts=4 fdm=marker
 * vim<600: noet sw=4 ts=4
 */
