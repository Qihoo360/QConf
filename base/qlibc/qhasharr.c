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
 * $Id: qhasharr.c 99 2012-05-04 22:18:22Z seungyoung.kim $
 ******************************************************************************/

/**
 * @file qhasharr.c Static(array) hash-table implementation.
 *
 * qhasharr implements a hash-table which maps keys to values and stores into
 * fixed size static memory like shared-memory and memory-mapped file.
 * The creator qhasharr() initializes static memory to makes small slots in it.
 * The default slot size factors are defined in _Q_HASHARR_KEYSIZE and
 * _Q_HASHARR_VALUESIZE. And they are applied at compile time.
 *
 * The value part of an element will be stored across several slots if it's size
 * exceeds the slot size. But the key part of an element will be truncated if
 * the size exceeds and it's length and more complex MD5 hash value will be
 * stored with the key. So to look up a particular key, first we find an element
 * which has same hash value. If the key was not truncated, we just do key
 * comparison. But if the key was truncated because it's length exceeds, we do
 * both md5 and key comparison(only stored size) to verify that the key is same.
 * So please be aware of that, theoretically there is a possibility we pick
 * wrong element in case a key exceeds the limit, has same length and MD5 hash
 * with lookup key. But this possibility is extreamly low and almost never
 * happen in practice. If you happpen to want to make sure everything,
 * you set _Q_HASHARR_KEYSIZE big enough at compile time to make sure all keys
 * fits in it.
 *
 * qhasharr hash-table does not support thread-safe. So users should handle
 * race conditions on application side by raising user lock before calling
 * functions which modify the table data.
 *
 * @code
 *  [Data Structure Diagram]
 *
 *  +--[Static Flat Memory Area]-----------------------------------------------+
 *  | +-[Header]---------+ +-[Slot 0]---+ +-[Slot 1]---+        +-[Slot N]---+ |
 *  | |Private table data| |KEY A|DATA A| |KEY B|DATA B|  ....  |KEY N|DATA N| |
 *  | +------------------+ +------------+ +------------+        +------------+ |
 *  +--------------------------------------------------------------------------+
 *
 *  Below diagram shows how a big value is stored.
 *  +--[Static Flat Memory Area------------------------------------------------+
 *  | +--------+ +-[Slot 0]---+ +-[Slot 1]---+ +-[Slot 2]---+ +-[Slot 3]-----+ |
 *  | |TBL INFO| |KEY A|DATA A| |DATA A cont.| |KEY B|DATA B| |DATA A cont.  | |
 *  | +--------+ +------------+ +------------+ +------------+ +--------------+ |
 *  |                      ^~~link~~^     ^~~~~~~~~~link~~~~~~~~~^             |
 *  +--------------------------------------------------------------------------+
 * @endcode
 *
 * @code
 *  // initialize hash-table.
 *  char memory[1000 * 10];
 *  qhasharr_t *tbl = qhasharr(memory, sizeof(memory));
 *  if(tbl == NULL) return;
 *
 *  // insert elements (key duplication does not allowed)
 *  tbl->putstr(tbl, "e1", "a");
 *  tbl->putstr(tbl, "e2", "b");
 *  tbl->putstr(tbl, "e3", "c");
 *
 *  // debug print out
 *  tbl->debug(tbl, stdout);
 *
 *  char *e2 = tbl->getstr(tbl, "e2");
 *  if(e2 != NULL) {
 *     printf("getstr('e2') : %s\n", e2);
 *     free(e2);
 *  }
 * @endcode
 *
 * An example for using hash table over shared memory.
 *
 * @code
 *  [CREATOR SIDE]
 *  int maxslots = 1000;
 *  int memsize = qhasharr_calculate_memsize(maxslots);
 *
 *  // create shared memory
 *  int shmid = qShmInit("/tmp/some_id_file", 'q', memsize, true);
 *  if(shmid < 0) return -1; // creation failed
 *  void *memory = qShmGet(shmid);
 *
 *  // initialize hash-table
 *  qhasharr_t *tbl = qhasharr(memory, memsize);
 *  if(hasharr == NULL) return -1;
 *
 *  (...your codes with your own locking mechanism...)
 *
 *  // destroy shared memory
 *  qShmFree(shmid);
 *
 *  [USER SIDE]
 *  int shmid = qShmGetId("/tmp/some_id_file", 'q');
 *
 *  // Every table data including internal private variables are stored in the
 *  // flat static memory area. So converting the memory pointer to the
 *  // qhasharr_t pointer type is everything we need.
 *  qhasharr_t *tbl = (qhasharr_t*)qShmGet(shmid);
 *
 *  (...your codes with your own locking mechanism...)
 * @endcode
 */
