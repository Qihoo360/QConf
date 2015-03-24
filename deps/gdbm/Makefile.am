# This file is part of GDBM.                                   -*- Makefile -*-
# Copyright (C) 2007, 2011, 2013 Free Software Foundation, Inc.
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

ACLOCAL_AMFLAGS = -I m4
EXTRA_DIST = build-aux/config.rpath  NOTE-WARNING
if COMPAT_OPT
  MAYBE_COMPAT = compat
endif
if ENABLE_EXPORT
  MAYBE_EXPORT = export
endif
SUBDIRS = po src doc $(MAYBE_COMPAT) $(MAYBE_EXPORT) tests

AM_DISTCHECK_CONFIGURE_FLAGS = --enable-libgdbm-compat

dist-hook:
	rm -f $(distdir)/src/version.c; \
	d=`date '+%d/%m/%Y'`; \
	sed 's|/\*@DIST_DATE@\*/|"'"$$d"'"|' $(srcdir)/src/version.c > \
		$(distdir)/src/version.c
