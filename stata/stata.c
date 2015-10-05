
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
#include "stataread.h"
#include "stata.h"

struct StataDataFile * stata_open(char * name)
{
  struct StataDataFile *dta;

  fprintf (stderr, "Opening stata file: %s\n", name);

  dta = do_readStata(name);

  if (dta != NULL)
  {
        return dta;
  }
  else
  {
        return NULL;
  }
}

int stata_nvariables(struct StataDataFile *dta)
{

  if (dta && dta->nvar > 0)
  {
     return (int)dta->nvar;
  }
  else
  {
      return 0;
  }
}

int stata_close(struct StataDataFile *dta)
{

  if (dta != NULL)
  {
 	  do_stataClose (dta);
    return 1;
  }
  else
     return 0;

}

PHP_FUNCTION (stata_observations)
{
  struct StataDataFile *dta = NULL;
  zval *stataData;


  if (zend_parse_parameters (ZEND_NUM_ARGS ()TSRMLS_CC, "r", &stataData) ==
      FAILURE)
    {
      RETURN_NULL ();
    }

  ZEND_FETCH_RESOURCE (dta, struct StataDataFile *, &stataData, -1,
		       PHP_STATA_FILE_RES_NAME, le_stata_file);

  if (dta && dta->nobs > 0)
    {
      RETURN_LONG (dta->nobs);
    }
  else
    {
      RETURN_LONG (0);
    }
}

PHP_FUNCTION (stata_variables)
{
  struct StataDataFile *dta = NULL;
  struct StataVariable *stv;
  zval *stataData;
  zval *variables;
  zval **innerarray;
  int i = 0;

  if (zend_parse_parameters (ZEND_NUM_ARGS ()TSRMLS_CC, "r", &stataData) ==
      FAILURE)
    {
      RETURN_NULL ();
    }

 

  ZEND_FETCH_RESOURCE (dta, struct StataDataFile *, &stataData, -1,
		       PHP_STATA_FILE_RES_NAME, le_stata_file);

  if (dta == NULL)
	RETURN_NULL();

  array_init (return_value);
  int count = 0;


  innerarray = malloc (sizeof (zval *) * dta->nvar);

  for (i = 0; i < dta->nvar; i++)
    {
      ALLOC_INIT_ZVAL (innerarray[i]);
      array_init (innerarray[i]);
    }

  for (stv = dta->variables; stv; stv = stv->next, count++)
    {
      add_assoc_string (innerarray[count], "vlabels", stv->vlabels, 1);
      add_assoc_string (innerarray[count], "dlabels", stv->dlabels, 1);
      add_assoc_string (innerarray[count], "vfmt", stv->vfmt, 1);
      add_assoc_long(innerarray[count], "valueType", stv->valueType);
      add_assoc_zval (return_value, stv->name, innerarray[count]);
    }

  free (innerarray);


}

PHP_FUNCTION (stata_labels)
{
  int i;
  struct StataDataFile *dta;
  struct StataVariable *stv;
  struct StataLabel *stl;
  zval *stataData;
  zval *innertable;

  if (zend_parse_parameters (ZEND_NUM_ARGS ()TSRMLS_CC, "r", &stataData) ==
      FAILURE)
    {
      RETURN_NULL ();
    }

  ZEND_FETCH_RESOURCE (dta, struct StataDataFile *, &stataData, -1,
		       PHP_STATA_FILE_RES_NAME, le_stata_file);

  if (dta == NULL)
    RETURN_NULL();
  array_init (return_value);
  int count = 0;
  char buff[256];
  char currName[256];

  buff[0] = 0;
  currName[0] = 0;

  ALLOC_INIT_ZVAL (innertable);
  array_init (innertable);

  zval **innerarray = malloc (sizeof (zval *) * dta->nlabels);

  for (i = 0; i < dta->nlabels; i++)
    {
      ALLOC_INIT_ZVAL (innerarray[i]);
      array_init (innerarray[i]);
    }

  int finishup = 0;

  for (stl = dta->labels; stl; stl = stl->next)
    {
      if (currName[0] == 0)
	{
	  strcpy (currName, stl->name);
	  sprintf (buff, "%d", stl->value);
	  add_assoc_string (innerarray[0], buff, stl->string, 1);
	}
      else
	{
	  if (!strcmp (currName, stl->name))
	    {
	      sprintf (buff, "%d", stl->value);
	      add_assoc_string (innerarray[count], buff, stl->string, 1);
	      strcpy (currName, stl->name);

	      if (count == dta->nlabels - 1)
		{
		  sprintf (buff, "%d", stl->value);
		  add_assoc_string (innerarray[count], buff, stl->string, 1);
		  finishup = 1;
		}

	    }
	  else
	    {
	      add_assoc_zval (innertable, currName, innerarray[count]);
	      count++;
	      strcpy (currName, stl->name);
	      sprintf (buff, "%d", stl->value);
	      add_assoc_string (innerarray[count], buff, stl->string, 1);

	    }
	}
    }

  if (finishup)
    add_assoc_zval (innertable, currName, innerarray[count]);


  add_assoc_zval (return_value, "labels", innertable);
  innertable = NULL;

  free (innerarray);



}



