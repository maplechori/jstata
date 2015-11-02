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

// make sure to chcon the file "chcon -t httpd_sys_content_t someFile" and issue plenty of memory

#include "stataread.h"
static int stata_endian;

/** Low-level input **/

static int InIntegerBinary(FILE * fp, int naok, int swapends)
{
	int i;
	if (fread(&i, sizeof(int), 1, fp) != 1)
		printf("a binary read error occurred");
	if (swapends)
		reverse_int(i);
	return ((i == STATA_INT_NA) & !naok ? NA_INTEGER : i);
}


/* read a 1-byte signed integer */
static int InByteBinary(FILE * fp, int naok)
{
	signed char i;
	if (fread(&i, sizeof(char), 1, fp) != 1)
		printf("a binary read error occurred");
	return  ((i == STATA_BYTE_NA) & !naok ? NA_INTEGER : (int) i);
}


/* read a single byte  */
static int RawByteBinary(FILE * fp, int naok)
{
	unsigned char i;
	if (fread(&i, sizeof(char), 1, fp) != 1)
		printf("a binary read error occurred");
	return  ((i == STATA_BYTE_NA) & !naok ? NA_INTEGER : (int) i);
}


static int InShortIntBinary(FILE * fp, int naok,int swapends)
{
	unsigned first,second;
	int result;

	first = RawByteBinary(fp,1);
	second = RawByteBinary(fp,1);
	if (stata_endian == CN_TYPE_BIG)
	{
		result= (first<<8) | second;
	}
	else
	{
		result= (second<<8) | first;
	}
	if (result > STATA_SHORTINT_NA) result -= 65536;
	return ((result == STATA_SHORTINT_NA) & !naok ? NA_INTEGER  : result);
}


static double InDoubleBinary(FILE * fp, int naok, int swapends)
{
	double i;
	if (fread(&i, sizeof(double), 1, fp) != 1)
		printf("a binary read error occurred");
	if (swapends)
		reverse_double(i);
	return ((i == STATA_DOUBLE_NA) & !naok ? NA_REAL() : i);
}


static double InFloatBinary(FILE * fp, int naok, int swapends)
{
	float i;
	if (fread(&i, sizeof(float), 1, fp) != 1)
		printf("a binary read error occurred");
	if (swapends)
		reverse_float(i);
	return ((i == STATA_FLOAT_NA) & !naok ? NA_REAL() :  (double) i);
}


static void InStringBinary(FILE * fp, int nchar, char* buffer)
{
	if (fread(buffer, nchar, 1, fp) != 1)
		printf("a binary read error occurred");
}


