/******************************************************************************
 * qLibc - http://www.qdecoder.org
 *
 * Copyright (c) 2010-2012 Seungyoung Kim.
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions are met:
 *
 * 1. Redistributions of source code must retain the above copyright notice,
 *    this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright notice,
 *    this list of conditions and the following disclaimer in the documentation
 *    and/or other materials provided with the distribution.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
 * AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT OWNER OR CONTRIBUTORS BE
 * LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
 * CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
 * SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
 * INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
 * CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
 * ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
 * POSSIBILITY OF SUCH DAMAGE.
 ******************************************************************************
 * $Id: qlibc.h 126 2012-06-11 22:20:01Z seungyoung.kim $
 ******************************************************************************/

/**
 * qlibc header file
 *
 * @file qlibc.h
 */

#ifndef _QLIBC_H
#define _QLIBC_H

#define _Q_PRGNAME "qlibc"  /*!< qlibc human readable name */
#define _Q_VERSION "2.1.0"  /*!< qlibc version number string */

#ifdef __cplusplus
extern "C" {
#endif

#include <stdint.h>
#include <pthread.h>

/******************************************************************************
 * COMMON DATA STRUCTURES
 ******************************************************************************/

typedef struct qmutex_s qmutex_t;    /*!< qlibc pthread mutex type*/
typedef struct qobj_s qobj_t;        /*!< object type*/
typedef struct qnobj_s qnobj_t;      /*!< named-object type*/
typedef struct qdlobj_s qdlobj_t;    /*!< doubly-linked-object type*/
typedef struct qdlnobj_s qdlnobj_t;  /*!< doubly-linked-named-object type*/
typedef struct qhnobj_s qhnobj_t;    /*!< hashed-named-object type*/

/**
 * qlibc pthread mutex data structure.
 */
struct qmutex_s
{
    pthread_mutex_t mutex;  /*!< pthread mutex */
    pthread_t owner;        /*!< mutex owner thread id */
    int count;              /*!< recursive lock counter */
};

/**
 * object data structure.
 */
struct qobj_s
{
    void *data;         /*!< data */
    size_t size;        /*!< data size */
    uint8_t type;       /*!< data type */
};

/**
 * named-object data structure.
 */
struct qnobj_s
{
    char *name;         /*!< object name */
    void *data;         /*!< data */
    size_t name_size;   /*!< object name size */
    size_t data_size;   /*!< data size */
};

/**
 * doubly-linked-object data structure.
 */
struct qdlobj_s
{
    void *data;         /*!< data */
    size_t size;        /*!< data size */

    qdlobj_t *prev;     /*!< previous link */
    qdlobj_t *next;     /*!< next link */
};

/**
 * doubly-linked-named-object data structure.
 */
struct qdlnobj_s
{
    uint32_t hash;      /*!< 32bit-hash value of object name */
    char *name;         /*!< object name */
    void *data;         /*!< data */
    size_t size;        /*!< data size */

    qdlnobj_t *prev;    /*!< previous link */
    qdlnobj_t *next;    /*!< next link */
};

/**
 * hashed-named-object data structure.
 */
struct qhnobj_s
{
    uint32_t hash;      /*!< 32bit-hash value of object name */
    char *name;         /*!< object name */
    void *data;         /*!< data */
    size_t size;        /*!< data size */

    qhnobj_t *next;     /*!< for chaining next collision object */
};

/******************************************************************************
 * Static Hash Table Container - works in fixed size memory
 * qhasharr.c
 ******************************************************************************/

/* tunable knobs */
#define _Q_HASHARR_KEYSIZE (32)    /*!< knob for maximum key size. */
#define _Q_HASHARR_VALUESIZE (96)  /*!< knob for maximum data size in a slot. */

/* types */
typedef struct qhasharr_s qhasharr_t;
typedef struct qhasharr_slot_s qhasharr_slot_t;

/* public functions */
extern qhasharr_t *qhasharr(void *memory, size_t memsize);
extern size_t qhasharr_calculate_memsize(int max);
extern int qhasharr_init(qhasharr_t *tbl, qhasharr_slot_t **_tbl_slots);

/* capsulated member functions */
extern bool qhasharr_put(qhasharr_t *tbl, const char *key, size_t key_size, const void *value, size_t val_size);
extern bool qhasharr_putstr(qhasharr_t *tbl, const char *key, const char *str);
extern bool qhasharr_putint(qhasharr_t *tbl, const char *key, int64_t num);
extern bool qhasharr_exist(qhasharr_t *tbl, const char *key, size_t key_size);
extern void *qhasharr_get(qhasharr_t *tbl, const char *key, size_t key_size, size_t *val_size);
extern char *qhasharr_getstr(qhasharr_t *tbl, const char *key);
extern int64_t qhasharr_getint(qhasharr_t *tbl, const char *key);
extern bool qhasharr_getnext(qhasharr_t *tbl, qnobj_t *obj, int *idx);

extern bool qhasharr_remove(qhasharr_t *tbl, const char *key, size_t key_size);

extern int  qhasharr_size(qhasharr_t *tbl, int *maxslots, int *usedslots);
extern void qhasharr_clear(qhasharr_t *tbl);

union _slot_data
{
    /*!< key/value data */
    struct _Q_HASHARR_SLOT_KEYVAL
    {
        unsigned char value[_Q_HASHARR_VALUESIZE];  /*!< value */

        char key[_Q_HASHARR_KEYSIZE];  /*!< key string, can be cut */
        uint16_t  keylen;              /*!< original key length */
        unsigned char keymd5[16];      /*!< md5 hash of the key */
    } pair;

    /*!< extended data block, used only when the count value is -2 */
    struct _Q_HASHARR_SLOT_EXT
    {
        unsigned char value[sizeof(struct _Q_HASHARR_SLOT_KEYVAL)];
    } ext;
} ;

/**
 * qhasharr internal data slot structure
 */
struct qhasharr_slot_s
{
    short  count;   /*!< hash collision counter. 0 indicates empty slot,
                     -1 is used for collision resolution, -2 is used for
                     indicating linked block */
    uint32_t  hash; /*!< key hash. we use FNV32 */

    uint8_t size;   /*!< value size in this slot*/
    int link;       /*!< next link */
    union _slot_data data;
};

/**
 * qhasharr container
 */
struct qhasharr_s
{
    /* private variables - do not access directly */
    int maxslots;       /*!< number of maximum slots */
    int usedslots;      /*!< number of used slots */
    int num;            /*!< number of stored keys */
    char slots[];       /*!< data area pointer */
};

/******************************************************************************
 * UTILITIES SECTION
 ******************************************************************************/

/* qhash.c */
extern bool qhashmd5(const void *data, size_t nbytes, void *retbuf);
extern uint32_t qhashmurmur3_32(const void *data, size_t nbytes);

/**
 * translate the binary of md5 into the string of md5
 */
extern int qhashmd5_bin_to_hex(char *md5_str, const unsigned char *md5_int, int md5_int_len);

#ifdef __cplusplus
}
#endif

#endif /*_QLIBC_H */
