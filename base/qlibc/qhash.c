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
 * $Id: qhash.c 94 2012-04-17 01:21:46Z seungyoung.kim $
 ******************************************************************************/

/**
 * @file qhash.c Hash APIs.
 */
#include <errno.h>
#include <fcntl.h>
#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <unistd.h>

#include "md5.h"
#include "qlibc.h"

/**
 * Calculate 128-bit(16-bytes) MD5 hash.
 *
 * @param data      source object
 * @param nbytes    size of data
 * @param retbuf    user buffer. It must be at leat 16-bytes long.
 *
 * @return true if successful, otherwise false.
 *
 * @code
 *   // get MD5
 *   unsigned char md5hash[16];
 *   qhashmd5((void*)"hello", 5, md5hash);
 *
 *   // hex encode
 *   char *md5ascii = qhex_encode(md5hash, 16);
 *   printf("Hex encoded MD5: %s\n", md5ascii);
 *   free(md5ascii);
 * @endcode
 */
bool qhashmd5(const void *data, size_t nbytes, void *retbuf)
{
    char *tmp = (char*)data;
    char *tmpretbuf = (char*)retbuf;
    if (data == NULL || retbuf == NULL)
    {
        errno = EINVAL;
        return false;
    }

    QMD5_CTX context;
    QMD5Init(&context);
    //QMD5Update(&context, (unsigned char *)data, (unsigned int)nbytes);
    //QMD5Final(retbuf, &context);
    QMD5Update(&context, (unsigned char *)tmp, (unsigned int)nbytes);
    QMD5Final((unsigned char*)tmpretbuf, &context);

    return true;
}

/**
 * Get 32-bit Murmur3 hash.
 *
 * @param data      source data
 * @param nbytes    size of data
 *
 * @return 32-bit unsigned hash value.
 *
 * @code
 *  uint32_t hashval = qhashmurmur3_32((void*)"hello", 5);
 * @endcode
 *
 * @code
 *  MurmurHash3 was created by Austin Appleby  in 2008. The cannonical
 *  implementations are in C++ and placed in the public.
 *
 *    https://sites.google.com/site/murmurhash/
 *
 *  Seungyoung Kim has ported it's cannonical implementation to C language
 *  in 2012 and published it as a part of qLibc component.
 * @endcode
 */
uint32_t qhashmurmur3_32(const void *data, size_t nbytes)
{
    if (data == NULL || nbytes == 0) return 0;

    const uint32_t c1 = 0xcc9e2d51;
    const uint32_t c2 = 0x1b873593;

    const int nblocks = nbytes / 4;
    const uint32_t *blocks = (const uint32_t *)(data);
    //const uint8_t *tail = (const uint8_t *)(data + (nblocks * 4));
    const uint8_t *tail = (const uint8_t *)((uint8_t*)data + (nblocks * 4));

    uint32_t h = 0;

    int i;
    uint32_t k;
    for (i = 0; i < nblocks; i++)
    {
        k = blocks[i];

        k *= c1;
        k = (k << 15) | (k >> (32 - 15));
        k *= c2;

        h ^= k;
        h = (h << 13) | (h >> (32 - 13));
        h = (h * 5) + 0xe6546b64;
    }

    k = 0;
    switch (nbytes & 3)
    {
    case 3:
        k ^= tail[2] << 16;
    case 2:
        k ^= tail[1] << 8;
    case 1:
        k ^= tail[0];
        k *= c1;
        k = (k << 13) | (k >> (32 - 15));
        k *= c2;
        h ^= k;
    };

    h ^= nbytes;

    h ^= h >> 16;
    h *= 0x85ebca6b;
    h ^= h >> 13;
    h *= 0xc2b2ae35;
    h ^= h >> 16;

    return h;
}

int qhashmd5_bin_to_hex(char *md5_str, const unsigned char *md5_int, int md5_int_len)
{ 
    const char *hexs = "0123456789abcdef"; 
    int i = 0; 
    for (i = 0; i < md5_int_len; i++) 
    {    
        md5_str[i * 2] = hexs[md5_int[i] >> 4];  
        md5_str[i * 2 + 1] = hexs[md5_int[i] & 0x0F]; 
    }    

    return 0; 
}
