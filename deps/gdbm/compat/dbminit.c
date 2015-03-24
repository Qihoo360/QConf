/* dbminit.c - Open the file for dbm operations. This looks like the
   DBM interface. */

/* This file is part of GDBM, the GNU data base manager.
   Copyright (C) 1990, 1991, 1993, 2007, 2011 Free Software Foundation,
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
#include "dbm-priv.h"

DBM *_gdbm_file;

/* Initialize dbm system.  FILE is a pointer to the file name.  In
   standard dbm, the database is found in files called FILE.pag and
   FILE.dir.  To make gdbm compatable with dbm using the dbminit call,
   the same file names are used.  Specifically, dbminit will use the file
   name FILE.pag in its call to gdbm open.  If the file (FILE.pag) has a
   size of zero bytes, a file initialization procedure is performed,
   setting up the initial structure in the file.  Any error detected will
   cause a return value of -1.  No errors cause the return value of 0.
   NOTE: file.dir will be linked to file.pag. */

int
dbminit (char *file)
{
  if (_gdbm_file != NULL)
    dbm_close (_gdbm_file);
  /* Try to open the file as a writer.  DBM never created a file. */
  _gdbm_file = dbm_open (file, O_RDWR, 0644);
  /* If it was not opened, try opening it as a reader. */
  if (_gdbm_file == NULL)
    {
      _gdbm_file = dbm_open (file, O_RDONLY, 0644);
      /* Did we successfully open the file? */
      if (_gdbm_file == NULL)
	{
	  gdbm_errno = GDBM_FILE_OPEN_ERROR;
	  return -1;
	}
    }
  return 0;
}
