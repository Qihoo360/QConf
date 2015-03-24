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
#include <assert.h>
#include "gdbmdefs.h"
#include "progname.h"

const char *progname;
const char *dbname;
int flags = 0;                  /* gdbm_open flags */
int mode = GDBM_WRCREAT;        /* gdbm_open mode */
int block_size = 0;             /* block size for the db. 0 means default */
size_t mapped_size_max = 32768; /* size of the memory mapped region */
size_t cache_size = 32;         /* cache size */

static size_t
get_max_mmap_size (const char *arg)
{
  char *p;
  size_t size;
  
  errno = 0;
  size = strtoul (arg, &p, 10);
	  
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
  return size;
}

/* Test results */
#define RES_PASS  0
#define RES_FAIL  1
#define RES_XFAIL 2
#define RES_SKIP  3

const char *resstr[] = { "PASS", "FAIL", "XFAIL", "SKIP" };
static int _res_max = sizeof(resstr) / sizeof(resstr[0]);

/* A single setopt testcase */
struct optest
{
  char *group;       /* Group this testcase belongs to */
  char *name;        /* Testcase name */  
  /* gdbm_setopt arguments: */
  int code;          /* option code */
  void *valptr;      /* points to the value */
  int valsize;       /* size of the value */
  /* end of arguments */
  int xfail;         /* if !0, expected value of gdbm_errno */
  int (*test) (void *valptr);   /* Test function (can be NULL) */
  void (*init) (void *valptr, int valsize); /* Initialization function
					       (can be NULL) */
};

/* Storage for the test value */
char *string;
size_t size;
int intval;
int retbool;

/* Individual test and initialization functions */

int
test_getflags (void *valptr)
{
  int expected = mode | flags;
#ifndef HAVE_MMAP
  expected |= GDBM_NOMMAP;
#endif
  return (*(int*) valptr == expected) ? RES_PASS : RES_FAIL;
}

int
test_dbname (void *valptr)
{
  char *s = *(char**)valptr;
  int rc = strcmp (string, dbname) == 0 ? RES_PASS : RES_FAIL;
  if (rc != RES_PASS)
    printf ("[got %s instead of %s] ", s, dbname);
  free (s);
  return rc;
}

void
init_cachesize (void *valptr, int valsize)
{
  *(size_t*) valptr = cache_size;
}

int
test_getcachesize (void *valptr)
{
  return *(size_t*) valptr == cache_size ? RES_PASS : RES_FAIL;
}

void
init_true (void *valptr, int valsize)
{
  *(int*) valptr = 1;
}

void
init_false (void *valptr, int valsize)
{
  *(int*) valptr = 0;
}

void
init_negate_bool (void *valptr, int valsize)
{
  *(int*) valptr = !retbool;
}

int
test_true (void *valptr)
{
  return *(int*) valptr == 1 ? RES_PASS : RES_FAIL;
}

int
test_false (void *valptr)
{
  return *(int*) valptr == 0 ? RES_PASS : RES_FAIL;
}

int
test_negate_bool (void *valptr)
{
  return *(int*) valptr == !retbool ? RES_PASS : RES_FAIL;
}

int
test_bool (void *valptr)
{
  return *(int*) valptr == retbool ? RES_PASS : RES_FAIL;
}

int
test_initial_maxmapsize(void *valptr)
{
  return *(size_t*) valptr == SIZE_T_MAX ? RES_PASS : RES_FAIL;
}

void
init_maxmapsize (void *valptr, int valsize)
{
  *(size_t*)valptr = mapped_size_max;
}

int
test_maxmapsize (void *valptr)
{
  size_t page_size = sysconf (_SC_PAGESIZE);
  size_t expected_size = ((mapped_size_max + page_size - 1) / page_size) *
	                          page_size;
  return (*(size_t*) valptr == expected_size) ? RES_PASS : RES_FAIL;
}

int
test_mmap_group (void *valptr)
{
#ifdef HAVE_MMAP
  return RES_PASS;
#else
  return RES_SKIP;
#endif
}

