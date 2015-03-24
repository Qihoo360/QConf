/* This file is part of GDBM, the GNU data base manager.
   Copyright (C) 1990, 1991, 1993, 2007, 2011, 2013 Free Software Foundation,
   Inc.

   GDBM is free software; you can redistribute it and/or modify
   it under the terms of the GNU General Public License as published by
   the Free Software Foundation; either version 3, or (at your option)
   any later version.

   GDBM is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
   GNU General Public License for more details.

   You should have received a copy of the GNU General Public License
   along with GDBM. If not, see <http://www.gnu.org/licenses/>.    */

#include "gdbmtool.h"
#include <pwd.h>

char *
mkfilename (const char *dir, const char *file, const char *suf)
{
  char *tmp;
  size_t dirlen = strlen (dir);
  size_t suflen = suf ? strlen (suf) : 0;
  size_t fillen = strlen (file);
  size_t len;
  
  while (dirlen > 0 && dir[dirlen-1] == '/')
    dirlen--;

  len = dirlen + (dir[0] ? 1 : 0) + fillen + suflen;
  tmp = emalloc (len + 1);
  memcpy (tmp, dir, dirlen);
  if (dir[0])
    tmp[dirlen++] = '/';
  memcpy (tmp + dirlen, file, fillen);
  if (suf)
    memcpy (tmp + dirlen + fillen, suf, suflen);
  tmp[len] = 0;
  return tmp;
}

char *
tildexpand (char *s)
{
  if (s[0] == '~')
    {
      char *p = s + 1;
      size_t len = strcspn (p, "/");
      struct passwd *pw;

      if (len == 0)
	pw = getpwuid (getuid ());
      else
	{
	  char *user = emalloc (len + 1);
	  
	  memcpy (user, p, len);
	  user[len] = 0;
	  pw = getpwnam (user);
	  free (user);
	}
      if (pw)
	return mkfilename (pw->pw_dir, p + len + 1, NULL);
    }
  return estrdup (s);
}

int
vgetyn (const char *prompt, va_list ap)
{
  int state = 0;
  int c, resp;

  do
    {
      switch (state)
	{
	case 1:
	  if (c == ' ' || c == '\t')
	    continue;
	  resp = c;
	  state = 2;
	  /* fall through */
	case 2:
	  if (c == '\n')
	    {
	      switch (resp)
		{
		case 'y':
		case 'Y':
		  return 1;
		case 'n':
		case 'N':
		  return 0;
		default:
		  fprintf (stdout, "%s\n", _("Please, reply 'y' or 'n'"));
		}
	      state = 0;
	    } else
	    break;
	  
	case 0:
	  vfprintf (stdout, prompt, ap);
	  fprintf (stdout, " [y/n]?");
	  fflush (stdout);
	  state = 1;
	  break;
	}
    } while ((c = getchar ()) != EOF);
  exit (EXIT_USAGE);
}
	
int
getyn (const char *prompt, ...)
{
  va_list ap;
  int rc;
  
  va_start (ap, prompt);
  rc = vgetyn (prompt, ap);
  va_end (ap);
  return rc;
}
	
