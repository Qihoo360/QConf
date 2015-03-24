/* gdbmsetopt.c - set options pertaining to a GDBM descriptor. */

/* This file is part of GDBM, the GNU data base manager.
   Copyright (C) 1993, 1994, 2007, 2011, 2013 Free Software Foundation, Inc.

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

#include "gdbmdefs.h"

/* operate on an already open descriptor. */

static int
getbool (void *optval, int optlen)
{
  int n;
  
  if (!optval || optlen != sizeof (int) ||
      (((n = *(int*)optval) != TRUE) && n != FALSE))
    {
      gdbm_errno = GDBM_OPT_ILLEGAL;
      return -1;
    }
  return n;
}

static int
get_size (void *optval, int optlen, size_t *ret)
{
  if (!optval)
    {
      gdbm_errno = GDBM_OPT_ILLEGAL;
      return -1;
    }
  if (optlen == sizeof (unsigned))
    *ret = *(unsigned*) optval;
  else if (optlen == sizeof (unsigned long))
    *ret = *(unsigned long*) optval;
  else if (optlen == sizeof (size_t))
    *ret = *(size_t*) optval;
  else
    {
      gdbm_errno = GDBM_OPT_ILLEGAL;
      return -1;
    }
  return 0;
}

int
gdbm_setopt (GDBM_FILE dbf, int optflag, void *optval, int optlen)
{
  int n;
  size_t sz;
  
  switch (optflag)
    {
      /* Cache size: */
      
      case GDBM_SETCACHESIZE:
        /* Optval will point to the new size of the cache. */
        if (dbf->bucket_cache != NULL)
          {
            gdbm_errno = GDBM_OPT_ALREADY_SET;
            return -1;
          }

	if (get_size (optval, optlen, &sz))
	  return -1;
        return _gdbm_init_cache (dbf, (sz > 9) ? sz : 10);

      case GDBM_GETCACHESIZE:
	if (!optval || optlen != sizeof (size_t))
	  {
	    gdbm_errno = GDBM_OPT_ILLEGAL;
	    return -1;
	  }
	*(size_t*) optval = dbf->cache_size;
	break;
	
      	/* Obsolete form of GDBM_SETSYNCMODE. */
      case GDBM_FASTMODE:
	if ((n = getbool (optval, optlen)) == -1)
	  return -1;
	dbf->fast_write = n;
	break;

	/* SYNC mode: */
	
      case GDBM_SETSYNCMODE:
      	/* Optval will point to either true or false. */
	if ((n = getbool (optval, optlen)) == -1)
	  return -1;
	dbf->fast_write = !n;
	break;

      case GDBM_GETSYNCMODE:
	if (!optval || optlen != sizeof (int))
	  {
	    gdbm_errno = GDBM_OPT_ILLEGAL;
	    return -1;
	  }
	*(int*) optval = !dbf->fast_write;
	break;

	/* CENTFREE - set or get the stat of the central block repository */
      case GDBM_SETCENTFREE:
      	/* Optval will point to either true or false. */
	if ((n = getbool (optval, optlen)) == -1)
	  return -1;
	dbf->central_free = n;
	break;

      case GDBM_GETCENTFREE:
	if (!optval || optlen != sizeof (int))
	  {
	    gdbm_errno = GDBM_OPT_ILLEGAL;
	    return -1;
	  }
	*(int*) optval = !dbf->central_free;
	break;

	/* Coalesce state: */
      case GDBM_SETCOALESCEBLKS:
      	/* Optval will point to either true or false. */
	if ((n = getbool (optval, optlen)) == -1)
	  return -1;
	dbf->coalesce_blocks = n;
	break;

      case GDBM_GETCOALESCEBLKS:
	if (!optval || optlen != sizeof (int))
	  {
	    gdbm_errno = GDBM_OPT_ILLEGAL;
	    return -1;
	  }
	*(int*) optval = dbf->coalesce_blocks;
	break;

	/* Mmap mode */
      case GDBM_SETMMAP:
#if HAVE_MMAP
	if ((n = getbool (optval, optlen)) == -1)
	  return -1;
	__fsync (dbf);
	if (n == dbf->memory_mapping)
	  return 0;
	if (n)
	  {
	    if (_gdbm_mapped_init (dbf) == 0)
	      dbf->memory_mapping = TRUE;
	    else
	      return -1;
	  }
	else
	  {
	    _gdbm_mapped_unmap (dbf);
	    dbf->memory_mapping = FALSE;
	  }
#else
	gdbm_errno = GDBM_OPT_ILLEGAL;
	return -1;
#endif
	break;
	
      case GDBM_GETMMAP:
	if (!optval || optlen != sizeof (int))
	  {
	    gdbm_errno = GDBM_OPT_ILLEGAL;
	    return -1;
	  }
	*(int*) optval = dbf->memory_mapping;
	break;

	/* Maximum size of a memory mapped region */
      case GDBM_SETMAXMAPSIZE:
#if HAVE_MMAP
	{
	  size_t page_size = sysconf (_SC_PAGESIZE);

	  if (get_size (optval, optlen, &sz))
	    return -1;
	  dbf->mapped_size_max = ((sz + page_size - 1) / page_size) *
	                          page_size;
	  _gdbm_mapped_init (dbf);
	  break;
	}
#else
	gdbm_errno = GDBM_OPT_ILLEGAL;
	return -1;
#endif
	
      case GDBM_GETMAXMAPSIZE:
	if (!optval || optlen != sizeof (size_t))
	  {
	    gdbm_errno = GDBM_OPT_ILLEGAL;
	    return -1;
	  }
	*(size_t*) optval = dbf->mapped_size_max;
	break;

	/* Flags */
      case GDBM_GETFLAGS:
	if (!optval || optlen != sizeof (int))
	  {
	    gdbm_errno = GDBM_OPT_ILLEGAL;
	    return -1;
	  }
	else
	  {
	    int flags = dbf->read_write;
	    if (!dbf->fast_write)
	      flags |= GDBM_SYNC;
	    if (!dbf->file_locking)
	      flags |= GDBM_NOLOCK;
	    if (!dbf->memory_mapping)
	      flags |= GDBM_NOMMAP;
	    *(int*) optval = flags;
	  }
	break;

      case GDBM_GETDBNAME:
	if (!optval || optlen != sizeof (char*))
	  {
	    gdbm_errno = GDBM_OPT_ILLEGAL;
	    return -1;
	  }
	else
	  {
	    char *p = strdup (dbf->name);
	    if (!p)
	      {
		gdbm_errno = GDBM_MALLOC_ERROR;
		return -1;
	      }
	    *(char**) optval = p;
	  }
	break;
	
      default:
        gdbm_errno = GDBM_OPT_ILLEGAL;
        return -1;
    }

  return 0;
}
