/* A Bison parser, made by GNU Bison 1.875.  */

/* Skeleton parser for Yacc-like parsing with Bison,
   Copyright (C) 1984, 1989, 1990, 2000, 2001, 2002 Free Software Foundation, Inc.

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
   Foundation, Inc., 59 Temple Place - Suite 330,
   Boston, MA 02111-1307, USA.  */

/* As a special exception, when this file is copied by Bison into a
   Bison output file, you may use that output file without restriction.
   This special exception was added by the Free Software Foundation
   in version 1.24 of Bison.  */

/* Tokens.  */
#ifndef YYTOKENTYPE
# define YYTOKENTYPE
   /* Put the tokens into the symbol table, so that GDB and other debuggers
      know about them.  */
   enum yytokentype {
     NEWLINE = 258,
     TEXT_SECTION = 259,
     DATA_SECTION = 260,
     LABELDECL = 261,
     INTEGER = 262,
     LABELREF = 263,
     REGISTER = 264,
     INVALID_REGISTER = 265,
     ADDI = 266,
     ADD = 267,
     SUBI = 268,
     BRANCH = 269,
     BRANCHEQZ = 270,
     BRANCHGE = 271,
     BRANCHNE = 272,
     LOADADDR = 273,
     LOADBYTE = 274,
     LOADIMMED = 275,
     SYSCALL = 276,
     NOOP = 277,
     WORD = 278,
     BYTE = 279,
     SPACE = 280,
     ASCII = 281,
     ASCIIZ = 282,
     STRING = 283,
     SECTION_IDENT = 284
   };
#endif
#define NEWLINE 258
#define TEXT_SECTION 259
#define DATA_SECTION 260
#define LABELDECL 261
#define INTEGER 262
#define LABELREF 263
#define REGISTER 264
#define INVALID_REGISTER 265
#define ADDI 266
#define ADD 267
#define SUBI 268
#define BRANCH 269
#define BRANCHEQZ 270
#define BRANCHGE 271
#define BRANCHNE 272
#define LOADADDR 273
#define LOADBYTE 274
#define LOADIMMED 275
#define SYSCALL 276
#define NOOP 277
#define WORD 278
#define BYTE 279
#define SPACE 280
#define ASCII 281
#define ASCIIZ 282
#define STRING 283
#define SECTION_IDENT 284




#if ! defined (YYSTYPE) && ! defined (YYSTYPE_IS_DECLARED)
typedef int YYSTYPE;
# define yystype YYSTYPE /* obsolescent; will be withdrawn */
# define YYSTYPE_IS_DECLARED 1
# define YYSTYPE_IS_TRIVIAL 1
#endif

extern YYSTYPE yylval;



