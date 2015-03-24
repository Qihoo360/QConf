/* gdbmimp.c - Import a GDBM database. */

/* This file is part of GDBM, the GNU data base manager.
   Copyright (C) 2007, 2011, 2013 Free Software Foundation, Inc.

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

# include "autoconf.h"
# include <arpa/inet.h>

# include "gdbmdefs.h"
# include "gdbm.h"

int
gdbm_import_from_file (GDBM_FILE dbf, FILE *fp, int flag)
{
  int seenbang, seennewline, rsize, size, kbufsize, dbufsize, rret;
  int c;
  char *kbuffer, *dbuffer;
  datum key, data;
  int count = 0;

  seenbang = 0;
  seennewline = 0;
  kbuffer = NULL;
  dbuffer = NULL;

  /* Read (and discard) four lines begining with ! and ending with \n. */
  while (1)
    {
      if ((c = fgetc (fp)) == -1)
	goto read_fail;
      
      if (c == '!')
	seenbang++;
      if (c == '\n')
	{
	  if (seenbang > 3 && seennewline > 2)
	    {
	      /* End of last line. */
	      break;
	    }
	  seennewline++;
	}
    }

  /* Allocate buffers. */
  kbufsize = 512;
  kbuffer = malloc (kbufsize);
  if (kbuffer == NULL)
    {
      gdbm_errno = GDBM_MALLOC_ERROR;
      return -1;
    }
  dbufsize = 512;
  dbuffer = malloc (dbufsize);
  if (dbuffer == NULL)
    {
      free (kbuffer);
      gdbm_errno = GDBM_MALLOC_ERROR;
      return -1;
    }

  /* Insert/replace records in the database until we run out of file. */
  while ((rret = fread (&rsize, sizeof (rsize), 1, fp)) == 1)
    {
      /* Read the key. */
      size = ntohl (rsize);
      if (size > kbufsize)
	{
	  kbufsize = (size + 512);
	  kbuffer = realloc (kbuffer, kbufsize);
	  if (kbuffer == NULL)
	    {
	      free (dbuffer);
	      gdbm_errno = GDBM_MALLOC_ERROR;
	      return -1;
	    }
	}
      if (fread (kbuffer, size, 1, fp) != 1)
	goto read_fail;

      key.dptr = kbuffer;
      key.dsize = size;

      /* Read the data. */
      if (fread (&rsize, sizeof (rsize), 1, fp) != 1)
	goto read_fail;

      size = ntohl (rsize);
      if (size > dbufsize)
	{
	  dbufsize = (size + 512);
	  dbuffer = realloc (dbuffer, dbufsize);
	  if (dbuffer == NULL)
	    {
	      free (kbuffer);
	      gdbm_errno = GDBM_MALLOC_ERROR;
	      return -1;
	    }
	}
      if (fread (dbuffer, size, 1, fp) != 1)
	goto read_fail;

      data.dptr = dbuffer;
      data.dsize = size;

      if (gdbm_store (dbf, key, data, flag) != 0)
	{
	  /* Keep the existing errno. */
	  free (kbuffer);
	  free (dbuffer);
	  return -1;
	}

      count++;
    }

  if (rret == 0)
    return count;

read_fail:

  free (kbuffer);
  free (dbuffer);
  gdbm_errno = GDBM_FILE_READ_ERROR;
  return -1;
}

int
gdbm_import (GDBM_FILE dbf, const char *importfile, int flag)
{
  FILE *fp;
  int rc;
  
  fp = fopen (importfile, "r");
  if (!fp)
    {
      gdbm_errno = GDBM_FILE_OPEN_ERROR;
      return -1;
    }
  rc = gdbm_import_from_file (dbf, fp, flag);
  fclose (fp);
  return rc;
}