json_t * R_jsLoadStataData(FILE *fp)
{
    int i, j = 0, nvar, nobs, charlen, version, swapends,
    varnamelength, nlabels, totlen, res;
    unsigned char abyte;
    /* timestamp is used for timestamp and for variable formats */
    char datalabel[81], timestamp[50], aname[33];
    char stringbuffer[245], *txt;
    int *off;
    int fmtlist_len = 12;
    
    json_t * stata_js;
    abyte = (unsigned char) RawByteBinary(fp, 1);
    version = 0;         /* -Wall */
    varnamelength = 0;       
    json_object_seed(1);

  switch (abyte)
  {
    case VERSION_5:
      version = 5;
      varnamelength = 8;
      break;
    case VERSION_6:
      version = 6;
      varnamelength = 8;
      break;
    case VERSION_7:
      version = 7;
      varnamelength = 32;
      break;
    case VERSION_7SE:
      version = -7;
      varnamelength = 32;
      break;
    case VERSION_8:
      version = -8;    /* version 8 automatically uses SE format */
      varnamelength = 32;
      break;
    case VERSION_114:
      version = -10;
      varnamelength = 32;
      fmtlist_len = 49;
    case VERSION_115:
      /* Stata say the formats are identical,
         but _115 allows business dates */
      version = -12;
      varnamelength = 32;
      fmtlist_len = 49;
      break;
    default:
      printf("not a Stata version 5-12 .dta file");
    }
    /* byte ordering */
    stata_endian = (int) RawByteBinary(fp, 1);
    swapends = stata_endian != CN_TYPE_NATIVE;
    
    RawByteBinary(fp, 1);    /* filetype -- junk */
    RawByteBinary(fp, 1);    /* padding */
    /* number of variables */
    nvar = (InShortIntBinary(fp, 1, swapends));
    /* number of cases */
    nobs = (InIntegerBinary(fp, 1, swapends));

    json_t * holder = json_object();


  /* data label - zero terminated string */
    switch (abs(version))
    {
      case 5:
        InStringBinary(fp, 32, datalabel);
        break;
      case 6:
      case 7:
      case 8:
      case 10:
      case 12:
        InStringBinary(fp, 81, datalabel);
        break;
    }
    /* file creation time - zero terminated string */
    InStringBinary(fp, 18, timestamp);

    json_object_set( holder, "observations", json_integer(nobs));
    json_object_set( holder, "variables",  json_integer(nvar));
    json_object_set( holder, "datalabel", json_string(datalabel));
    json_object_set( holder, "version", json_integer(version));
    json_object_set( holder, "timestamp", json_string(timestamp));
    stata_js = json_object();
    json_object_set(stata_js, "metadata", holder);

    /** and now stick the labels on it **/
    /** read variable descriptors **/
    /** types **/
  
    json_t * valueTypeArray = json_array();
    if (version > 0)
    {
      for(i = 0; i < nvar; i++)
      {
  
        abyte = (unsigned char) RawByteBinary(fp, 1);
        json_array_append(valueTypeArray, json_integer(abyte));
        switch (abyte)
        {
          case STATA_FLOAT:
          case STATA_DOUBLE:
            break;
          case STATA_INT:
          case STATA_SHORTINT:
          case STATA_BYTE:
            break;
          default:
            if (abyte < STATA_STRINGOFFSET)
              printf("unknown data type");
         break;
        }
      }
    }
    else
    {
  
      for(i = 0; i < nvar; i++)
      {
  
        abyte = (unsigned char) RawByteBinary(fp, 1);
        json_array_append(valueTypeArray, json_integer(abyte));

        switch (abyte)
        {
          case STATA_SE_FLOAT:
          case STATA_SE_DOUBLE:
            break;
          case STATA_SE_INT:
          case STATA_SE_SHORTINT:
          case STATA_SE_BYTE:
            break;
          default:
            if (abyte > 244)
              printf("unknown data type");
            break;
        }
    }
  }

    /** names **/
    json_t * namedVariables = json_array(); 
 
    for (i = 0; i < nvar; i++)
    {
      InStringBinary(fp, varnamelength+1, aname);
      json_t * varname = json_object();
      json_object_set(varname, "name", json_string(aname));
      json_array_append(namedVariables, varname );
    }

    

    /** sortlist -- not relevant **/
    for (i = 0; i < 2*(nvar+1); i++) 
        RawByteBinary(fp, 1);
  

    for (i = 0; i < nvar; i++)
    {

      json_t * varholder = json_array_get(namedVariables,i);
      json_object_set(varholder, "valueType", json_array_get(valueTypeArray, i)); 
      InStringBinary(fp, fmtlist_len, timestamp);
      json_object_set(varholder, "vfmt", json_string(timestamp));
      json_array_set(namedVariables, i, varholder);

    }


    /** value labels.  These are stored as the names of label formats,
    which are themselves stored later in the file. **/
    
    for(i = 0 ; i < nvar; i++)
    {
      InStringBinary(fp, varnamelength+1, aname);
      json_t * varholder = json_array_get(namedVariables,i);
      json_object_set(varholder, "vlabels", json_string(aname));
      json_array_set(namedVariables, i, varholder);
    }

    /** Variable Labels **/

    switch(abs(version))
    {
      case 5:
        for(i = 0; i < nvar; i++)
        {
          InStringBinary(fp, 32, datalabel);
          json_t * varholder = json_array_get(namedVariables, i);
          json_object_set(varholder, "dlabels", json_string(datalabel));
          json_array_set(namedVariables, i, varholder);
        }
        break;
      case 6:
      case 7:
      case 8:
      case 10:
      case 12:
        for(i = 0; i < nvar; i++)
        {
          InStringBinary(fp, 81, datalabel);
          json_t * varholder = json_array_get(namedVariables, i);
          json_object_set(varholder, "dlabels", json_string(datalabel));
          json_array_set(namedVariables, i, varholder);

        }
    }

    json_object_set(stata_js, "variables", namedVariables);
    j = 0;
    while(RawByteBinary(fp, 1))
    {
      if (abs(version) >= 7)   /* manual is wrong here */
        charlen = (InIntegerBinary(fp, 1, swapends));
      else
       charlen = (InShortIntBinary(fp, 1, swapends));

      if((charlen > 66))
      {
        InStringBinary(fp, 33, datalabel);
        InStringBinary(fp, 33, datalabel);
        txt = calloc(1, (size_t) (charlen-66));
        InStringBinary(fp, (charlen-66), txt);
        free(txt);
        j++;
      } 
      else
        for (i = 0; i < charlen; i++) InByteBinary(fp, 1);
     }
 //    if(j > 0)
 //       ;

    if (abs(version) >= 7)
        charlen = (InIntegerBinary(fp, 1, swapends));
    else
        charlen = (InShortIntBinary(fp, 1, swapends));
    if (charlen != 0)
        printf("something strange in the file\n (Type 0 characteristic of nonzero length)");

  /* Data time */
  
  json_t * data = json_array();

    
  if (version > 0)       /* not Stata/SE */
  {
    for(i = 0; i < nobs; i++)
    {
      json_t * currentObservation = json_array();

      for(j = 0; j < nvar; j++)
      {
         
        switch (json_integer_value(json_object_get(json_array_get(namedVariables,j),"valueType")))
        {
          case STATA_FLOAT:
            json_array_append(currentObservation,  json_real( InFloatBinary(fp, 0, swapends)));
            break;
          case STATA_DOUBLE:
            json_array_append(currentObservation,  json_real( InDoubleBinary(fp, 0, swapends)));
            break;
          case STATA_INT:
            json_array_append(currentObservation,  json_integer( InIntegerBinary(fp, 0, swapends)));
            break;
          case STATA_SHORTINT:
            json_array_append(currentObservation,  json_integer( InShortIntBinary(fp, 0, swapends)));
            break;
          case STATA_BYTE:
            json_array_append(currentObservation,  json_integer( InByteBinary(fp, 0)));
            break;
          default:
            charlen = json_integer_value(json_object_get(json_array_get(namedVariables,j),"valueType"))  - STATA_SE_STRINGOFFSET;

            if(charlen > 244)
            {
              printf("invalid character string length -- truncating to 244 bytes");
              charlen = 244;
            }

            InStringBinary(fp, charlen, stringbuffer);
            stringbuffer[charlen] = 0;
            json_array_append(currentObservation, json_string(stringbuffer));

            break;
        }
      }
        json_array_set(data, i, currentObservation);
    }
  }
  else
  {
    for(i = 0; i < nobs; i++)
    {
          json_t * currentObservation = json_array();

      for(j = 0; j < nvar; j++)
      {
        switch (json_integer_value(json_object_get(json_array_get(namedVariables,j),"valueType")))
        {
          case STATA_SE_FLOAT:
            json_array_append(currentObservation, json_real( InFloatBinary(fp, 0, swapends)));
            break;
          case STATA_SE_DOUBLE:
            json_array_append(currentObservation, json_real( InDoubleBinary(fp, 0, swapends)));
            break;
          case STATA_SE_INT:
            json_array_append(currentObservation, json_integer( InIntegerBinary(fp, 0, swapends)));
            break;
          case STATA_SE_SHORTINT:
            json_array_append(currentObservation, json_integer( InShortIntBinary(fp, 0, swapends)));
            break;
          case STATA_SE_BYTE:
            json_array_append(currentObservation, json_integer( InByteBinary(fp, 0)));
            break;
          default:
            charlen = json_integer_value(json_object_get(json_array_get(namedVariables,j),"valueType"))  - STATA_SE_STRINGOFFSET;
            if(charlen > 244)
            {
              printf("invalid character string length -- truncating to 244 bytes");
              charlen = 244;
            }
            InStringBinary(fp, charlen, stringbuffer);
            stringbuffer[charlen] = 0;
            json_array_append(currentObservation, json_string(stringbuffer));
            break;
        }
      }
        json_array_append(data, currentObservation);
    }
}


    //json_t * labels = json_object();
    
    /** value labels **/
    if (abs(version) > 5)
    {
      for(j = 0; ; j++)
      {
        /* first int not needed, use fread directly to trigger EOF */
        res = (int) fread((int *) aname, sizeof(int), 1, fp);
        if (feof(fp)) break;
        if (res != 1) printf("a binary read error occurred");
  
        InStringBinary(fp, varnamelength+1, aname);
        /*padding*/
        RawByteBinary(fp, 1); RawByteBinary(fp, 1); RawByteBinary(fp, 1);
        nlabels = InIntegerBinary(fp, 1, swapends);
        totlen = InIntegerBinary(fp, 1, swapends);
        
        off =  calloc(sizeof(int), (size_t) nlabels);
        for(i = 0; i < nlabels; i++)
          off[i] = InIntegerBinary(fp, 1, swapends);
  
        int * levels = calloc(sizeof(int), (size_t)nlabels);
        for(i = 0; i < nlabels; i++)
          levels[i] =  InIntegerBinary(fp, 0, swapends);
        txt =  calloc(sizeof(char), (size_t) totlen);
        InStringBinary(fp, totlen, txt);
 
        json_t * vlabel = json_object();
        
        json_t * jlevels = json_object();
        for(i = 0; i < nlabels; i++)
        { 
            char buf[50];
            sprintf(buf, "%d", levels[i]);
            json_object_set(jlevels, buf, json_string(txt+off[i]));
        }
       
        if (json_object_get(stata_js, "labels"))    
        {
            json_t * tmplev = json_object_get(stata_js, "labels");
            json_object_set(tmplev, aname, jlevels);
            json_object_set(stata_js, "labels", tmplev);
        }
        else
        { 
            json_object_set(vlabel, aname, jlevels);
            json_object_set(stata_js, "labels", vlabel);
        }

        free(off);
        free(txt);
        free(levels);
        
      }
    }
 
    json_object_set( holder, "nlabels", json_integer(j));
    json_object_set(stata_js, "metadata", holder);

    
    json_object_set(stata_js, "data", data);
    //json_dumpf(stata_js, stdout, JSON_VALIDATE_ONLY);

    return stata_js;
}

