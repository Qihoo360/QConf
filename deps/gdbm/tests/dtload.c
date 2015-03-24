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
#include <unistd.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <errno.h>
#include "dbm.h"
#include "progname.h"

#define PAGSUF ".pag"

int
main (int argc, char **argv)
{
  const char *progname = canonical_progname (argv[0]);
  char *dbname;
  int line = 0;
  char buf[1024];
  datum key;
  datum data;
  int delim = '\t';
  int data_z = 0;
  
  while (--argc)
    {
      char *arg = *++argv;

      if (strcmp (arg, "-h") == 0)
	{
	  printf ("usage: %s [-null] [-delim=CHR] DBFILE\n", progname);
	  exit (0);
	}
      else if (strcmp (arg, "-null") == 0)
	data_z = 1;
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

  /* Check if .pag file exists. Create it if it doesn't, as DBM
     cannot do it itself. */
  
  dbname = malloc (strlen (*argv) + sizeof (PAGSUF));
  if (!dbname)
    abort ();

  strcat (strcpy (dbname, *argv), PAGSUF);

  if (access (dbname, F_OK))
    {
      int fd = creat (dbname, 0644);
      if (fd < 0)
	{
	  fprintf (stderr, "%s: ", progname);
	  perror (dbname);
	  exit (1);
	}
      close (fd);
    }
  free (dbname);

  if (dbminit (*argv))
    {
      fprintf (stderr, "dbminit failed\n");
      exit (1);
    }

  while (fgets (buf, sizeof buf, stdin))
    {
      size_t i, j;
      size_t len = strlen (buf);

      if (buf[len - 1] != '\n')
	{
	  fprintf (stderr, "%s: %d: line too long\n",
		   progname, line);
	  continue;
	}

      buf[--len] = 0;
      
      line++;

      for (i = j = 0; i < len; i++)
	{
	  if (buf[i] == '\\')
	    i++;
	  else if (buf[i] == delim)
	    break;
	  else
	    buf[j++] = buf[i];
	}

      if (buf[i] != delim)
	{
	  fprintf (stderr, "%s: %d: malformed line\n",
		   progname, line);
	  continue;
	}
      buf[j] = 0;
      
      key.dptr = buf;
      key.dsize = j + data_z;
      data.dptr = buf + i + 1;
      data.dsize = strlen (data.dptr) + data_z;
      if (store (key, data) != 0)
	{
	  fprintf (stderr, "%s: %d: item not inserted\n",
		   progname, line);
	  exit (1);
	}
    }
  dbmclose ();
  exit (0);
}
