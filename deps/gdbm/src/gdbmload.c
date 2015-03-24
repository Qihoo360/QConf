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
# include <sys/types.h>
# include <pwd.h>
# include <grp.h>

struct datbuf
{
  unsigned char *buffer;
  size_t size;
};

struct dump_file
{
  FILE *fp;
  size_t line;

  char *linebuf;
  size_t lbsize;
  size_t lblevel;
  
  char *buffer;
  size_t bufsize;
  size_t buflevel;

  size_t parmc;

  struct datbuf data[2];
  char *header;
};

static void
dump_file_free (struct dump_file *file)
{
  free (file->linebuf);
  free (file->buffer);
  free (file->data[0].buffer);
  free (file->data[1].buffer);
  free (file->header);
}

static const char *
getparm (const char *buf, const char *parm)
{
  if (!buf)
    return NULL;
  while (*buf)
    {
      const char *p;
      for (p = parm; *p == *buf; p++, buf++)
	;
      if (*p == 0 && *buf == '=')
	return buf + 1;
      buf += strlen (buf) + 1;
    }
  return NULL;
}

static size_t
get_dump_line (struct dump_file *file)
{
  char buf[80];
  
  if (file->lblevel == 0)
    {
      while (fgets (buf, sizeof buf, file->fp))
	{
	  size_t n = strlen (buf);
	  
	  if (buf[n-1] == '\n')
	    {
	      file->line++;
	      --n;
	    }
	  
	  if (n + 1 + file->lblevel > file->lbsize)
	    {
	      size_t s = ((file->lblevel + n + _GDBM_MAX_DUMP_LINE_LEN)
			  / _GDBM_MAX_DUMP_LINE_LEN)
		          * _GDBM_MAX_DUMP_LINE_LEN;
	      char *newp = realloc (file->linebuf, s);
	      if (!newp)
		return GDBM_MALLOC_ERROR;
	      file->linebuf = newp;
	      file->lbsize = s;
	    }
	  
	  memcpy (file->linebuf + file->lblevel, buf, n);
	  file->lblevel += n;
	  if (buf[n])
	    {
	      file->linebuf[file->lblevel] = 0;
	      break;
	    }
	}
    }
  return file->lblevel;
}

static int
get_data (struct dump_file *file)
{
  size_t n;

  file->buflevel = 0;
  file->parmc = 0;
  
  while ((n = get_dump_line (file)))
    {
      if (file->linebuf[0] == '#')
	return 0;
      if (n + file->buflevel > file->bufsize)
	{
	  size_t s = ((file->buflevel + n + _GDBM_MAX_DUMP_LINE_LEN - 1)
		      / _GDBM_MAX_DUMP_LINE_LEN)
	              * _GDBM_MAX_DUMP_LINE_LEN;
	  char *newp = realloc (file->buffer, s);
	  if (!newp)
	    return GDBM_MALLOC_ERROR;
	  file->buffer = newp;
	  file->bufsize = s;
	}
      memcpy (file->buffer + file->buflevel, file->linebuf, n);
      file->buflevel += n;
      file->lblevel = 0;
    }
  return ferror (file->fp) ? GDBM_FILE_READ_ERROR : 0;
}

static int
get_parms (struct dump_file *file)
{
  size_t n;

  file->buflevel = 0;
  file->parmc = 0;
  while ((n = get_dump_line (file)))
    {
      char *p;

      p = file->linebuf;
      if (*p != '#')
	return 0;
      if (n == 0 || *++p != ':')
	{
	  file->lblevel = 0;
	  continue;
	}
      if (--n == 0)
	{
	  file->lblevel = 0;
	  continue;
	}
      
      if (n + 1 + file->buflevel > file->bufsize)
	{
	  size_t s = ((file->buflevel + n + _GDBM_MAX_DUMP_LINE_LEN)
		      / _GDBM_MAX_DUMP_LINE_LEN)
	              * _GDBM_MAX_DUMP_LINE_LEN;
	  char *newp = realloc (file->buffer, s);
	  if (!newp)
	    return GDBM_MALLOC_ERROR;
	  file->buffer = newp;
	  file->bufsize = s;
	}

      while (*p)
	{
	  p++;
	  while (*p == ' ' || *p == '\t')
	    p++;
	  if (*p)
	    {
	      while (*p && *p != '=')
		file->buffer[file->buflevel++] = *p++;
	      
	      if (*p == '=')
		{
		  file->buffer[file->buflevel++] = *p++;
		  if (*p == '"')
		    {
		      p++;
		      while (*p && *p != '"')
			file->buffer[file->buflevel++] = *p++;

		      if (*p == '"')
			p++;
		    }
		  else
		    {
		      while (!(*p == 0 || *p == ','))
			file->buffer[file->buflevel++] = *p++;
		    }
		  file->parmc++;
		  file->buffer[file->buflevel++] = 0;
		}
	      else
		return GDBM_ILLEGAL_DATA;
	    }
	  else
	    break;
	}
      file->lblevel = 0;
    }

  file->buffer[file->buflevel] = 0;
  
  return ferror (file->fp) ? GDBM_FILE_READ_ERROR : 0;
}

