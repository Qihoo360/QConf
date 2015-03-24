/* store.c - Add a new key/data pair to the database. */

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

/* Add a new element to the database.  CONTENT is keyed by KEY.  The file on
   disk is updated to reflect the structure of the new database before
   returning from this procedure.  */

int
store (datum key, datum content)
{
  return dbm_store (_gdbm_file, key, content, DBM_REPLACE);
}