struct StataDataFile * R_LoadStataData(FILE *fp)
{
	int i, j = 0, nvar, nobs, charlen, version, swapends,
		varnamelength, nlabels, totlen, res;
	unsigned char abyte;
	/* timestamp is used for timestamp and for variable formats */
	char datalabel[81], timestamp[50], aname[33];
	char stringbuffer[245], *txt;
	int *off;
	int fmtlist_len = 12;

	/** first read the header **/

								 /* release version */
	abyte = (unsigned char) RawByteBinary(fp, 1);
	version = 0;				 /* -Wall */
	varnamelength = 0;			 /* -Wall */
	switch (abyte)
	{
		case VERSION_5:
			version = 5;
			varnamelength = 8;
			break;
		case VERSION_6:
			version = 6;
			varnamelength = 8;
			break;
		case VERSION_7:
			version = 7;
			varnamelength = 32;
			break;
		case VERSION_7SE:
			version = -7;
			varnamelength = 32;
			break;
		case VERSION_8:
			version = -8;		 /* version 8 automatically uses SE format */
			varnamelength = 32;
			break;
		case VERSION_114:
			version = -10;
			varnamelength = 32;
			fmtlist_len = 49;
		case VERSION_115:
			/* Stata say the formats are identical,
			   but _115 allows business dates */
			version = -12;
			varnamelength = 32;
			fmtlist_len = 49;
			break;
		default:
			printf("not a Stata version 5-12 .dta file");
	}
								 /* byte ordering */
	stata_endian = (int) RawByteBinary(fp, 1);
	swapends = stata_endian != CN_TYPE_NATIVE;

