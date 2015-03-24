%{
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

#include "gdbmtool.h"
#include "gram.h"

struct point point;
 
/* Advance locus to the next line */
void
advance_line ()
{
  ++point.line;
  point.col = 0;
}

#define YY_USER_ACTION					    \
  do							    \
    {							    \
      if (YYSTATE == 0)					    \
	{						    \
	  yylloc.beg = point;				    \
	  yylloc.beg.col++;				    \
	}						    \
      point.col += yyleng;				    \
      yylloc.end = point;				    \
    }							    \
  while (0);

#undef YY_INPUT
#define YY_INPUT(buf,result,max_size)                                   \
  do									\
    {									\
      result = read_input (buf, max_size);				\
    }									\
  while (0);
 
void string_begin (void);
void string_add (const char *s, int l);
void string_addc (int c);
char *string_end (void);
int unescape (int c);

static ssize_t read_input (char *buf, size_t size);

struct context                /* Input context */
{
  struct context *parent;     /* Pointer to the parent context */
  struct locus locus;         /* Locus */
  struct point point;
  int interactive;
  ino_t ino;                  /* Inode number */ 
  dev_t dev;                  /* Device number */
  FILE *file;                 /* Input file */
  YY_BUFFER_STATE buf;        /* Buffer */
};

static struct context *context_tos;
static ino_t ino;
static dev_t dev;
int interactive;              /* Are we running in interactive mode? */
static int initialized;
 
static void
context_push ()
{
  struct context *cp = ecalloc (1, sizeof (*cp));

  cp->locus = yylloc;
  cp->point = point;
  cp->interactive = interactive;
  cp->ino = ino;
  cp->dev = dev;
  cp->file = yyin;
  cp->buf = YY_CURRENT_BUFFER;
  cp->parent = context_tos;
  context_tos = cp;
}

int
context_pop ()
{
  struct context *cp = context_tos;

  fclose (yyin);
  yyin = NULL;
  free (point.file);
  point.file = NULL;
  memset (&yylloc, 0, sizeof (yylloc));
  
  if (!cp)
    return 1;

  context_tos = cp->parent;
  
  yylloc = cp->locus;
  point = cp->point;
  interactive = cp->interactive;
  ino = cp->ino;
  dev = cp->dev;
  yyin = cp->file;
  yy_delete_buffer (YY_CURRENT_BUFFER);
  yy_switch_to_buffer (cp->buf);

  return 0;
}

static struct context *
findctx (struct stat *st)
{
  struct context *cp;

  for (cp = context_tos; cp; cp = cp->parent)
    if (cp->dev == st->st_dev && cp->ino == st->st_ino)
      break;
  return cp;
}
 
int
setsource (const char *name, int intr)
{
  struct stat st;
  struct context *cp;
  FILE *fp;
  
  if (strcmp (name, "-") == 0)
    {
      fp = stdin;
      name = "stdin";
    }
  else
    {
      if (stat (name, &st))
	{
	  terror (_("cannot open `%s': %s"), name, strerror (errno));
	  return -1;
	}
      else if (!S_ISREG (st.st_mode))
	{
	  terror (_("%s is not a regular file"), name);
	  return -1;
	}

      cp = findctx (&st);
      if (cp)
	{
	  terror (_("recursive sourcing"));
	  if (cp->parent)
	    lerror (&cp->locus, _("%s already sourced here"), name);
	  return 1;
	}
      
      fp = fopen (name, "r");
      if (!fp)
	{
	  terror (_("cannot open %s for reading: %s"), name,
			strerror (errno));
	  return 1;
	}
    }
  
  if (yyin)
    context_push ();

  yyin = fp;
  yy_switch_to_buffer (yy_create_buffer (yyin, YY_BUF_SIZE));

  interactive = intr;
  dev = st.st_dev;
  ino = st.st_ino;
  
  point.file = estrdup (name);
  point.line = 1;
  point.col = 0;

  initialized = 1;
  
  return 0;
}
%}

%option nounput

%x STR MLSTR DEF

WS [ \t][ \t]*
IDENT [a-zA-Z_][a-zA-Z_0-9-]*
N [0-9][0-9]*
P [1-9][0-9]*
X [0-9a-fA-F]
O [0-7]

