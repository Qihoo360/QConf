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
# include "gdbmdefs.h"
# include "gdbm.h"
# include <pwd.h>
# include <grp.h>
# include <time.h>

static int
print_datum (datum const *dat, unsigned char **bufptr,
	     size_t *bufsize, FILE *fp)
{
  int rc;
  size_t len;
  unsigned char *p;
  
  fprintf (fp, "#:len=%lu\n", (unsigned long) dat->dsize);
  rc = _gdbm_base64_encode ((unsigned char*) dat->dptr, dat->dsize,
			    bufptr, bufsize, &len);
  if (rc)
    return rc;
  
  p = *bufptr;
  while (len)
    {
      size_t n = len;
      if (n > _GDBM_MAX_DUMP_LINE_LEN)
	n = _GDBM_MAX_DUMP_LINE_LEN;
      if (fwrite (p, n, 1, fp) != 1)
	return GDBM_FILE_WRITE_ERROR;
      fputc ('\n', fp);
      len -= n;
      p += n;
    }
  return 0;
}

int
_gdbm_dump_ascii (GDBM_FILE dbf, FILE *fp)
{
  time_t t;
  int fd;
  struct stat st;
  struct passwd *pw;
  struct group *gr;
  datum key;
  size_t count = 0;
  unsigned char *buffer = NULL;
  size_t bufsize = 0;
  int rc;

  fd = gdbm_fdesc (dbf);
  if (fstat (fd, &st))
    return GDBM_FILE_STAT_ERROR;

  /* Print header */
  time (&t);
  fprintf (fp, "# GDBM dump file created by %s on %s",
	   gdbm_version, ctime (&t));
  fprintf (fp, "#:version=1.0\n");

  fprintf (fp, "#:file=%s\n", dbf->name);
  fprintf (fp, "#:uid=%lu,", (unsigned long) st.st_uid);
  pw = getpwuid (st.st_uid);
  if (pw)
    fprintf (fp, "user=%s,", pw->pw_name);
  fprintf (fp, "gid=%lu,", (unsigned long) st.st_gid);
  gr = getgrgid (st.st_gid);
  if (gr)
    fprintf (fp, "group=%s,", gr->gr_name);
  fprintf (fp, "mode=%03o\n", st.st_mode & 0777);
  fprintf (fp, "# End of header\n");
  
  key = gdbm_firstkey (dbf);

  while (key.dptr)
    {
      datum nextkey;
      datum data = gdbm_fetch (dbf, key);
      if (data.dptr)
 	{
	  if ((rc = print_datum (&key, &buffer, &bufsize, fp)) ||
	      (rc = print_datum (&data, &buffer, &bufsize, fp)))
	    {
	      free (key.dptr);
	      free (data.dptr);
	      gdbm_errno = rc;
	      break;
	    }
 	}
      nextkey = gdbm_nextkey (dbf, key);
      free (key.dptr);
      free (data.dptr);
      key = nextkey;
      count++;
    }

  if (rc == 0)
    {
      /* FIXME: Something like that won't hurt, although load does not
	 use it currently. */
      fprintf (fp, "#:count=%lu\n", (unsigned long) count);
      fprintf (fp, "# End of data\n");
    }
  free (buffer);

  
  return rc ? -1 : 0;
}

int
gdbm_dump_to_file (GDBM_FILE dbf, FILE *fp, int format)
{
  int rc;
  
  switch (format)
    {
    case GDBM_DUMP_FMT_BINARY:
      rc = gdbm_export_to_file (dbf, fp) == -1;
      break;

    case GDBM_DUMP_FMT_ASCII:
      rc = _gdbm_dump_ascii (dbf, fp);
      break;

    default:
      return EINVAL;
    }
  
  if (rc == 0 && ferror (fp))
    rc = gdbm_errno = GDBM_FILE_WRITE_ERROR;

  return rc;
}

int
gdbm_dump (GDBM_FILE dbf, const char *filename, int fmt, int open_flags,
	   int mode)
{
  int nfd, rc;
  FILE *fp;
  
  /* Only support GDBM_WCREAT or GDBM_NEWDB */
  switch (open_flags)
    {
    case GDBM_WRCREAT:
      nfd = open (filename, O_WRONLY | O_CREAT | O_EXCL, mode);
      if (nfd == -1)
	{
	  gdbm_errno = GDBM_FILE_OPEN_ERROR;
	  return -1;
	}
      break;
    case GDBM_NEWDB:
      nfd = open (filename, O_WRONLY | O_CREAT | O_TRUNC, mode);
      if (nfd == -1)
	{
	  gdbm_errno = GDBM_FILE_OPEN_ERROR;
	  return -1;
	}
      break;
    default:
      gdbm_errno = GDBM_BAD_OPEN_FLAGS;
      return -1;
  }

  fp = fdopen (nfd, "w");
  if (!fp)
    {
      close (nfd);
      gdbm_errno = GDBM_FILE_OPEN_ERROR;
      return -1;
    }
  rc = gdbm_dump_to_file (dbf, fp, fmt);
  fclose (fp);
  return rc;
}



