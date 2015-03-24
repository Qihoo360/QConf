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
#include "gdbm.h"
#include "progname.h"

#define major_number(lib) ((lib) ? gdbm_version_number[0] : GDBM_VERSION_MAJOR)
#define minor_number(lib) ((lib) ? gdbm_version_number[1] : GDBM_VERSION_MINOR)
#define patch_number(lib) ((lib) ? gdbm_version_number[2] : GDBM_VERSION_PATCH)

int
main (int argc, char **argv)
{
  const char *progname = canonical_progname (argv[0]);
  int library = 0;

  if (argc == 1)
    {
      printf ("%s\n", gdbm_version);
      exit (0);
    }
  
  while (--argc)
    {
      char *arg = *++argv;

      if (strcmp (arg, "-help") == 0)
	{
	  printf ("usage: %s [-string] [-lib] [-header] [-major] [-minor] [-patch] [-full]\n", progname);
	  exit (0);
	}
      else if (strcmp (arg, "-string") == 0)
	printf ("%s\n", gdbm_version);
      else if (strcmp (arg, "-lib") == 0)
	library = 1;
      else if (strcmp (arg, "-header") == 0)
	library = 0;
      else if (strcmp (arg, "-major") == 0)
	printf ("%d\n", major_number (library));
      else if (strcmp (arg, "-minor") == 0)
	printf ("%d\n", minor_number (library));
      else if (strcmp (arg, "-patch") == 0)
	printf ("%d\n", patch_number (library));
      else if (strcmp (arg, "-full") == 0)
	printf ("%d.%d.%d\n",
		major_number (library),
		minor_number (library),
		patch_number (library));
      else
	{
	  fprintf (stderr, "%s: unknown option %s\n",
		   progname, arg);
	  exit (1);
	}
    }
  exit (0);
}
