/* This file is part of GDBM.
   Copyright (C) 2007, 2011, 2013 Free Software Foundation, Inc.

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

#include "autoconf.h"

#if HAVE_MMAP

# include "gdbmdefs.h"

# include <sys/types.h>
# include <sys/time.h>
# include <sys/file.h>
# include <sys/stat.h>
# include <sys/mman.h>
# include <stdio.h>

/* Some systems fail to define this */
# ifndef MAP_FAILED
#  define MAP_FAILED ((void*)-1)
# endif

/* Translate current offset in the mapped region into the absolute position */
# define _GDBM_MMAPPED_POS(dbf) ((dbf)->mapped_off + (dbf)->mapped_pos)
/* Return true if the absolute offset OFF lies within the currentlty mmapped
   region */
# define _GDBM_IN_MAPPED_REGION_P(dbf, off) \
  ((off) >= (dbf)->mapped_off \
   && ((off) - (dbf)->mapped_off) < (dbf)->mapped_size)
/* Return true if the current region needs to be remapped */
# define _GDBM_NEED_REMAP(dbf) \
  (!(dbf)->mapped_region || (dbf)->mapped_pos == (dbf)->mapped_size)
/* Return the sum of the currently mapped size and DELTA */
# define SUM_FILE_SIZE(dbf, delta) \
  ((dbf)->mapped_off + (dbf)->mapped_size + (delta))

/* Store the size of the GDBM file DBF in *PSIZE.
   Return 0 on success and -1 on failure. */
int
_gdbm_file_size (GDBM_FILE dbf, off_t *psize)
{
  struct stat sb;
  if (fstat (dbf->desc, &sb))
    return -1; 
  *psize = sb.st_size;
  return 0;
}

/* Unmap the region. Reset all mapped fields to initial values. */
void
_gdbm_mapped_unmap (GDBM_FILE dbf)
{
  if (dbf->mapped_region)
    {
      munmap (dbf->mapped_region, dbf->mapped_size);
      dbf->mapped_region = NULL;
      dbf->mapped_size = 0;
      dbf->mapped_pos = 0;
      dbf->mapped_off = 0;
    }
}

/* Remap the DBF file according to dbf->{mapped_off,mapped_pos,mapped_size}.
   Take care to recompute {mapped_off,mapped_pos} so that the former lies
   on a page size boundary. */
int
_gdbm_internal_remap (GDBM_FILE dbf, size_t size)
{
  void *p;
  int flags = PROT_READ;
  size_t page_size = sysconf (_SC_PAGESIZE);

  munmap (dbf->mapped_region, dbf->mapped_size);
  dbf->mapped_size = size;

  if (size == 0)
    return 0;
  
  dbf->mapped_pos += dbf->mapped_off % page_size;
  dbf->mapped_off = (dbf->mapped_off / page_size) * page_size;

  if (dbf->read_write)
    flags |= PROT_WRITE;
  
  p = mmap (NULL, dbf->mapped_size, flags, MAP_SHARED,
	    dbf->desc, dbf->mapped_off);
  if (p == MAP_FAILED)
    {
      dbf->mapped_region = NULL;
      gdbm_errno = GDBM_MALLOC_ERROR;
      return -1;
    }
  
  dbf->mapped_region = p;
  return 0;
}

# define _REMAP_DEFAULT 0
# define _REMAP_EXTEND  1
# define _REMAP_END     2

/* Remap the GDBM file so that its mapped region ends on SIZEth byte.
   If the file is opened with write permissions, FLAG controls how
   it is expanded.  The value _REMAP_DEFAULT truncates SIZE to the
   actual file size.  The value _REMAP_EXTEND extends the file, if
   necessary, to accomodate max(SIZE,dbf->header->next_block) bytes.
   Finally, the value _REMAP_END instructs the function to use 
   max(SIZE, file_size) as the upper bound of the mapped region.

   If the file is opened read-only, FLAG is ignored and SIZE is
   truncated to the actual file size.

   The upper bound obtained that way is used as a *hint* to select
   the actual size of the mapped region. which can never exceed
   dbf->mapped_size_max.
   
   The function returns 0 on success, -1 on failure. */
int
_gdbm_mapped_remap (GDBM_FILE dbf, off_t size, int flag)
{
  off_t file_size, pos;

  if (_gdbm_file_size (dbf, &file_size))
    {
      SAVE_ERRNO (_gdbm_mapped_unmap (dbf));
      gdbm_errno = GDBM_FILE_STAT_ERROR;
      return -1; 
    }

  if (flag == _REMAP_END && size < file_size)
    size = file_size;
  
  if (dbf->read_write)
    {
      if (size > file_size)
	{
	  if (flag != _REMAP_DEFAULT)
	    {
	      char c = 0;

	      if (size < dbf->header->next_block)
		size = dbf->header->next_block;
	      lseek (dbf->desc, size - 1, SEEK_SET);
	      write (dbf->desc, &c, 1);
	      file_size = size;
	    }
	  else
	    {
	      size = file_size;
	      return 0;
	    }
	}
    }
  else
    {
      if (size > file_size)
	size = file_size;
      
      if (size == SUM_FILE_SIZE (dbf, 0))
	return 0;
    }

  pos = _GDBM_MMAPPED_POS (dbf);
  if (size > dbf->mapped_size_max)
    {
      dbf->mapped_off = pos;
      dbf->mapped_pos = 0;
      size = dbf->mapped_size_max;
      if (dbf->mapped_off + size > file_size)
	size = file_size - dbf->mapped_off;
    }
  else
    {
      dbf->mapped_pos += dbf->mapped_off;
      dbf->mapped_off = 0;
    }

  return _gdbm_internal_remap (dbf, size);
}