%%
^[ \t]*#[ \t]*line[ \t].*\n {
  char *p;
  char *file = NULL;
  int line, len;
  
  for (p = strchr (yytext, '#') + 1; *p == ' ' || *p == '\t'; p++);
  p += 4;
  for (; *p == ' ' || *p == '\t'; p++);

  line = strtol (p, &p, 10);
  for (; *p == ' ' || *p == '\t'; p++);

  if (*p == '"')
    {
      p++;
      len = strcspn (p, "\"");
      if (p[len] == 0)
	{
	  yyerror (_("invalid #line statement"));
	  REJECT;
	}
      file = emalloc (len + 1);
      memcpy (file, p, len);
      file[len] = 0;
      for (p += len + 1; *p == ' ' || *p == '\t'; p++);
    }
  if (*p != '\n' )
    {
      yyerror (_("invalid #line statement"));
      free (file);
      REJECT;
    }
  if (file)
    point.file = file;
  point.line = line;
  point.col = 0;
}
#.*\n              advance_line ();
#.*     /* end-of-file comment */;

<DEF>off           { return T_OFF; }
<DEF>pad           { return T_PAD; }
<DEF>0[xX]{X}{X}*  { yylval.num = strtoul (yytext, NULL, 16);
                     return T_NUM; };
<DEF>0{O}{O}*      { yylval.num = strtoul (yytext, NULL, 8);
                     return T_NUM; };
<DEF>0|{P}         { yylval.num = strtoul (yytext, NULL, 10);
                     return T_NUM; };
^[ \t]*\?          { return command_lookup ("help", &yylloc, &yylval.cmd); }
^[ \t]*{IDENT}     { char *p = yytext + strspn (yytext, " \t");
		     return command_lookup (p, &yylloc, &yylval.cmd);
		   }
<DEF>{IDENT}       { if ((yylval.type = datadef_lookup (yytext)))
		       return T_TYPE;
                     else
		       {
			 yylval.string = estrdup (yytext);
			 return T_IDENT;
		       }
                   }
{IDENT}            { yylval.string = estrdup (yytext);
                     return T_IDENT;
                   }
<INITIAL,DEF>[^ \"\t\n\[\]{},=]+  { yylval.string = estrdup (yytext);
                                    return T_WORD; }
\"[^\\\"\n]*\"     { yylval.string = emalloc (yyleng - 1);
                     memcpy (yylval.string, yytext+1, yyleng-2);
		     yylval.string[yyleng-2] = 0;
		     return T_WORD; }
\"[^\\\"\n]*\\$    { string_begin ();
                     string_add (yytext + 1, yyleng - 2);
                     BEGIN (MLSTR); }
\"[^\\\"\n]*\\.    { string_begin ();
                     string_add (yytext + 1, yyleng - 3);
		     string_addc (unescape (yytext[yyleng-1]));
                     BEGIN (STR); }
<STR,MLSTR>[^\\\"\n]*\"  { if (yyleng > 1)
		             string_add (yytext, yyleng - 1);
		           yylval.string = string_end ();
		           BEGIN (INITIAL);
                           return T_WORD; }