#include <errno.h>
#include <inttypes.h>
#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "qlibc.h"

#ifndef _DOXYGEN_SKIP
// internal usages
static int  _find_empty(qhasharr_t *tbl, int startidx);
static int  _get_idx(qhasharr_t *tbl, const char *key, size_t key_size, unsigned int hash);
static void *_get_data(qhasharr_t *tbl, int idx, size_t *size);
static bool _put_data(qhasharr_t *tbl, int idx, unsigned int hash, const char *key, size_t key_size, const void *value, size_t val_size, int count);
static bool _copy_slot(qhasharr_t *tbl, int idx1, int idx2);
static bool _remove_slot(qhasharr_t *tbl, int idx);
static bool _remove_data(qhasharr_t *tbl, int idx);

#endif

/**
 * Get how much memory is needed for N slots.
 *
 * @param max       a number of maximum internal slots
 *
 * @return memory size needed
 *
 * @note
 *  This can be used for calculating minimum memory size for N slots.
 */
size_t qhasharr_calculate_memsize(int max)
{
    size_t memsize = sizeof(qhasharr_t) + (sizeof(qhasharr_slot_t) * (max));
    return memsize;
}

/**
 * Initialize static hash table
 *
 * @param memory    a pointer of buffer memory.
 * @param memsize   a size of buffer memory.
 *
 * @return qhasharr_t container pointer. structure(same as buffer pointer),
 *  otherwise returns NULL.
 * @retval errno  will be set in error condition.
 *  - EINVAL : Assigned memory is too small. It must bigger enough to allocate
 *  at least 1 slot.
 *
 * @code
 *  // initialize hash-table with 100 slots.
 *  // A single element can take several slots.
 *  char memory[112 * 100];
 *  qhasharr_t *tbl = qhasharr(memory, sizeof(memory));
 * @endcode
 *
 * @note
 *  Every information is stored in user memory. So the returning container
 *  pointer exactly same as memory pointer.
 */
qhasharr_t *qhasharr(void *memory, size_t memsize)
{
    if (NULL == memory) return (qhasharr_t *)memory;

    // calculate max
    int maxslots = (memsize - sizeof(qhasharr_t)) / sizeof(qhasharr_slot_t);
    if (maxslots < 1 || memsize <= sizeof(qhasharr_t))
    {
        errno = EINVAL;
        return NULL;
    }

    // clear memory
    qhasharr_t *tbl = (qhasharr_t *)memory;
    memset((void *)tbl, 0, memsize);

    tbl->maxslots = maxslots;
    tbl->usedslots = 0;
    tbl->num = 0;

//  _tbl_slots = (qhasharr_slot_t *)((char*)memory + sizeof(qhasharr_t));

    return (qhasharr_t *)memory;
}

int qhasharr_init(qhasharr_t *tbl, qhasharr_slot_t **_tbl_slots)
{
    if (NULL == tbl ||  NULL == _tbl_slots) return -1;

    *_tbl_slots = (qhasharr_slot_t *)((char*)tbl + sizeof(qhasharr_t));
    return 0;
}

