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

#include "autoconf.h"
#include "gdbmdefs.h"
#include "gdbm.h"
#include "gdbmapp.h"
#include <stdlib.h>
#include <stdio.h>
#include <stdarg.h>
#include <ctype.h>

/* Position in input file */
struct point
{
  char *file;             /* file name */
  unsigned line;          /* line number */  
  unsigned col;           /* column number */ 
};

/* Location in input file */
struct locus
{
  struct point beg, end;
};

typedef struct locus gdbm_yyltype_t;
 
#define YYLTYPE gdbm_yyltype_t 

#define YYLLOC_DEFAULT(Current, Rhs, N)			      \
  do							      \
    {							      \
      if (N)						      \
	{						      \
	  (Current).beg = YYRHSLOC(Rhs, 1).beg;		      \
	  (Current).end = YYRHSLOC(Rhs, N).end;		      \
	}						      \
      else						      \
	{						      \
	  (Current).beg = YYRHSLOC(Rhs, 0).end;		      \
	  (Current).end = (Current).beg;		      \
	}						      \
    }							      \
  while (0)

#define YY_LOCATION_PRINT(File, Loc)			  \
  do							  \
    {							  \
      if ((Loc).beg.col == 0)				  \
	fprintf (File, "%s:%u",				  \
		 (Loc).beg.file,			  \
		 (Loc).beg.line);			  \
      else if (strcmp ((Loc).beg.file, (Loc).end.file))	  \
	fprintf (File, "%s:%u.%u-%s:%u.%u",		  \
		 (Loc).beg.file,			  \
		 (Loc).beg.line, (Loc).beg.col,		  \
		 (Loc).end.file,			  \
		 (Loc).end.line, (Loc).end.col);	  \
      else if ((Loc).beg.line != (Loc).end.line)	  \
	fprintf (File, "%s:%u.%u-%u.%u",		  \
		 (Loc).beg.file,			  \
		 (Loc).beg.line, (Loc).beg.col,		  \
		 (Loc).end.line, (Loc).end.col);	  \
      else if ((Loc).beg.col != (Loc).end.col)		  \
	fprintf (File, "%s:%u.%u-%u",			  \
		 (Loc).beg.file,			  \
		 (Loc).beg.line, (Loc).beg.col,		  \
		 (Loc).end.col);			  \
      else						  \
	fprintf (File, "%s:%u.%u",			  \
		 (Loc).beg.file,			  \
		 (Loc).beg.line,			  \
		 (Loc).beg.col);			  \
    }							  \
  while (0)

void vlerror (struct locus *loc, const char *fmt, va_list ap);
void lerror (struct locus *loc, const char *fmt, ...);

void terror (const char *fmt, ...);

void print_prompt (void);

int setsource (const char *filename, int intr);

extern char *file_name;
extern int interactive;
extern int open_mode;

#define GDBMTOOLRC ".gdbmtoolrc"
#define GDBMTOOL_DEFFILE "junk.gdbm"


struct slist
{
  struct slist *next;
  char *str;
};

struct slist *slist_new (char *s);
void slist_free (struct slist *);

#define KV_STRING 0
#define KV_LIST   1

struct kvpair
{
  struct kvpair *next;
  int type;
  struct locus loc;
  char *key;
  union
  {
    char *s;
    struct slist *l;
  } val;
};

struct kvpair *kvpair_string (struct locus *loc, char *val);
struct kvpair *kvpair_list (struct locus *loc, struct slist *s);


#define ARG_STRING 0
#define ARG_DATUM  1
#define ARG_KVPAIR 2
#define ARG_MAX    3

/* Argument to a command handler */
struct gdbmarg
{
  struct gdbmarg *next;
  int type;
  int ref;
  struct locus loc;
  union
  {
    char *string;
    datum dat;
    struct kvpair *kvpair;
  } v;
};

/* List of arguments */
struct gdbmarglist
{
  struct gdbmarg *head, *tail;
};

void gdbmarglist_init (struct gdbmarglist *, struct gdbmarg *);
void gdbmarglist_add (struct gdbmarglist *, struct gdbmarg *);
void gdbmarglist_free (struct gdbmarglist *lst);

struct gdbmarg *gdbmarg_string (char *, struct locus *);
struct gdbmarg *gdbmarg_datum (datum *, struct locus *);
struct gdbmarg *gdbmarg_kvpair (struct kvpair *kvl, struct locus *);

int gdbmarg_free (struct gdbmarg *arg);
void gdbmarg_destroy (struct gdbmarg **parg);

struct command;
int command_lookup (const char *str, struct locus *loc, struct command **pcmd);

int run_command (struct command *cmd, struct gdbmarglist *arglist);

struct xdatum;
void xd_expand (struct xdatum *xd, size_t size);
void xd_store (struct xdatum *xd, void *val, size_t size);


struct datadef
{
  char *name;
  int size;
  int (*format) (FILE *, void *ptr, int size);
  int (*scan) (struct xdatum *xd, char *str);
};

struct datadef *datadef_lookup (const char *name);

struct field
{
  struct datadef *type;
  int dim;
  char *name;
};

#define FDEF_FLD 0
#define FDEF_OFF 1
#define FDEF_PAD 2

struct dsegm
{
  struct dsegm *next;
  int type;
  union
  {
    int n;
    struct field field;
  } v;
};

struct dsegm *dsegm_new (int type);
struct dsegm *dsegm_new_field (struct datadef *type, char *id, int dim);
void dsegm_free_list (struct dsegm *dp);

#define DS_KEY     0
#define DS_CONTENT 1
#define DS_MAX     2

extern struct dsegm *dsdef[];

#define VART_STRING 0
#define VART_BOOL   1
#define VART_INT    2

#define VAR_OK           0       /* operation succeeded */
#define VAR_ERR_NOTSET   1       /* Only for variable_get:
				    variable is not set */ 
#define VAR_ERR_NOTDEF   2       /* no such variable */ 
#define VAR_ERR_BADTYPE  3       /* variable cannot be coerced to the
				    requested type (software error) */
#define VAR_ERR_BADVALUE 4       /* Only for variable_set: the value is
				    not valid for this variable. */

int variable_set (const char *name, int type, void *val);
int variable_get (const char *name, int type, void **val);
int variable_is_set (const char *name);
int variable_is_true (const char *name);
void variable_print_all (FILE *fp);


int unescape (int c);
int escape (int c);
void begin_def (void);
void end_def (void);

int yylex (void);
int yyerror (char *s);
int yyparse (void);

void datum_format (FILE *fp, datum const *dat, struct dsegm *ds);
int datum_scan (datum *dat, struct dsegm *ds, struct kvpair *kv);
void dsprint (FILE *fp, int what, struct dsegm *ds);

char *mkfilename (const char *dir, const char *file, const char *suf);
char *tildexpand (char *s);
int vgetyn (const char *prompt, va_list ap);
int getyn (const char *prompt, ...);
