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
#include <unistd.h>

/* usage:  fdop FD [FD...]
   Return: false if any of the FDs is open, true otherwise.
*/
int
main (int argc, char **argv)
{
  int fd = dup (0);
  
  while (--argc)
    {
      int n = atoi (*++argv);
      if (n < fd)
	return 1;
    }
  return 0;
}