bool qhasharr_exist(qhasharr_t *tbl, const char *key, size_t key_size)
{
    if ( NULL == tbl ||  NULL == key)
    {
        errno = EINVAL;
        return false;
    }
    
    if (tbl->maxslots == 0)
    {
        return false;  
    }

    // get hash integer
    unsigned int hash = qhashmurmur3_32(key, key_size) % tbl->maxslots;
    if (_get_idx(tbl, key, key_size, hash) >= 0)    //same key
    {
        return true;
    }

    return false;
}

/**
 * Put an object into table.
 *
 * @param tbl       qhasharr_t container pointer.
 * @param key       key string
 * @param value     value object data
 * @param size      size of value
 *
 * @return true if successful, otherwise returns false
 * @retval errno will be set in error condition.
 *  - ENOBUFS   : Table doesn't have enough space to store the object.
 *  - EINVAL    : Invalid argument.
 *  - EFAULT    : Unexpected error. Data structure is not constant.
*/
bool qhasharr_put(qhasharr_t *tbl, const char *key, size_t key_size, const void *value, size_t val_size)
{
    if (NULL == tbl || NULL == key || NULL == value)
    {
        errno = EINVAL;
        return false;
    }

    qhasharr_slot_t *_tbl_slots = NULL;
    qhasharr_init(tbl, &_tbl_slots);
    
    if (tbl->maxslots == 0)
    {
        return false;  
    }
    // check full
    if (tbl->usedslots >= tbl->maxslots)
    {
        errno = ENOBUFS;
        return false;
    }

    // get hash integer
    unsigned int hash = qhashmurmur3_32(key, key_size) % tbl->maxslots;

    // check, is slot empty
    if (_tbl_slots[hash].count == 0)   // empty slot
    {
        // put data
        if (_put_data(tbl, hash, hash, key, key_size, value, val_size, 1) == false)
        {
            return false;
        }
    }
    else if (_tbl_slots[hash].count > 0)     // same key or hash collision
    {
        // check same key;
        int idx = _get_idx(tbl, key, key_size, hash);
        if (idx >= 0)   // same key
        {
            // remove and recall
            if (!qhasharr_remove(tbl, key, key_size))
                return false;
            return qhasharr_put(tbl, key, key_size, value, val_size);
        }
        else     // no same key, just hash collision
        {
            // find empty slot
            int idx = _find_empty(tbl, hash);
            if (idx < 0)
            {
                errno = ENOBUFS;
                return false;
            }

            // put data. -1 is used for collision resolution (idx != hash);
            if (_put_data(tbl, idx, hash, key, key_size, value, val_size, -1) == false)
            {
                return false;
            }

            // increase counter from leading slot
            _tbl_slots[hash].count++;

            //      key, idx, hash, tbl->usedslots);
        }
    }
    else
    {
        // in case of -1 or -2, move it. -1 used for collision resolution,
        // -2 used for oversized value data.

        // find empty slot
        int idx = _find_empty(tbl, hash + 1);
        if (idx < 0)
        {
            errno = ENOBUFS;
            return false;
        }

        // move dup slot to empty
        _copy_slot(tbl, idx, hash);

        _remove_slot(tbl, hash);

        // in case of -2, adjust link of mother
        if (_tbl_slots[idx].count == -2)
        {
            _tbl_slots[ _tbl_slots[idx].hash ].link = idx;
            if (_tbl_slots[idx].link != -1)
            {
                _tbl_slots[ _tbl_slots[idx].link ].hash = idx;
            }
        }
        else if (_tbl_slots[idx].count == -1)
        {
            if (_tbl_slots[idx].link != -1)
            {
                _tbl_slots[ _tbl_slots[idx].link ].hash = idx;
            }
        }

        // store data
        if (_put_data(tbl, hash, hash, key, key_size, value, val_size, 1) == false)
        {
            return false;
        }
    }

    return true;
}