PHP_FUNCTION (stata_data)
{
  struct StataDataFile *dta;
  struct StataObservation *obs;
  struct StataObservationData *obd;
  struct StataVariable *stv;
  zval *stataData, **vararray, *table;
 
  struct StataObservation *obsprev = NULL;
  int counterObs;
  int counterVars;
  int observ;
  char *var, buffer[256];
  int str_len;

  if (zend_parse_parameters (ZEND_NUM_ARGS ()TSRMLS_CC, "r", &stataData) ==
      FAILURE)
    {
      RETURN_NULL ();
    }


  ZEND_FETCH_RESOURCE (dta, struct StataDataFile *, &stataData, -1, PHP_STATA_FILE_RES_NAME, le_stata_file);

  if (dta == NULL)
    RETURN_NULL();
  array_init (return_value);

  vararray = malloc(sizeof(zval*) * dta->nobs);


  ALLOC_INIT_ZVAL(table);
  array_init(table);

  obs = dta->observations;

  for (obs = dta->observations, counterObs = 0; obs;
       obs = obs->next, counterObs++)
    {

      ALLOC_INIT_ZVAL(vararray[counterObs]);
      array_init(vararray[counterObs]);       

     	
      for (obd = obs->data, counterVars = 0, stv = dta->variables; obd, stv;
	   obd = obd->next, stv = stv->next, counterVars++)
	{
 
	  switch (stv->valueType)
	    {
	    case STATA_SE_FLOAT:
	    case STATA_SE_DOUBLE:
	    case STATA_FLOAT:
	    case STATA_DOUBLE:
	      add_assoc_double (vararray[counterObs], stv->name, obd->value.d);
	      break;
	    case STATA_SE_INT:
	    case STATA_INT:
	    case STATA_SE_SHORTINT:
	    case STATA_SHORTINT:
	    case STATA_SE_BYTE:
	    case STATA_BYTE:
	      add_assoc_long (vararray[counterObs], stv->name, obd->value.i);
	      break;
	    default:
	      if (stv->valueType > 244)
		error ("unknown data type");
	      add_assoc_string (vararray[counterObs], stv->name, obd->value.string, 1);
	      break;
	    }


	}

	add_index_zval(table, counterObs, vararray[counterObs]);
    }

  free(vararray);

  add_assoc_zval(return_value, "data", table);


}

PHP_FUNCTION(stata_write)
{
    zval *labels, *data, *variables, **entry;
    char *fname;
    int str_len; 
    int i;



    if (zend_parse_parameters (ZEND_NUM_ARGS ()TSRMLS_CC, "saaa", &fname, &str_len, &data, &variables, &labels) == FAILURE)
    {
      RETURN_NULL ();
    }


    zval **datas;
    char **data_array = NULL;


    if (zend_hash_exists(Z_ARRVAL_P(labels), "labels", sizeof("labels"))) {
          fprintf(stderr, "Key \"labels\" exists<br>");
          zval **innerLabels;
	  zval **vlabels;
	  HashPosition position;
          zend_hash_find(Z_ARRVAL_P(labels), "labels", sizeof("labels"), (void **)&innerLabels);

          for (zend_hash_internal_pointer_reset_ex(Z_ARRVAL_PP(innerLabels), &position); 
                              zend_hash_get_current_data_ex((*innerLabels)->value.ht, (void**) &vlabels, &position) == SUCCESS;
                                zend_hash_move_forward_ex((*innerLabels)->value.ht, &position)) {

				
				if (Z_TYPE_PP(vlabels) == IS_ARRAY)
				{
					 HashPosition pointer;
					 char *key;
					 uint key_len, key_type;
					 long index;
 
					 key_type = zend_hash_get_current_key_ex(Z_ARRVAL_PP(vlabels), &key, &key_len, &index, 0, &position);
 
					 switch (key_type) {
						  case HASH_KEY_IS_STRING:
							    // associative array keys
							    //php_printf("key: %s<br>", key);
							    break;
						  case HASH_KEY_IS_LONG:
							    // numeric indexes
							    //php_printf("index: %ld<br>", index);
							    break;
						  default:
							    php_printf("error<br>");
					  }
				}

				
    

	  }
           
    }




    do_writeStata(fname, data, variables, labels);


}
