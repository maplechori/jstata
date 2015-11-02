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

// http://www.stata.com/help.cgi?dta_115

#include "stataread.h"
#include "stata.h"
//static int stata_endian;
#define R_PosInf INFINITY
#define R_NegInf -INFINITY

double NA_REAL()
{
    /* The gcc shipping with RedHat 9 gets this wrong without
*      * the volatile declaration. Thanks to Marc Schwartz. */
    volatile ieee_double x;
    x.word[hw] = 0x7ff00000;
    x.word[lw] = 1954;
    return x.value;
}




void R_SaveStataData(FILE *fp, json_int_t obs, json_int_t nvars, json_t *data, json_t *vars, json_t *labels, json_t *metadata);

/* Mainly for use in packages */
int R_finite(double x)
{
	#ifdef HAVE_WORKING_ISFINITE
	return isfinite(x);
	#else
	return (!isnan(x) & (x != R_PosInf) & (x != R_NegInf));
	#endif
}


#define R_FINITE(x)  R_finite(x)

static void OutIntegerBinary(int i, FILE * fp, int naok)
{
	i=((i == NA_INTEGER) & !naok ? STATA_INT_NA : i);
	if (fwrite(&i, sizeof(int), 1, fp) != 1)
		printf("a binary write error occurred");

}


static void OutByteBinary(unsigned char i, FILE * fp)
{
	if (fwrite(&i, sizeof(char), 1, fp) != 1)
		printf("a binary write error occurred");
}

/*
static void OutDataByteBinary(int i, FILE * fp)
{
	i=(unsigned char) ((i == NA_INTEGER) ? STATA_BYTE_NA : i);
	if (fwrite(&i, sizeof(char), 1, fp) != 1)
		printf("a binary write error occurred");
}
*/

static void OutShortIntBinary(int i,FILE * fp)
{
	unsigned char first,second;

	#ifdef WORDS_BIGENDIAN
	first = (unsigned char)(i >> 8);
	second = i & 0xff;
	#else
	first = i & 0xff;
	second = (unsigned char)(i >> 8);
	#endif
	if (fwrite(&first, sizeof(char), 1, fp) != 1)
		printf("a binary write error occurred");
	if (fwrite(&second, sizeof(char), 1, fp) != 1)
		printf("a binary write error occurred");
}


static void OutDoubleBinary(double d, FILE * fp, int naok)
{
	d = (R_FINITE(d) ? d : STATA_DOUBLE_NA);
	if (fwrite(&d, sizeof(double), 1, fp) != 1)
		printf("a binary write error occurred");
}


static void OutStringBinary(const char * buffer, FILE *fp, int nchar)
{
	if (nchar == 0)
		return;
	if (fwrite(buffer, nchar, 1, fp) != 1)
		printf("a binary write error occurred");
}


static char* nameMangleOut(char *stataname, int len)
{
	int i;
	for(i = 0; i < len; i++)
		if (stataname[i] == '.') stataname[i] = '_';
	return stataname;
}


/* Writes out a value label (name, and then labels and levels).
 * theselevels can be R_NilValue in which case the level values will be
 * written out as 1,2,3, ...
 */
static int
writeStataValueLabel(const char *labelName, json_t * theselabels, const int namelength, FILE *fp)
{
	int i = 0, txtlen = 0;
	size_t len = 0;
  printf("[]->%s\n\r", json_dumps(json_array_get(theselabels,0), JSON_VALIDATE_ONLY));
	len = 4*2*(json_array_size(theselabels) + 1); 
  printf("len: %d %d", (int)len, (int)json_array_size(theselabels)); 
	txtlen = 0;

  
  for(i=0; i<(int)json_array_size(theselabels); i++)
  {
    txtlen += json_string_length(json_array_get(json_array_get(theselabels,i),1))+1;
  }

	len += txtlen;
	OutIntegerBinary((int)len, fp, 0);
	/* length of table */
	char labelName2[namelength + 1];
  // nameMangleOut changes its arg.
	strncpy(labelName2, labelName, namelength + 1);
	OutStringBinary(nameMangleOut(labelName2, strlen(labelName)), fp, namelength);
	OutByteBinary(0, fp);
	/* label format name */
	OutByteBinary(0, fp);
	OutByteBinary(0, fp);
	OutByteBinary(0, fp);
	/*padding*/
	OutIntegerBinary(json_array_size(theselabels), fp, 0);
	OutIntegerBinary(txtlen, fp, 0);
	printf("offsets \n\r");
	/* offsets */
	len = 0;

  
  for(i=0; i<(int)json_array_size(theselabels); i++)
  {

		OutIntegerBinary((int)len, fp, 0);
    len += json_string_length(json_array_get(json_array_get(theselabels,i),1))+1;
	}
	/* values: just 1,2,3,...*/

  for(i=0; i<(int)json_array_size(theselabels); i++)
	{

		long index;

		index = json_integer_value(json_array_get(json_array_get(theselabels,i),0));
    printf("currentValue: %ld\n\r", index);
		OutIntegerBinary(index, fp, 0);
	}

	printf("levels \n\r");

	/* the actual labels */

  for(i=0; i<(int)json_array_size(theselabels); i++)
  {

    len = json_string_length(json_array_get(json_array_get(theselabels,i),1));
		OutStringBinary(json_string_value(json_array_get(json_array_get(theselabels,i),1)), fp, (int)len);
		OutByteBinary(0, fp);
		txtlen -= len+1;

		if (txtlen < 0)
			printf("this should happen: overrun");

	}

	if (txtlen > 0)
		printf("this should happen: underrun");

	return 1;
}