/* Initialize mapping system. If the file size is less than MAPPED_SIZE_MAX,
   map the entire file into the memory. Otherwise, map first MAPPED_SIZE_MAX
   bytes. */
int
_gdbm_mapped_init (GDBM_FILE dbf)
{
  if (dbf->mapped_size_max == 0)
    dbf->mapped_size_max = SIZE_T_MAX;
  return _gdbm_mapped_remap (dbf, 0, _REMAP_END);
}

/* Read LEN bytes from the GDBM file DBF into BUFFER. If mmapping is
   not initialized or if it fails, fall back to the classical read(1).
   Return number of bytes read or -1 on failure. */
ssize_t
_gdbm_mapped_read (GDBM_FILE dbf, void *buffer, size_t len)
{
  if (dbf->memory_mapping)
    {
      ssize_t total = 0;
      char *cbuf = buffer;
      
      while (len)
	{
	  size_t nbytes;

	  if (_GDBM_NEED_REMAP (dbf))
	    {
	      off_t pos = _GDBM_MMAPPED_POS (dbf);
	      if (_gdbm_mapped_remap (dbf, SUM_FILE_SIZE (dbf, len),
				      _REMAP_DEFAULT))
		{
		  int rc;

		  dbf->memory_mapping = FALSE;
		  if (lseek (dbf->desc, pos, SEEK_SET) != pos)
		    return total > 0 ? total : -1;
		  rc = read (dbf->desc, cbuf, len);
		  if (rc == -1)
		    return total > 0 ? total : -1;
		  return total + rc;
		}
	    }

	  nbytes = dbf->mapped_size - dbf->mapped_pos;
	  if (nbytes == 0)
	    break;
	  if (nbytes > len)
	    nbytes = len;

	  memcpy (cbuf, (char*) dbf->mapped_region + dbf->mapped_pos, nbytes);
	  cbuf += nbytes;
	  dbf->mapped_pos += nbytes;
	  total += nbytes;
	  len -= nbytes;
	}
      return total;
    }
  return read (dbf->desc, buffer, len);
}

/* Write LEN bytes from BUFFER to the GDBM file DBF. If mmapping is
   not initialized or if it fails, fall back to the classical write(1).
   Return number of bytes written or -1 on failure. */
ssize_t
_gdbm_mapped_write (GDBM_FILE dbf, void *buffer, size_t len)
{
  if (dbf->memory_mapping)
    {
      ssize_t total = 0;
      char *cbuf = buffer;

      while (len)
	{
	  size_t nbytes;

	  if (_GDBM_NEED_REMAP (dbf))
	    {
	      off_t pos = _GDBM_MMAPPED_POS (dbf);
	      if (_gdbm_mapped_remap (dbf, SUM_FILE_SIZE (dbf, len),
				      _REMAP_EXTEND))
		{
		  int rc;

		  dbf->memory_mapping = FALSE;
		  if (lseek (dbf->desc, pos, SEEK_SET) != pos)
		    return total > 0 ? total : -1;
		  rc = write (dbf->desc, cbuf, len);
		  if (rc == -1)
		    return total > 0 ? total : -1;
		  return total + rc;
		}
	    }

	  nbytes = dbf->mapped_size - dbf->mapped_pos;
	  if (nbytes == 0)
	    break;
	  if (nbytes > len)
	    nbytes = len;

	  memcpy ((char*) dbf->mapped_region + dbf->mapped_pos, cbuf, nbytes);
	  cbuf += nbytes;
	  dbf->mapped_pos += nbytes;
	  total += nbytes;
	  len -= nbytes;
	}
      return total;
    }
  return write (dbf->desc, buffer, len);
}

/* Seek to the offset OFFSET in the GDBM file DBF, according to the
   lseek(1)-style directive WHENCE. Return the resulting absolute
   offset or -1 in case of failure. If mmapping is not initialized or
   if it fails, fall back to the classical lseek(1).

   Return number of bytes written or -1 on failure. */
   
off_t
_gdbm_mapped_lseek (GDBM_FILE dbf, off_t offset, int whence)
{
  if (dbf->memory_mapping)
    {
      off_t needle;
      
      switch (whence)
	{
	case SEEK_SET:
	  needle = offset;
	  break;
	  
	case SEEK_CUR:
	  needle = offset + _GDBM_MMAPPED_POS (dbf);
	  break;
	  
	case SEEK_END:
	  {
 	    off_t file_size;
	    if (_gdbm_file_size (dbf, &file_size))
	      {
		gdbm_errno = GDBM_FILE_STAT_ERROR;
		return -1;
	      }
	    needle = file_size - offset; 
	    break;
	  }
	}

      if (!_GDBM_IN_MAPPED_REGION_P (dbf, needle))
	{
	  _gdbm_mapped_unmap (dbf);
	  dbf->mapped_off = needle;
	  dbf->mapped_pos = 0;
	}
      else
	dbf->mapped_pos = needle - dbf->mapped_off;
      return needle;
    }
  return lseek (dbf->desc, offset, whence);
}

/* Sync the mapped region to disk. */
int
_gdbm_mapped_sync (GDBM_FILE dbf)
{
  if (dbf->mapped_region)
    {
      return msync (dbf->mapped_region, dbf->mapped_size,
		    MS_SYNC | MS_INVALIDATE);
    }
  return fsync (dbf->desc);
}

#endif
