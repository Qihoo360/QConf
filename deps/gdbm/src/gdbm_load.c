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
# include <pwd.h>
# include <grp.h>

int replace = 0;
int meta_mask = 0;
int no_meta_option;

int mode;
uid_t owner_uid;
gid_t owner_gid;

char *parseopt_program_doc = "load a GDBM database from a file";
char *parseopt_program_args = "FILE [DB_FILE]";
struct gdbm_option optab[] = {
  { 'r', "replace", NULL, N_("replace records in the existing database") },
  { 'm', "mode", N_("MODE"), N_("set file mode") },
  { 'u', "user", N_("NAME|UID[:NAME|GID]"), N_("set file owner") },
  { 'n', "no-meta", NULL, N_("do not attempt to set file meta-data") },
  { 'M', "mmap", NULL, N_("use memory mapping") },
  { 'c', "cache-size", N_("NUM"), N_("set the cache size") },
  { 'b', "block-size", N_("NUM"), N_("set the block size") },
  { 0 }
};

static int
set_meta_info (GDBM_FILE dbf)
{
  if (meta_mask)
    {
      int fd = gdbm_fdesc (dbf);

      if (meta_mask & GDBM_META_MASK_OWNER)
	{
	  if (fchown (fd, owner_uid, owner_gid))
	    {
	      gdbm_errno = GDBM_ERR_FILE_OWNER;
	      return 1;
	    }
	}
      if ((meta_mask & GDBM_META_MASK_MODE) && fchmod (fd, mode))
	{
	  gdbm_errno = GDBM_ERR_FILE_OWNER;
	  return 1;
	}
    }
  return 0;
}

static int
get_int (const char *arg)
{
  char *p;
  long n;
 
  errno = 0;
  n = strtol (arg, &p, 0);
  if (*p)
    {
      error (_("invalid number: %s"), arg);
      exit (EXIT_USAGE);
    }
  if (errno)
    {
      error (_("invalid number: %s: %s"), arg, strerror (errno));
      exit (EXIT_USAGE);
    }
  return n;
}

int
main (int argc, char **argv)
{
  GDBM_FILE dbf = NULL;
  int rc, opt;
  char *dbname, *filename;
  FILE *fp;
  unsigned long err_line, n;
  char *end;
  int oflags = GDBM_NEWDB|GDBM_NOMMAP;
  int cache_size = 0;
  int block_size = 0;
  
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
      case 'b':
	block_size = get_int (optarg);
	break;
	
      case 'c':
	cache_size = get_int (optarg);
	break;

      case 'm':
	{
	  errno = 0;
	  n = strtoul (optarg, &end, 8);
	  if (*end == 0 && errno == 0)
	    {
	      mode = n & 0777;
	      meta_mask |= GDBM_META_MASK_MODE;
	    }
	  else
	    {
	      error ("%s", _("invalid octal number"));
	      exit (EXIT_USAGE);
	    }
	}
	break;

      case 'u':
	{
	  size_t len;
	  struct passwd *pw;
	  
	  len = strcspn (optarg, ".:");
	  if (optarg[len])
	    optarg[len++] = 0;
	  pw = getpwnam (optarg);
	  if (pw)
	    owner_uid = pw->pw_uid;
	  else
	    {
	      errno = 0;
	      n = strtoul (optarg, &end, 10);
	      if (*end == 0 && errno == 0)
		owner_uid = n;
	      else
		{
		  error (_("invalid user name: %s"), optarg);
		  exit (EXIT_USAGE);
		}
	    }
	  
	  if (optarg[len])
	    {
	      char *grname = optarg + len;
	      struct group *gr = getgrnam (grname);
	      if (gr)
		owner_gid = gr->gr_gid;
	      else
		{
		  errno = 0;
		  n = strtoul (grname, &end, 10);
		  if (*end == 0 && errno == 0)
		    owner_gid = n;
		  else
		    {
		      error (_("invalid group name: %s"), grname);
		      exit (EXIT_USAGE);
		    }
		}
	    }
	  else
	    {
	      if (!pw)
		{
		  pw = getpwuid (owner_uid);
		  if (!pw)
		    {
		      error (_("no such UID: %lu"), (unsigned long)owner_uid);
		      exit (EXIT_USAGE);
		    }
		}
	      owner_gid = pw->pw_gid;
	    }
	  meta_mask |= GDBM_META_MASK_OWNER;
	}
	break;
	  
      case 'r':
	replace = 1;
	break;

      case 'n':
	no_meta_option = 1;
	break;

      case 'M':
	oflags &= ~GDBM_NOMMAP;
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
  
  filename = argv[0];
  if (argc == 2)
    dbname = argv[1];
  else
    dbname = NULL;

  if (strcmp (filename, "-") == 0)
    {
      filename = "<stdin>";
      fp = stdin;
    }
  else
    {
      fp = fopen (filename, "r");
      if (!fp)
	{
	  sys_perror (errno, _("cannot open %s"), filename);
	  exit (EXIT_FATAL);
	}
    }
  
  if (dbname)
    {
      dbf = gdbm_open (dbname, block_size, oflags, 0600, NULL);
      if (!dbf)
	{
	  gdbm_perror (_("gdbm_open failed"));
	  exit (EXIT_FATAL);
	}

      if (cache_size &&
	  gdbm_setopt (dbf, GDBM_SETCACHESIZE, &cache_size, sizeof (int)) == -1)
	error (_("gdbm_setopt failed: %s"), gdbm_strerror (gdbm_errno));
    }
  
  rc = gdbm_load_from_file (&dbf, fp, replace,
			    no_meta_option ?
			      (GDBM_META_MASK_MODE | GDBM_META_MASK_OWNER) :
			      meta_mask,
			    &err_line);
  if (rc)
    {
      switch (gdbm_errno)
	{
	case GDBM_ERR_FILE_OWNER:
	case GDBM_ERR_FILE_MODE:
	  error (_("error restoring metadata: %s (%s)"),
		 gdbm_strerror (gdbm_errno), strerror (errno));
	  rc = EXIT_MILD;
	  break;
	  
	default:
	  if (err_line)
	    gdbm_perror ("%s:%lu", filename, err_line);
	  else
	    gdbm_perror (_("cannot load from %s"), filename);
	  rc = EXIT_FATAL;
	}
    }

  if (dbf)
    {
      if (!no_meta_option && set_meta_info (dbf))
	{
	  error (_("error restoring metadata: %s (%s)"),
		 gdbm_strerror (gdbm_errno), strerror (errno));
	  rc = EXIT_MILD;
	}
      
      if (!dbname)
	{
	  if (gdbm_setopt (dbf, GDBM_GETDBNAME, &dbname, sizeof (dbname)))
	    gdbm_perror (_("gdbm_setopt failed"));
	  else
	    {
	      printf ("%s: created %s\n", progname, dbname);
	      free (dbname);
	    }
	}
      gdbm_close (dbf);
    }
  exit (rc);
}
