#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "qconf_errno.h"
#include "driver_common.h"

int init_qconf_batch_nodes(qconf_batch_nodes *bnodes)
{
    if (NULL == bnodes) return QCONF_ERR_PARAM;

    memset((void*)bnodes, 0, sizeof(qconf_batch_nodes));

    return QCONF_OK;
}

int destroy_qconf_batch_nodes(qconf_batch_nodes *bnodes)
{
    if (NULL == bnodes) return QCONF_ERR_PARAM;

    free_qconf_batch_nodes(bnodes, bnodes->count);

    return QCONF_OK;
}

void free_qconf_batch_nodes(qconf_batch_nodes *bnodes, size_t free_size)
{
    if (NULL == bnodes) return;

    for (size_t i = 0; i < free_size; ++i)
    {
        free(bnodes->nodes[i].key);
        free(bnodes->nodes[i].value);
        bnodes->nodes[i].key = NULL;
        bnodes->nodes[i].value = NULL;
    }
    free(bnodes->nodes);
    bnodes->nodes = NULL;
    bnodes->count = 0;
}
