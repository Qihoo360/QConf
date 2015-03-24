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
#include "dbm.h"
#include "progname.h"

int
main (int argc, char **argv)
{
  const char *progname = canonical_progname (argv[0]);
  char *dbname;
  datum key;
  datum data;
  int delim = '\t';
  
  while (--argc)
    {
      char *arg = *++argv;

      if (strcmp (arg, "-h") == 0)
	{
	  printf ("usage: %s [-delim=CHR] DBFILE\n", progname);
	  exit (0);
	}
      else if (strncmp (arg, "-delim=", 7) == 0)
	delim = arg[7];
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

  if (argc != 1)
    {
      fprintf (stderr, "%s: wrong arguments\n", progname);
      exit (1);
    }
  dbname = *argv;
  
  if (dbminit (dbname))
    {
      fprintf (stderr, "dbminit failed\n");
      exit (1);
    }

  for (key = firstkey (); key.dptr; key = nextkey (key))
    {
      int i;
      
      for (i = 0; i < key.dsize && key.dptr[i]; i++)
	{
	  if (key.dptr[i] == delim || key.dptr[i] == '\\')
	    fputc ('\\', stdout);
	  fputc (key.dptr[i], stdout);
	}

      fputc (delim, stdout);

      data = fetch (key);
      i = data.dsize;
      if (data.dptr[i-1] == 0)
	i--;
      
      fwrite (data.dptr, i, 1, stdout);
      
      fputc ('\n', stdout);
    }

  dbmclose ();
  exit (0);
}
