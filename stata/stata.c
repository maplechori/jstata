
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

char * js_stata_open(char * name)
{
    json_t * dta = NULL;
    fprintf(stderr, "Opening stata file: %s\n", name);

    dta = do_jsReadStata(name);

    if (dta != NULL)
    {
        return json_dumps(dta, JSON_VALIDATE_ONLY);
    }
    
    return NULL;
}


int stata_write()
{
/*	zval *labels, *data, *variables, **entry;
	char *fname;
	int str_len;
	int i;


	zval **datas;
	char **data_array = NULL;

	if (zend_hash_exists(Z_ARRVAL_P(labels), "labels", sizeof("labels")))
	{
		fprintf(stderr, "Key \"labels\" exists<br>");
		zval **innerLabels;
		zval **vlabels;
		HashPosition position;
		zend_hash_find(Z_ARRVAL_P(labels), "labels", sizeof("labels"), (void **)&innerLabels);

		for (zend_hash_internal_pointer_reset_ex(Z_ARRVAL_PP(innerLabels), &position);
			zend_hash_get_current_data_ex((*innerLabels)->value.ht, (void**) &vlabels, &position) == SUCCESS;
			zend_hash_move_forward_ex((*innerLabels)->value.ht, &position))
		{

			if (Z_TYPE_PP(vlabels) == IS_ARRAY)
			{
				HashPosition pointer;
				char *key;
				uint key_len, key_type;
				long index;

				key_type = zend_hash_get_current_key_ex(Z_ARRVAL_PP(vlabels), &key, &key_len, &index, 0, &position);

				switch (key_type)
				{
					case HASH_KEY_IS_STRING:
						// associative array keys
						//php_printf("key: %s<br>", key);
						break;
					case HASH_KEY_IS_LONG:
						// numeric indexes
						//php_printf("index: %ld<br>", index);
						break;
					default:
						printf("error<br>");
				}
			}

		}

	}

	do_writeStata(fname, data, variables, labels);
*/
}
