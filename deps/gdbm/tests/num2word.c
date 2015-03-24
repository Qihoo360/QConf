/* This file is part of GDBM test suite.
   Copyright (C) 2011 Free Software Foundation, Inc.

   GDBM is free software; you can redistribute it and/or modify
   it under the terms of the GNU General Public License as published by
   the Free Software Foundation; either version 2, or (at your option)
   any later version.

   GDBM is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
   GNU General Public License for more details.

   You should have received a copy of the GNU General Public License
   along with GDBM. If not, see <http://www.gnu.org/licenses/>.
*/
#include "autoconf.h"
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <errno.h>

const char *progname;

const char *nstr[][10] = {
  { "zero", 
    "one",
    "two",
    "three",
    "four",
    "five",
    "six",
    "seven",
    "eight",
    "nine"
  },
  { "ten",
    "eleven",
    "twelve",
    "thirteen",
    "fourteen",  
    "fifteen",   
    "sixteen",   
    "seventeen", 
    "eighteen",  
    "nineteen"
  },
  { NULL,
    NULL,
    "twenty",
    "thirty",
    "fourty",
    "fifty",
    "sixty",
    "seventy",
    "eighty",
    "ninety"
  }
};

const char *short_scale[] = {
  "one",
  "thousand",
  "million",
  "billion"
  /* End of range for 32-bit unsigned long */
};
size_t short_scale_max = sizeof (short_scale) / sizeof (short_scale[0]);

char buffer[1024];
size_t bufsize = sizeof(buffer);
size_t bufoff;
int delim = 0;

void
copy (const char *str, int dch)
{
  size_t len = strlen (str);
  if (len + !!dch > bufoff)
    abort ();
  if (dch)
    buffer[--bufoff] = dch;
  bufoff -= len;
  memcpy (buffer + bufoff, str, len);
  delim = ' ';
}

void
format_100 (unsigned long num)
{
  if (num == 0)
    ;
  else if (num < 10)
    copy (nstr[0][num], delim);
  else if (num < 20)
    copy (nstr[1][num-10], delim);
  else 
    {
      unsigned long tens = num / 10;
      num %= 10;
      if (num)
	{
	  copy (nstr[0][num], delim);
	  copy ("-", 0);
	  copy (nstr[2][tens], 0);
	}
      else
	copy (nstr[2][tens], delim);
    }
}

void
format_1000 (unsigned long num, int more)
{
  size_t n = num % 100;
  num /= 100;
  format_100 (n);
  more |= num != 0;
  if (n && more)
    copy ("and", delim);
  if (num)
    {
      copy ("hundred", delim);
      copy (nstr[0][num], delim);
    }
}


void
format_number (unsigned long num)
{
  int s = 0;
  size_t off;

  bufoff = bufsize;
  buffer[--bufoff] = 0;
  off = bufoff;
  delim = 0;
  
  do
    {
      unsigned long n = num % 1000;

      num /= 1000;
      
      if (s > 0 && ((n && off > bufoff) || num == 0))
	copy (short_scale[s], delim);
      s++;
      
      if (s > short_scale_max)
	abort ();
      
      format_1000 (n, num != 0);
    }
  while (num);
  
  if (bufoff + 1 == bufsize)
    copy (nstr[0][0], 0);
}
      

void
print_number (unsigned long num)
{
  format_number (num);
  printf ("%lu\t%s\n", num, buffer + bufoff);
}

void
print_range (unsigned long num, unsigned long to)
{
  for (; num <= to; num++)
    print_number (num);
}

unsigned long
xstrtoul (char *arg, char **endp)
{
  unsigned long num;
  char *p;

  errno = 0;
  num = strtoul (arg, &p, 10);
  if (errno)
    {
      fprintf (stderr, "%s: invalid number: ", progname);
      perror (arg);
      exit (2);
    }
  if (endp)
    *endp = p;
  else if (*p)
    {
      fprintf (stderr, "%s: invalid number (near %s)\n",
	       progname, p);
      exit (2);
    }
    
  return num;
}

int
main (int argc, char **argv)
{
  progname = argv[0];
  
  if (argc == 1 || strcmp (argv[1], "-h") == 0)
    {
      printf ("usage: %s NUM [NUM...]\n", progname);
      printf ("where NUM is a decimal number, NUM:COUNT or NUM-NUM\n");
      exit (0);
    }

  while (--argc)
    {
      char *arg = *++argv;
      unsigned long num, num2;
      char *p;

      num = xstrtoul (arg, &p);
      if (*p == 0)
	print_number (num);
      else if (*p == ':')
	{
	  *p++ = 0;
	  num2 = xstrtoul (p, NULL);
	  if (num2 == 0)
	    {
	      fprintf (stderr, "%s: invalid count\n", progname);
	      exit (2);
	    }
	  print_range (num, num + num2 - 1);
	}
      else if (*p == '-')
	{
	  *p++ = 0;
	  num2 = xstrtoul (p, NULL);
	  if (num2 < num)
	    {
	      fprintf (stderr, "%s: invalid range: %lu-%lu\n",
		       progname, num, num2);
	      exit (2);
	    }
	    
	  print_range (num, num2);
	}
      else
	{
	  fprintf (stderr, "%s: invalid argument\n", progname);
	  exit (2);
	}
    }
  exit (0);
}
