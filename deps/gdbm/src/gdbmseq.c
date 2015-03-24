/* gdbmseq.c - Routines to visit all keys.  Not in sorted order. */

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

/* Include system configuration before all else. */
#include "autoconf.h"

#include "gdbmdefs.h"

/* Special extern for this file. */
extern char *_gdbm_read_entry (GDBM_FILE , int);


/* Find and read the next entry in the hash structure for DBF starting
   at ELEM_LOC of the current bucket and using RETURN_VAL as the place to
   put the data that is found. */

static void
get_next_key (GDBM_FILE dbf, int elem_loc, datum *return_val)
{
  int   found;			/* Have we found the next key. */
  char  *find_data;		/* Data pointer returned by find_key. */

  /* Find the next key. */
  found = FALSE;
  while (!found)
    {
      /* Advance to the next location in the bucket. */
      elem_loc++;
      if (elem_loc == dbf->header->bucket_elems)
	{
	  /* We have finished the current bucket, get the next bucket.  */
	  elem_loc = 0;

	  /* Find the next bucket.  It is possible several entries in
	     the bucket directory point to the same bucket. */
	  while (dbf->bucket_dir < GDBM_DIR_COUNT (dbf)
		 && dbf->cache_entry->ca_adr == dbf->dir[dbf->bucket_dir])
	    dbf->bucket_dir++;

	  /* Check to see if there was a next bucket. */
	  if (dbf->bucket_dir < GDBM_DIR_COUNT (dbf))
	    _gdbm_get_bucket (dbf, dbf->bucket_dir);	      
	  else
	    /* No next key, just return. */
	    return ;
	}
      found = dbf->bucket->h_table[elem_loc].hash_value != -1;
    }
  
  /* Found the next key, read it into return_val. */
  find_data = _gdbm_read_entry (dbf, elem_loc);
  return_val->dsize = dbf->bucket->h_table[elem_loc].key_size;
  if (return_val->dsize == 0)
    return_val->dptr = (char *) malloc (1);
  else
    return_val->dptr = (char *) malloc (return_val->dsize);
  if (return_val->dptr == NULL) _gdbm_fatal (dbf, _("malloc error"));
  memcpy (return_val->dptr, find_data, return_val->dsize);
}


/* Start the visit of all keys in the database.  This produces something in
   hash order, not in any sorted order.  */

datum
gdbm_firstkey (GDBM_FILE dbf)
{
  datum return_val;		/* To return the first key. */

  /* Set the default return value for not finding a first entry. */
  return_val.dptr = NULL;

  /* Initialize the gdbm_errno variable. */
  gdbm_errno = GDBM_NO_ERROR;

  /* Get the first bucket.  */
  _gdbm_get_bucket (dbf, 0);

  /* Look for first entry. */
  get_next_key (dbf, -1, &return_val);

  return return_val;
}


/* Continue visiting all keys.  The next key following KEY is returned. */

datum
gdbm_nextkey (GDBM_FILE dbf, datum key)
{
  datum  return_val;		/* The return value. */
  int    elem_loc;		/* The location in the bucket. */
  char  *find_data;		/* Data pointer returned by _gdbm_findkey. */
  int    hash_val;		/* Returned by _gdbm_findkey. */

  /* Initialize the gdbm_errno variable. */
  gdbm_errno = GDBM_NO_ERROR;

  /* Set the default return value for no next entry. */
  return_val.dptr = NULL;

  /* Do we have a valid key? */
  if (key.dptr == NULL) return return_val;

  /* Find the key.  */
  elem_loc = _gdbm_findkey (dbf, key, &find_data, &hash_val);
  if (elem_loc == -1) return return_val;

  /* Find the next key. */  
  get_next_key (dbf, elem_loc, &return_val);

  return return_val;
}
