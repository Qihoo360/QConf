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

# include "autoconf.h"
# include "gdbm.h"
# include "gdbmapp.h"
# include "gdbmdefs.h"
# include <stdio.h>
# include <stdarg.h>
# include <errno.h>
# include <string.h>
# include <ctype.h>
# ifdef HAVE_GETOPT_H
#  include <getopt.h>
# endif

static int argc;
static char **argv;

static struct gdbm_option *option_tab;
static size_t option_count;
static size_t option_max;
static char *short_options;
static size_t short_option_count;
static size_t short_option_max;
#ifdef HAVE_GETOPT_LONG
static struct option *long_options;
static size_t long_option_count;
static size_t long_option_max;
#endif

#define OPT_USAGE -2

struct gdbm_option parseopt_default_options[] = {
  { 0, NULL, NULL, "" },
  { 'h', "help", NULL, N_("give this help list") },
  { 'V', "version", NULL, N_("print program version") },
  { OPT_USAGE, "usage", NULL, N_("give a short usage message") },
  { 0 }
};

#define OPT_END(opt) \
  ((opt)->opt_short == 0 && (opt)->opt_long == 0 && (opt)->opt_descr == NULL)
#define IS_OPTION(opt) \
  ((opt)->opt_short || (opt)->opt_long)
#define IS_GROUP_HEADER(opt)			\
  (!IS_OPTION(opt) && (opt)->opt_descr)
#define IS_VALID_SHORT_OPTION(opt) \
  ((opt)->opt_short > 0 && (opt)->opt_short < 127 && \
   isalnum ((opt)->opt_short))
#define IS_VALID_LONG_OPTION(opt) \
  ((opt)->opt_long != NULL)
  

static int
optcmp (const void *a, const void *b)
{
  struct gdbm_option const *ap = (struct gdbm_option const *)a;
  struct gdbm_option const *bp = (struct gdbm_option const *)b;

  while (ap->opt_flags & PARSEOPT_ALIAS)
    ap--;
  while (bp->opt_flags & PARSEOPT_ALIAS)
    bp--;
  
  if (IS_VALID_SHORT_OPTION(ap) && IS_VALID_SHORT_OPTION(bp))
    return ap->opt_short - bp->opt_short;
  if (IS_VALID_LONG_OPTION(ap) && IS_VALID_LONG_OPTION(bp))
    return strcmp (ap->opt_long, bp->opt_long);
  if (IS_VALID_LONG_OPTION(ap))
    return 1;
  return -1;
}

static void
sort_options (int start, int count)
{
  qsort (option_tab + start, count, sizeof (option_tab[0]), optcmp);
}

static size_t
sort_group (size_t start)
{
  size_t i;
  
  for (i = start; i < option_count && !IS_GROUP_HEADER (&option_tab[i]); i++)
    ;
  sort_options (start, i - start);
  return i + 1;
}

static void
sort_all_options (void)
{
  size_t start;

  /* Ensure sane start of options.  This is necessary because optcmp backs up
     until it finds an element with cleared PARSEOPT_ALIAS flag bit. */
  option_tab[0].opt_flags &= PARSEOPT_ALIAS;
  for (start = 0; start < option_count; )
    {
      if (IS_GROUP_HEADER (&option_tab[start]))
	start = sort_group (start + 1);
      else 
	start = sort_group (start);
    }
}