	struct StataDataFile * df = calloc(1, sizeof(struct StataDataFile));

	RawByteBinary(fp, 1);		 /* filetype -- junk */
	RawByteBinary(fp, 1);		 /* padding */
								 /* number of variables */
	nvar = (InShortIntBinary(fp, 1, swapends));
								 /* number of cases */
	nobs = (InIntegerBinary(fp, 1, swapends));

	df->nvar = nvar;
	df->nobs = nobs;
	/* data label - zero terminated string */
	switch (abs(version))
	{
		case 5:
			InStringBinary(fp, 32, datalabel);
			break;
		case 6:
		case 7:
		case 8:
		case 10:
		case 12:
			InStringBinary(fp, 81, datalabel);
			break;
	}
	/* file creation time - zero terminated string */
	InStringBinary(fp, 18, timestamp);

	/** and now stick the labels on it **/
	df->version = abs(version);
	df->datalabel = (char *)strdup(datalabel);
	df->timestamp = (char *)strdup(timestamp);
	/** read variable descriptors **/

	/** types **/

	struct StataVariable * stvcurr = NULL;

	if (version > 0)
	{
		for(i = 0; i < nvar; i++)
		{
			struct StataVariable * stv = calloc(1, sizeof(struct StataVariable));

			if (stv == NULL)
				fprintf(stderr, "Out of memory\n\r");

			if (df->variables == NULL)
			{
				stvcurr = stv;
				df->variables = stv;

			}
			else
			{
				stvcurr->next = stv;
				stvcurr = stv;
			}

			abyte = (unsigned char) RawByteBinary(fp, 1);
			stvcurr->valueType = abyte;
			switch (abyte)
			{
				case STATA_FLOAT:
				case STATA_DOUBLE:
					break;
				case STATA_INT:
				case STATA_SHORTINT:
				case STATA_BYTE:
					break;
				default:
					if (abyte < STATA_STRINGOFFSET)
						printf("unknown data type");
					break;
			}
		}
	}
	else
	{

		for(i = 0; i < nvar; i++)
		{
			struct StataVariable * stv = calloc(1, sizeof(struct StataVariable));
			if (stv == NULL)
				fprintf(stderr, "Out of memory\n\r");

			if (df->variables == NULL)
			{
				stvcurr = stv;
				df->variables = stv;
			}
			else
			{
				stvcurr->next = stv;
				stvcurr = stv;
			}

			abyte = (unsigned char) RawByteBinary(fp, 1);
			stvcurr->valueType = abyte;
			switch (abyte)
			{
				case STATA_SE_FLOAT:
				case STATA_SE_DOUBLE:
					break;
				case STATA_SE_INT:
				case STATA_SE_SHORTINT:
				case STATA_SE_BYTE:
					break;
				default:
					if (abyte > 244)
						printf("unknown data type");
					break;
			}
		}
	}

