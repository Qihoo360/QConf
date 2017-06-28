/* $Id$ */

#ifndef PHP_QCONF_MANAGER_H
#define PHP_QCONF_MANAGER_H

extern "C"{
#include "php.h"
}

#define PHP_QCONF_MANAGER_VERSION "1.0.0" /* Replace with version number for your extension */

extern zend_module_entry qconf_manager_module_entry;
#define phpext_qconf_manager_ptr &qconf_manager_module_entry


#ifdef PHP_WIN32
#	define PHP_QCONF_MANAGER_API __declspec(dllexport)
#elif defined(__GNUC__) && __GNUC__ >= 4
#	define PHP_QCONF_MANAGER_API __attribute__ ((visibility("default")))
#else
#	define PHP_QCONF_MANAGER_API
#endif

PHP_MINIT_FUNCTION(qconf_manager);
PHP_MSHUTDOWN_FUNCTION(qconf_manager);
PHP_MINFO_FUNCTION(qconf_manager);

#ifdef ZTS
#include "TSRM.h"
#endif


#ifdef ZTS
#define QCONF_MANAGER_G(v) TSRMG(qconf_manager_globals_id, zend_qconf_manager_globals *, v)
#else
#define QCONF_MANAGER_G(v) (qconf_manager_globals.v)
#endif



#endif	/* PHP_QCONF_MANAGER_H */


/*
 * Local variables:
 * tab-width: 4
 * c-basic-offset: 4
 * End:
 * vim600: noet sw=4 ts=4 fdm=marker
 * vim<600: noet sw=4 ts=4
 */
