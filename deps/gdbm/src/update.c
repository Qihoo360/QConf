/* update.c - The routines for updating the file to a consistent state. */

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

static void write_header (GDBM_FILE);

/* This procedure writes the header back to the file described by DBF. */

static void
write_header (GDBM_FILE dbf)
{
  off_t file_pos;	/* Return value for lseek. */
  int rc;
  
  file_pos = __lseek (dbf, 0L, SEEK_SET);
  if (file_pos != 0) _gdbm_fatal (dbf, _("lseek error"));
  rc = _gdbm_full_write (dbf, dbf->header, dbf->header->block_size);
  if (rc)
    _gdbm_fatal (dbf, gdbm_strerror (rc));

  /* Sync the file if fast_write is FALSE. */
  if (dbf->fast_write == FALSE)
    __fsync (dbf);
}


/* After all changes have been made in memory, we now write them
   all to disk. */
void
_gdbm_end_update (GDBM_FILE dbf)
{
  off_t file_pos;	/* Return value for lseek. */
  int rc;
  
  /* Write the current bucket. */
  if (dbf->bucket_changed && (dbf->cache_entry != NULL))
    {
      _gdbm_write_bucket (dbf, dbf->cache_entry);
      dbf->bucket_changed = FALSE;
    }

  /* Write the other changed buckets if there are any. */
  if (dbf->second_changed)
    {
      if (dbf->bucket_cache != NULL)
        {
          int index;

          for (index = 0; index < dbf->cache_size; index++)
	    {
	      if (dbf->bucket_cache[index].ca_changed)
	        _gdbm_write_bucket (dbf, &dbf->bucket_cache[index]);
            }
        }
      dbf->second_changed = FALSE;
    }
  
  /* Write the directory. */
  if (dbf->directory_changed)
    {
      file_pos = __lseek (dbf, dbf->header->dir, SEEK_SET);
      if (file_pos != dbf->header->dir) _gdbm_fatal (dbf, _("lseek error"));
      rc = _gdbm_full_write (dbf, dbf->dir, dbf->header->dir_size);
      if (rc)
	_gdbm_fatal (dbf, gdbm_strerror (rc));
      dbf->directory_changed = FALSE;
      if (!dbf->header_changed && dbf->fast_write == FALSE)
	__fsync (dbf);
    }

  /* Final write of the header. */
  if (dbf->header_changed)
    {
      write_header (dbf);
      dbf->header_changed = FALSE;
    }
}


/* If a fatal error is detected, come here and exit. VAL tells which fatal
   error occured. */

void
_gdbm_fatal (GDBM_FILE dbf, const char *val)
{
  if ((dbf != NULL) && (dbf->fatal_err != NULL))
    (*dbf->fatal_err) (val);
  else
    {
      fprintf (stderr, _("gdbm fatal: %s\n"), val ? val : "");
    }
  exit (1);
  /* NOTREACHED */
}
