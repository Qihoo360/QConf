/* dbmfetch.c - Find a key and return the associated data.  */

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

/* NDBM Look up a given KEY and return the information associated with that
   KEY. The pointer in the structure that is  returned is a pointer to
   dynamically allocated memory block.  */

datum
dbm_fetch (DBM *dbm, datum key)
{
  datum  ret_val;		/* The return value. */

  /* Free previous dynamic memory, do actual call, and save pointer to new
     memory. */
  ret_val = gdbm_fetch (dbm->file, key);
  if (dbm->_dbm_fetch_val != NULL)
    free (dbm->_dbm_fetch_val);
  dbm->_dbm_fetch_val = ret_val.dptr;
  __gdbm_error_to_ndbm (dbm);
  /* Return the new value. */
  return ret_val;
}
