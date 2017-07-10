/*
  +----------------------------------------------------------------------+
  | PHP Version 7                                                        |
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

void qconfzk_free_storage(zend_object *object TSRMLS_DC)
{
    qconfzk_object *obj = (qconfzk_object *)object;
    delete obj->qconfzk;
    // zend_hash_destroy(obj->std.properties);
    // FREE_HASHTABLE(obj->std.properties);
    efree(obj);
}

static inline struct qconfzk_object * php_custom_object_fetch_object(zend_object *obj) {
      return (struct qconfzk_object *)((char *)obj - XtOffsetOf(struct qconfzk_object , std));
}

#define Z_QCONFZK_OBJ_P(zv) php_custom_object_fetch_object(Z_OBJ_P(zv));

zend_object * qconfzk_create_handler(zend_class_entry *ce TSRMLS_DC) {
     // Allocate sizeof(custom) + sizeof(properties table requirements)
	struct qconfzk_object *intern = (struct qconfzk_object *)ecalloc(
		1,
		sizeof(struct qconfzk_object) +
		zend_object_properties_size(ce));
     // Allocating:
     // struct custom_object {
	 //    void *custom_data;
	 //    zend_object std;
	 // }
     // zval[ce->default_properties_count-1]
     zend_object_std_init(&intern->std, ce TSRMLS_CC);
     qconfzk_object_handlers.offset = XtOffsetOf(struct qconfzk_object, std);
     qconfzk_object_handlers.free_obj = qconfzk_free_storage;

     intern->std.handlers = &qconfzk_object_handlers;

     return &intern->std;
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

    if (zend_parse_parameters(ZEND_NUM_ARGS() TSRMLS_CC, "s", &host, &host_len) == FAILURE)
    {
        RETURN_NULL();
    }

    QConfZK *qconfZk = new QConfZK();
    qconfZk->zk_init(std::string(host, host_len));

    qconfzk_object *obj = Z_QCONFZK_OBJ_P(getThis());
    obj->qconfzk = qconfZk;
}
/* }}} */

/* {{{ QConfZK::__construct ( .. )
   Creates a QConfZK object */