/* Create a group of testcases for testing a boolean option.
   Arguments:

     grp  -  group name
     set  -  GDBM_SETxxx option
     get  -  GDBM_GETxxx option
*/
#define TEST_BOOL_OPTION(grp, set,get)				\
  { #grp, },							\
  { #grp, "initial " #get, get, &retbool, sizeof (retbool),	\
      0, NULL, NULL },						\
  { #grp, #set, set, &intval, sizeof (intval),			\
      0, NULL, init_negate_bool },				\
  { #grp, #get, get, &intval, sizeof (intval),			\
      0, test_negate_bool, NULL },				\
  { #grp, #set " true", set, &intval, sizeof (intval),		\
      0, NULL, init_true },					\
  { #grp, #get, get, &intval, sizeof (intval),			\
      0, test_true, NULL },					\
  { #grp, #set " false", set, &intval, sizeof (intval),		\
      0, NULL, init_false },					\
  { #grp, #get, get, &intval, sizeof (intval),			\
      0, test_false, NULL }


/* Table of testcases: */
struct optest optest_tab[] = {
  { "GETFLAGS", "GDBM_GETFLAGS", GDBM_GETFLAGS, &intval, sizeof (intval),
    0, test_getflags },

  { "CACHESIZE" },
  { "CACHESIZE", "initial GDBM_SETCACHESIZE", GDBM_SETCACHESIZE,
    &size, sizeof (size), 0,
    NULL, init_cachesize },
  { "CACHESIZE", "GDBM_GETCACHESIZE", GDBM_GETCACHESIZE,
    &size, sizeof (size), 0,
    test_getcachesize },
  { "CACHESIZE", "second GDBM_SETCACHESIZE", GDBM_SETCACHESIZE,
    &size, sizeof (size),
    GDBM_OPT_ALREADY_SET, NULL, init_cachesize },

  TEST_BOOL_OPTION (SYNCMODE, GDBM_SETSYNCMODE, GDBM_GETSYNCMODE),
  TEST_BOOL_OPTION (CENTFREE, GDBM_SETCENTFREE, GDBM_GETCENTFREE),
  TEST_BOOL_OPTION (COALESCEBLKS, GDBM_SETCOALESCEBLKS, GDBM_GETCOALESCEBLKS),

  /* MMAP group */
  { "MMAP", NULL, 0, NULL, 0, 0, test_mmap_group }, 

  { "MMAP", "initial GDBM_GETMMAP", GDBM_GETMMAP,
    &intval, sizeof (intval), 0,
    test_true },
  { "MMAP", "GDBM_SETMMAP false", GDBM_SETMMAP,
    &intval, sizeof (intval), 0,
    NULL, init_false },
  { "MMAP", "GDBM_GETMMAP", GDBM_GETMMAP,
    &intval, sizeof (intval), 0,
    test_false },

  { "MMAP", "initial GDBM_GETMAXMAPSIZE", GDBM_GETMAXMAPSIZE,
    &size, sizeof (size), 0,
    test_initial_maxmapsize, NULL },
  { "MMAP", "GDBM_SETMAXMAPSIZE", GDBM_SETMAXMAPSIZE,
    &size, sizeof (size), 0,
    NULL, init_maxmapsize },
  { "MMAP", "GDBM_GETMAXMAPSIZE", GDBM_GETMAXMAPSIZE,
    &size, sizeof (size), 0,
    test_maxmapsize, NULL },
  
  
  { "GETDBNAME", "GDBM_GETDBNAME", GDBM_GETDBNAME,
    &string, sizeof (string), 0,
    test_dbname, NULL },
  { NULL }
};

/* Use ARGV to determine whether to run the given GROUP of
   testcases.

   ARGV is a NULL-terminated array of allowed group names.  A "!"
   prefix can be used to denote negation. */
int
groupok (char **argv, const char *group)
{
  int retval = 1;
  
  if (*argv)
    {
      char *arg;
    
      while ((arg = *argv++))
	{
	  if (*arg == '!')
	    {
	      if (strcasecmp (arg + 1, group) == 0)
		return 0;
	      retval = 1;
	    }
	  else
	    {
	      if (strcasecmp (arg, group) == 0)
		return 1;
	      retval = 0;
	    }
	}
    }

  return retval;
}

int
main (int argc, char **argv)
{
  GDBM_FILE dbf;
  struct optest *op;
  
  progname = canonical_progname (argv[0]);
  while (--argc)
    {
      char *arg = *++argv;

      if (strcmp (arg, "-h") == 0)
	{
	  printf ("usage: %s [-blocksize=N] [-nolock] [-sync] [-maxmap=N] DBFILE [GROUP [GROUP...]\n",
		  progname);
	  exit (0);
	}
      else if (strcmp (arg, "-nolock") == 0)
	flags |= GDBM_NOLOCK;
      else if (strcmp (arg, "-sync") == 0)
	flags |= GDBM_SYNC;
      else if (strncmp (arg, "-blocksize=", 11) == 0)
	block_size = atoi (arg + 11);
      else if (strncmp (arg, "-maxmap=", 8) == 0)
	mapped_size_max = get_max_mmap_size (arg + 8);
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

  if (argc == 0)
    {
      fprintf (stderr, "%s: wrong arguments\n", progname);
      exit (1);
    }
  dbname = *argv;
  ++argv;
  --argc;
  
  dbf = gdbm_open (dbname, block_size, mode|flags, 00664, NULL);
  if (!dbf)
    {
      fprintf (stderr, "gdbm_open failed: %s\n", gdbm_strerror (gdbm_errno));
      exit (1);
    }

  for (op = optest_tab; op->group; op++)
    {
      int rc;

      if (!groupok (argv, op->group))
	continue;

      if (!op->name)
	{
	  /* Group header */
	  const char *grp = op->group;

	  printf ("* %s:", grp);
	  if (op->test && (rc = op->test (NULL)) != RES_PASS)
	    {
	      printf (" %s", resstr[rc]);
	      for (op++; op->name && strcmp (op->group, grp) == 0; op++)
		;
	      op--;
	    }
	  putchar ('\n');
	  continue;
	}
	
      printf ("%s: ", op->name);
      if (op->init)
	op->init (op->valptr, op->valsize);
      
      rc = gdbm_setopt (dbf, op->code, op->valptr, op->valsize);
      if (rc)
	{
	  if (gdbm_errno == op->xfail)
	    puts (resstr[RES_XFAIL]);
	  else
	    printf ("%s: %s\n", resstr[RES_FAIL],
		    gdbm_strerror (gdbm_errno));
	}
      else if (!op->test)
	puts (resstr[RES_PASS]);
      else
	{
	  rc = op->test (op->valptr);
	  assert (rc >= 0 && rc < _res_max);
	  puts (resstr[rc]);
	}
    }
  
  gdbm_close (dbf);
  exit (0);
}
  
	  
	  