int
get_len (const char *param, size_t *plen)
{
  unsigned long n;
  const char *p = getparm (param, "len");
  char *end;
  
  if (!p)
    return GDBM_ITEM_NOT_FOUND;

  errno = 0;
  n = strtoul (p, &end, 10);
  if (*end == 0 && errno == 0)
    {
      *plen = n;
      return 0;
    }

  return GDBM_ILLEGAL_DATA;
}

int
read_record (struct dump_file *file, char *param, int n, datum *dat)
{
  int rc;
  size_t len, consumed_size, decoded_size;

  if (!param)
    {
      rc = get_parms (file);
      if (rc)
	return rc;
      if (file->parmc == 0)
	return GDBM_ITEM_NOT_FOUND;
      param = file->buffer;
    }
  rc = get_len (param, &len);
  if (rc)
    return rc;
  dat->dsize = len; /* FIXME: data type mismatch */
  rc = get_data (file);
  if (rc)
    return rc;

  rc = _gdbm_base64_decode ((unsigned char *)file->buffer, file->buflevel,
			    &file->data[n].buffer, &file->data[n].size,
			    &consumed_size, &decoded_size);
  if (rc)
    return rc;
  if (consumed_size != file->buflevel || decoded_size != len)
    return GDBM_ILLEGAL_DATA;
  dat->dptr = (void*) file->data[n].buffer;
  return 0;
}

#define META_UID  0x01
#define META_GID  0x02
#define META_MODE 0x04

static int
_set_gdbm_meta_info (GDBM_FILE dbf, char *param, int meta_mask)
{
  unsigned long n;
  uid_t owner_uid;
  uid_t owner_gid;
  mode_t mode;
  int meta_flags = 0;
  const char *p;
  char *end;
  int rc = 0;

  if (!(meta_mask & GDBM_META_MASK_OWNER))
    {
      p = getparm (param, "user");
      if (p)
	{
	  struct passwd *pw = getpwnam (p);
	  if (pw)
	    {
	      owner_uid = pw->pw_uid;
	      meta_flags |= META_UID;
	    }
	}

      if (!(meta_flags & META_UID) && (p = getparm (param, "uid")))
	{
	  errno = 0;
	  n = strtoul (p, &end, 10);
	  if (*end == 0 && errno == 0)
	    {
	      owner_uid = n;
	      meta_flags |= META_UID;
	    }
	}

      p = getparm (param, "group");
      if (p)
	{
	  struct group *gr = getgrnam (p);
	  if (gr)
	    {
	      owner_gid = gr->gr_gid;
	      meta_flags |= META_GID;
	    }
	}
      if (!(meta_flags & META_GID) && (p = getparm (param, "gid")))
	{
	  errno = 0;
	  n = strtoul (p, &end, 10);
	  if (*end == 0 && errno == 0)
	    {
	      owner_gid = n;
	      meta_flags |= META_GID;
	    }
	}
    }
  
  if (!(meta_mask & GDBM_META_MASK_MODE))
    {
      p = getparm (param, "mode");
      if (p)
	{
	  errno = 0;
	  n = strtoul (p, &end, 8);
	  if (*end == 0 && errno == 0)
	    {
	      mode = n & 0777;
	      meta_flags |= META_MODE;
	    }
	}
    }
  
  if (meta_flags)
    {
      int fd = gdbm_fdesc (dbf);
      if (getuid () == 0 && (meta_flags & (META_UID|META_GID)))
	{
	  if ((meta_flags & (META_UID|META_GID)) != (META_UID|META_GID))
	    {
	      struct stat st;
	      fstat (fd, &st);
	      if (!(meta_flags & META_UID))
		owner_uid = st.st_uid;
	      if (!(meta_flags & META_GID))
		owner_gid = st.st_gid;
	    }
	  if (fchown (fd, owner_uid, owner_gid))
	    {
	      gdbm_errno = GDBM_ERR_FILE_OWNER;
	      rc = 1;
	    }
	}
      if ((meta_flags & META_MODE) && fchmod (fd, mode))
	{
	  gdbm_errno = GDBM_ERR_FILE_OWNER;
	  rc = 1;
	}
    }
  return rc;
}