	/** names **/
	struct StataVariable * stv = NULL;

	for (i = 0, stv=df->variables; i < nvar && stv; i++, stv = stv->next)
	{
		InStringBinary(fp, varnamelength+1, aname);
		stv->name = strdup(aname);
	}

	/** sortlist -- not relevant **/

	for (i = 0; i < 2*(nvar+1); i++) RawByteBinary(fp, 1);

	/** format list
	passed back to R as attributes.
	Used to identify date variables.
	**/

	for (i = 0, stv=df->variables; i < nvar; i++, stv=stv->next)
	{
		InStringBinary(fp, fmtlist_len, timestamp);
		stv->vfmt = strdup(timestamp);
	}

	/** value labels.  These are stored as the names of label formats,
	which are themselves stored later in the file. **/

	for(i = 0, stv=df->variables; i < nvar; i++, stv=stv->next)
	{
		InStringBinary(fp, varnamelength+1, aname);
		stv->vlabels = strdup(aname);
	}

	/** Variable Labels **/

	switch(abs(version))
	{
		case 5:
			for(i = 0, stv=df->variables; i < nvar; i++, stv=stv->next)
			{
				InStringBinary(fp, 32, datalabel);
				stv->dlabels = strdup(datalabel);
			}
			break;
		case 6:
		case 7:
		case 8:
		case 10:
		case 12:
			for(i = 0, stv=df->variables; i < nvar; i++, stv=stv->next)
			{
				InStringBinary(fp, 81, datalabel);
				stv->dlabels = strdup(datalabel);

			}
	}

