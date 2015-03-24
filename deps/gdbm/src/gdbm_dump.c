/* This file is part of GDBM, the GNU data base manager.
   Copyright (C) 2011, 2013 Free Software Foundation, Inc.

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

# include "autoconf.h"
# include "gdbm.h"
# include "gdbmapp.h"
# include "gdbmdefs.h"

char *parseopt_program_doc = "dump a GDBM database to a file";
char *parseopt_program_args = "DB_FILE [FILE]";
struct gdbm_option optab[] = {
  { 'H', "format", "binary|ascii|0|1", N_("select dump format") },
  { 0 }
};

int format = GDBM_DUMP_FMT_ASCII;

int
main (int argc, char **argv)
{
  GDBM_FILE dbf;
  int rc, opt;
  char *dbname, *filename;
  FILE *fp;

#ifdef HAVE_SETLOCALE
  setlocale (LC_ALL, "");
#endif
  bindtextdomain (PACKAGE, LOCALEDIR);
  textdomain (PACKAGE);

  set_progname (argv[0]);

  for (opt = parseopt_first (argc, argv, optab);
       opt != EOF;
       opt = parseopt_next ())
    {
    switch (opt)
      {
      case 'H':
	if (strcmp (optarg, "binary") == 0)
	  format = GDBM_DUMP_FMT_BINARY;
	else if (strcmp (optarg, "ascii") == 0)
	  format = GDBM_DUMP_FMT_ASCII;
	else
	  {
	    format = atoi (optarg);
	    switch (format)
	      {
	      case GDBM_DUMP_FMT_BINARY:
	      case GDBM_DUMP_FMT_ASCII:
		break;
	      default:
		error (_("unknown dump format"));
		exit (EXIT_USAGE);
	      }
	  }
	break;
	
      default:
	error (_("unknown option"));
	exit (EXIT_USAGE);
      }
    }

  argc -= optind;
  argv += optind;

  if (argc == 0)
    {
      parseopt_print_help ();
      exit (EXIT_OK);
    }

  if (argc > 2)
    {
      error (_("too many arguments; try `%s -h' for more info"), progname);
      exit (EXIT_USAGE);
    }
  
  dbname = argv[0];
  if (argc == 2)
    filename = argv[1];
  else
    filename = NULL;

  if (!filename || strcmp (filename, "-") == 0)
    {
      filename = "<stdout>";
      fp = stdout;
    }
  else
    {
      fp = fopen (filename, "w");
      if (!fp)
	{
	  sys_perror (errno, _("cannot open %s"), filename);
	  exit (EXIT_FATAL);
	}
    }

  dbf = gdbm_open (dbname, 0, GDBM_READER, 0600, NULL);
  if (!dbf)
    {
      gdbm_perror (_("gdbm_open failed"));
      exit (EXIT_FATAL);
    }

  rc = gdbm_dump_to_file (dbf, fp, format);
  if (rc)
    {
      gdbm_perror (_("dump error"), filename);
    }
  
  gdbm_close (dbf);

  exit (rc ? EXIT_OK : EXIT_FATAL);
}
  
