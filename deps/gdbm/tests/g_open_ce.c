/* This file is part of GDBM test suite.
   Copyright (C) 2011 Free Software Foundation, Inc.

   GDBM is free software; you can redistribute it and/or modify
   it under the terms of the GNU General Public License as published by
   the Free Software Foundation; either version 2, or (at your option)
   any later version.

   GDBM is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
   GNU General Public License for more details.

   You should have received a copy of the GNU General Public License
   along with GDBM. If not, see <http://www.gnu.org/licenses/>.
*/
#include "autoconf.h"
#include <stdlib.h>
#include <stdio.h>
#include <unistd.h>
#include <fcntl.h>
#include <gdbm.h>

char *
ntos (int n, char *buf, size_t size)
{
  char *p = buf + size;
  *--p = 0;
  do
    {
      int x = n % 10;
      *--p = '0' + x;
      n /= 10;
    }
  while (n);
  return p;
}

#ifndef O_CLOEXEC
# define O_CLOEXEC 0
#endif

int
main (int argc, char *argv[])
{
  GDBM_FILE d;
  char fdbuf[80];

  if (argc != 2)
    {
      fprintf (stderr, "usage: %s PATH-TO-FDOP\n", argv[0]);
      return 2;
    }
  if (!O_CLOEXEC)
    return 77;
  d = gdbm_open ("file.db", 0, GDBM_NEWDB|GDBM_CLOEXEC, 0600, NULL);
  if (!d)
    {
      fprintf (stderr, "gdbm_open: %s\n", gdbm_strerror (gdbm_errno));
      return 3;
    }
  execl (argv[1], "fdop",
	 ntos (gdbm_fdesc (d), fdbuf, sizeof (fdbuf)), NULL);
  return 127;
}
  