	/* Expansion Fields. These include
	   variable/dataset 'characteristics' (-char-)
	   variable/dataset 'notes' (-notes-)
	   variable/dataset/values non-current language labels (-label language-)
	*/

	j = 0;
	while(RawByteBinary(fp, 1))
	{
		if (abs(version) >= 7)	 /* manual is wrong here */
			charlen = (InIntegerBinary(fp, 1, swapends));
		else
			charlen = (InShortIntBinary(fp, 1, swapends));

		if((charlen > 66))
		{
			InStringBinary(fp, 33, datalabel);
			InStringBinary(fp, 33, datalabel);
			txt = calloc(1, (size_t) (charlen-66));
			InStringBinary(fp, (charlen-66), txt);
			free(txt);
			j++;
		} else
		for (i = 0; i < charlen; i++) InByteBinary(fp, 1);
	}
	//if(j > 0)
	//	;

	if (abs(version) >= 7)
		charlen = (InIntegerBinary(fp, 1, swapends));
	else
		charlen = (InShortIntBinary(fp, 1, swapends));
	if (charlen != 0)
		printf("something strange in the file\n (Type 0 characteristic of nonzero length)");

	struct StataObservation *obspcurr = NULL;
	struct StataObservationData * dptr = NULL;
	/** The Data **/
	if (version > 0)			 /* not Stata/SE */
	{
		for(i = 0; i < nobs; i++)
		{
			if (df->observations == NULL)
			{
				df->observations = calloc(1, sizeof(struct StataObservation));
				if (df->observations == NULL)
					fprintf(stderr, "Out of memory\n\r");
				obspcurr = df->observations;
				obspcurr->n = i;
			}
			else
			{
				obspcurr->next = calloc(1, sizeof(struct StataObservation));
				if (obspcurr->next == NULL)
					fprintf(stderr, "Out of memory\n\r");
				obspcurr = obspcurr->next;
				obspcurr->n = i;
			}

			for(j = 0, stv = df->variables; j < nvar && stv; j++, stv = stv->next)
			{

				if (obspcurr->data == NULL)
				{
					obspcurr->data = calloc(1, sizeof(struct StataObservationData));
					dptr = obspcurr->data;
				}
				else
				{
					dptr->next = calloc(1, sizeof(struct StataObservationData));
					dptr = dptr->next;
				}
				switch (stv->valueType)
				{
					case STATA_FLOAT:
						dptr->value.d = InFloatBinary(fp, 0, swapends);
						break;
					case STATA_DOUBLE:
						dptr->value.d = InDoubleBinary(fp, 0, swapends);
						break;
					case STATA_INT:
						dptr->value.i = InIntegerBinary(fp, 0, swapends);
						break;
					case STATA_SHORTINT:
						dptr->value.i = InShortIntBinary(fp, 0, swapends);
						//if (dptr->value.i == 32741)
						//dptr->value.i = 2147483621;
						break;
					case STATA_BYTE:
						dptr->value.i = (int) InByteBinary(fp, 0);
						break;
					default:
						charlen = stv->valueType - STATA_STRINGOFFSET;
						if(charlen > 244)
						{
							printf("invalid character string length -- truncating to 244 bytes");
							charlen = 244;
						}
						InStringBinary(fp, charlen, stringbuffer);
						stringbuffer[charlen] = 0;
						strncpy(dptr->value.string, stringbuffer, charlen);
						break;
				}

			}
		}
	}
	else
	{
		for(i = 0; i < nobs; i++)
		{
			if (df->observations == NULL)
			{
				df->observations = calloc(1, sizeof(struct StataObservation));
				obspcurr = df->observations;
				obspcurr->n = i;
			}
			else
			{
				obspcurr->next = calloc(1, sizeof(struct StataObservation));
				obspcurr = obspcurr->next;
				obspcurr->n = i;
			}

			for(j = 0, stv = df->variables; j < nvar && stv; j++, stv = stv->next)
			{

				if (obspcurr->data == NULL)
				{
					obspcurr->data = calloc(1, sizeof(struct StataObservationData));
					dptr = obspcurr->data;
				}
				else
				{
					dptr->next = calloc(1, sizeof(struct StataObservationData));
					dptr = dptr->next;
				}

				switch (stv->valueType)
				{
					case STATA_SE_FLOAT:
						dptr->value.d = InFloatBinary(fp, 0, swapends);
						break;
					case STATA_SE_DOUBLE:
						dptr->value.d = InDoubleBinary(fp, 0, swapends);
						break;
					case STATA_SE_INT:
						dptr->value.i = InIntegerBinary(fp, 0, swapends);
						break;
					case STATA_SE_SHORTINT:
						dptr->value.i = InShortIntBinary(fp, 0, swapends);
						//if (dptr->value.i == 32741)
						//  dptr->value.i = 2147483621;

						break;
					case STATA_SE_BYTE:
						dptr->value.i = (int) InByteBinary(fp, 0);
						break;
					default:
						charlen = stv->valueType-STATA_SE_STRINGOFFSET;
						if(charlen > 244)
						{
							printf("invalid character string length -- truncating to 244 bytes");
							charlen = 244;
						}
						InStringBinary(fp, charlen, stringbuffer);
						stringbuffer[charlen] = 0;
						strncpy(dptr->value.string, stringbuffer, charlen);
						break;
				}
			}

		}
	}

