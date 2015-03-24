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

#define VARF_DFL    0x00   /* Default flags -- everything disabled */
#define VARF_SET    0x01   /* Variable is set */
#define VARF_INIT   0x02   /* Variable is initialized */
#define VARF_PROT   0x04   /* Variable is protected, i.e. cannot be unset */
#define VARF_OCTAL  0x08   /* For integer variables -- use octal base */

#define VAR_IS_SET(v) ((v)->flags & (VARF_SET|VARF_INIT))

union value
{
  char *string;
  int bool;
  int num;
};

struct variable
{
  char *name;
  int type;
  int flags;
  union value v;
  int (*hook) (struct variable *, union value *);
};

static int open_hook (struct variable *, union value *);

static struct variable vartab[] = {
  /* Top-level prompt */
  { "ps1", VART_STRING, VARF_INIT, { "%p>%_" } },
  /* Second-level prompt (used within "def" block) */
  { "ps2", VART_STRING, VARF_INIT, { "%_>%_" } },
  /* This delimits array members */
  { "delim1", VART_STRING, VARF_INIT|VARF_PROT, { "," } },
  /* This delimits structure members */
  { "delim2", VART_STRING, VARF_INIT|VARF_PROT, { "," } },
  { "confirm", VART_BOOL, VARF_INIT, { num: 1 } },
  { "cachesize", VART_INT, VARF_DFL },
  { "blocksize", VART_INT, VARF_DFL },
  { "open", VART_STRING, VARF_DFL, { NULL }, open_hook },
  { "lock", VART_BOOL, VARF_INIT, { num: 1 } },
  { "mmap", VART_BOOL, VARF_INIT, { num: 1 } },
  { "sync", VART_BOOL, VARF_INIT, { num: 0 } },
  { "filemode", VART_INT, VARF_INIT|VARF_OCTAL|VARF_PROT, { num: 0644 } },
  { "pager", VART_STRING, VARF_DFL },
  { "quiet", VART_BOOL, VARF_DFL },
  { NULL }
};

static int
open_hook (struct variable *var, union value *v)
{
  static struct {
    char *s;
    int t;
  } trans[] = {
    { "newdb", GDBM_NEWDB },
    { "wrcreat", GDBM_WRCREAT },
    { "rw", GDBM_WRCREAT },
    { "reader", GDBM_READER },
    { "readonly", GDBM_READER },
    { NULL }
  };
  int i;

  if (!v)
    return VAR_ERR_BADVALUE;
  
  for (i = 0; trans[i].s; i++)
    if (strcmp (trans[i].s, v->string) == 0)
      {
	open_mode = trans[i].t;
	return VAR_OK;
      }

  return VAR_ERR_BADVALUE;
}
	    
static struct variable *
varfind (const char *name)
{
  struct variable *vp;

  for (vp = vartab; vp->name; vp++)
    if (strcmp (vp->name, name) == 0)
      return vp;
  
  return NULL;
}

typedef int (*setvar_t) (union value *, void *, int);

static int
s2s (union value *vp, void *val, int flags)
{
  vp->string = estrdup (val);
  return VAR_OK;
}

static int
b2s (union value *vp, void *val, int flags)
{
  vp->string = estrdup (*(int*)val ? "true" : "false");
  return VAR_OK;
}

static int
i2s (union value *vp, void *val, int flags)
{
  char buf[128];
  snprintf (buf, sizeof buf, "%d", *(int*)val);
  vp->string = estrdup (buf);
  return VAR_OK;
}

static int
s2b (union value *vp, void *val, int flags)
{
  static char *trueval[] = { "on", "true", "yes", NULL };
  static char *falseval[] = { "off", "false", "no", NULL };
  int i;
  unsigned long n;
  char *p;
  
  for (i = 0; trueval[i]; i++)
    if (strcasecmp (trueval[i], val) == 0)
      {
	vp->bool = 1;
	return VAR_OK;
      }
  
  for (i = 0; falseval[i]; i++)
    if (strcasecmp (falseval[i], val) == 0)
      {
	vp->bool = 0;
	return VAR_OK;
      }
  
  n = strtoul (val, &p, 0);
  if (*p)
    return VAR_ERR_BADTYPE;
  vp->bool = !!n;
  return VAR_OK;
}
  
static int
s2i (union value *vp, void *val, int flags)
{
  char *p;
  int n = strtoul (val, &p, (flags & VARF_OCTAL) ? 8 : 10);

  if (*p)
    return VAR_ERR_BADTYPE;

  vp->num = n;
  return VAR_OK;
}

static int
b2b (union value *vp, void *val, int flags)
{
  vp->bool = !!*(int*)val;
  return VAR_OK;
}

