#ifndef QCONF_H
#define QCONF_H

#ifdef __cplusplus
extern "C" {
#endif

#include "driver_common.h"
#include "qconf_errno.h"

// agent version
#define QCONF_DRIVER_CC_VERSION "1.0.0"

// The max length of conf
#define QCONF_CONF_BUF_MAX_LEN   (1024*1024)
// The max length of one host
#define QCONF_HOST_BUF_MAX_LEN   256

/**
 * The array for keeping the services
 */
#ifndef __STRING_VECTOR_T_FLAGS__
#define __STRING_VECTOR_T_FLAGS__
typedef struct
{
    int count;      // the number of services
    char **data;    // the array of services
} string_vector_t;
#endif

/**
 * Init qconf environment
 * @Note: the function should be called before using qconf
 *
 * @return QCONF_OK: if success
 *         QCONF_ERR_OTHER: if failed
 */
int qconf_init();

/**
 * Destroy qconf environment
 * @Note: the function should only be called before process exit
 *
 * @return QCONF_OK: if success
 *         QCONF_ERR_OTHER: if failed
 */
int qconf_destroy();

/**
 * Init the array for keeping services
 * @Note: the function should be called before calling qconf_get_allhost
 *
 * @param nodes: the array for keeping services
 *
 * @return QCONF_Ok: if success
 *         QCONF_ERR_PARAM: if nodes is null
 *         QCONF_ERR_OTHER: other failed
 */
int init_string_vector(string_vector_t *nodes);

/**
 * Destroy the array for keeping services
 * @Note: the function should be called after the last use of calling qconf_get_allhost
 *
 * @param nodes: the array for keeping services
 *
 * @return QCONF_Ok: if success
 *         QCONF_ERR_PARAM: if nodes is null
 *         QCONF_ERR_OTHER: other failed
 */
int destroy_string_vector(string_vector_t *nodes);

/**
 * Synchronize get the value of key which is path
 *
 * @param path: the key of the value
 * @param buf: the buffer for keeping the value,
 * @param buf_len: the length of buf
 * @param idc: the place to get value;
 *             NULL is default value
 *
 * @return QCONF_OK: if success
 *         QCONF_ERR_NOT_FOUND: if the key not exists
 *         QCONF_ERR_OTHER: other failed
 */
int qconf_get_conf(const char *path, char *buf, unsigned int buf_len, const char *idc);

/**
 * Synchronize get all children nodes of key which is path
 *
 * @param path: the key of the children nodes
 * @param nodes: the array for keeping all children nodes including nodes' key and value
 * @param idc: the place to get children nodes;
 *             NULL is default value
 *
 * @return QCONF_OK: if success
 *         QCONF_ERR_NOT_FOUND: if the key not exists
 *         QCONF_ERR_OTHER: other failed
 */
int qconf_get_batch_conf(const char *path, qconf_batch_nodes *bnodes, const char *idc);

/**
 * Synchronize get all children nodes' key of path
 *
 * @param path: the key of the children nodes
 * @param nodes: the array for keeping all children nodes' key
 * @param idc: the place to get children nodes;
 *             NULL is default value
 *
 * @return QCONF_OK: if success
 *         QCONF_ERR_NOT_FOUND: if the key not exists
 *         QCONF_ERR_OTHER: other failed
 */
int qconf_get_batch_keys(const char *path, string_vector_t *nodes, const char *idc);

/**
 * Synchronize get all services of key which is path
 *
 * @param path: the key of the services 
 * @param nodes: the array for keeping all the services
 * @param idc: the place to get services;
 *             NULL is default value
 *
 * @return QCONF_OK: if success
 *         QCONF_ERR_NOT_FOUND: if the key not exists
 *         QCONF_ERR_OTHER: other failed
 */
int qconf_get_allhost(const char *path, string_vector_t *nodes, const char *idc);

/**
 * Synchronize get one service of key which is path
 *
 * @param path: the key of the service 
 * @param buf: the buffer for keeping the service,
 * @param buf_len: the length of buf
 * @param idc: the place to get service;
 *             NULL is default value
 *
 * @return QCONF_OK: if success
 *         QCONF_ERR_NOT_FOUND: if the key not exists
 *         QCONF_ERR_OTHER: other failed
 */
int qconf_get_host(const char *path, char *buf, unsigned int buf_len, const char *idc);


/**
 * Asynchronize get the value of key which is path
 *
 * @param path: the key of the value
 * @param buf: the buffer for keeping the value,
 * @param buf_len: the length of buf
 * @param idc: the place to get value;
 *             NULL is default value
 *
 * @return QCONF_OK: if success
 *         QCONF_ERR_NOT_FOUND: if the key not exists
 *         QCONF_ERR_OTHER: other failed
 */
int qconf_aget_conf(const char *path, char *buf, unsigned int buf_len, const char *idc);

/**
 * Asynchronize get all children nodes of key which is path
 *
 * @param path: the key of the children nodes
 * @param nodes: the array for keeping all children nodes
 * @param idc: the place to get children nodes;
 *             NULL is default value
 *
 * @return QCONF_OK: if success
 *         QCONF_ERR_NOT_FOUND: if the key not exists
 *         QCONF_ERR_OTHER: other failed
 */
int qconf_aget_batch_conf(const char *path, qconf_batch_nodes *bnodes, const char *idc);

/**
 * Asynchronize get all children nodes' key of path
 *
 * @param path: the key of the children nodes
 * @param nodes: the array for keeping all children nodes' key
 * @param idc: the place to get children nodes;
 *             NULL is default value
 *
 * @return QCONF_OK: if success
 *         QCONF_ERR_NOT_FOUND: if the key not exists
 *         QCONF_ERR_OTHER: other failed
 */
int qconf_aget_batch_keys(const char *path, string_vector_t *nodes, const char *idc);

/**
 * Asynchronize get all services of key which is path
 *
 * @param path: the key of the services 
 * @param nodes: the array for keeping all the services
 * @param idc: the place to get services;
 *             NULL is default value
 *
 * @return QCONF_OK: if success
 *         QCONF_ERR_NOT_FOUND: if the key not exists
 *         QCONF_ERR_OTHER: other failed
 */
int qconf_aget_allhost(const char *path, string_vector_t *nodes, const char *idc);

/**
 * Asynchronize get one service of key which is path
 *
 * @param path: the key of the service 
 * @param buf: the buffer for keeping the service,
 * @param buf_len: the length of buf
 * @param idc: the place to get service;
 *             NULL is default value
 *
 * @return QCONF_OK: if success
 *         QCONF_ERR_NOT_FOUND: if the key not exists
 *         QCONF_ERR_OTHER: other failed
 */
int qconf_aget_host(const char *path, char *buf, unsigned int buf_len, const char *idc);

/**
 * Synchronize get all children nodes' key of path
 *
 * @param path: the key of the children nodes, and the path will be used directly without adding head "/qconf/"
 * @param nodes: the array for keeping all children nodes' key
 * @param idc: the place to get children nodes;
 *             NULL is default value
 *
 * @return QCONF_OK: if success
 *         QCONF_ERR_NOT_FOUND: if the key not exists
 *         QCONF_ERR_OTHER: other failed
 */
int qconf_get_batch_keys_native(const char *path, string_vector_t *nodes, const char *idc);

/**
 * Asynchronize get all children nodes' key of path
 *
 * @param path: the key of the children nodes, and the path will be used directly without adding head "/qconf/"
 * @param nodes: the array for keeping all children nodes' key
 * @param idc: the place to get children nodes;
 *             NULL is default value
 *
 * @return QCONF_OK: if success
 *         QCONF_ERR_NOT_FOUND: if the key not exists
 *         QCONF_ERR_OTHER: other failed
 */
int qconf_aget_batch_keys_native(const char *path, string_vector_t *nodes, const char *idc);

/**
 * Get the qconf version
 * @Note: it must not change the return string
 *
 * @return: the pointer that pointing to the string of version
 */
const char* qconf_version();

#ifdef __cplusplus
}
#endif

#endif
