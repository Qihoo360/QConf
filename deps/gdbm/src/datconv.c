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

#define DEFFMT(name, type, fmt)			\
static int					\
name (FILE *fp, void *ptr, int size)            \
{                                               \
  fprintf (fp, fmt, *(type*) ptr);              \
  return size;                                  \
}

DEFFMT (f_char, char, "%c")
DEFFMT (f_short, short, "%hd")
DEFFMT (f_ushort, unsigned short, "%hu")
DEFFMT (f_int, int, "%d")
DEFFMT (f_uint, unsigned, "%u")
DEFFMT (f_long, long, "%ld")
DEFFMT (f_ulong, unsigned long, "%lu")
DEFFMT (f_llong, long long, "%lld")
DEFFMT (f_ullong, unsigned long long, "%llu")
DEFFMT (f_float, float, "%f")
DEFFMT (f_double, double, "%e")

static int
f_stringz (FILE *fp, void *ptr, int size)
{
  int sz;
  char *s;
  
  for (sz = 1, s = ptr; *s; s++, sz++)
    {
      int c;
      
      if (isprint (*s))
	fputc (*s, fp);
      else if ((c = escape (*s)))
	fprintf (fp, "\\%c", c);
      else
	fprintf (fp, "\\%03o", *s);
    }
  return sz;
}

static int
f_string (FILE *fp, void *ptr, int size)
{
  int sz;
  char *s;
  
  for (sz = 0, s = ptr; sz < size; s++, sz++)
    {
      int c;
      
      if (isprint (*s))
	fputc (*s, fp);
      else if ((c = escape (*s)))
	fprintf (fp, "\\%c", c);
      else
	fprintf (fp, "\\%03o", *s);
    }
  return sz;
}

int
s_char (struct xdatum *xd, char *str)
{
  xd_store (xd, str, 1);
  return 0;
}

#define DEFNSCAN(name, type, temptype, strto)	\
int						\
name (struct xdatum *xd, char *str)             \
{                                               \
  temptype n;                                   \
  type t;                                       \
  char *p;                                      \
                                                \
  errno = 0;                                    \
  n = strto (str, &p, 0);                       \
  if (*p)                                       \
    return 1;                                   \
  if (errno == ERANGE || (t = n) != n)          \
    return 1;                                   \
  xd_store (xd, &n, sizeof (n));                \
  return 0;                                     \
}

DEFNSCAN(s_short, short, long, strtol);
DEFNSCAN(s_ushort, unsigned short, unsigned long, strtoul);
DEFNSCAN(s_int, int, long, strtol)
DEFNSCAN(s_uint, unsigned, unsigned long, strtol)
DEFNSCAN(s_long, long, long, strtoul)
DEFNSCAN(s_ulong, unsigned long, unsigned long, strtoul)
DEFNSCAN(s_llong, long long, long long, strtoll)
DEFNSCAN(s_ullong, unsigned long long, unsigned long long, strtoull)

int
s_double (struct xdatum *xd, char *str)
{
  double d;
  char *p;
  
  errno = 0;
  d = strtod (str, &p);
  if (errno || *p)
    return 1;
  xd_store (xd, &d, sizeof (d));
  return 0;
}

int
s_float (struct xdatum *xd, char *str)
{
  float d;
  char *p;
  
  errno = 0;
  d = strtod (str, &p);
  if (errno || *p)
    return 1;
  xd_store (xd, &d, sizeof (d));
  return 0;
}

int
s_stringz (struct xdatum *xd, char *str)
{
  xd_store (xd, str, strlen (str) + 1);
  return 0;
}

int
s_string (struct xdatum *xd, char *str)
{
  xd_store (xd, str, strlen (str));
  return 0;
}

static struct datadef datatab[] = {
  { "char",     sizeof(char),      f_char, s_char },
  { "short",    sizeof(short),     f_short, s_short },
  { "ushort",   sizeof(unsigned short), f_ushort, s_ushort },
  { "int",      sizeof(int),       f_int, s_int  },
  { "unsigned", sizeof(unsigned),  f_uint, s_uint },
  { "uint",     sizeof(unsigned),  f_uint, s_uint },
  { "long",     sizeof(long),      f_long, s_long },
  { "ulong",    sizeof(unsigned long),     f_ulong, s_ulong },
  { "llong",    sizeof(long long), f_llong, s_llong },
  { "ullong",   sizeof(unsigned long long), f_ullong, s_ullong },
  { "float",    sizeof(float),     f_float, s_float }, 
  { "double",   sizeof(double),    f_double, s_double },
  { "stringz",  0, f_stringz, s_stringz },
  { "string",   0, f_string, s_string },
  { NULL }
};

struct datadef *
datadef_lookup (const char *name)
{
  struct datadef *p;

  for (p = datatab; p->name; p++)
    if (strcmp (p->name, name) == 0)
      return p;
  return NULL;
}

struct dsegm *
dsegm_new (int type)
{
  struct dsegm *p = emalloc (sizeof (*p));
  p->next = NULL;
  p->type = type;
  return p;
}

struct dsegm *
dsegm_new_field (struct datadef *type, char *id, int dim)
{
  struct dsegm *p = dsegm_new (FDEF_FLD);
  p->v.field.type = type;
  p->v.field.name = id;
  p->v.field.dim = dim;
  return p;
}

void
dsegm_free_list (struct dsegm *dp)
{
  while (dp)
    {
      struct dsegm *next = dp->next;
      free (dp);
      dp = next;
    }
}