static void
add_options (struct gdbm_option *options)
{
  size_t optcnt = 0;
  size_t argcnt = 0;
  size_t count = 0;
  struct gdbm_option *opt;
  
  for (opt = options; !OPT_END(opt); opt++)
    {
      count++;
      if (IS_OPTION(opt))
	{
	  optcnt++;
	  if (opt->opt_arg)
	    argcnt++;
	}
    }

  if (option_count + count + 1 > option_max)
    {
      option_max = option_count + count + 1;
      option_tab = erealloc (option_tab,
			  sizeof (option_tab[0]) * option_max);
    }
  
#ifdef HAVE_GETOPT_LONG
  if (long_option_count + optcnt + 1 > long_option_max)
    {
      long_option_max = long_option_count + optcnt + 1;
      long_options = erealloc (long_options,
			       sizeof (long_options[0]) * long_option_max);
    }
#endif
  if (short_option_count + optcnt + argcnt + 1 > short_option_max)
    {
      short_option_max = short_option_count + optcnt + argcnt + 1;
      short_options = erealloc (short_options,
				sizeof (short_options[0]) * short_option_max);
    }

  for (opt = options; !OPT_END(opt); opt++)
    {
      option_tab[option_count++] = *opt;
      if (!IS_OPTION (opt))
	continue;
      if (IS_VALID_SHORT_OPTION (opt))
	{
	  short_options[short_option_count++] = opt->opt_short;
	  if (opt->opt_arg)
	    short_options[short_option_count++] = ':';
	}
#ifdef HAVE_GETOPT_LONG
      if (IS_VALID_LONG_OPTION (opt))
	{
	  long_options[long_option_count].name = opt->opt_long;
	  long_options[long_option_count].has_arg = opt->opt_arg != NULL;
	  long_options[long_option_count].flag = NULL;
	  long_options[long_option_count].val = opt->opt_short;
	  long_option_count++;
	}
#endif
    }
  short_options[short_option_count] = 0;
#ifdef HAVE_GETOPT_LONG
  memset (&long_options[long_option_count], 0,
	  sizeof long_options[long_option_count]);
#endif
}

int
parseopt_first (int pc, char **pv, struct gdbm_option *opts)
{
  free (option_tab);
  free (short_options);
  short_option_count = short_option_max = 0;
#ifdef HAVE_GETOPT_LONG
  free (long_options);
  long_option_count = long_option_max = 0;
#endif
  add_options (opts);
  add_options (parseopt_default_options);
  opterr = 0;
  argc = pc;
  argv = pv;
  return parseopt_next ();
}

#define LMARGIN 2
#define DESCRCOLUMN 30
#define RMARGIN 79
#define GROUPCOLUMN 2
#define USAGECOLUMN 13

static void
indent (size_t start, size_t col)
{
  for (; start < col; start++)
    putchar (' ');
}

static void
print_option_descr (const char *descr, size_t lmargin, size_t rmargin)
{
  while (*descr)
    {
      size_t s = 0;
      size_t i;
      size_t width = rmargin - lmargin;
      
      for (i = 0; ; i++)
	{
	  if (descr[i] == 0 || descr[i] == ' ' || descr[i] == '\t')
	    {
	      if (i > width)
		break;
	      s = i;
	      if (descr[i] == 0)
		break;
	    }
	}
      printf ("%*.*s\n", s, s, descr);
      descr += s;
      if (*descr)
	{
	  indent (0, lmargin);
	  descr++;
	}
    }
}

char *parseopt_program_name;
char *parseopt_program_doc;
char *parseopt_program_args;
const char *program_bug_address = "<" PACKAGE_BUGREPORT ">";
void (*parseopt_help_hook) (FILE *stream);

static int argsused;

size_t
print_option (size_t num)
{
  struct gdbm_option *opt = option_tab + num;
  size_t next, i;
  int delim;
  int w;
  
  if (IS_GROUP_HEADER (opt))
    {
      if (num)
	putchar ('\n');
      indent (0, GROUPCOLUMN);
      print_option_descr (gettext (opt->opt_descr),
			  GROUPCOLUMN, RMARGIN);
      putchar ('\n');
      return num + 1;
    }

  /* count aliases */
  for (next = num + 1;
       next < option_count && option_tab[next].opt_flags & PARSEOPT_ALIAS;
       next++);

  if (opt->opt_flags & PARSEOPT_HIDDEN)
    return next;

  w = 0;
  for (i = num; i < next; i++)
    {
      if (IS_VALID_SHORT_OPTION (&option_tab[i]))
	{
	  if (w == 0)
	    {
	      indent (0, LMARGIN);
	      w = LMARGIN;
	    }
	  else
	    w += printf (", ");
	  w += printf ("-%c", option_tab[i].opt_short);
	  delim = ' ';
	}
    }
#ifdef HAVE_GETOPT_LONG
  for (i = num; i < next; i++)
    {
      if (IS_VALID_LONG_OPTION (&option_tab[i]))
	{
	  if (w == 0)
	    {
	      indent (0, LMARGIN);
	      w = LMARGIN;
	    }
	  else
	    w += printf (", ");
	  w += printf ("--%s", option_tab[i].opt_long);
	  delim = '=';
	}
    }
#else
  if (!w)
    return next;
#endif
  if (opt->opt_arg)
    {
      argsused = 1;
      w += printf ("%c%s", delim, gettext (opt->opt_arg));
    }
  if (w >= DESCRCOLUMN)
    {
      putchar ('\n');
      w = 0;
    }
  indent (w, DESCRCOLUMN);
  print_option_descr (gettext (opt->opt_descr), DESCRCOLUMN, RMARGIN);

  return next;
}

