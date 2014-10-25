/* strutil.h -- header file for strutil.c

Copyright (c) <2014> Ernest Takada <takada@gmx.com>

Part of psdad project distributed under MIT Licence (see LICENSE.txt)

*/


#ifndef __STRUTIL_H__
#define __STRUTIL_H__

#include <stddef.h>

char** str_split(const char* a_str, const char a_delim, size_t *strscount);
void remove_spaces(char* source);
#endif //__STRUTIL_H__