int
_gdbm_load_file (struct dump_file *file, GDBM_FILE dbf, GDBM_FILE *ofp,
		 int replace, int meta_mask)
{
  char *param = NULL;
  int rc;
  GDBM_FILE tmp = NULL;
  
  rc = get_parms (file);
  if (rc)
    return rc;
  
  if (file->parmc)
    {
      file->header = file->buffer;
      file->buffer = NULL;
      file->bufsize = file->buflevel = 0;
    }
  else
    return GDBM_ILLEGAL_DATA;

  if (!dbf)
    {
      const char *filename = getparm (file->header, "file");
      if (!filename)
	return GDBM_NO_DBNAME;
      tmp = gdbm_open (filename, 0,
		       replace ? GDBM_WRCREAT : GDBM_NEWDB, 0600, NULL);
      if (!tmp)
	return gdbm_errno;
      dbf = tmp;
    }
  
  param = file->header;
  while (1)
    {
      datum key, content;
      rc = read_record (file, param, 0, &key);
      if (rc)
	{
	  if (rc == GDBM_ITEM_NOT_FOUND && feof (file->fp))
	    rc = 0;
	  break;
	}
      param = NULL;

      rc = read_record (file, NULL, 1, &content);
      if (rc)
	break;
      
      if (gdbm_store (dbf, key, content, replace))
	{
	  rc = gdbm_errno;
	  break;
	}
    }

  if (rc == 0)
    {
      rc = _set_gdbm_meta_info (dbf, file->header, meta_mask);
      *ofp = dbf;
    }
  else if (tmp)
    gdbm_close (tmp);
    
  return rc;
}

static int
read_bdb_header (struct dump_file *file)
{    
  char buf[256];
  
  file->line = 1;
  if (!fgets (buf, sizeof (buf), file->fp))
    return -1;
  if (strcmp (buf, "VERSION=3\n"))
    return -1;
  while (fgets (buf, sizeof (buf), file->fp))
    {
      ++file->line;
      if (strcmp (buf, "HEADER=END\n") == 0)
	return 0;
    }
  return -1;
}

static int
c2x (int c)
{
  static char xdig[] = "0123456789abcdef";
  char *p = strchr (xdig, c);
  if (!p)
    return -1;
  return p - xdig;
} 

#define DINCR 128

static int
xdatum_read (FILE *fp, datum *d, size_t *pdmax)
{
  int c;
  size_t dmax = *pdmax;
  
  d->dsize = 0;
  while ((c = fgetc (fp)) != EOF && c != '\n')
    {
      int t, n;
      
      t = c2x (c);
      if (t == -1)
	return EOF;
      t <<= 4;

      if ((c = fgetc (fp)) == EOF)
	break;
    
      n = c2x (c);
      if (n == -1)
	return EOF;
      t += n;

      if (d->dsize == dmax)
	{
	  char *np = realloc (d->dptr, dmax + DINCR);
	  if (!np)
	    return GDBM_MALLOC_ERROR;
	  d->dptr = np;
	  dmax += DINCR;
	}
      d->dptr[d->dsize++] = t;
    }
  *pdmax = dmax;
  if (c == '\n')
    return 0;
  return c;
}

int
gdbm_load_bdb_dump (struct dump_file *file, GDBM_FILE dbf, int replace)
{
  datum xd[2];
  size_t xs[2];
  int rc, c;
  int i;
  
  if (read_bdb_header (file))
    return -1;
  memset (&xd, 0, sizeof (xd));
  xs[0] = xs[1] = 0;
  i = 0;
  while ((c = fgetc (file->fp)) == ' ')
    {
      rc = xdatum_read (file->fp, &xd[i], &xs[i]);
      if (rc)
	break;
      ++file->line;

      if (i == 1)
	{
	  if (gdbm_store (dbf, xd[0], xd[1], replace))
	    return gdbm_errno;
	}
      i = !i;
    }
  //FIXME: Read "DATA=END"
  free (xd[0].dptr);
  free (xd[1].dptr);
  if (rc == 0 && i)
    rc = EOF;
    
  return rc;
}

int
gdbm_load_from_file (GDBM_FILE *pdbf, FILE *fp, int replace,
		     int meta_mask,
		     unsigned long *line)
{
  struct dump_file df;
  int rc;

  if (!pdbf || !fp)
    return EINVAL;

  /* Guess input file format */
  rc = fgetc (fp);
  ungetc (rc, fp);
  if (rc == '!')
    {
      if (line)
	*line = 0;
      if (!*pdbf)
	{
	  gdbm_errno = GDBM_NO_DBNAME;
	  return -1;
	}
      if (gdbm_import_from_file (*pdbf, fp, replace) == -1)
	return -1;
      return 0;
    }

  memset (&df, 0, sizeof df);
  df.fp = fp;

  if (rc == 'V')
    {
      if (!*pdbf)
	{
	  gdbm_errno = GDBM_NO_DBNAME;
	  return -1;
	}
      rc = gdbm_load_bdb_dump (&df, *pdbf, replace);
    }
  else
    rc = _gdbm_load_file (&df, *pdbf, pdbf, replace, meta_mask);
  dump_file_free (&df);
  if (rc)
    {
      if (line)
	*line = df.line;
      gdbm_errno = rc;
      return -1;
    }
  return 0;
}

int
gdbm_load (GDBM_FILE *pdbf, const char *filename, int replace,
	   int meta_mask,
	   unsigned long *line)
{
  FILE *fp;
  int rc;
  
  fp = fopen (filename, "r");
  if (!fp)
    {
      gdbm_errno = GDBM_FILE_OPEN_ERROR;
      return -1;
    }
  rc = gdbm_load_from_file (pdbf, fp, replace, meta_mask, line);
  fclose (fp);
  return rc;
}