static int
b2i (union value *vp, void *val, int flags)
{
  vp->num = *(int*)val;
  return VAR_OK;
}

static int
i2i (union value *vp, void *val, int flags)
{
  vp->num = *(int*)val;
  return VAR_OK;
}

static int
i2b (union value *vp, void *val, int flags)
{
  vp->bool = *(int*)val;
  return VAR_OK;
}

static setvar_t setvar[3][3] = {
            /*    s     b    i */
  /* s */    {   s2s,  b2s, i2s },
  /* b */    {   s2b,  b2b, i2b },
  /* i */    {   s2i,  b2i, i2i }
};

int
variable_set (const char *name, int type, void *val)
{
  struct variable *vp = varfind (name);
  int rc;
  union value v, *valp;
  
  if (!vp)
    return VAR_ERR_NOTDEF;

  if (val)
    {
      memset (&v, 0, sizeof (v));
      rc = setvar[vp->type][type] (&v, val, vp->flags);
      if (rc)
	return rc;
      valp = &v; 
    }
  else
    {
      if (vp->flags & VARF_PROT)
	return VAR_ERR_BADVALUE;
      valp = NULL;
    }
  
  if (vp->hook && (rc = vp->hook (vp, valp)) != VAR_OK)
    return rc;

  if (vp->type == VART_STRING && (vp->flags & VARF_SET))
    free (vp->v.string);

  if (!val)
    {
      vp->flags &= (VARF_INIT|VARF_SET);
    }
  else
    {
      vp->v = v;
      vp->flags &= ~VARF_INIT;
      vp->flags |= VARF_SET;
    }
  
  return VAR_OK;
}

int
variable_unset (const char *name)
{
  struct variable *vp = varfind (name);
  int rc;
    
  if (!vp)
    return VAR_ERR_NOTDEF;
  if (vp->flags & VARF_PROT)
    return VAR_ERR_BADVALUE;

  if (vp->hook && (rc = vp->hook (vp, NULL)) != VAR_OK)
    return rc;

  vp->flags &= ~(VARF_INIT|VARF_SET);

  return VAR_OK;
}

int
variable_get (const char *name, int type, void **val)
{
  struct variable *vp = varfind (name);

  if (!vp)
    return VAR_ERR_NOTDEF;
  
  if (type != vp->type)
    return VAR_ERR_BADTYPE;

  if (!VAR_IS_SET (vp))
    return VAR_ERR_NOTSET;
  
  switch (vp->type)
    {
    case VART_STRING:
      *val = vp->v.string;
      break;

    case VART_BOOL:
      *(int*)val = vp->v.bool;
      break;
      
    case VART_INT:
      *(int*)val = vp->v.num;
      break;
    }

  return VAR_OK;
}

static int
varcmp (const void *a, const void *b)
{
  return strcmp (((struct variable const *)a)->name,
		 ((struct variable const *)b)->name);
}

void
variable_print_all (FILE *fp)
{
  struct variable *vp;
  char *s;
  static int sorted;
  
  if (!sorted)
    {
      qsort (vartab, sizeof (vartab) / sizeof (vartab[0]) - 1,
	     sizeof (vartab[0]), varcmp);
      sorted = 1;
    }
  
  for (vp = vartab; vp->name; vp++)
    {
      if (!VAR_IS_SET (vp))
	{
	  fprintf (fp, "# %s is unset", vp->name);
	}
      else
	{
	  switch (vp->type)
	    {
	    case VART_INT:
	      fprintf (fp, (vp->flags & VARF_OCTAL) ? "%s=%03o" : "%s=%d",
		       vp->name, vp->v.num);
	      break;
	      
	    case VART_BOOL:
	      fprintf (fp, "%s%s", vp->v.bool ? "" : "no", vp->name);
	      break;
	      
	    case VART_STRING:
	      fprintf (fp, "%s=\"", vp->name);
	      for (s = vp->v.string; *s; s++)
		{
		  int c;
		  
		  if (isprint (*s))
		    fputc (*s, fp);
		  else if ((c = escape (*s)))
		    fprintf (fp, "\\%c", c);
		  else
		    fprintf (fp, "\\%03o", *s);
		}
	      fprintf (fp, "\"");
	    }
	}
      fputc ('\n', fp);
    }
}

int
variable_is_set (const char *name)
{
  struct variable *vp = varfind (name);

  if (!vp)
    return 0;
  return VAR_IS_SET (vp);
}

int
variable_is_true (const char *name)
{
  int n;

  if (variable_get (name, VART_BOOL, (void **) &n) == VAR_OK)
    return n;
  return 0;
}
