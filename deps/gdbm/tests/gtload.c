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
#include <errno.h>
#include "gdbm.h"
#include "progname.h"

int
main (int argc, char **argv)
{
  const char *progname = canonical_progname (argv[0]);
  const char *dbname;
  int line = 0;
  char buf[1024];
  datum key;
  datum data;
  int replace = 0;
  int flags = 0;
  int mode = GDBM_WRCREAT;
  int block_size = 0;
  GDBM_FILE dbf;
  int delim = '\t';
  int data_z = 0;
  size_t mapped_size_max = 0;
  
  while (--argc)
    {
      char *arg = *++argv;

      if (strcmp (arg, "-h") == 0)
	{
	  printf ("usage: %s [-replace] [-clear] [-blocksize=N] [-null] [-nolock] [-nommap] [-maxmap=N] [-sync] [-delim=CHR] DBFILE\n", progname);
	  exit (0);
	}
      else if (strcmp (arg, "-replace") == 0)
	replace |= GDBM_REPLACE;
      else if (strcmp (arg, "-clear") == 0)
	mode = GDBM_NEWDB;
      else if (strcmp (arg, "-null") == 0)
	data_z = 1;
      else if (strcmp (arg, "-nolock") == 0)
	flags |= GDBM_NOLOCK;
      else if (strcmp (arg, "-nommap") == 0)
	flags |= GDBM_NOMMAP;
      else if (strcmp (arg, "-sync") == 0)
	flags |= GDBM_SYNC;
      else if (strncmp (arg, "-blocksize=", 11) == 0)
	block_size = atoi (arg + 11);
      else if (strncmp (arg, "-maxmap=", 8) == 0)
	{
	  char *p;

	  errno = 0;
	  mapped_size_max = strtoul (arg + 8, &p, 10);
	  
	  if (errno)
	    {
	      fprintf (stderr, "%s: ", progname);
	      perror ("maxmap");
	      exit (1);
	    }

	  if (*p)
	    {
	      fprintf (stderr, "%s: bad maxmap\n", progname);
	      exit (1);
	    }
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
  
  dbf = gdbm_open (dbname, block_size, mode|flags, 00664, NULL);
  if (!dbf)
    {
      fprintf (stderr, "gdbm_open failed: %s\n", gdbm_strerror (gdbm_errno));
      exit (1);
    }

  if (mapped_size_max)
    {
      if (gdbm_setopt (dbf, GDBM_SETMAXMAPSIZE, &mapped_size_max,
		       sizeof (mapped_size_max)))
	{
	  fprintf (stderr, "gdbm_setopt failed: %s\n",
		   gdbm_strerror (gdbm_errno));
	  exit (1);
	}
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
      if (gdbm_store (dbf, key, data, replace) != 0)
	{
	  fprintf (stderr, "%s: %d: item not inserted\n",
		   progname, line);
	  exit (1);
	}
    }
  gdbm_close (dbf);
  exit (0);
}
