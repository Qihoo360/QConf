/* gdbmdefs.h - The include file for dbm.  Defines structure and constants. */

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

#include "systems.h"
#include "gdbmconst.h"
#include "gdbm.h"
#define DEFAULT_TEXT_DOMAIN PACKAGE
#include "gettext.h"

#define _(s) gettext (s)
#define N_(s) s

/* The type definitions are next.  */

/* The available file space is stored in an "avail" table.  The one with
   most activity is contained in the file header. (See below.)  When that
   one filles up, it is split in half and half is pushed on an "avail
   stack."  When the active avail table is empty and the "avail stack" is
   not empty, the top of the stack is popped into the active avail table. */

/* The following structure is the element of the avaliable table.  */
typedef struct {
  	int   av_size;		/* The size of the available block. */
	off_t  av_adr;		/* The file address of the available block. */
      } avail_elem;

/* This is the actual table. The in-memory images of the avail blocks are
   allocated by malloc using a calculated size.  */
typedef struct {
	int   size;		/* The number of avail elements in the table.*/
	int   count;		/* The number of entries in the table. */
	off_t next_block;	/* The file address of the next avail block. */
	avail_elem av_table[1]; /* The table.  Make it look like an array.  */
      } avail_block;

/* The dbm file header keeps track of the current location of the hash
   directory and the free space in the file.  */

typedef struct {
	int   header_magic;  /* Version of file. */
	int   block_size;    /* The optimal i/o blocksize from stat. */
	off_t dir;	     /* File address of hash directory table. */
	int   dir_size;	     /* Size in bytes of the table.  */
	int   dir_bits;	     /* The number of address bits used in the table.*/
	int   bucket_size;   /* Size in bytes of a hash bucket struct. */
	int   bucket_elems;  /* Number of elements in a hash bucket. */
	off_t next_block;    /* The next unallocated block address. */
	avail_block avail;   /* This must be last because of the pseudo
				array in avail.  This avail grows to fill
				the entire block. */
      }  gdbm_file_header;


/* The dbm hash bucket element contains the full 31 bit hash value, the
   "pointer" to the key and data (stored together) with their sizes.  It also
   has a small part of the actual key value.  It is used to verify the first
   part of the key has the correct value without having to read the actual
   key. */

typedef struct {
	int   hash_value;	/* The complete 31 bit value. */
	char  key_start[SMALL];	/* Up to the first SMALL bytes of the key.  */
	off_t data_pointer;	/* The file address of the key record. The
				   data record directly follows the key.  */
	int   key_size;		/* Size of key data in the file. */
	int   data_size;	/* Size of associated data in the file. */
      } bucket_element;


/* A bucket is a small hash table.  This one consists of a number of
   bucket elements plus some bookkeeping fields.  The number of elements
   depends on the optimum blocksize for the storage device and on a
   parameter given at file creation time.  This bucket takes one block.
   When one of these tables gets full, it is split into two hash buckets.
   The contents are split between them by the use of the first few bits
   of the 31 bit hash function.  The location in a bucket is the hash
   value modulo the size of the bucket.  The in-memory images of the
   buckets are allocated by malloc using a calculated size depending of
   the file system buffer size.  To speed up write, each bucket will have
   BUCKET_AVAIL avail elements with the bucket. */

typedef struct {
        int   av_count;		   /* The number of bucket_avail entries. */
        avail_elem bucket_avail[BUCKET_AVAIL];  /* Distributed avail. */
	int   bucket_bits;	   /* The number of bits used to get here. */
	int   count;		   /* The number of element buckets full. */
	bucket_element h_table[1]; /* The table.  Make it look like an array.*/
      } hash_bucket;

/* We want to keep from reading buckets as much as possible.  The following is
   to implement a bucket cache.  When full, buckets will be dropped in a
   least recently read from disk order.  */

/* To speed up fetching and "sequential" access, we need to implement a
   data cache for key/data pairs read from the file.  To find a key, we
   must exactly match the key from the file.  To reduce overhead, the
   data will be read at the same time.  Both key and data will be stored
   in a data cache.  Each bucket cached will have a one element data
   cache.  */

typedef struct {
        int   hash_val;
	int   data_size;
	int   key_size;
	char *dptr;
	int   elem_loc;
      }  data_cache_elem;

typedef struct {
        hash_bucket *   ca_bucket;
	off_t           ca_adr;
	char		ca_changed;   /* Data in the bucket changed. */
	data_cache_elem ca_data;
      } cache_elem;

/* This final structure contains all main memory based information for
   a gdbm file.  This allows multiple gdbm files to be opened at the same
   time by one program. */

struct gdbm_file_info {
	/* Global variables and pointers to dynamic variables used by gdbm.  */

  	/* The file name. */
	char *name;

	/* The reader/writer status. */
	unsigned read_write :2;

	/* Fast_write is set to 1 if no fsyncs are to be done. */
	unsigned fast_write :1;

	/* Central_free is set if all free blocks are kept in the header. */
	unsigned central_free :1;

	/* Coalesce_blocks is set if we should try to merge free blocks. */
	unsigned coalesce_blocks :1;

	/* Whether or not we should do file locking ourselves. */
	unsigned file_locking :1;

	/* Whether or not we're allowing mmap() use. */
        unsigned memory_mapping :1;

        /* Whether the database was open with GDBM_CLOEXEC flag */
        unsigned cloexec :1;
  
	/* Type of file locking in use. */
	enum { LOCKING_NONE = 0, LOCKING_FLOCK, LOCKING_LOCKF,
	       LOCKING_FCNTL } lock_type;

	/* The fatal error handling routine. */
	void (*fatal_err) (const char *);

	/* The gdbm file descriptor which is set in gdbm_open.  */
	int desc;

	/* The file header holds information about the database. */
	gdbm_file_header *header;

	/* The hash table directory from extendable hashing.  See Fagin et al, 
	   ACM Trans on Database Systems, Vol 4, No 3. Sept 1979, 315-344 */
	off_t *dir;

	/* The bucket cache. */
	cache_elem *bucket_cache;
        size_t cache_size;
	int last_read;

	/* Points to the current hash bucket in the cache. */
	hash_bucket *bucket;

	/* The directory entry used to get the current hash bucket. */
	int bucket_dir;

	/* Pointer to the current bucket's cache entry. */
	cache_elem *cache_entry;

	/* Bookkeeping of things that need to be written back at the
	   end of an update. */
	unsigned header_changed :1;
	unsigned directory_changed :1;
	unsigned bucket_changed :1;
	unsigned second_changed :1;

	/* Mmap info */
        size_t mapped_size_max;/* Max. allowed value for mapped_size */
	void  *mapped_region;  /* Mapped region */
	size_t mapped_size;    /* Size of the region */
	off_t  mapped_pos;     /* Current offset in the region */
	off_t  mapped_off;     /* Position in the file where the region
				  begins */
      };

#define GDBM_DIR_COUNT(db) ((db)->header->dir_size / sizeof (off_t))

/* Execute CODE without clobbering errno */
#define SAVE_ERRNO(code)			\
  do						\
    {						\
      int __ec = errno;				\
      code;					\
      errno = __ec;				\
    }						\
  while (0)					\

#define _GDBM_MAX_DUMP_LINE_LEN 76

/* Now define all the routines in use. */
#include "proto.h"
