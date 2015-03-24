/* version.c - This is file contains the version number for gdbm source. */

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

#include "autoconf.h"
#include "gdbm.h"

/* Keep a string with the version number in it.
   The DIST_DATE magic below is replaced by the actual date when
   making the distdir. */
const char * gdbm_version = "GDBM version " PACKAGE_VERSION ". "
"25/12/2013"
#if defined(__STDC__) && defined(__DATE__) && defined(__TIME__)
		" (built " __DATE__ " " __TIME__ ")"
#endif
;
int const gdbm_version_number[3] = {
  GDBM_VERSION_MAJOR,
  GDBM_VERSION_MINOR,
  GDBM_VERSION_PATCH
};

int
gdbm_version_cmp (int const a[], int const b[])
{
  if (a[0] > b[0])
    return 1;
  else if (a[0] < b[0])
    return -1;

  if (a[1] > b[1])
    return 1;
  else if (a[1] < b[1])
    return -1;

  if (a[2] > b[2])
    return 1;
  else if (a[2] < b[2])
    return -1;

  return 0;
}
