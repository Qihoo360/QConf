/* gdbmcount.c - get number of items in a gdbm file. */

/* This file is part of GDBM, the GNU data base manager.
   Copyright (C) 1993, 1994, 2007, 2011, 2013 Free Software Foundation, Inc.

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

/* Include system configuration before all else. */
#include "autoconf.h"
#include "gdbmdefs.h"

static int
compoff (const void *a, const void *b)
{
  if (*(off_t*)a < *(off_t*)b)
    return -1;
  if (*(off_t*)a > *(off_t*)b)
    return 1;
  return 0;
}
  
int
gdbm_count (GDBM_FILE dbf, gdbm_count_t *pcount)
{
  hash_bucket bucket;
  int nbuckets = GDBM_DIR_COUNT (dbf);
  off_t *sdir;
  gdbm_count_t count = 0;
  int i, last;
  
  sdir = malloc (dbf->header->dir_size);
  if (!sdir)
    {
      gdbm_errno = GDBM_MALLOC_ERROR;
      return -1;
    }
  
  memcpy (sdir, dbf->dir, dbf->header->dir_size);
  qsort (sdir, nbuckets, sizeof (off_t), compoff);

  for (i = last = 0; i < nbuckets; i++)
    {
      if (i == 0 || sdir[i] != sdir[last])
	{
	  if (_gdbm_read_bucket_at (dbf, sdir[i], &bucket, sizeof bucket))
	    return -1;
	  count += bucket.count;
	  last = i;
	}
    }
  free (sdir);
  *pcount = count;
  return 0;
}