	/** value labels **/
	if (abs(version) > 5)
	{

		struct StataLabel * lblcurr = NULL;

		for(j = 0; ; j++)
		{
			/* first int not needed, use fread directly to trigger EOF */
			res = (int) fread((int *) aname, sizeof(int), 1, fp);
			if (feof(fp)) break;
			if (res != 1) printf("a binary read error occurred");

			InStringBinary(fp, varnamelength+1, aname);
								 /*padding*/
			RawByteBinary(fp, 1); RawByteBinary(fp, 1); RawByteBinary(fp, 1);
			nlabels = InIntegerBinary(fp, 1, swapends);

			totlen = InIntegerBinary(fp, 1, swapends);
			off =  calloc(sizeof(int), (size_t) nlabels);
			for(i = 0; i < nlabels; i++)
				off[i] = InIntegerBinary(fp, 1, swapends);

			int * levels = calloc(sizeof(int), (size_t)nlabels);
			for(i = 0; i < nlabels; i++)
				levels[i] =  InIntegerBinary(fp, 0, swapends);
			txt =  calloc(sizeof(char), (size_t) totlen);
			InStringBinary(fp, totlen, txt);

			for(i = 0; i < nlabels; i++)
			{
				if (df->labels == NULL)
				{
					df->labels = calloc(1, sizeof(struct StataLabel));
					lblcurr = df->labels;
					lblcurr->name = strdup(aname);
					lblcurr->value=levels[i];
					lblcurr->string = strdup(txt+off[i]);

				}
				else
				{
					lblcurr->next = calloc(1,  sizeof(struct StataLabel));
					lblcurr = lblcurr->next;
					lblcurr->name = strdup(aname);
					lblcurr->value=levels[i];
					lblcurr->string = strdup(txt+off[i]);

				}

			}

			free(off);
			free(txt);
			free(levels);
		}
	}

	df->nlabels = j;
	return df;
}


json_t * do_jsReadStata(char * fileName)
{
  FILE * fp;
  json_t * df = NULL;

  if ((sizeof(double)!=8) | (sizeof(int)!=4) | (sizeof(float)!=4))
  {
    fprintf(stderr, "can not yet read Stata .dta on this platform");
    return NULL;
  }
  
  fp = fopen(fileName, "rb");
  if (!fp)
  {
    fprintf(stderr, "Unable to open file: '%s'", strerror(errno));
    return NULL;
  }

  df = R_jsLoadStataData(fp);
  fclose(fp);
  return df;
}