static PHP_METHOD(QConfZK, __destruct)
{
    QConfZK *qconfzk = NULL;
    qconfzk_object *obj = Z_QCONFZK_OBJ_P(getThis());
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
    qconfzk_object *obj = Z_QCONFZK_OBJ_P(getThis());
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
    qconfzk_object *obj = Z_QCONFZK_OBJ_P(getThis());
    qconfzk = obj->qconfzk;
    if (qconfzk != NULL) {
        std::string value;
        if (QCONF_OK != qconfzk->zk_node_get(std::string(path, path_len), value)) RETURN_NULL();
        RETURN_STRINGL(const_cast<char*>(value.c_str()), value.size());
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
    qconfzk_object *obj = Z_QCONFZK_OBJ_P(getThis());
    qconfzk = obj->qconfzk;
    if (qconfzk != NULL) {
        RETURN_LONG(qconfzk->zk_node_delete(std::string(path, path_len)));
    }

    RETURN_LONG(QCONF_PHP_ERROR);
}

static PHP_METHOD(QConfZK, servicesSet)
{
    char *path = NULL;
    zval *array_input, *val;
	ulong num_key;
    int path_len = 0;
    std::map<std::string, char> mserv;

    if (zend_parse_parameters(ZEND_NUM_ARGS() TSRMLS_CC, "sa", &path, &path_len, &array_input) == FAILURE)
    {
        RETURN_LONG(QCONF_PHP_ERROR);
    }

	HashTable *arr_hash = Z_ARRVAL_P(array_input);
	zend_string *key;
	ZEND_HASH_FOREACH_KEY_VAL(arr_hash, num_key, key, val) {
		if (!key || !isascii(Z_LVAL_P(val))) {
			RETURN_LONG(QCONF_PHP_ERROR);
		}
		mserv.insert(std::make_pair(std::string(key->val, key->len), Z_LVAL_P(val)));
	} ZEND_HASH_FOREACH_END();

    QConfZK *qconfzk = NULL;
    qconfzk_object *obj = Z_QCONFZK_OBJ_P(getThis());
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
    qconfzk_object *obj = Z_QCONFZK_OBJ_P(getThis());
    qconfzk = obj->qconfzk;
    if (qconfzk == NULL) RETURN_NULL();

    int ret = QCONF_PHP_ERROR;
    std::set<std::string> servs;
    ret = qconfzk->zk_services_get(std::string(path, path_len), servs);
    if (QCONF_OK != ret) RETURN_NULL();

    array_init(return_value);
    std::set<std::string>::const_iterator it;
    int i = 0;
    for (it = servs.begin(); it != servs.end(); ++it, ++i)
    {
        add_index_string(return_value, i, const_cast<char*>((*it).c_str()));
    }
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
    qconfzk_object *obj = Z_QCONFZK_OBJ_P(getThis());
    qconfzk = obj->qconfzk;
    if (qconfzk == NULL)
        RETURN_NULL();

    int ret = QCONF_PHP_ERROR;
    std::map<std::string, char> servs;
    ret = qconfzk->zk_services_get_with_status(std::string(path, path_len), servs);
    if (QCONF_OK != ret) RETURN_NULL();

    array_init(return_value);

    std::map<std::string, char>::iterator it = servs.begin();
    for (int i = 0; it != servs.end(); ++it, ++i)
    {
        add_assoc_long(return_value, const_cast<char*>((it->first).c_str()), it->second);
    }
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
    qconfzk_object *obj = Z_QCONFZK_OBJ_P(getThis());
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
    qconfzk_object *obj = Z_QCONFZK_OBJ_P(getThis());
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
    qconfzk_object *obj = Z_QCONFZK_OBJ_P(getThis());
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
    qconfzk_object *obj = Z_QCONFZK_OBJ_P(getThis());
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
    qconfzk_object *obj = Z_QCONFZK_OBJ_P(getThis());
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
    qconfzk_object *obj = Z_QCONFZK_OBJ_P(getThis());
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
    qconfzk_object *obj = Z_QCONFZK_OBJ_P(getThis());
    qconfzk = obj->qconfzk;
    if (qconfzk == NULL) RETURN_NULL();

    int ret = QCONF_PHP_ERROR;
    std::set<std::string> children;
    ret = qconfzk->zk_list(std::string(path, path_len), children);
    if (QCONF_OK != ret) RETURN_NULL();

    array_init(return_value);
    std::set<std::string>::const_iterator it;
    int i = 0;
    for (it = children.begin(); it != children.end(); ++it, ++i)
    {
        add_index_string(return_value, i, const_cast<char*>((*it).c_str()));
    }
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
    qconfzk_object *obj = Z_QCONFZK_OBJ_P(getThis());
    qconfzk = obj->qconfzk;
    if (qconfzk == NULL)
        RETURN_NULL();

    int ret = QCONF_PHP_ERROR;
    std::map<std::string, std::string> children;
    ret = qconfzk->zk_list_with_values(std::string(path, path_len), children);
    if (QCONF_OK != ret) RETURN_NULL();

    array_init(return_value);

    std::map<std::string, std::string>::iterator it = children.begin();
    for (int i = 0; it != children.end(); ++it, ++i)
    {
        add_assoc_string(return_value, const_cast<char*>((it->first).c_str()), (char*)(it->second).c_str());
    }
    return;
}

static PHP_METHOD(QConfZK, grayBegin)
{
    zval *apath = NULL, *amachine = NULL;
	zval *val;
	zend_string* key;
	ulong num_key;
    std::map<std::string, std::string> mpaths;
    std::vector<std::string> mmachines;

    if (zend_parse_parameters(ZEND_NUM_ARGS() TSRMLS_CC, "aa", &apath, &amachine) == FAILURE)
    {
        RETURN_NULL();
    }

    // paths value map
	HashTable *path_arr = Z_ARRVAL_P(apath);
	ZEND_HASH_FOREACH_KEY_VAL(path_arr, num_key, key, val) {
		if (!key) {
			RETURN_NULL();
		}
		mpaths.insert(std::make_pair(std::string(key->val, key->len),
									 std::string(Z_STR_P(val)->val,
												 Z_STR_P(val)->len)));
	} ZEND_HASH_FOREACH_END();

    // machine vector
	HashTable *machine_arr = Z_ARRVAL_P(amachine);
	ZEND_HASH_FOREACH_KEY_VAL(machine_arr, num_key, key, val) {
		if (key) { // HASH_KEY_IS_STRING
			RETURN_NULL();
		}
		mmachines.push_back(std::string(Z_STR_P(val)->val, Z_STR_P(val)->len));
	} ZEND_HASH_FOREACH_END();

    QConfZK *qconfzk = NULL;
    std::string gray_id;
    qconfzk_object *obj = Z_QCONFZK_OBJ_P(getThis());
    qconfzk = obj->qconfzk;
    if (qconfzk != NULL) {
        if (QCONF_OK == qconfzk->zk_gray_begin(mpaths, mmachines, gray_id))
            RETURN_STRINGL(const_cast<char*>(gray_id.c_str()), gray_id.size());
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
    qconfzk_object *obj = Z_QCONFZK_OBJ_P(getThis());
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
    qconfzk_object *obj = Z_QCONFZK_OBJ_P(getThis());
    qconfzk = obj->qconfzk;
    if (qconfzk != NULL) {
        RETURN_LONG(qconfzk->zk_gray_commit(std::string(gray_id, gray_id_len)));
    }

    RETURN_LONG(QCONF_PHP_ERROR);
}

static PHP_METHOD(QConfZK, grayContent)
{
    zend_string *gray_id;
    std::vector<std::pair<std::string, std::string> > nodes;
    std::vector<std::pair<std::string, std::string> >::iterator it;
    array_init(return_value);

    if(zend_parse_parameters(ZEND_NUM_ARGS() TSRMLS_CC, "S", &gray_id) == FAILURE)
    {
        RETURN_LONG(QCONF_PHP_ERROR);
    }

    QConfZK *qconfzk = NULL;
    qconfzk_object *obj = Z_QCONFZK_OBJ_P(getThis());
    qconfzk = obj->qconfzk;
    if(qconfzk != NULL){
        if (QCONF_OK != qconfzk->zk_gray_get_content(ZSTR_VAL(gray_id), nodes)) RETURN_LONG(QCONF_PHP_ERROR);
        for (it = nodes.begin(); it != nodes.end(); ++it)
        {
            add_assoc_string(return_value,(*it).first.c_str(),(char *)(*it).second.c_str());
        }
        RETURN_ZVAL(return_value,0,0);
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
    PHP_ME(QConfZK,  grayContent,           NULL, ZEND_ACC_PUBLIC)
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
