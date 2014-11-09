/* err.c -- error handling library

Copyright (c) <2014> Ernest Takada <takada@gmx.com>

Part of psdad project distributed under MIT Licence (see LICENSE.txt)

*/


#include "err.h"
#include <stdio.h>
#include <string.h>

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
    "Can not init socket",
    "Can not create thread",
    "Can not set TTL option",
    "Request for non locking IO failed",
    "Permission denied",
    "Can not set UID",
    "SIOCGIFHWADDR",
    "Can not set SO_BROADCAST option",
    "Sendto failed",
    "Can not receive PING response",
};

int getErrString(unsigned int err, char *errStr)
{
    if (err < ERR_COUNT)
    {
        strcpy(errStr,errMsg[err]);
        return ERR_OK;
    }
    return ERR_MSG_STR_NOT_FOUND;
}

int printErr(unsigned int err)
{
    char errStr[MAX_ERR_STR_SIZE];
    if (getErrString(err,errStr)==ERR_OK)
    {
        printf("%s\n",errStr);
        return ERR_OK;
    }
    return ERR_MSG_STR_NOT_FOUND;
}
