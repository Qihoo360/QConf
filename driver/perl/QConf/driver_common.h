#ifndef DRIVER_COMMON_H
#define DRIVER_COMMON_H

#ifdef __cplusplus
extern "C" {
#endif

#include <stdio.h>

#ifndef __QCONF_BATCH_NODES__
#define __QCONF_BATCH_NODES__
typedef struct qconf_node
{
    char *key;
    char *value;
} qconf_node;

typedef struct qconf_batch_nodes
{
    int count;
    qconf_node *nodes;
} qconf_batch_nodes;
#endif


/**
 * Free qconf_batch_nodes according to the free_size
 */
void free_qconf_batch_nodes(qconf_batch_nodes *bnodes, size_t free_size);

/**
 * Init the nodes array for keeping batch conf
 * @Note: the function should be called before calling qconf_get_batchconf
 *
 * @param bnodes: the array for keeping batch nodes
 *
 * @return QCONF_Ok: if success
 *         QCONF_ERR_PARAM: if nodes is null
 *         QCONF_ERR_OTHER: other failed
 */
int init_qconf_batch_nodes(qconf_batch_nodes *bnodes);

/**
 * Destroy the array for keeping batch nodes
 * @Note: the function should be called after the last use of calling qconf_get_batchconf
 *
 * @param bnodes: the array for keeping batch nodes
 *
 * @return QCONF_Ok: if success
 *         QCONF_ERR_PARAM: if nodes is null
 *         QCONF_ERR_OTHER: other failed
 */
int destroy_qconf_batch_nodes(qconf_batch_nodes *bnodes);

#ifdef __cplusplus
}
#endif

#endif
