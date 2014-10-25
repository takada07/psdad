/* err.c -- error handling library

Copyright (c) <2014> Ernest Takada <takada@gmx.com>

Part of psdad project distributed under MIT Licence (see LICENSE.txt)

*/


#include "err.h"
#include <stdio.h>

char errMsg[ERR_COUNT][MAX_ERR_STR_SIZE] = 
{
    "All OK",
    "Can not open configuration file",
    "Message string not found",
    "Bad IP format in config",
    "Couldn't find default device",
    "Couldn't get netmask for device",
    "Couldn't open device",
    "Device is not an Ethernet",
    "Couldn't install filter",
};

int getErrString(unsigned int err, char *errStr)
{
    if (err < ERR_COUNT)
    {
        errStr = &errMsg[err][0];
        return ERR_OK;
    }
    return ERR_MSG_STR_NOT_FOUND;
}

int printErr(unsigned int err)
{
    char *errStr=NULL;
    if (getErrString(err,errStr)==ERR_OK && errStr != NULL)
    {
        printf("%s\n",errStr);
        return ERR_OK;
    }
    return ERR_MSG_STR_NOT_FOUND;
}
