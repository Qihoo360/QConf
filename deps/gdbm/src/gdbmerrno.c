/* gdbmerrno.c - convert gdbm errors into english. */

/* This file is part of GDBM, the GNU data base manager.
   Copyright (C) 1993, 2007, 2011, 2013  Free Software Foundation, Inc.

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

/* The dbm error number is placed in the variable GDBM_ERRNO. */
gdbm_error gdbm_errno = GDBM_NO_ERROR;

/* this is not static so that applications may access the array if they
   like. it must be in the same order as the error codes! */

const char * const gdbm_errlist[_GDBM_MAX_ERRNO+1] = {
  N_("No error"),                /* GDBM_NO_ERROR               */
  N_("Malloc error"),		 /* GDBM_MALLOC_ERROR       	*/
  N_("Block size error"),	 /* GDBM_BLOCK_SIZE_ERROR   	*/
  N_("File open error"),	 /* GDBM_FILE_OPEN_ERROR    	*/
  N_("File write error"),	 /* GDBM_FILE_WRITE_ERROR   	*/
  N_("File seek error"),	 /* GDBM_FILE_SEEK_ERROR    	*/
  N_("File read error"),	 /* GDBM_FILE_READ_ERROR    	*/
  N_("Bad magic number"),	 /* GDBM_BAD_MAGIC_NUMBER   	*/
  N_("Empty database"),		 /* GDBM_EMPTY_DATABASE     	*/
  N_("Can't be reader"),	 /* GDBM_CANT_BE_READER     	*/
  N_("Can't be writer"),	 /* GDBM_CANT_BE_WRITER     	*/
  N_("Reader can't delete"),	 /* GDBM_READER_CANT_DELETE 	*/
  N_("Reader can't store"),	 /* GDBM_READER_CANT_STORE  	*/
  N_("Reader can't reorganize"), /* GDBM_READER_CANT_REORGANIZE */
  N_("Unknown update"),		 /* GDBM_UNKNOWN_UPDATE     	*/
  N_("Item not found"),		 /* GDBM_ITEM_NOT_FOUND     	*/
  N_("Reorganize failed"),	 /* GDBM_REORGANIZE_FAILED  	*/
  N_("Cannot replace"),		 /* GDBM_CANNOT_REPLACE     	*/
  N_("Illegal data"),		 /* GDBM_ILLEGAL_DATA       	*/
  N_("Option already set"),	 /* GDBM_OPT_ALREADY_SET    	*/
  N_("Illegal option"),		 /* GDBM_OPT_ILLEGAL        	*/
  N_("Byte-swapped file"),	 /* GDBM_BYTE_SWAPPED       	*/
  N_("Wrong file offset"),	 /* GDBM_BAD_FILE_OFFSET    	*/
  N_("Bad file flags"),		 /* GDBM_BAD_OPEN_FLAGS     	*/
  N_("Cannot stat file"),        /* GDBM_FILE_STAT_ERROR        */
  N_("Unexpected end of file"),  /* GDBM_FILE_EOF               */
  N_("Database name not given"), /* GDBM_NO_DBNAME              */
  N_("Failed to restore file owner"), /* GDBM_ERR_FILE_OWNER    */
  N_("Failed to restore file mode"),  /* GDBM_ERR_FILE_MODE     */
};

const char *
gdbm_strerror (gdbm_error error)
{
  if (((int)error < _GDBM_MIN_ERRNO) || ((int)error > _GDBM_MAX_ERRNO))
    {
      return _("Unknown error");
    }
  else
    {
      return gettext (gdbm_errlist[(int)error]);
    }
}
