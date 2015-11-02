/*
 *  PHP Stata Extension 
 *  Copyright (C) 2014 Adrian Montero
 *
 *  This program is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation; either version 2 of the License, or
 *  (at your option) any later version.
 *
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with this program; if not, a copy is available at
 *  http://www.gnu.org/licenses/gpl-2.0.html
 */




#ifndef STATAREAD_H
#define STATAREAD_H

#include <stdio.h>
#include <stdlib.h> /* for abs */
#include <malloc.h>
#include <errno.h>
#include "swap_bytes.h"
#include <math.h>
#include <string.h>
#include <limits.h>
#include <string.h>
#include <jansson.h>
#include <time.h>
/* versions */
#define VERSION_5 0x69
#define VERSION_6 'l'
#define VERSION_7 0x6e
#define VERSION_7SE 111
#define VERSION_8 113
#define VERSION_114 114
#define VERSION_115 115

#define STATA_FLOAT  'f'
#define STATA_DOUBLE 'd'
#define STATA_INT    'l'
#define STATA_SHORTINT 'i'
#define STATA_BYTE  'b'

#define STATA_SE_STRINGOFFSET 0
#define STATA_SE_FLOAT  254
#define STATA_SE_DOUBLE 255
#define STATA_SE_INT    253
#define STATA_SE_SHORTINT 252
#define STATA_SE_BYTE  251

#define STATA_STRINGOFFSET 0x7f

#define STATA_BYTE_NA 127
#define STATA_SHORTINT_NA 32767
#define STATA_INT_NA 2147483647

#define STATA_FLOAT_NA pow(2.0, 127)
#define STATA_DOUBLE_NA pow(2.0, 1023)
#define NA_INTEGER (-2147483648)


#define         CN_TYPE_BIG   1
#define         CN_TYPE_LITTLE   2
#define         CN_TYPE_IEEEL   CN_TYPE_LITTLE
#define         CN_TYPE_NATIVE   CN_TYPE_IEEEL




typedef union
{
    double value;
    unsigned int word[2];
} ieee_double;


#ifdef WORDS_BIGENDIAN
static int hw = 0;
static int lw = 1;
#else  /* !WORDS_BIGENDIAN */
static int hw = 1;
static int lw = 0;
#endif /* WORDS_BIGENDIAN */


//#define NA_REAL R_ValueOfNA()

double NA_REAL();

struct StataLabel
{
    struct StataLabel * next;
	  char *name;
    int value;
    char *string;
};

struct StataObservationData
{
     int n;
     struct StataObservationData * next;
     union
     {
          double d;
          int i;
          char string[256];
     } value;

};

struct StataObservation
{
    int n;
    struct StataObservationData * data;
    struct StataObservation * next;
};

struct StataVariable
{
        struct StataVariable * next;
        int valueType;
        union
        {
          double d;
          int i;
          char string[256];
        } value;
        char * name;
        char * vfmt;
        char * vlabels;
        char * dlabels;

};


struct StataDataFile
{
   int version;
   int nvar;
   int nobs;
   char *datalabel;
   char *timestamp;
   struct StataObservation * observations;
   struct StataVariable * variables;
   int nlabels;
   struct StataLabel * labels;

};


#endif
