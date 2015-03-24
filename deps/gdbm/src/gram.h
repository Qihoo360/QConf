/* A Bison parser, made by GNU Bison 2.3.  */

/* Skeleton interface for Bison's Yacc-like parsers in C

   Copyright (C) 1984, 1989, 1990, 2000, 2001, 2002, 2003, 2004, 2005, 2006
   Free Software Foundation, Inc.

   This program is free software; you can redistribute it and/or modify
   it under the terms of the GNU General Public License as published by
   the Free Software Foundation; either version 2, or (at your option)
   any later version.

   This program is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
   GNU General Public License for more details.

   You should have received a copy of the GNU General Public License
   along with this program; if not, write to the Free Software
   Foundation, Inc., 51 Franklin Street, Fifth Floor,
   Boston, MA 02110-1301, USA.  */

/* As a special exception, you may create a larger work that contains
   part or all of the Bison parser skeleton and distribute that work
   under terms of your choice, so long as that work isn't itself a
   parser generator using the skeleton or a modified version thereof
   as a parser skeleton.  Alternatively, if you modify or redistribute
   the parser skeleton itself, you may (at your option) remove this
   special exception, which will cause the skeleton and the resulting
   Bison output files to be licensed under the GNU General Public
   License without this special exception.

   This special exception was added by the Free Software Foundation in
   version 2.2 of Bison.  */

/* Tokens.  */
#ifndef YYTOKENTYPE
# define YYTOKENTYPE
   /* Put the tokens into the symbol table, so that GDB and other debuggers
      know about them.  */
   enum yytokentype {
     T_TYPE = 258,
     T_OFF = 259,
     T_PAD = 260,
     T_DEF = 261,
     T_SET = 262,
     T_UNSET = 263,
     T_BOGUS = 264,
     T_CMD = 265,
     T_NUM = 266,
     T_IDENT = 267,
     T_WORD = 268
   };
#endif
/* Tokens.  */
#define T_TYPE 258
#define T_OFF 259
#define T_PAD 260
#define T_DEF 261
#define T_SET 262
#define T_UNSET 263
#define T_BOGUS 264
#define T_CMD 265
#define T_NUM 266
#define T_IDENT 267
#define T_WORD 268




#if ! defined YYSTYPE && ! defined YYSTYPE_IS_DECLARED
typedef union YYSTYPE
#line 50 "gram.y"
{
  char *string;
  struct kvpair *kvpair;
  struct { struct kvpair *head, *tail; } kvlist;
  struct { struct slist *head, *tail; } slist;
  struct gdbmarg *arg;
  struct gdbmarglist arglist;
  int num;
  struct datadef *type;
  struct dsegm *dsegm;
  struct { struct dsegm *head, *tail; } dsegmlist;
  struct command *cmd;
}
/* Line 1489 of yacc.c.  */
#line 89 "gram.h"
	YYSTYPE;
# define yystype YYSTYPE /* obsolescent; will be withdrawn */
# define YYSTYPE_IS_DECLARED 1
# define YYSTYPE_IS_TRIVIAL 1
#endif

extern YYSTYPE yylval;

#if ! defined YYLTYPE && ! defined YYLTYPE_IS_DECLARED
typedef struct YYLTYPE
{
  int first_line;
  int first_column;
  int last_line;
  int last_column;
} YYLTYPE;
# define yyltype YYLTYPE /* obsolescent; will be withdrawn */
# define YYLTYPE_IS_DECLARED 1
# define YYLTYPE_IS_TRIVIAL 1
#endif

extern YYLTYPE yylloc;
