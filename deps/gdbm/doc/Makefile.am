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

# Documentation

info_TEXINFOS = gdbm.texinfo
gdbm_TEXINFOS=\
  fdl.texi

man_MANS   = gdbm.3 gdbm_dump.1 gdbm_load.1 gdbmtool.1
EXTRA_DIST = $(man_MANS)

GENDOCS=gendocs.sh 

TEXI2DVI=texi2dvi -E

# Make sure you set TEXINPUTS.
# TEXINPUTS=/usr/share/texmf/pdftex/plain/misc/ is ok for most distributions
.PHONY: manual
manual:
	TEXINPUTS=$(srcdir):$(top_srcdir)/build-aux:$(TEXINPUTS) \
	 MAKEINFO="$(MAKEINFO) $(MAKEINFOFLAGS)" \
	 TEXI2DVI="$(TEXI2DVI) -t @finalout" \
	 $(GENDOCS) --texi2html $(PACKAGE) '$(PACKAGE_NAME) manual'

# Checking
check-tabs:
	@if test -n "`cat $(info_TEXINFOS) $(gdbm_TEXINFOS) | tr -d -c '\t'`"; then \
		echo "Sources contain tabs; run make untabify"; \
		false; \
	fi

check-sentence-spacing:
	@if grep -q '\. [@A-Z]' $(info_TEXINFOS) $(gdbm_TEXINFOS); then \
		echo >&2 "Sources contain single-space sentence separators"; \
		echo >&2 "Run make fix-sentence-spacing to fix"; \
	fi

check-fixmes:
	@for file in $(info_TEXINFOS) $(gdbm_TEXINFOS); \
	do \
	  sed -e = $$file | \
           sed -n 'N;/@c  *FIXME:/{s/\(^[0-9][0-9]*\).*@c  *FIXME:\(.*\)/'$$file':\1: \2/gp}'; \
	done > $@-t; \
	if [ -s $@-t ]; then \
	  echo "Unresolved FIXMEs:"; \
	  cat $@-t; \
	  rm $@-t; \
	  false; \
	else \
          rm -f $@-t; \
	fi

check-format: check-tabs check-sentence-spacing

check-docs: check-format check-fixmes

untabify:
	emacs -batch -l untabify.el $(info_TEXINFOS) $(gdbm_TEXINFOS)

fix-sentence-spacing:
	for file in $(info_TEXINFOS) $(gdbm_TEXINFOS); \
	do \
		if grep -q '\. [@A-Z]' $$file; then \
			mv $$file $${file}~; \
			sed -r 's/\. ([@A-Z])/.  \1/g' $${file}~ > $$file; \
		fi; \
	done

final: untabify fix-sentence-spacing