<STR,MLSTR>[^\\\"\n]*\\$ { string_add (yytext, yyleng - 1); }
<STR,MLSTR>[^\\\"\n]*\\. { string_add (yytext, yyleng - 2);
                           string_addc (unescape (yytext[yyleng-1])); }
<INITIAL,DEF>{WS}  ;
<DEF>\n            { advance_line (); }
\n                 { advance_line (); return '\n'; }
<INITIAL,DEF>.     return yytext[0];
%%

int
yywrap ()
{
  return context_pop ();
}

void
begin_def (void)
{
  BEGIN (DEF);
}

void
end_def (void)
{
  BEGIN (INITIAL);
} 

static ssize_t
read_input (char *buf, size_t size)
{
  if (interactive)
    {
      if (YY_AT_BOL ())
	print_prompt ();
      if (fgets (buf, size, yyin) == NULL)
	return 0;
      return strlen (buf);
    }
  return fread (buf, 1, size, yyin);
}
  

struct strseg
{
  struct strseg *next;
  int len;
  char ptr[1];
};

static struct strseg *strseg_head, *strseg_tail;

void
string_begin (void)
{
  strseg_head = strseg_tail = NULL;
}

void
strseg_attach (struct strseg *seg)
{
  seg->next = NULL;
  if (strseg_tail)
    strseg_tail->next = seg;
  else
    strseg_head = seg;
  strseg_tail = seg;
}  

void
string_add (const char *s, int l)
{
  struct strseg *seg = emalloc (sizeof (*seg) + l);
  memcpy (seg->ptr, s, l);
  seg->len = l;
  strseg_attach (seg);
}

void
string_addc (int c)
{
  struct strseg *seg = emalloc (sizeof (*seg));
  seg->ptr[0] = c;
  seg->len = 1;
  strseg_attach (seg);
}

char *
string_end (void)
{
  int len = 1;
  struct strseg *seg;
  char *ret, *p;
  
  for (seg = strseg_head; seg; seg = seg->next)
    len += seg->len;

  ret = emalloc (len);
  p = ret;
  for (seg = strseg_head; seg; )
    {
      struct strseg *next = seg->next;
      memcpy (p, seg->ptr, seg->len);
      p += seg->len;
      free (seg);
      seg = next;
    }
  *p = 0;

  strseg_head = strseg_tail = NULL;

  return ret;
}

static char transtab[] = "\\\\\"\"a\ab\bf\fn\nr\rt\tv\v";

int
unescape (int c)
{
  char *p;

  for (p = transtab; *p; p += 2)
    {
      if (*p == c)
	return p[1];
    }
  return c;
}

int
escape (int c)
{
  char *p;
  for (p = transtab + sizeof (transtab) - 2; p > transtab; p -= 2)
    {
      if (*p == c)
	return p[-1];
    }
  return 0;
}

void
vlerror (struct locus *loc, const char *fmt, va_list ap)
{
  if (!interactive)
    fprintf (stderr, "%s: ", progname);
  if (initialized && loc && loc->beg.file)
    {
      YY_LOCATION_PRINT (stderr, *loc);
      fprintf (stderr, ": ");
    }
  vfprintf (stderr, fmt, ap);
  fputc ('\n', stderr);
}

void
lerror (struct locus *loc, const char *fmt, ...)
{
  va_list ap;

  va_start (ap, fmt);
  vlerror (loc, fmt, ap);
  va_end (ap);
}


struct prompt_exp;

void
pe_file_name (struct prompt_exp *p)
{
  if (file_name)
    fwrite (file_name, strlen (file_name), 1, stdout);
}

void
pe_program_name (struct prompt_exp *p)
{
  fwrite (progname, strlen (progname), 1, stdout);
}

void
pe_package_name (struct prompt_exp *p)
{
  fwrite (PACKAGE_NAME, sizeof (PACKAGE_NAME) - 1, 1, stdout);
}

void
pe_program_version (struct prompt_exp *p)
{
  fwrite (PACKAGE_VERSION, sizeof (PACKAGE_VERSION) - 1, 1, stdout);
}

void
pe_space (struct prompt_exp *p)
{
  fwrite (" ", 1, 1, stdout);
}

struct prompt_exp
{
  int ch;
  void (*fun) (struct prompt_exp *);
  char *cache;
};

struct prompt_exp prompt_exp[] = {
  { 'f', pe_file_name },
  { 'p', pe_program_name },
  { 'P', pe_package_name },
  { 'v', pe_program_version },
  { '_', pe_space },
  { 0 }
};

static void
expand_char (int c)
{
  struct prompt_exp *p;

  if (c && c != '%')
    {
      for (p = prompt_exp; p->ch; p++)
	{
	  if (c == p->ch)
	    {
	      if (p->cache)
		free (p->cache);
	      return p->fun (p);
	    }
	}
    }
  putchar ('%');
  putchar (c);
}

char const *
psname ()
{
  if (YYSTATE == DEF || YYSTATE == MLSTR)
    return "ps2";
  return "ps1";
}

void
print_prompt ()
{
  const char *s;
  const char *prompt;

  switch (variable_get (psname (), VART_STRING, (void *) &prompt))
    {
    case VAR_OK:
      break;

    case VAR_ERR_NOTSET:
      return;
      
    default:
      abort ();
    }
  
  for (s = prompt; *s; s++)
    {
      if (*s == '%')
	{
	  if (!*++s)
	    {
	      putchar ('%');
	      break;
	    }
	  expand_char (*s);
	}
      else
	putchar (*s);
    }

  fflush (stdout);
}