/**
 * Put a string into table
 *
 * @param tbl       qhasharr_t container pointer.
 * @param key       key string.
 * @param value     string data.
 *
 * @return true if successful, otherwise returns false
 * @retval errno will be set in error condition.
 *  - ENOBUFS   : Table doesn't have enough space to store the object.
 *  - EINVAL    : Invalid argument.
 *  - EFAULT    : Unexpected error. Data structure is not constant.
 */
bool qhasharr_putstr(qhasharr_t *tbl, const char *key, const char *str)
{
    if (NULL == tbl || NULL == key)
    {
        errno = EINVAL;
        return false;
    }

    size_t key_size = strlen(key) + 1;
    size_t val_size = (str != NULL) ? (strlen(str) + 1) : 0;
    return qhasharr_put(tbl, key, key_size, (void *)str, val_size);
}

/**
 * Put an integer into table as string type.
 *
 * @param tbl       qhasharr_t container pointer.
 * @param key       key string
 * @param value     value integer
 *
 * @return true if successful, otherwise returns false
 * @retval errno will be set in error condition.
 *  - ENOBUFS   : Table doesn't have enough space to store the object.
 *  - EINVAL    : Invalid argument.
 *  - EFAULT    : Unexpected error. Data structure is not constant.
 *
 * @note
 * The integer will be converted to a string object and stored as string object.
 */
bool qhasharr_putint(qhasharr_t *tbl, const char *key, int64_t num)
{
    if (NULL == tbl || NULL == key)
    {
        errno = EINVAL;
        return false;
    }

    char str[20+1];
    snprintf(str, sizeof(str), "%"PRId64, num);
    return qhasharr_putstr(tbl, key, str);
}

/**
 * Get an object from table
 *
 * @param tbl       qhasharr_t container pointer.
 * @param key       key string
 * @param size      if not NULL, oject size will be stored
 *
 * @return malloced object pointer if successful, otherwise(not found)
 *  returns NULL
 * @retval errno will be set in error condition.
 *  - ENOENT    : No such key found.
 *  - EINVAL    : Invalid argument.
 *  - ENOMEM    : Memory allocation failed.
 *
 * @note
 * returned object must be freed after done using.
 */
void *qhasharr_get(qhasharr_t *tbl, const char *key, size_t key_size, size_t *val_size)
{
    if (NULL == tbl || NULL == key)
    {
        errno = EINVAL;
        return NULL;
    }

    qhasharr_slot_t *_tbl_slots = NULL;
    qhasharr_init(tbl, &_tbl_slots);
    // get hash integer
    if (tbl->maxslots == 0)
    {
        return NULL;  
    }

    unsigned int hash = qhashmurmur3_32(key, key_size) % tbl->maxslots;

    int idx = _get_idx(tbl, key, key_size, hash);
    if (idx < 0)
    {
        errno = ENOENT;
        return NULL;
    }

    return _get_data(tbl, idx, val_size);
}

/**
 * Finds an object with given name and returns as
 * string type.
 *
 * @param tbl       qhasharr_t container pointer.
 * @param key       key string
 *
 * @return string pointer if successful, otherwise(not found) returns NULL
 * @retval errno will be set in error condition.
 *  - ENOENT    : No such key found.
 *  - EINVAL    : Invalid argument.
 *  - ENOMEM    : Memory allocation failed.
 *
 * @note
 * returned object must be freed after done using.
 */
char *qhasharr_getstr(qhasharr_t *tbl, const char *key, size_t key_size)
{
    return (char *)qhasharr_get(tbl, key, key_size, NULL);
}

/**
 * Finds an object with given name and returns as integer type.
 *
 * @param tbl       qhasharr_t container pointer.
 * @param key       key string
 *
 * @return value integer if successful, otherwise(not found) returns 0
 * @retval errno will be set in error condition.
 *  - ENOENT    : No such key found.
 *  - EINVAL    : Invalid argument.
 *  - ENOMEM    : Memory allocation failed.
 */
