/* dbmclose.c - The the dbm file.  This is the NDBM interface. */

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
#include "ndbm.h"
#include "gdbmdefs.h"

/* Close the DBF file. */

void
dbm_close (DBM *dbm)
{
  gdbm_close (dbm->file);
  close (dbm->dirfd);
  if (dbm->_dbm_memory.dptr)
    free (dbm->_dbm_memory.dptr);
  if (dbm->_dbm_fetch_val)
    free (dbm->_dbm_fetch_val);
  free (dbm);
}
