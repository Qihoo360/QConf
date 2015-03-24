/* seq.c - This is the sequential visit of the database.  This defines two
   user-visable routines that are used together. This is the DBM interface. */

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

/* Start the visit of all keys in the database.  This produces something in
   hash order, not in any sorted order.  */

datum
firstkey (void)
{
  return dbm_firstkey (_gdbm_file);
}


/* Continue visiting all keys.  The next key following KEY is returned. */

datum
nextkey (datum key)
{
  return dbm_nextkey (_gdbm_file);
}
