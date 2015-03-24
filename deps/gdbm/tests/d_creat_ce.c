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
#include <string.h>
#include <unistd.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <ndbm.h>

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
   DBM *d;
   char fdbuf[2][80];
   int i;
   int flags = O_RDONLY;
   
   if (argc < 2)
     {
       fprintf (stderr, "usage: %s PATH-TO-FDOP [-creat] [-write]\n", argv[0]);
       return 2;
     }

   for (i = 2; i < argc; i++)
     {
       if (strcmp (argv[i], "-creat") == 0)
	 flags = O_RDWR|O_CREAT;
       else if (strcmp (argv[i], "-write") == 0)
	 flags = O_RDWR;
       else
	 {
	   fprintf (stderr, "%s: unknown option: %s\n",
		    argv[0], argv[i]);
	   return 2;
	 }
       
     }
   
   if (!O_CLOEXEC)
     return 77;
   
   d = dbm_open ("file", flags|O_CLOEXEC, 0600);
   if (!d)
     {
       perror ("dbm_open");
       return 3;
     }

   execl (argv[1], "fdop",
	  ntos (dbm_pagfno (d), fdbuf[0], sizeof (fdbuf[0])),
	  ntos (dbm_dirfno (d), fdbuf[1], sizeof (fdbuf[1])),
	  NULL);
   return 127;
}

