/* export.c - Stand alone flat file exporter for older versions of GDBM. */

/* This file is part of GDBM, the GNU data base manager.
   Copyright (C) 2007,  Free Software Foundation, Inc.

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

/* Include system configuration before all else. */
#include "autoconf.h"

#include "systems.h"

#include <gdbm.h>

/* Pull in gdbm_export() */
#include "gdbmexp.c"

void
usage (char *s)
{
  printf ("Usage: %s database outfile\n", s);
  printf ("   or: %s [-hv]\n", s);
  printf ("Convert GDBM database into a flat dump format.\n");
  printf ("Linked with %s\n", gdbm_version);
  printf ("\n");
  printf ("Report bugs to <%s>.\n", PACKAGE_BUGREPORT);
}


void
version ()
{
  printf ("gdbmexport (%s) %s\n", PACKAGE_NAME, PACKAGE_VERSION);
  printf ("Copyright (C) 2007 Free Software Foundation, Inc.\n");
  printf ("License GPLv3+: GNU GPL version 3 or later <http://gnu.org/licenses/gpl.html>\n");
  printf ("This is free software: you are free to change and redistribute it.\n");
  printf ("There is NO WARRANTY, to the extent permitted by law.\n");
}


int
main(int argc, char *argv[])
{
  int c;
  GDBM_FILE dbf;
  int flags = 0;

  while ((c = getopt (argc, argv, "hlv")) != -1)
    switch (c)
      {
      case 'h':
	usage (argv[0]);
	exit (0);

      case 'l':
	flags = GDBM_NOLOCK;
	break;

      case 'v':
	version ();
	exit (0);
	
      default:
	usage (argv[0]);
	exit (1);
      }

  if (argc != 3)
    {
      usage (argv[0]);
      exit (1);
    }

  dbf = gdbm_open (argv[1], 0, GDBM_READER | flags, 0600, NULL);
  if (dbf == NULL)
    {
      fprintf (stderr, "%s: couldn't open database, %s\n", argv[0],
	       gdbm_strerror (gdbm_errno));
      exit (1);
    }

  if (gdbm_export (dbf, argv[2], GDBM_WRCREAT | flags, 0600) == -1)
    {
      fprintf (stderr, "%s: export failed, %s\n",
	       argv[0], gdbm_strerror (gdbm_errno));
      gdbm_close (dbf);
      exit (1);
    }
  gdbm_close (dbf);
  exit (0);
}
