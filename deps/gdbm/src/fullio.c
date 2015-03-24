/* This file is part of GDBM, the GNU data base manager.
   Copyright (C) 2011, 2013  Free Software Foundation, Inc.

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

#include "autoconf.h"
#include "gdbmdefs.h"

/* Read exactly SIZE bytes of data into BUFFER.  Return value is 0 on
   success, GDBM_FILE_EOF, if not enough data is available, and
   GDBM_FILE_READ_ERROR, if a read error occurs.  In the latter case
   errno keeps actual system error code. */
int
_gdbm_full_read (GDBM_FILE dbf, void *buffer, size_t size)
{
  char *ptr = buffer;
  while (size)
    {
      ssize_t rdbytes = __read (dbf, ptr, size);
      if (rdbytes == -1)
	{
	  if (errno == EINTR)
	    continue;
	  return GDBM_FILE_READ_ERROR;
	}
      if (rdbytes == 0)
	return GDBM_FILE_EOF;
      ptr += rdbytes;
      size -= rdbytes;
    }
  return 0;
}

/* Write exactly SIZE bytes of data from BUFFER tp DBF.  Return 0 on
   success, and GDBM_FILE_READ_ERROR on error.  In the latter case errno
   will keep actual system error code. */
int
_gdbm_full_write (GDBM_FILE dbf, void *buffer, size_t size)
{
  char *ptr = buffer;
  while (size)
    {
      ssize_t wrbytes = __write (dbf, ptr, size);
      if (wrbytes == -1)
	{
	  if (errno == EINTR)
	    continue;
	  return GDBM_FILE_WRITE_ERROR;
	}
      if (wrbytes == 0)
	{
	  errno = ENOSPC;
	  return GDBM_FILE_WRITE_ERROR;
	}
      ptr += wrbytes;
      size -= wrbytes;
    }
  return 0;
}
