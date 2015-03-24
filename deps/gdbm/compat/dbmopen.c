/* dbmopen.c - Open the file for dbm operations. This looks like the
   NDBM interface for dbm use. */

/* This file is part of GDBM, the GNU data base manager.
   Copyright (C) 1990, 1991, 1993, 2007, 2011 Free Software Foundation,
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
   along with GDBM. If not, see <http://www.gnu.org/licenses/>.   */

/* Include system configuration before all else. */
#include "autoconf.h"
#include "ndbm.h"
#include "gdbmdefs.h"
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>

#define DIRSUF ".dir"

/* dir file structure:

   GDBM_DIR_MAGIC         4 bytes
   GDBM_VERSION_MAJOR     4 bytes
   GDBM_VERSION_MINOR     4 bytes
   GDBM_VERSION_PATCH     4 bytes

   Total length:         16 bytes
*/

#define GDBM_DIR_MAGIC 0x4744424d  /* GDBM */
#define DEF_DIR_SIZE 16

static unsigned
getint (const unsigned char *cp)
{
  return (cp[0] << 24) + (cp[1] << 16) + (cp[2] << 8) + cp[3];
}

static void
putint (unsigned char *cp, unsigned n)
{
  cp[0] = (n >> 24) & 0xff;
  cp[1] = (n >> 16) & 0xff;
  cp[2] = (n >> 8) & 0xff;
  cp[3] = n & 0xff;
}

/* FIXME: revise return codes */
static int
ndbm_open_dir_file0 (const char *file_name, int pagfd, int mode)
{
  int fd = -1;
  struct stat st, pagst;
  unsigned char dirbuf[DEF_DIR_SIZE];
  int flags = (mode & GDBM_OPENMASK) == GDBM_READER ?
                O_RDONLY : O_RDWR;

  if (mode & GDBM_CLOEXEC)
    flags |= O_CLOEXEC;
      
  if (fstat (pagfd, &pagst))
    {
      gdbm_errno = GDBM_FILE_OPEN_ERROR; /* FIXME: special code? */
      return -1;
    } 
      
  /* Previous versions of GDBM linked pag to dir. Try to detect this: */
  if (stat (file_name, &st) == 0)
    {
      if (st.st_nlink >= 2)
	{
	  if (st.st_dev == pagst.st_dev && st.st_ino == pagst.st_ino)
	    {
	      if (unlink (file_name))
		{
		  if ((mode & GDBM_OPENMASK) == GDBM_READER)
		    /* Ok, try to cope with it. */
		    return pagfd;
		  else
		    {
		      gdbm_errno = GDBM_FILE_OPEN_ERROR; 
		      return -1;
		    } 
		}
	    }
	  else
	    {
	      gdbm_errno = GDBM_FILE_OPEN_ERROR;
	      return -1;
	    }
	}
      else if (st.st_size == 0)
	/* ok */;
      else if (st.st_size != DEF_DIR_SIZE)
	{
	  gdbm_errno = GDBM_BAD_MAGIC_NUMBER;
	  return -1;
	}
      else
	{
	  fd = open (file_name, flags);
	  if (fd == -1)
	    {
	      gdbm_errno = GDBM_FILE_OPEN_ERROR;
	      return fd;
	    }
	  
	  if (read (fd, dirbuf, sizeof (dirbuf)) != sizeof (dirbuf))
	    {
	      gdbm_errno = GDBM_FILE_OPEN_ERROR;
	      close (fd);
	      return -1;
	    } 
	  
	  if (getint (dirbuf) == GDBM_DIR_MAGIC)
	    {
	      int v[3];
	      
	      v[0] = getint (dirbuf + 4);
	      v[1] = getint (dirbuf + 8);
	      v[2] = getint (dirbuf + 12);
	      
	      if (gdbm_version_cmp (v, gdbm_version_number) <= 0)
		return fd;
	    }
	  close (fd);
	  gdbm_errno = GDBM_BAD_MAGIC_NUMBER;
	  return -1;
	}
    }
  
  /* File does not exist.  Create it. */
  fd = open (file_name, flags | O_CREAT, pagst.st_mode & 0777);
  if (fd >= 0)
    {
      putint (dirbuf, GDBM_DIR_MAGIC);
      putint (dirbuf + 4, gdbm_version_number[0]);
      putint (dirbuf + 8, gdbm_version_number[1]);
      putint (dirbuf + 12, gdbm_version_number[2]);

      if (write (fd, dirbuf, sizeof (dirbuf)) != sizeof (dirbuf))
	{
	  gdbm_errno = GDBM_FILE_WRITE_ERROR;
	  close (fd);
	  fd = -1;
	} 
    }
  
  return fd;
}

