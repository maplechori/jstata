
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

char * js_stata_read(char * name)
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