int64_t qhasharr_getint(qhasharr_t *tbl, const char *key, size_t key_size)
{
    int64_t num = 0;
    char *str = qhasharr_getstr(tbl, key, key_size);
    if (str != NULL)
    {
        num = atoll(str);
        free(str);
        str = NULL;
    }

    return num;
}

/**
 * Get next element.
 *
 * @param tbl       qhasharr_t container pointer.
 * @param idx       index pointer
 *
 * @return key name string if successful, otherwise(end of table) returns NULL
 * @retval errno will be set in error condition.
 *  - ENOENT    : No next element.
 *  - EINVAL    : Invald argument.
 *  - ENOMEM    : Memory allocation failed.
 *
 * @code
 *  int idx = 0;
 *  qnobj_t obj;
 *  while(tbl->getnext(tbl, &obj, &idx) == true) {
 *    printf("NAME=%s, DATA=%s, SIZE=%zu\n",
 *           obj.name, (char*)obj.data, obj.size);
 *    free(obj.name);
 *    free(obj.data);
 *  }
 * @endcode
 *
 * @note
 *  Please be aware a key name will be returned with truncated length
 *  because key name is truncated when it put into the table if it's length is
 *  longer than _Q_HASHARR_KEYSIZE.
 */
bool qhasharr_getnext(qhasharr_t *tbl, qnobj_t *obj, int *idx)
{
    if (NULL == tbl || NULL == obj || NULL == idx)
    {
        errno = EINVAL;
        return false;
    }

    qhasharr_slot_t *_tbl_slots = NULL;
    qhasharr_init(tbl, &_tbl_slots);

    for (; *idx < tbl->maxslots; (*idx)++)
    {
        if (_tbl_slots[*idx].count == 0 || _tbl_slots[*idx].count == -2)
        {
            continue;
        }

        size_t keylen = _tbl_slots[*idx].data.pair.keylen;
        if (keylen > _Q_HASHARR_KEYSIZE) keylen = _Q_HASHARR_KEYSIZE;

        obj->name = (char *)malloc(keylen);
        if (obj->name == NULL)
        {
            errno = ENOMEM;
            return false;
        }
        memcpy(obj->name, _tbl_slots[*idx].data.pair.key, keylen);
        obj->name_size = keylen;

        obj->data = _get_data(tbl, *idx, &obj->data_size);
        if (obj->data == NULL)
        {
            free(obj->name);
            obj->name = NULL;
            errno = ENOMEM;
            return false;
        }

        *idx += 1;
        return true;
    }

    errno = ENOENT;
    return false;
}

/**
 * Remove an object from this table.
 *
 * @param tbl       qhasharr_t container pointer.
 * @param key       key string
 *
 * @return true if successful, otherwise(not found) returns false
 * @retval errno will be set in error condition.
 *  - ENOENT    : No such key found.
 *  - EINVAL    : Invald argument.
 *  - EFAULT        : Unexpected error. Data structure is not constant.
 */
