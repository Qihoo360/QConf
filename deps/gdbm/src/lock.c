/* lock.c - Implement basic file locking for GDBM. */

/* This file is part of GDBM, the GNU data base manager.
   Copyright (C) 2008, 2011, 2013 Free Software Foundation, Inc.

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

#include <errno.h>

#if HAVE_FLOCK
# ifndef LOCK_SH
#  define LOCK_SH 1
# endif

# ifndef LOCK_EX
#  define LOCK_EX 2
# endif

# ifndef LOCK_NB
#  define LOCK_NB 4
# endif

# ifndef LOCK_UN
#  define LOCK_UN 8
# endif
#endif

#if defined(F_SETLK) && defined(F_RDLCK) && defined(F_WRLCK)
# define HAVE_FCNTL_LOCK 1
#else
# define HAVE_FCNTL_LOCK 0
#endif

#if 0
int
gdbm_locked (GDBM_FILE dbf)
{
  return (dbf->lock_type != LOCKING_NONE);
}
#endif

void
_gdbm_unlock_file (GDBM_FILE dbf)
{
#if HAVE_FCNTL_LOCK
  struct flock fl;
#endif

  switch (dbf->lock_type)
    {
      case LOCKING_FLOCK:
#if HAVE_FLOCK
	flock (dbf->desc, LOCK_UN);
#endif
	break;

      case LOCKING_LOCKF:
#if HAVE_LOCKF
	lockf (dbf->desc, F_ULOCK, (off_t)0L);
#endif
	break;

      case LOCKING_FCNTL:
#if HAVE_FCNTL_LOCK
	fl.l_type = F_UNLCK;
	fl.l_whence = SEEK_SET;
	fl.l_start = fl.l_len = (off_t)0L;
	fcntl (dbf->desc, F_SETLK, &fl);
#endif
	break;

      case LOCKING_NONE:
        break;
    }

  dbf->lock_type = LOCKING_NONE;
}

/* Try each supported locking mechanism. */
int
_gdbm_lock_file (GDBM_FILE dbf)
{
#if HAVE_FCNTL_LOCK
  struct flock fl;
#endif
  int lock_val = -1;

#if HAVE_FLOCK
  if (dbf->read_write == GDBM_READER)
    lock_val = flock (dbf->desc, LOCK_SH + LOCK_NB);
  else
    lock_val = flock (dbf->desc, LOCK_EX + LOCK_NB);

  if ((lock_val == -1) && (errno == EWOULDBLOCK))
    {
      dbf->lock_type = LOCKING_NONE;
      return lock_val;
    }
  else if (lock_val != -1)
    {
      dbf->lock_type = LOCKING_FLOCK;
      return lock_val;
    }
#endif

#if HAVE_LOCKF
  /* Mask doesn't matter for lockf. */
  lock_val = lockf (dbf->desc, F_LOCK, (off_t)0L);
  if ((lock_val == -1) && (errno == EDEADLK))
    {
      dbf->lock_type = LOCKING_NONE;
      return lock_val;
    }
  else if (lock_val != -1)
    {
      dbf->lock_type = LOCKING_LOCKF;
      return lock_val;
    }
#endif

#if HAVE_FCNTL_LOCK
  /* If we're still here, try fcntl. */
  if (dbf->read_write == GDBM_READER)
    fl.l_type = F_RDLCK;
  else
    fl.l_type = F_WRLCK;
  fl.l_whence = SEEK_SET;
  fl.l_start = fl.l_len = (off_t)0L;
  lock_val = fcntl (dbf->desc, F_SETLK, &fl);

  if (lock_val != -1)
    dbf->lock_type = LOCKING_FCNTL;
#endif

  if (lock_val == -1)
    dbf->lock_type = LOCKING_NONE;
  return lock_val;
}
