#include <string.h>
#include <jansson.h>
#include "stataread.h"
#include "stata.h"

void jsonType(int ty)
{
    switch(ty)
    {
        case JSON_OBJECT:
            printf("JSON_OBJECT\n\r");
            break;
        case JSON_ARRAY:
            printf("JSON_ARRAY\n\r");
            break;
        case JSON_STRING:
            printf("JSON_STRING\n\r");
            break;
        case JSON_INTEGER:
            printf("JSON_INTEGER\n\r");
            break;
        case JSON_REAL:
            printf("JSON_REAL\n\r");
            break;
        case JSON_TRUE:
            printf("JSON_TRUE\n\r");
            break;
        case JSON_FALSE:
            printf("JSON_FALSE\n\r");
            break;
        case JSON_NULL:
            printf("JSON_NULL\n\r");
            break;
        }
}

int main(int argc, char *argv[])
{
    size_t i;
    char *text;

    json_t *root;
    json_error_t error;

    if (argc != 1)
    {
        root = json_load_file(argv[1], 0, &error); 
        if (root)
        {
            if (JSON_OBJECT == json_typeof(root))
            {
                json_t * metadata = json_object_get(root, "metadata");
                json_int_t vars = json_integer_value(json_object_get(metadata, "variables"));
                json_int_t obs = json_integer_value(json_object_get(metadata, "observations"));
                printf("Variables: %ld\n\r" , (long)vars);
                printf("Observations: %ld\n\r", (long)obs); 

            }
            
        }
        else
        {
            printf("Error! %s\n\r", error.text);
        }
    }
    else
    {
        printf("Usage: %s <filename>\n\r", argv[0]);
    }
    
}