void R_SaveStataData(FILE *fp, json_int_t obs, json_int_t nvars, json_t *data, json_t *vars, json_t *labels, json_t *metadata)
{
	int version = 10;
	int i, j, k = 0, nvar, nobs, charlen;

	char datalabel[81];  
	char	timestamp[18], aname[33];
	char format9g[50] = "%9.0g", strformat[50] = "";
  memset(datalabel, 0, 81);
  if (json_string_length(json_object_get(metadata, "datalabel")) < 80)
      strncpy(datalabel, json_string_value(json_object_get(metadata, "datalabel")),json_string_length(json_object_get(metadata,"datalabel")));
  else
      strncpy(datalabel, "JStata", 6);
  version = abs((int) json_integer_value(json_object_get(metadata, "version")));

	int namelength = 8;
	int fmtlist_len = 12;

	if (version >= 7) namelength=32;
	if (version >= 10) fmtlist_len = 49;

	/* names are 32 characters in version 7 */
	/** first write the header **/
	if (version == 6)
		/* release */
		OutByteBinary((char) VERSION_6, fp);
	else if (version == 7)
		OutByteBinary((char) VERSION_7, fp);
	else if (version == 8)		 /* and also 9, mapped in R code */
		OutByteBinary((char) VERSION_8, fp);
	else if (version == 10)		 /* see comment above */
		OutByteBinary((char) VERSION_114, fp);
	OutByteBinary((char) CN_TYPE_NATIVE, fp);
	OutByteBinary(1, fp);		 /* filetype */
	OutByteBinary(0, fp);		 /* padding */

	nvar = (int)nvars;
	nobs = (int)obs;

	OutShortIntBinary(nvar, fp);
	OutIntegerBinary(nobs, fp, 1);

	int ** types = calloc(nvar, sizeof(int*));
	int ** wrTypes = calloc(nvar, sizeof(int*));

	for (i = 0; i < nvar; i++)
	{
		types[i] = calloc(1, sizeof(int*));
		wrTypes[i] = calloc(1, sizeof(int*));
		*(types[i]) = -1;
	}

	/* number of cases */
	datalabel[80] = '\0';
	OutStringBinary(datalabel, fp, 81);

	time_t rawtime;
	time(&rawtime);
	struct tm  *timeinfo = localtime (&rawtime);

	for(i = 0; i < 18; i++)
		timestamp[i] = 0;

  if (json_string_length(json_object_get(metadata, "timestamp")) > 17)
	    strftime(timestamp, 18, "%d %b %Y %H:%M",timeinfo);
  else
      strncpy(timestamp, json_string_value(json_object_get(metadata, "timestamp")),json_string_length(json_object_get(metadata,"timestamp")));

	timestamp[17] = 0;
	/* file creation time - zero terminated string */
	OutStringBinary(timestamp, fp, 18);

	/** write variable descriptors **/
	/* version 8, 10 */

	const char *vkey;

  for (i = 0; i < nvar; i++)
	{
		*wrTypes[i] = (int)json_integer_value(json_object_get(json_array_get(vars,i), "valueType"));

		switch(*wrTypes[i])
		{
			case STATA_SE_BYTE:
				OutByteBinary(STATA_SE_BYTE, fp);
				break;
			case STATA_SE_INT:
				OutByteBinary(STATA_SE_INT, fp);
				break;
			case STATA_SE_SHORTINT:
				OutByteBinary(STATA_SE_SHORTINT, fp);
				break;
			case STATA_SE_FLOAT:
				OutByteBinary(STATA_SE_DOUBLE, fp);
				break;
			case STATA_SE_DOUBLE:
				OutByteBinary(STATA_SE_DOUBLE, fp);
				break;
			default:
				charlen = 0;
        for (j = 0; j < nvar; j++)
        {
            k = (int) json_string_length(json_array_get(json_array_get(data, j), i));
            if (k > charlen)
            {
                charlen = k;
                *types[i] = charlen;
            }
        }
        
				if (charlen > 244)
					printf("character strings of >244 bytes in column %s will be truncated", vkey);

				charlen = ( charlen < 244) ? charlen : 244;

				if (charlen == 0)
				{
					charlen = 2;
					*types[i] = charlen;
				}

				OutByteBinary((unsigned char)(charlen+STATA_SE_STRINGOFFSET), fp);
				break;

		}

	}
	/** names truncated to 8 (or 32 for v>=7) characters**/

	for (i = 0; i < nvar;i ++)
  {
   
    strncpy(aname, json_string_value(json_object_get(json_array_get(vars,i), "name")), namelength);
    OutStringBinary(nameMangleOut(aname, namelength), fp, namelength);
    OutByteBinary(0, fp);

  }

	//    }

	/** sortlist -- not relevant **/

	for (i = 0; i < 2*(nvar+1); i++)
		OutByteBinary(0, fp);

	/** format list: arbitrarily write numbers as %9g format
	but strings need accurate types */

	for (i = 0; i < nvar; i++)
	{
		if (*types[i] != -1)
		{

			/* string types are at most 244 characters
			   so we can't get a buffer overflow in sprintf **/
			memset(strformat, 0, 50);
			sprintf(strformat, "%%%ds", *types[i]);

			OutStringBinary(strformat, fp, fmtlist_len);
		}
		else
		{
			OutStringBinary(format9g, fp, fmtlist_len);
		}
	}

	/** value labels.  These are stored as the names of label formats,
	which are themselves stored later in the file.
	The label format has the same name as the variable. **/
    for(i = 0; i < nvar; i++)
    {
  
        const char * vLabels = json_string_value(json_object_get(json_array_get(vars, i), "vlabels"));
  
        if (strlen(vLabels) == 0)
        {
            /* no label */
            for(j = 0; j < namelength+1; j++)
              OutByteBinary(0, fp);

        }
        else
        {
            strncpy(aname, vLabels, namelength);
            OutStringBinary(nameMangleOut(aname, namelength), fp, namelength);
            OutByteBinary(0, fp);
        }
  
    }


	  memset(datalabel, 0, 81);
 
    for(i = 0; i < nvar; i++)
    {

        const char * dLabels = json_string_value(json_object_get(json_array_get(vars, i), "dlabels"));

        if (strlen(dLabels) == 0)
        { 
            memset(datalabel, 0, 81);
            OutStringBinary(datalabel, fp, 81);
        }
        else
        {
            strncpy(datalabel, dLabels, 81);
            datalabel[80] = 0;
            OutStringBinary(datalabel, fp, 81);
        }
    }

	//The last block is always zeros
	OutByteBinary(0, fp);
	OutByteBinary(0, fp);
	OutByteBinary(0, fp);
	if (version >= 7)
	{

		/*longer in version 7. This is wrong in the manual*/

		OutByteBinary(0, fp);
		OutByteBinary(0, fp);
	}

	/** The Data **/
  for(i = 0; i < nobs; i++)
  {        
        
        for (j = 0; j < nvar; j++)
        {
            json_t * variables = json_array_get(json_array_get(data, i), j);
            switch((int)json_integer_value(json_object_get(json_array_get(vars, j),"valueType")))
            {
		        	case STATA_SE_BYTE:
              case STATA_SE_SHORTINT:
              case STATA_SE_INT:
                
     					if (*wrTypes[i] == STATA_SE_SHORTINT)
        					OutShortIntBinary(json_integer_value(variables), fp);
        			else
        					OutIntegerBinary(json_integer_value(variables), fp, 0);
        					break;
    				  case STATA_SE_FLOAT:
              case STATA_SE_DOUBLE:
        					OutDoubleBinary(json_real_value(variables), fp, 0);
        					break;
              default:
                k = json_string_length(json_array_get(json_array_get(data, i), j));
        				if (k == 0)
        				{
            				OutByteBinary(0, fp);
        						k=1;
        				}
                else
        				{
        						if (k > 244)
            						k = 244;
                		OutStringBinary(json_string_value(json_array_get(json_array_get(data, i), j)), fp, k);
        				}
        				int l = 0;
        				for (l = *(types[i])-k; l > 0; l--)
        				{
        					OutByteBinary(0, fp);
                }
    					      break;
              }
	      }

  	}
 
  void *iter = json_object_iter(labels);
  printf("--> %s\n\r", json_dumps(labels, JSON_VALIDATE_ONLY));
  while(iter)
  {

		char *keyStr;
    
    keyStr = (char *)json_object_iter_key(iter);
    printf("BeforeWriteStataValueLabel: %s\n\r",json_object_iter_key(iter));
		strncpy(aname, keyStr, namelength);
		writeStataValueLabel(aname, json_object_iter_value(iter), namelength, fp);
    iter = json_object_iter_next(labels, iter);
	}

	for (i=0; i < nvar; i++)
	{
		free(types[i]);
		free(wrTypes[i]);
	}
	free(types);
	free(wrTypes);

}

void do_writeStata(char *fileName, json_int_t nobs, json_int_t vars, json_t *data, json_t *variables, json_t *labels, json_t *metadata)
{
	FILE *fp;

	if ((sizeof(double) != 8) | (sizeof(int) != 4) | (sizeof(float) != 4))
		printf("cannot yet read write .dta on this platform");

	fp = fopen(fileName, "wb");
	if (!fp)
		printf("unable to open file for writing: '%s'", strerror(errno));
	R_SaveStataData(fp, nobs, vars, data, variables, labels, metadata);
	fclose(fp);
}