bool qhasharr_remove(qhasharr_t *tbl, const char *key, size_t key_size)
{
    if (NULL == tbl || NULL == key)
    {
        errno = EINVAL;
        return false;
    }

    qhasharr_slot_t *_tbl_slots = NULL;
    qhasharr_init(tbl, &_tbl_slots);

    if (tbl->maxslots == 0)
    {
        return false;  
    }

    // get hash integer
    unsigned int hash = qhashmurmur3_32(key, key_size) % tbl->maxslots;

    int idx = _get_idx(tbl, key, key_size, hash);
    if (idx < 0)
    {
        errno = ENOENT;
        return false;
    }

    if (_tbl_slots[idx].count == 1)
    {
        // just remove
        _remove_data(tbl, idx);
    }
    else if (_tbl_slots[idx].count > 1)     // leading slot and has dup
    {
        // find dup
        int idx2;
        for (idx2 = idx + 1; ; idx2++)
        {
            if (idx2 >= tbl->maxslots) idx2 = 0;
            if (idx2 == idx)
            {
                errno = EFAULT;
                return false;
            }
            if (_tbl_slots[idx2].count == -1 && _tbl_slots[idx2].hash == hash)
            {
                break;
            }
        }

        // move to leading slot
        int backupcount = _tbl_slots[idx].count;

        _remove_data(tbl, idx); // remove leading data

        _copy_slot(tbl, idx, idx2); // copy slot
        _remove_slot(tbl, idx2); // remove moved slot

        _tbl_slots[idx].count = backupcount - 1; // adjust collision counter
        if (_tbl_slots[idx].link != -1)
        {
            _tbl_slots[_tbl_slots[idx].link].hash = idx;
        }
    }
    else if (_tbl_slots[idx].count == -1)   // in case of -1. used for collision resolution
    {
          // decrease counter from leading slot
          if (_tbl_slots[ _tbl_slots[idx].hash ].count <= 1)
          {
              errno = EFAULT;
              return false;
          }
          _tbl_slots[ _tbl_slots[idx].hash ].count--;

          // remove data
          _remove_data(tbl, idx);
    }
    else 
    {
        errno = ENOENT;
        return false;
    }

    return true;
}

/**
 * Returns the number of objects in table.
 *
 * @param tbl       qhasharr_t container pointer.
 *
 * @return a number of elements stored
 */
int qhasharr_size(qhasharr_t *tbl, int *maxslots, int *usedslots)
{
    if (NULL == tbl) return false;

    if (maxslots != NULL) *maxslots = tbl->maxslots;
    if (usedslots != NULL) *usedslots = tbl->usedslots;

    return tbl->num;
}

/**
 * Clears this table so that it contains no keys.
 *
 * @param tbl       qhasharr_t container pointer.
 *
 * @return true if successful, otherwise returns false
 */
void qhasharr_clear(qhasharr_t *tbl)
{
    if (NULL == tbl)
        return;
    if (tbl->usedslots == 0) return;
    qhasharr_slot_t *_tbl_slots = NULL;
    qhasharr_init(tbl, &_tbl_slots);

    tbl->usedslots = 0;
    tbl->num = 0;

    // clear memory
    memset((void *)_tbl_slots,
           '\0',
           (tbl->maxslots * sizeof(qhasharr_slot_t)));
}

#ifndef _DOXYGEN_SKIP

// find empty slot : return empty slow number, otherwise returns -1.
static int _find_empty(qhasharr_t *tbl, int startidx)
{
    if (tbl->maxslots <= 0) return -1;

    qhasharr_slot_t *_tbl_slots = NULL;
    qhasharr_init(tbl, &_tbl_slots);

    if (startidx >= tbl->maxslots) startidx = 0;

    int idx = startidx;
    //qhasharr_init(tbl);
    while (true)
    {
        if (_tbl_slots[idx].count == 0) return idx;

        idx++;
        if (idx >= tbl->maxslots) idx = 0;
        if (idx == startidx) break;
    }

    return -1;
}

