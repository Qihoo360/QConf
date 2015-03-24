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

void
ealloc_die ()
{
  error ("%s", strerror (ENOMEM));
  exit (EXIT_FATAL);
}

void *
emalloc (size_t size)
{
  void *p = malloc (size);
  if (!p)
    ealloc_die ();
  return p;
}

void *
erealloc (void *ptr, size_t size)
{
  void *newptr = realloc (ptr, size);
  if (!newptr)
    ealloc_die ();
  return newptr;
}

void *
ecalloc (size_t nmemb, size_t size)
{
  void *p = calloc (nmemb, size);
  if (!p)
    ealloc_die ();
  return p;
}

void *
ezalloc (size_t size)
{
  return ecalloc (1, size);
}

char *
estrdup (const char *str)
{
  char *p;

  if (!str)
    return NULL;
  p = emalloc (strlen (str) + 1);
  strcpy (p, str);
  return p;
}

  
