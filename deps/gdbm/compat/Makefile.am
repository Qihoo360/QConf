# This file is part of GDBM.                                   -*- Makefile -*-
# Copyright (C) 2007, 2011 Free Software Foundation, Inc.
#
# GDBM is free software; you can redistribute it and/or modify
# it under the terms of the GNU General Public License as published by
# the Free Software Foundation; either version 3, or (at your option)
# any later version.
#
# GDBM is distributed in the hope that it will be useful,
# but WITHOUT ANY WARRANTY; without even the implied warranty of
# MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
# GNU General Public License for more details.
#
# You should have received a copy of the GNU General Public License
# along with GDBM. If not, see <http://www.gnu.org/licenses/>. */

# Flags
AM_CPPFLAGS=-I$(srcdir)/../src

# The libraries
VI_CURRENT  = 4
VI_REVISION = 0
VI_AGE      = 0

lib_LTLIBRARIES = libgdbm_compat.la
libgdbm_compat_la_LIBADD = ../src/libgdbm.la

include_HEADERS = dbm.h ndbm.h
noinst_HEADERS = dbm-priv.h

DBM_CF=\
 dbminit.c\
 delete.c\
 fetch.c\
 store.c\
 seq.c\
 close.c

NDBM_CF=\
 dbmopen.c\
 dbmdelete.c\
 dbmerr.c\
 dbmfetch.c\
 dbmstore.c\
 dbmseq.c\
 dbmclose.c\
 dbmdirfno.c\
 dbmpagfno.c\
 dbmrdonly.c

libgdbm_compat_la_SOURCES = $(DBM_CF) $(NDBM_CF)

libgdbm_compat_la_LDFLAGS = -version-info $(VI_CURRENT):$(VI_REVISION):$(VI_AGE)