static int _get_idx(qhasharr_t *tbl, const char *key, size_t key_size, unsigned int hash)
{
    if (NULL == tbl || NULL == key)
        return -1;
    qhasharr_slot_t *_tbl_slots = NULL;
    qhasharr_init(tbl, &_tbl_slots);

    if (_tbl_slots[hash].count > 0)
    {
        unsigned int idx;
        int count = 0;
        for (count = 0, idx = hash; count < _tbl_slots[hash].count; )
        {
            if (_tbl_slots[idx].hash == hash
                    && (_tbl_slots[idx].count > 0 || _tbl_slots[idx].count == -1))
            {
                // same hash
                count++;

                // first check key length
                if (key_size == _tbl_slots[idx].data.pair.keylen)
                {
                    if (key_size <= _Q_HASHARR_KEYSIZE)
                    {
                        // original key is stored
                        if (!memcmp(key, _tbl_slots[idx].data.pair.key, key_size))
                        {
                            return idx;
                        }
                    }
                    else
                    {
                        // key is truncated, compare MD5 also.
                        unsigned char keymd5[16];
                        qhashmd5(key, key_size, keymd5);
                        if (!memcmp(key, _tbl_slots[idx].data.pair.key,
                                    _Q_HASHARR_KEYSIZE) &&
                                !memcmp(keymd5, _tbl_slots[idx].data.pair.keymd5,
                                        16))
                        {
                            return idx;
                        }
                    }
                }
            }

            // increase idx
            idx++;
            if (idx >= (unsigned int)tbl->maxslots) idx = 0;

            // check loop
            if (idx == hash) break;

            continue;
        }
    }

    return -1;
}

static void *_get_data(qhasharr_t *tbl, int idx, size_t *size)
{
    if (idx < 0)
    {
        errno = ENOENT;
        return NULL;
    }

    int newidx;
    size_t valsize;
    qhasharr_slot_t *_tbl_slots = NULL;
    qhasharr_init(tbl, &_tbl_slots);
    int loop_count = 0;

    for (newidx = idx, valsize = 0; newidx != -1 ; newidx = _tbl_slots[newidx].link)
    {
        valsize += _tbl_slots[newidx].size;
        if (_tbl_slots[newidx].link == -1) break;

        // check the dead lock
        loop_count++;
        if (loop_count > tbl->maxslots)
        {
            return NULL;
        }
    }

    //void *value, *vp;
    char *value, *vp;
    value = (char*)malloc(valsize);
    if (value == NULL)
    {
        errno = ENOMEM;
        return NULL;
    }
    memset(value, 0, valsize);
    loop_count = 0;
    for (newidx = idx, vp = value; (size_t)(vp - value) < valsize && newidx != -1; newidx = _tbl_slots[newidx].link)
    {
        uint8_t vsize = _tbl_slots[newidx].size;
        if ((size_t)(vp - value + vsize) > valsize)
        {
            // if the size is larger than valsize, then the value is wrong, but this may not enough
            free(value);
            value = NULL;
            return NULL;
        }

        if (_tbl_slots[newidx].count == -2)
        {
            // extended data block
            memcpy(vp, (void *)_tbl_slots[newidx].data.ext.value,
                   vsize);
        }
        else
        {
            // key/value pair data block
            memcpy(vp, (void *)_tbl_slots[newidx].data.pair.value,
                   vsize);
        }

        vp += vsize;
        if (_tbl_slots[newidx].link == -1) break;

        // check the dead lock
        loop_count++;
        if (loop_count > tbl->maxslots)
        {
            free(value);
            value = NULL;
            return NULL;
        }
    }

    if (size != NULL) *size = valsize;
    return value;
}

