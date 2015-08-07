/*
 * Copyright (C) 1991-2, RSA Data Security, Inc. Created 1991. All
 * rights reserved.
 *
 * License to copy and use this software is granted provided that it
 * is identified as the "RSA Data Security, Inc. MD5 Message-Digest
 * Algorithm" in all material mentioning or referencing this software
 * or this function.
 *
 * License is also granted to make and use derivative works provided
 * that such works are identified as "derived from the RSA Data
 * Security, Inc. MD5 Message-Digest Algorithm" in all material
 * mentioning or referencing the derived work.
 *
 * RSA Data Security, Inc. makes no representations concerning either
 * the merchantability of this software or the suitability of this
 * software for any particular purpose. It is provided "as is"
 * without express or implied warranty of any kind.
 *
 * These notices must be retained in any copies of any part of this
 * documentation and/or software.
 */

#ifndef _Q_MD5_H_
#define _Q_MD5_H_

#define QMD5_BLOCK_LENGTH		64
#define QMD5_DIGEST_LENGTH		16
#define QMD5_DIGEST_STRING_LENGTH	(QMD5_DIGEST_LENGTH * 2 + 1)

/* QMD5 context. */
typedef struct QMD5Context
{
    u_int32_t state[4];	/* state (ABCD) */
    u_int32_t count[2];	/* number of bits, modulo 2^64 (lsb first) */
    unsigned char buffer[64];	/* input buffer */
} QMD5_CTX;

#include <sys/cdefs.h>

__BEGIN_DECLS
void   QMD5Init (QMD5_CTX *);
void   QMD5Update (QMD5_CTX *, const unsigned char *, unsigned int);
void   QMD5Final (unsigned char [16], QMD5_CTX *);
char * QMD5End(QMD5_CTX *, char *);
char * QMD5File(const char *, char *);
char * QMD5FileChunk(const char *, char *, off_t, off_t);
char * QMD5Data(const unsigned char *, unsigned int, char *);
__END_DECLS

#endif /* _Q_MD5_H_ */
