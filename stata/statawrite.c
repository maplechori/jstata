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
static int stata_endian;
#define R_PosInf INFINITY
#define R_NegInf -INFINITY

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


static void OutDataByteBinary(int i, FILE * fp)
{
	i=(unsigned char) ((i == NA_INTEGER) ? STATA_BYTE_NA : i);
	if (fwrite(&i, sizeof(char), 1, fp) != 1)
		printf("a binary write error occurred");
}


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
	len = 4*2;  //fix->*(zend_hash_num_elements(Z_ARRVAL_PP(theselabels)) + 1);
	txtlen = 0;
	//-fix	HashPosition labelposition;
	//-fix	HashPosition labelposition2;
	//-fix	zval ** currentLabel;

	//-fix	for (zend_hash_internal_pointer_reset_ex(Z_ARRVAL_PP(theselabels), &labelposition), i=0;
	//-fix		zend_hash_get_current_data_ex(Z_ARRVAL_PP(theselabels), (void**) &currentLabel, &labelposition) == SUCCESS;
	//-fix		zend_hash_move_forward_ex(Z_ARRVAL_PP(theselabels), &labelposition), i++)
	//-fix	{
	//-fix		printf("type: %d , len: %d\n\r", Z_TYPE_PP(currentLabel), Z_STRLEN_PP(currentLabel));
	//-fix		txtlen += Z_STRLEN_PP(currentLabel) + 1;
	//-fix	}

	len += txtlen;
	OutIntegerBinary((int)len, fp, 0);
	printf("len: %ld\n\r", len);
	/* length of table */
	char labelName2[namelength + 1];
	printf("labelName: %s\n\r", labelName);
								 // nameMangleOut changes its arg.
	strncpy(labelName2, labelName, namelength + 1);
	printf("labelName2: %s\n\r", labelName2);
	OutStringBinary(nameMangleOut(labelName2, strlen(labelName)), fp, namelength);
	OutByteBinary(0, fp);
	/* label format name */
	OutByteBinary(0, fp);
	OutByteBinary(0, fp);
	OutByteBinary(0, fp);
	/*padding*/
	//-fix	OutIntegerBinary(zend_hash_num_elements(Z_ARRVAL_PP(theselabels)), fp, 0);
	OutIntegerBinary(txtlen, fp, 0);
	printf("offsets \n\r");
	/* offsets */
	len = 0;

	//-fix	for (zend_hash_internal_pointer_reset_ex(Z_ARRVAL_PP(theselabels), &labelposition), i=0;
	//-fix		zend_hash_get_current_data_ex(Z_ARRVAL_PP(theselabels), (void**) &currentLabel, &labelposition) == SUCCESS;
	//-fix		zend_hash_move_forward_ex(Z_ARRVAL_PP(theselabels), &labelposition), i++)
	{

		OutIntegerBinary((int)len, fp, 0);
		//-fix		printf("%s %d\n\r", Z_STRVAL_PP(currentLabel), Z_STRLEN_PP(currentLabel));
		//-fix		len += Z_STRLEN_PP(currentLabel) + 1;
	}
	/* values: just 1,2,3,...*/

	//if(isNull(theselevels)){
	//	for (i = 0; i < length(theselabels); i++)
	//	    OutIntegerBinary(i+1, fp, 0);
	// }
	// else{
	//	if(TYPEOF(theselevels) == INTSXP){
	//-fix	for (zend_hash_internal_pointer_reset_ex(Z_ARRVAL_PP(theselabels), &labelposition), i=0;
	//-fix		zend_hash_get_current_data_ex(Z_ARRVAL_PP(theselabels), (void**) &currentLabel, &labelposition) == SUCCESS;
	//-fix		zend_hash_move_forward_ex(Z_ARRVAL_PP(theselabels), &labelposition), i++)
	{

		char *keyStr;
		uint key_len, key_type;
		long index;
		int k;

		//-fix		key_type = zend_hash_get_current_key_ex(Z_ARRVAL_PP(theselabels), &keyStr, &key_len, &index, 0, &labelposition);
		printf("currentValue: %ld\n\r", index);
		OutIntegerBinary(index, fp, 0);
	}

	printf("levels \n\r");

	//	}
	//	else{
	//		for (i = 0; i < length(theselevels); i++)
	//		    OutIntegerBinary((int) REAL(theselevels)[i], fp, 0);
	//	}
	//  }
	/* the actual labels */

	//-fix	for (zend_hash_internal_pointer_reset_ex(Z_ARRVAL_PP(theselabels), &labelposition), i=0;
	//-fix		zend_hash_get_current_data_ex(Z_ARRVAL_PP(theselabels), (void**) &currentLabel, &labelposition) == SUCCESS;
	//-fix		zend_hash_move_forward_ex(Z_ARRVAL_PP(theselabels), &labelposition), i++)
	{

		//-fix		len = Z_STRLEN_PP(currentLabel);
		//-fix		OutStringBinary(Z_STRVAL_PP(currentLabel), fp, (int)len);
		OutByteBinary(0, fp);
		txtlen -= len+1;

		if (txtlen < 0)
			printf("this should happen: overrun");

	}

	if (txtlen > 0)
		printf("this should happen: underrun");

	return 1;
}