static bool _put_data(qhasharr_t *tbl, int idx, unsigned int hash,
                      const char *key, size_t key_size, const void *value, size_t val_size,
                      int count)
{
    size_t tmp_size = 0;
    qhasharr_slot_t *_tbl_slots = NULL;
    qhasharr_init(tbl, &_tbl_slots);

    // check if used
    if (_tbl_slots[idx].count != 0)
    {
        errno = EFAULT;
        return false;
    }

    unsigned char keymd5[16];
    qhashmd5(key, key_size, keymd5);

    // store key
    _tbl_slots[idx].count = count;
    _tbl_slots[idx].hash = hash;
    tmp_size = (key_size <= _Q_HASHARR_KEYSIZE) ? key_size : _Q_HASHARR_KEYSIZE;
    memcpy(_tbl_slots[idx].data.pair.key, key, tmp_size);
    memcpy((char *)_tbl_slots[idx].data.pair.keymd5, (char *)keymd5, 16);
    _tbl_slots[idx].data.pair.keylen = key_size;
    _tbl_slots[idx].link = -1;

    // store value
    int newidx;
    size_t savesize;
    for (newidx = idx, savesize = 0; savesize < val_size;)
    {
        if (savesize > 0)   // find next empty slot
        {
            int tmpidx = _find_empty(tbl, newidx + 1);
            if (tmpidx < 0)
            {
                _remove_data(tbl, idx);
                errno = ENOBUFS;
                return false;
            }

            // clear & set
            memset((void *)(&_tbl_slots[tmpidx]), '\0',
                   sizeof(qhasharr_slot_t));

            _tbl_slots[tmpidx].count = -2;      // extended data block
            _tbl_slots[tmpidx].hash = newidx;   // prev link
            _tbl_slots[tmpidx].link = -1;       // end block mark
            _tbl_slots[tmpidx].size = 0;

            _tbl_slots[newidx].link = tmpidx;   // link chain

            newidx = tmpidx;
        }

        // copy data
        size_t copysize = val_size - savesize;

        if (_tbl_slots[newidx].count == -2)
        {
            // extended value
            //if (copysize > sizeof(struct _Q_HASHARR_SLOT_EXT)) {
            //    copysize = sizeof(struct _Q_HASHARR_SLOT_EXT);
            //}
            if (copysize > sizeof(union _slot_data))
            {
                copysize = sizeof(union _slot_data);
            }

            memcpy(_tbl_slots[newidx].data.ext.value,
                   (char*)value + savesize, copysize);
        }
        else
        {
            // first slot
            if (copysize > _Q_HASHARR_VALUESIZE)
            {
                copysize = _Q_HASHARR_VALUESIZE;
            }
            memcpy(_tbl_slots[newidx].data.pair.value,
                   (char*)value + savesize, copysize);

            // increase stored key counter
            tbl->num++;
        }
        _tbl_slots[newidx].size = copysize;
        savesize += copysize;

        // increase used slot counter
        tbl->usedslots++;
    }

    return true;
}

static bool _copy_slot(qhasharr_t *tbl, int idx1, int idx2)
{
    qhasharr_slot_t *_tbl_slots = NULL;
    qhasharr_init(tbl, &_tbl_slots);

    if (_tbl_slots[idx1].count != 0 || _tbl_slots[idx2].count == 0)
    {
        errno = EFAULT;
        return false;
    }

    memcpy((void *)(&_tbl_slots[idx1]), (void *)(&_tbl_slots[idx2]),
           sizeof(qhasharr_slot_t));

    // increase used slot counter
    tbl->usedslots++;

    return true;
}

static bool _remove_slot(qhasharr_t *tbl, int idx)
{
    qhasharr_slot_t *_tbl_slots = NULL;
    qhasharr_init(tbl, &_tbl_slots);

    if (_tbl_slots[idx].count == 0)
    {
        errno = EFAULT;
        return false;
    }

    _tbl_slots[idx].count = 0;
    // decrease used slot counter
    tbl->usedslots--;

    return true;
}

static bool _remove_data(qhasharr_t *tbl, int idx)
{
    qhasharr_slot_t *_tbl_slots = NULL;
    qhasharr_init(tbl, &_tbl_slots);
    int loop_count = 0;

    if (_tbl_slots[idx].count == 0)
    {
        errno = EFAULT;
        return false;
    }

    while (true)
    {
        int link = _tbl_slots[idx].link;
        _remove_slot(tbl, idx);

        if (link == -1) break;

        idx = link;

        /***************************/
        /*  should delete the tbl->num */
        /***************************/
        // check the dead lock
        loop_count++;
        if (loop_count > tbl->maxslots)
        {
            break;
        }
    }

    // decrease stored key counter
    tbl->num--;

    return true;
}

#endif /* _DOXYGEN_SKIP */
