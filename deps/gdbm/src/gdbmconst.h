/* gdbmconst.h - The constants defined for use in gdbm. */

/* This file is part of GDBM, the GNU data base manager.
   Copyright (C) 1990, 1991, 1993, 2007, 2011, 2013 Free Software Foundation,
   Inc.

   GDBM is free software; you can redistribute it and/or modify
   it under the terms of the GNU General Public License as published by
   the Free Software Foundation; either version 3, or (at your option)
   any later version.

   GDBM is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
   GNU General Public License for more details.

   You should have received a copy of the GNU General Public License
   along with GDBM. If not, see <http://www.gnu.org/licenses/>.   */

/* Start with the constant definitions.  */
#define TRUE    1
#define FALSE   0

/* Header magic numbers.  Since we don't have space for flags or versions, we
   use different static numbers to determine what kind of file it is.

   This should've been done back when off_t was added to the library, but
   alas...  We just have to assume that an OMAGIC file is readable. */

#define GDBM_OMAGIC		0x13579ace	/* Original magic number. */
#define GDBM_MAGIC32		0x13579acd	/* New 32bit magic number. */
#define GDBM_MAGIC64		0x13579acf	/* New 64bit magic number. */

#define GDBM_OMAGIC_SWAP	0xce9a5713	/* OMAGIC swapped. */
#define GDBM_MAGIC32_SWAP	0xcd9a5713	/* MAGIC32 swapped. */
#define GDBM_MAGIC64_SWAP	0xcf9a5713	/* MAGIC64 swapped. */

/* In freeing blocks, we will ignore any blocks smaller (and equal) to
   IGNORE_SIZE number of bytes. */
#define IGNORE_SIZE 4

/* The number of key bytes kept in a hash bucket. */
#define SMALL    4

/* The number of bucket_avail entries in a hash bucket. */
#define BUCKET_AVAIL 6

/* The size of the bucket cache. */
#define DEFAULT_CACHESIZE  100

/* Maximum size representable by a size_t variable */
#define SIZE_T_MAX ((size_t)-1)
