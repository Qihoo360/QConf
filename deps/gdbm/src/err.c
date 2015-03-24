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
# include <stdio.h>
# include <errno.h>
# include <string.h>

static void
prerror (const char *fmt, va_list ap, const char *diag)
{
  fprintf (stderr, "%s: ", progname);
  vfprintf (stderr, fmt, ap);
  if (diag)
    fprintf (stderr, ": %s", diag);
  fputc ('\n', stderr);
}

void
verror (const char *fmt, va_list ap)
{
  prerror (fmt, ap, NULL);
}

void
error (const char *fmt, ...)
{
  va_list ap;
  va_start (ap, fmt);
  verror (fmt, ap);
  va_end (ap);
}

void
sys_perror (int code, const char *fmt, ...)
{
  va_list ap;
  va_start (ap, fmt);
  prerror (fmt, ap, strerror (code));
  va_end (ap);
}

void
gdbm_perror (const char *fmt, ...)
{
  va_list ap;
  va_start (ap, fmt);
  prerror (fmt, ap, gdbm_strerror (gdbm_errno));
  va_end (ap);
}

