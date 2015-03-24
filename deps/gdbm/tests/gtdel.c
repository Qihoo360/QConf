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
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "gdbm.h"
#include "progname.h"

int
main (int argc, char **argv)
{
  const char *progname = canonical_progname (argv[0]);
  const char *dbname;
  datum key;
  int flags = 0;
  GDBM_FILE dbf;
  int data_z = 0;
  int rc = 0;
  
  while (--argc)
    {
      char *arg = *++argv;

      if (strcmp (arg, "-h") == 0)
	{
	  printf ("usage: %s [-null] [-nolock] [-nommap] [-sync] DBFILE KEY [KEY...]\n",
		  progname);
	  exit (0);
	}
      else if (strcmp (arg, "-null") == 0)
	data_z = 1;
      else if (strcmp (arg, "-nolock") == 0)
	flags |= GDBM_NOLOCK;
      else if (strcmp (arg, "-nommap") == 0)
	flags |= GDBM_NOMMAP;
      else if (strcmp (arg, "-sync") == 0)
	flags |= GDBM_SYNC;
      else if (strcmp (arg, "--") == 0)
	{
	  --argc;
	  ++argv;
	  break;
	}
      else if (arg[0] == '-')
	{
	  fprintf (stderr, "%s: unknown option %s\n", progname, arg);
	  exit (1);
	}
      else
	break;
    }

  if (argc < 2)
    {
      fprintf (stderr, "%s: wrong arguments\n", progname);
      exit (1);
    }
  dbname = *argv;
  
  dbf = gdbm_open (dbname, 0, GDBM_WRITER|flags, 0, NULL);
  if (!dbf)
    {
      fprintf (stderr, "gdbm_open failed: %s\n", gdbm_strerror (gdbm_errno));
      exit (1);
    }

  while (--argc)
    {
      char *arg = *++argv;

      key.dptr = arg;
      key.dsize = strlen (arg) + !!data_z;

      if (gdbm_delete(dbf, key))
	{
	  fprintf (stderr, "%s: cannot delete %s: %s\n",
		   progname, arg, gdbm_strerror (gdbm_errno));
	  rc = 2;
	}
    }
  gdbm_close (dbf);
  exit (rc);
}
