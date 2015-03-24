/* This file is part of GDBM, the GNU data base manager.
   Copyright (C) 2011, 2013 Free Software Foundation, Inc.

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

#include <stdlib.h>
#include <stdarg.h>
#include "gettext.h"
#ifdef HAVE_LOCALE_H
# include <locale.h>
#endif

extern const char *progname;

void set_progname (const char *arg);
void gdbm_perror (const char *fmt, ...);
void sys_perror (int code, const char *fmt, ...);
void error (const char *fmt, ...);
void verror (const char *fmt, va_list ap);

void *emalloc (size_t size);
void *erealloc (void *ptr, size_t size);
void *ecalloc (size_t nmemb, size_t size);
void *ezalloc (size_t size);
char *estrdup (const char *str);

#define PARSEOPT_HIDDEN 0x01
#define PARSEOPT_ALIAS  0x02

struct gdbm_option
{
  int opt_short;
  char *opt_long;
  char *opt_arg;
  char *opt_descr;
  int opt_flags;
};

int parseopt_first (int pc, char **pv, struct gdbm_option *options);
int parseopt_next (void);
void parseopt_print_help (void);

extern char *parseopt_program_name;
extern char *parseopt_program_doc;
extern char *parseopt_program_args;

/* Application exit codes */
#define EXIT_OK      0
#define EXIT_FATAL   1
#define EXIT_MILD    2
#define EXIT_USAGE   3 

