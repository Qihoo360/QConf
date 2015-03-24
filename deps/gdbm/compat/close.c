/* close.c - Close the "original" style database. */

/* This file is part of GDBM, the GNU data base manager.
   Copyright (C) 1993, 2007, 2011 Free Software Foundation, Inc.

   GDBM is free software; you can redistribute it and/or modify
   it under the terms of the GNU General Public License as published by
   the Free Software Foundation; either version 3, or (at your option)
   any later version.

   GDBM is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
   GNU General Public License for more details.

   You should have received a copy of the GNU General Public License
   along with GDBM. If not, see <http://www.gnu.org/licenses/>.    */

/* Include system configuration before all else. */
#include "autoconf.h"
#include "dbm-priv.h"

/* It's unclear whether dbmclose() is *always* a void function in old
   C libraries.  We use int, here. */

int
dbmclose ()
{
  if (_gdbm_file != NULL)
    {
      dbm_close (_gdbm_file);
      _gdbm_file = NULL;
    }
  return (0);
}
