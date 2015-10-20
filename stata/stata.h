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




#ifndef STATA_H
#define STATA_H 1

#define JS_STATA_VERSION "1.0"
#define JS_STATA_EXTNAME "jstata"
#define JS_STATA_AUTHOR "Adrian Montero"

struct StataDataFile * stata_open(char * name);
struct StataDataFile * do_readStata(char * fileName);
int stata_close(struct StataDataFile *dta);
int do_stataClose(struct StataDataFile *);
json_t * do_jsReadStata(char * fileName);
char * js_stata_open(char * name);

#endif
