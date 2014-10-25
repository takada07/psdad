/* strutil.c -- utils for string parser

Copyright (c) <2014> Ernest Takada <takada@gmx.com>

utils for manipulatin strings. Part of psdad project 
distributed under MIT Licence (see LICENSE.txt)

*/

#include "strutil.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <assert.h>

char** str_split(const char* a_str, const char a_delim, size_t *strscount)
{
    char** result    = 0;
    size_t count     = 0;
    char* last_comma = 0;
    char delim[2];
    delim[0] = a_delim;
    delim[1] = 0;
    char *str;
    str = (char *)malloc(strlen(a_str)+1);
    strcpy(str,a_str);
    char* tmp        = str;
    /* Count how many elements will be extracted. */
    while (*tmp)
    {
        if (a_delim == *tmp)
        {
            count++;
            last_comma = tmp;
        }
        tmp++;
    }

    /* Add space for trailing token. */
    count += last_comma < (str + strlen(str) - 1);

    /* Add space for terminating null string so caller
       knows where the list of returned strings ends. */
    count++;

    result = malloc(sizeof(char*) * count);

    if (result)
    {
        size_t idx  = 0;
        char* token = strtok(str, delim);

        while (token)
        {
            assert(idx < count);
            *(result + idx++) = strdup(token);
            token = strtok(0, delim);
        }
        assert(idx == count - 1);
        *(result + idx) = 0;
    }
    *strscount = count-1;
    free (str);
    return result;
}

void remove_spaces(char* source)
{
    char* i = source;
    char* j = source;
    while(*j != 0)
    {
        *i = *j++;
        if (*i == '\n' || *i == '\r')
        {
            *i=0;
            return;
        }
        if(*i != ' ')
            i++;
    }
    *i = 0;
}