void
parseopt_print_help (void)
{
  unsigned i;

  argsused = 0;

  printf ("%s %s [%s]... %s\n", _("Usage:"),
	  parseopt_program_name ? parseopt_program_name : progname,
	  _("OPTION"),
	  gettext (parseopt_program_args));
  if (parseopt_program_doc)
    print_option_descr (gettext (parseopt_program_doc), 0, RMARGIN);
  putchar ('\n');

  sort_all_options ();
  for (i = 0; i < option_count; )
    {
      i = print_option (i);
    }
  putchar ('\n');
#ifdef HAVE_GETOPT_LONG
  if (argsused)
    {
      print_option_descr (_("Mandatory or optional arguments to long options are also mandatory or optional for any corresponding short options."), 0, RMARGIN);
      putchar ('\n');
    }
#endif
  if (parseopt_help_hook)
    parseopt_help_hook (stdout);

 /* TRANSLATORS: The placeholder indicates the bug-reporting address
    for this package.  Please add _another line_ saying
    "Report translation bugs to <...>\n" with the address for translation
    bugs (typically your translation team's web or email address).  */
  printf (_("Report bugs to %s.\n"), program_bug_address);
  
#ifdef PACKAGE_URL
  printf (_("%s home page: <%s>\n"), PACKAGE_NAME, PACKAGE_URL);
#endif
}

static int
cmpidx_short (const void *a, const void *b)
{
  unsigned const *ai = (unsigned const *)a;
  unsigned const *bi = (unsigned const *)b;

  return option_tab[*ai].opt_short - option_tab[*bi].opt_short;
}
  
#ifdef HAVE_GETOPT_LONG
static int
cmpidx_long (const void *a, const void *b)
{
  unsigned const *ai = (unsigned const *)a;
  unsigned const *bi = (unsigned const *)b;
  struct gdbm_option const *ap = option_tab + *ai;
  struct gdbm_option const *bp = option_tab + *bi;
  return strcmp (ap->opt_long, bp->opt_long);
}
#endif