void R_SaveStataData(FILE *fp, json_int_t obs, json_int_t nvars, json_t *data, json_t *vars, json_t *labels)
{
	int version = 10;
	int i, j, k = 0, l, nvar, nobs, charlen;
	char datalabel[81] = "Created by JStata ",
		timestamp[18], aname[33];
	char format9g[50] = "%9.0g", strformat[50] = "";
	const char *thisnamechar;

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

	printf("Observations: %d\n\r", nobs);
	printf("Variables: %d\n\r", nvar);

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

	strftime(timestamp, 18, "%d %b %Y %H:%M",timeinfo);
	timestamp[17] = 0;
	/* file creation time - zero terminated string */
	OutStringBinary(timestamp, fp, 18);

	/** write variable descriptors **/
	/* version 8, 10 */

	const char *vkey, *dkey;
	json_t *value, *dvalue;

	void *iter = json_object_iter(vars);
	i=0;
	while(iter)
	{
		vkey = json_object_iter_key(iter);
		value = json_object_iter_value(iter);

		*wrTypes[i] = (int)json_integer_value(json_object_get(value, "valueType"));

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
        
        void *iter_data = json_object_iter(data);
    
        while (iter_data)
        {
            dkey = json_object_iter_key(iter_data);
            dvalue = json_object_iter_value(iter_data);
    
            k = (int) json_string_length(json_object_get(value, vkey));
            if (k > charlen)
            {
                charlen = k;
                *types[i] = charlen;

            }

            iter_data = json_object_iter_next(data, iter_data);
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

		iter = json_object_iter_next(vars, iter);
		i++;
	}

	/** names truncated to 8 (or 32 for v>=7) characters**/

	//    for (i = 0; i < nvar;i ++){

  iter = json_object_iter(vars);
  i=0;
  while(iter)
  {
   
    vkey = json_object_iter_key(iter);
    strncpy(aname, vkey, namelength);
    OutStringBinary(nameMangleOut(aname, namelength), fp, namelength);
    OutByteBinary(0, fp);

    iter = json_object_iter_next(vars, iter);
    i++;
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
    iter = json_object_iter(vars);
    i=0;
    while(iter)
    {
        vkey = json_object_iter_key(iter);
        value = json_object_iter_value(iter);
  
        const char * vLabels = json_string_value(json_object_get(value, "vlabels"));
  
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
  
        iter = json_object_iter_next(vars, iter);
        i++;
    }


	  memset(datalabel, 0, 81);
 
    iter = json_object_iter(vars);
    i=0;
    while(iter)
    {
        vkey = json_object_iter_key(iter);
        value = json_object_iter_value(iter);

        const char * dLabels = json_string_value(json_object_get(value, "dlabels"));

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
        iter = json_object_iter_next(vars, iter);
        i++;
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
    i=0;
    while(iter)
    {        
        iter = json_object_iter(data);
        
        vkey = json_object_iter_key(iter);
        value = json_object_iter_value(iter);
      
        
        
        iter = json_object_iter_next(vars, iter);
        i++;
    }


//->fix			key_type = zend_hash_get_current_key_ex(Z_ARRVAL_PP(observations), &keyStr, &key_len, &index, 0, &variable_position);

//->fix			printf("%s", keyStr);

//->fix			switch(Z_TYPE_PP(variables))
//->fix			{
//->fix				case IS_LONG:
//->fix				case IS_BOOL:
//->fix					printf("IS LONG %ld\n\r", Z_LVAL_PP(variables));
//->fix					printf("%ld %d", Z_LVAL_PP(variables), *types[i]);
//->fix					if (*wrTypes[i] == STATA_SE_SHORTINT)
//->fix						OutShortIntBinary(Z_LVAL_PP(variables), fp);
//->fix					else
//->fix						OutIntegerBinary(Z_LVAL_PP(variables), fp, 0);
//->fix					break;
//->fix				case IS_DOUBLE:
//->fix					OutDoubleBinary(Z_DVAL_PP(variables), fp, 0);
//->fix					printf("IS DOUBLE %lf\n\r", Z_DVAL_PP(variables));
//->fix					break;
					/*
								case IS_BOOL:
									//printf("IS BOOL %s\n\r", Z_LVAL_PP(variables) ? "TRUE" : "FALSE");
									OutDataByteBinary(Z_LVAL_PP(variables), fp);
									break;
					*/
//->fix				case IS_STRING:
//->fix					printf("IS STRING %s %d\n\r", Z_STRVAL_PP(variables), Z_STRLEN_PP(variables));
//->fix					k = Z_STRLEN_PP(variables);
//->fix					if (k == 0)
//->fix					{
						//k=1;
						//printf("WRITING X\n\r");
						//OutStringBinary(" ", fp,1); //printf("empty string is not valid in Stata's documented format\n\r");
						//for (l = 1; l < *types[i]; l++)
//->fix						OutByteBinary(0, fp);
						//printf("types before: %d\n\r", *types[i]);
//->fix						k=1;
						//printf("types after: %d\n\r", *types[i]);
//->fix					}
//->fix					else
//->fix					{
//->fix						if (k > 244)
//->fix							k = 244;
//->fix						OutStringBinary(Z_STRVAL_PP(variables), fp, k);
//->fix					}
//->fix					int l = 0;
//->fix					for (l = *(types[i])-k; l > 0; l--)
//->fix					{
						//printf("data str: %d %d %d\n\r", *types[i], l, k);
//->fix						OutByteBinary(0, fp);
//->fix					}
//->fix					break;
//->fix				default:
//->fix					printf("this should not happen\n\r");
//->fix					break;

	//->fix		}

//->fix		}
//->fix	}

	/** value labels: pp92-94 of 'Programming' manual in v7.0 **/
	//PROTECT(curr_val_labels = getAttrib(df, install("val.labels")));
	//-fix	HashPosition valuePosition;
	//-fix	zval ** value_labels;
	//-fix	zval ** inner_labels;
	//-fix	zend_hash_find(Z_ARRVAL_P(labels), "labels", sizeof("labels"), (void **)&value_labels);

	//-fix	for (zend_hash_internal_pointer_reset_ex(Z_ARRVAL_PP(value_labels), &valuePosition);
	//-fix		zend_hash_get_current_data_ex(Z_ARRVAL_PP(value_labels), (void**) &inner_labels, &valuePosition) == SUCCESS;
	//-fix		zend_hash_move_forward_ex(Z_ARRVAL_PP(value_labels), &valuePosition))
	{

		char *keyStr;
		uint key_len, key_type;
		long index;
		int k;

		//-fix		key_type = zend_hash_get_current_key_ex(Z_ARRVAL_PP(value_labels), &keyStr, &key_len, &index, 0, &valuePosition);
		strncpy(aname, keyStr, namelength);
		// 	printf("count: %s %d\n\r", keyStr, zend_hash_num_elements(Z_ARRVAL_PP(inner_labels)));
		//-fix		writeStataValueLabel(aname, inner_labels, 0, namelength, fp);
	}

	//for(i = 0;i < nvar; i++){
	//	theselabels = VECTOR_ELT(leveltable, i);
	//	if (!isNull(theselabels)){
	//If we remember what the value label was called, use that. Otherwise use the var name
	//	    if(!isNull(curr_val_labels) && isString(curr_val_labels) && LENGTH(curr_val_labels) > i)
	//	    	strncpy(aname, CHAR(STRING_ELT(curr_val_labels, i)), namelength);
	//	    else
	//		strncpy(aname, CHAR(STRING_ELT(names, i)), namelength);

	//	    writeStataValueLabel(aname, theselabels, R_NilValue, namelength, fp);
	//	}
	//  }

	for (i=0; i < nvar; i++)
	{
		free(types[i]);
		free(wrTypes[i]);
	}
	free(types);
	free(wrTypes);

}

int js_stata_write(char * fileName, char * jsonStringified)
{
    json_t * json_r;

    json_error_t error;
    json_r = json_loads(jsonStringified, JSON_VALIDATE_ONLY, &error);
    if (!json_r)
    {
        return -1;
    }
    
    json_int_t obs = json_integer_value(json_object_get(json_object_get(json_r, "metadata"), "observations"));
    json_int_t vars = json_integer_value(json_object_get(json_object_get(json_r, "metadata"), "variables"));

    printf("obs: %d variables: %d\n\r", (int)obs, (int)vars);

    do_writeStata(fileName, obs, vars, json_object_get(json_r, "data"), json_object_get(json_r, "variables"), json_object_get(json_r, "labels"), json_object_get(json_r, "metadata"));
    return 0;
}

void do_writeStata(char *fileName, json_int_t nobs, json_int_t vars, json_t *data, json_t *variables, json_t *labels, json_t *metadata)
{
	FILE *fp;

	if ((sizeof(double) != 8) | (sizeof(int) != 4) | (sizeof(float) != 4))
		printf("cannot yet read write .dta on this platform");

	fp = fopen(fileName, "wb");
	if (!fp)
		printf("unable to open file for writing: '%s'", strerror(errno));
	//R_SaveStataData(fp, nobs, vars, data, variables, labels);
	fclose(fp);

}
