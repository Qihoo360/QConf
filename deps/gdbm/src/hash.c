/* hash.c - The gdbm hash function. */

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

/* Include system configuration before all else. */
#include "autoconf.h"

#include "gdbmdefs.h"


/* This hash function computes a 31 bit value.  The value is used to index
   the hash directory using the top n bits.  It is also used in a hash bucket
   to find the home position of the element by taking the value modulo the
   bucket hash table size. */

int
_gdbm_hash (datum key)
{
  unsigned int value;	/* Used to compute the hash value.  */
  int   index;		/* Used to cycle through random values. */


  /* Set the initial value from key. */
  value = 0x238F13AF * key.dsize;
  for (index = 0; index < key.dsize; index++)
    value = (value + (key.dptr[index] << (index*5 % 24))) & 0x7FFFFFFF;

  value = (1103515243 * value + 12345) & 0x7FFFFFFF;  

  /* Return the value. */
  return((int) value);
}
