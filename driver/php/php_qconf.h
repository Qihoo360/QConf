/* $ Id: $ */

#ifndef PHP_QCONF_H
#define PHP_QCONF_H

#include <php.h>

#ifdef HAVE_CONFIG_H
# include "config.h"
#endif

#define QCONF_DRIVER_PHP_VERSION 		"1.2.2"
#define QCONF_WAIT                      0
#define QCONF_NOWAIT                    1 

extern zend_module_entry qconf_module_entry;
#define phpext_qconf_ptr &qconf_module_entry

#ifdef PHP_WIN32
#define PHP_QCONF_API __declspec(dllexport)
#else
#define PHP_QCONF_API
#endif

ZEND_BEGIN_MODULE_GLOBALS(php_qconf)
	HashTable callbacks;
	zend_bool session_lock;
	long sess_lock_wait;
	long max_times; 		// max_repeat_read_times
ZEND_END_MODULE_GLOBALS(php_qconf)

PHP_MINIT_FUNCTION(qconf);
PHP_MSHUTDOWN_FUNCTION(qconf);
PHP_RSHUTDOWN_FUNCTION(qconf);
PHP_MINFO_FUNCTION(qconf);

ZEND_EXTERN_MODULE_GLOBALS(php_qconf)

#ifdef ZTS
#define QCONF_G(v) TSRMG(php_qconf_globals_id, zend_php_qconf_globals *, v)
#else
#define QCONF_G(v) (php_qconf_globals.v)
#endif

#endif /* PHP_QCONF_H */


/*
 * Local variables:
 * tab-width: 4
 * c-basic-offset: 4
 * End:
 * vim600: noet sw=4 ts=4 fdm=marker
 * vim<600: noet sw=4 ts=4
 */
