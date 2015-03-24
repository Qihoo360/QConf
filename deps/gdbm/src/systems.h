/* systems.h - Most of the system dependant code and defines are here. */

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

/* Include all system headers first. */
#include <sys/types.h>
#include <stdio.h>
#if HAVE_SYS_FILE_H
# include <sys/file.h>
#endif
#include <sys/stat.h>
#include <stdlib.h>
#if HAVE_STRING_H
# include <string.h>
#else
# include <strings.h>
#endif
#include <unistd.h>
#include <fcntl.h>
#include <errno.h>

#ifndef SEEK_SET
# define SEEK_SET        0
#endif

#ifndef O_CLOEXEC
# define O_CLOEXEC 0
#endif

/* Default block size.  Some systems do not have blocksize in their
   stat record. This code uses the BSD blocksize from stat. */

#if HAVE_STRUCT_STAT_ST_BLKSIZE
# define STATBLKSIZE file_stat.st_blksize
#else
# define STATBLKSIZE 1024
#endif

/* Do we have ftruncate? */
#if HAVE_FTRUNCATE
# define TRUNCATE(dbf) ftruncate (dbf->desc, 0)
#else
# define TRUNCATE(dbf) close( open (dbf->name, O_RDWR|O_TRUNC, mode));
#endif

#ifndef STDERR_FILENO
# define STDERR_FILENO 2
#endif

/* I/O macros. */
#if HAVE_MMAP
# define __read(_dbf, _buf, _size)	_gdbm_mapped_read(_dbf, _buf, _size)
# define __write(_dbf, _buf, _size)	_gdbm_mapped_write(_dbf, _buf, _size)
# define __lseek(_dbf, _off, _whn)	_gdbm_mapped_lseek(_dbf, _off, _whn)
# define __fsync(_dbf)			_gdbm_mapped_sync(_dbf)
#else
# define __read(_dbf, _buf, _size)	read(_dbf->desc, _buf, _size)
# define __write(_dbf, _buf, _size)	write(_dbf->desc, _buf, _size)
# define __lseek(_dbf, _off, _whn)	lseek(_dbf->desc, _off, _whn)
# if HAVE_FSYNC
#  define __fsync(_dbf)			fsync(_dbf->desc)
# else
#  define __fsync(_dbf)			{ sync(); sync(); }
# endif
#endif