static int
ndbm_open_dir_file (const char *base, int pagfd, int mode)
{
  char *file_name = malloc (strlen (base) + sizeof (DIRSUF));
  int fd;
  
  if (!file_name)
    {
      gdbm_errno = GDBM_MALLOC_ERROR;
      return -1;
    }
  fd = ndbm_open_dir_file0 (strcat (strcpy (file_name, base), DIRSUF),
			    pagfd, mode);
  free (file_name);
  return fd;
}
  
/* Initialize ndbm system.  FILE is a pointer to the file name.  In
   standard dbm, the database is found in files called FILE.pag and
   FILE.dir.  To make gdbm compatable with dbm using the dbminit call,
   the same file names are used.  Specifically, dbminit will use the file
   name FILE.pag in its call to gdbm open.  If the file (FILE.pag) has a
   size of zero bytes, a file initialization procedure is performed,
   setting up the initial structure in the file.  Any error detected will
   cause a return value of -1.  No errors cause the return value of 0.

   The file.dir file is used only to ensure that the corresponding file.pag
   was created using a compatible version of GDBM.  If file.dir is a hard
   link to file.pag, as was in GDBM 1.8.x, it is removed and recreated in
   the right format (see above).  If the database is being opened read-only,
   the file.dir remains untouched.

   FLAGS and MODE are the same as for the open(2) call.  This call will
   look at the FLAGS and decide what call to make to gdbm_open.  For
   FLAGS == O_RDONLY, it will be a GDBM_READER, if FLAGS == O_RDWR|O_CREAT,
   it will be a GDBM_WRCREAT (creater and writer) and if the FLAGS == O_RDWR,
   it will be a GDBM_WRITER and if FLAGS contain O_TRUNC then it will be
   a GDBM_NEWDB.  The O_CLOEXEC bit raises GDBM_CLOEXEC flag.
   All other values of FLAGS are ignored. */

DBM *
dbm_open (char *file, int flags, int mode)
{
  char *pag_file;	    /* Used to construct "file.pag". */
  DBM *dbm = NULL;
  int open_flags;
  int f;
  
  /* Prepare the correct name for "file.pag". */
  pag_file = (char *) malloc (strlen (file) + 5);
  if (!pag_file)
    {
      gdbm_errno = GDBM_MALLOC_ERROR;	/* For the hell of it. */
      return NULL;
    }

  strcpy (pag_file, file);
  strcat (pag_file, ".pag");

  /* Call the actual routine, saving the pointer to the file information. */
  f = flags & (O_RDONLY | O_RDWR | O_CREAT | O_TRUNC);

  if (f == O_RDONLY)
    {
      open_flags = GDBM_READER;
      mode = 0;
    }
  else if (f == (O_RDWR | O_CREAT))
    {
      open_flags = GDBM_WRCREAT;
    }
  else if ((f & O_TRUNC) == O_TRUNC)
    {
      open_flags = GDBM_NEWDB;
    }
  else
    {
      open_flags = GDBM_WRITER;
      mode = 0;
    }

  if (flags & O_CLOEXEC)
    open_flags |= GDBM_CLOEXEC;

  open_flags |= GDBM_NOLOCK;
  
  dbm = calloc (1, sizeof (*dbm));
  if (!dbm)
    {
      free (pag_file);
      gdbm_errno = GDBM_MALLOC_ERROR;	/* For the hell of it. */
      return NULL;
    }
      
  dbm->file = gdbm_open (pag_file, 0, open_flags, mode, NULL);

  /* Did we successfully open the file? */
  if (dbm->file == NULL)
    {
      gdbm_errno = GDBM_FILE_OPEN_ERROR;
      free (dbm);
      dbm = NULL;
    }
  else
    {
      dbm->dirfd = ndbm_open_dir_file (file, dbm->file->desc, open_flags);
      if (dbm->dirfd == -1)
	{
	  gdbm_close (dbm->file);
	  free (dbm);
	  dbm = NULL;
	}
    }

  free (pag_file);
  return dbm;
}