void
datum_format (FILE *fp, datum const *dat, struct dsegm *ds)
{
  int off = 0;
  char *delim[2];
  int first_field = 1;
  
  if (!ds)
    {
      fprintf (fp, "%.*s\n", dat->dsize, dat->dptr);
      return;
    }

  if (variable_get ("delim1", VART_STRING, (void*) &delim[0]))
    abort ();
  if (variable_get ("delim2", VART_STRING, (void*) &delim[1]))
    abort ();
  
  for (; ds && off <= dat->dsize; ds = ds->next)
    {
      switch (ds->type)
	{
	case FDEF_FLD:
	  if (!first_field)
	    fwrite (delim[1], strlen (delim[1]), 1, fp);
	  if (ds->v.field.name)
	    fprintf (fp, "%s=", ds->v.field.name);
	  if (ds->v.field.dim > 1)
	    fprintf (fp, "{ ");
	  if (ds->v.field.type->format)
	    {
	      int i, n;

	      for (i = 0; i < ds->v.field.dim; i++)
		{
		  if (i)
		    fwrite (delim[0], strlen (delim[0]), 1, fp);
		  if (off + ds->v.field.type->size > dat->dsize)
		    {
		      fprintf (fp, _("(not enough data)"));
		      off += dat->dsize;
		      break;
		    }
		  else
		    {
		      n = ds->v.field.type->format (fp,
						    (char*) dat->dptr + off,
						    ds->v.field.type->size ?
						      ds->v.field.type->size :
						      dat->dsize - off);
		      off += n;
		    }
		}
	    }
	  if (ds->v.field.dim > 1)
	    fprintf (fp, " }");
	  first_field = 0;
	  break;
	  
	case FDEF_OFF:
	  off = ds->v.n;
	  break;
	  
	case FDEF_PAD:
	  off += ds->v.n;
	  break;
	}
    }
}

struct xdatum
{
  char *dptr;
  size_t dsize;
  size_t dmax;
  int off;
};

void
xd_expand (struct xdatum *xd, size_t size)
{
  if (xd->dmax < size)
    {
      xd->dptr = erealloc (xd->dptr, size);
      memset (xd->dptr + xd->dmax, 0, size - xd->dmax);
      xd->dmax = size;
    }
}
  
void
xd_store (struct xdatum *xd, void *val, size_t size)
{
  xd_expand (xd, xd->off + size);
  memcpy (xd->dptr + xd->off, val, size);
  xd->off += size;
  if (xd->off > xd->dsize)
    xd->dsize = xd->off;
} 

static int
datum_scan_notag (datum *dat, struct dsegm *ds, struct kvpair *kv)
{
  struct xdatum xd;
  int i;
  struct slist *s;
  int err = 0;
  
  memset (&xd, 0, sizeof (xd));
  
  for (; err == 0 && ds && kv; ds = ds->next, kv = kv->next)
    {
      if (kv->key)
	{
	  lerror (&kv->loc,
		       _("mixing tagged and untagged values is not allowed"));
	  err = 1;
	  break;
	}
      
      switch (ds->type)
	{
	case FDEF_FLD:
	  if (!ds->v.field.type->scan)
	    abort ();

	  switch (kv->type)
	    {
	    case KV_STRING:
	      err = ds->v.field.type->scan (&xd, kv->val.s);
	      if (err)
		lerror (&kv->loc, _("cannot convert"));
	      break;
	      
	    case KV_LIST:
	      for (i = 0, s = kv->val.l; i < ds->v.field.dim && s;
		   i++, s = s->next)
		{
		  err = ds->v.field.type->scan (&xd, s->str);
		  if (err)
		    {
		      lerror (&kv->loc,
				   _("cannot convert value #%d: %s"),
				   i, s->str);
		      break;
		    }
		}
	      /* FIXME: Warn if (s) -> "extra data" */
	    }				      
	  break;

	case FDEF_OFF:
	  xd_expand (&xd, ds->v.n);
	  xd.off = ds->v.n;
	  break;
	  
	case FDEF_PAD:
	  xd_expand (&xd, xd.off + ds->v.n);
	  xd.off += ds->v.n;
	  break;
	}
    }

  if (err)
    {
      free (xd.dptr);
      return 1;
    }

  dat->dptr  = xd.dptr;
  dat->dsize = xd.dsize;
      
  return 0;
}

static int
datum_scan_tag (datum *dat, struct dsegm *ds, struct kvpair *kv)
{
  lerror (&kv->loc, "tagged values are not yet supported");
  return 1;
}

int
datum_scan (datum *dat, struct dsegm *ds, struct kvpair *kv)
{
  return (kv->key ? datum_scan_tag : datum_scan_notag) (dat, ds, kv);
}

void
dsprint (FILE *fp, int what, struct dsegm *ds)
{
  static char *dsstr[] = { "key", "content" };
  int delim;
  
  fprintf (fp, "define %s", dsstr[what]);
  if (ds->next)
    {
      fprintf (fp, " {\n");
      delim = '\t';
    }
  else
    delim = ' ';
  for (; ds; ds = ds->next)
    {
      switch (ds->type)
	{
	case FDEF_FLD:
	  fprintf (fp, "%c%s", delim, ds->v.field.type->name);
	  if (ds->v.field.name)
	    fprintf (fp, " %s", ds->v.field.name);
	  if (ds->v.field.dim > 1)
	    fprintf (fp, "[%d]", ds->v.field.dim);
	  break;
	  
	case FDEF_OFF:
	  fprintf (fp, "%coffset %d", delim, ds->v.n);
	  break;

	case FDEF_PAD:
	  fprintf (fp, "%cpad %d", delim, ds->v.n);
	  break;
	}
      if (ds->next)
	fputc (',', fp);
      fputc ('\n', fp);
    }
  if (delim == '\t')
    fputs ("}\n", fp);
}