void
print_usage (void)
{
  unsigned i;
  unsigned n;
  char buf[RMARGIN+1];
  unsigned *idxbuf;
  unsigned nidx;
  
#define FLUSH								\
  do									\
    {									\
      buf[n] = 0;							\
      printf ("%s\n", buf);						\
      n = USAGECOLUMN;							\
      memset (buf, ' ', n);						\
    }									\
  while (0)
#define ADDC(c)							        \
  do									\
    {									\
      if (n == RMARGIN) FLUSH;						\
      buf[n++] = c;							\
    }									\
  while (0)

  idxbuf = ecalloc (option_count, sizeof (idxbuf[0]));

  n = snprintf (buf, sizeof buf, "%s %s ", _("Usage:"),
		parseopt_program_name ? parseopt_program_name : progname);

  /* Print a list of short options without arguments. */
  for (i = nidx = 0; i < option_count; i++)
    if (IS_VALID_SHORT_OPTION (&option_tab[i]) && !option_tab[i].opt_arg)
      idxbuf[nidx++] = i;

  if (nidx)
    {
      qsort (idxbuf, nidx, sizeof (idxbuf[0]), cmpidx_short);

      ADDC ('[');
      ADDC ('-');
      for (i = 0; i < nidx; i++)
	{
	  ADDC (option_tab[idxbuf[i]].opt_short);
	}
      ADDC (']');
    }

  /* Print a list of short options with arguments. */
  for (i = nidx = 0; i < option_count; i++)
    {
      if (IS_VALID_SHORT_OPTION (&option_tab[i]) && option_tab[i].opt_arg)
	idxbuf[nidx++] = i;
    }

  if (nidx)
    {
      qsort (idxbuf, nidx, sizeof (idxbuf[0]), cmpidx_short);
    
      for (i = 0; i < nidx; i++)
	{
	  struct gdbm_option *opt = option_tab + idxbuf[i];
	  const char *arg = gettext (opt->opt_arg);
	  size_t len = 5 + strlen (arg) + 1;
	  
	  if (n + len > RMARGIN) FLUSH;
	  buf[n++] = ' ';
	  buf[n++] = '[';
	  buf[n++] = '-';
	  buf[n++] = opt->opt_short;
	  buf[n++] = ' ';
	  strcpy (&buf[n], arg);
	  n += strlen (arg);
	  buf[n++] = ']';
	}
    }
  
#ifdef HAVE_GETOPT_LONG
  /* Print a list of long options */
  for (i = nidx = 0; i < option_count; i++)
    {
      if (IS_VALID_LONG_OPTION (&option_tab[i]))
	idxbuf[nidx++] = i;
    }

  if (nidx)
    {
      qsort (idxbuf, nidx, sizeof (idxbuf[0]), cmpidx_long);
	
      for (i = 0; i < nidx; i++)
	{
	  struct gdbm_option *opt = option_tab + idxbuf[i];
	  const char *arg = opt->opt_arg ? gettext (opt->opt_arg) : NULL;
	  size_t len = 3 + strlen (opt->opt_long)
	                 + (arg ? 1 + strlen (arg) : 0);
	  if (n + len > RMARGIN) FLUSH;
	  buf[n++] = ' ';
	  buf[n++] = '[';
	  buf[n++] = '-';
	  buf[n++] = '-';
	  strcpy (&buf[n], opt->opt_long);
	  n += strlen (opt->opt_long);
	  if (opt->opt_arg)
	    {
	      buf[n++] = '=';
	      strcpy (&buf[n], arg);
	      n += strlen (arg);
	    }
	  buf[n++] = ']';
	}
    }
#endif
  FLUSH;
  free (idxbuf);
}

const char version_etc_copyright[] =
  /* Do *not* mark this string for translation.  First %s is a copyright
     symbol suitable for this locale, and second %s are the copyright
     years.  */
  "Copyright %s %s Free Software Foundation, Inc";

const char license_text[] =
  "License GPLv3+: GNU GPL version 3 or later <http://gnu.org/licenses/gpl.html>\n"
  "This is free software: you are free to change and redistribute it.\n"
  "There is NO WARRANTY, to the extent permitted by law.";

void
print_version_only (void)
{
  printf ("%s (%s) %s\n",
	   parseopt_program_name ? parseopt_program_name : progname,
	   PACKAGE_NAME,
	   PACKAGE_VERSION);
  /* TRANSLATORS: Translate "(C)" to the copyright symbol
     (C-in-a-circle), if this symbol is available in the user's
     locale.  Otherwise, do not translate "(C)"; leave it as-is.  */
  printf (version_etc_copyright, _("(C)"), "2011");
  puts (license_text);
  putchar ('\n');
}


static int
handle_option (int c)
{
  switch (c)
    {
    case 'h':
      parseopt_print_help ();
      exit (0);
      
    case 'V':
      print_version_only ();
      exit (0);
      
    case OPT_USAGE:
      print_usage ();
      exit (0);
      
    default:
      break;
    }
  return 0;
}

int
parseopt_next ()
{
  int rc;
  
  do
    {
#ifdef HAVE_GETOPT_LONG
      rc = getopt_long (argc, argv, short_options, long_options, NULL);
#else
      rc = getopt (argc, argv, short_options);
#endif
    }
  while (handle_option (rc));
  return rc;
}
