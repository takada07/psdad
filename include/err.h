/* err.h -- header file for err.c

Copyright (c) <2014> Ernest Takada <takada@gmx.com>

Part of psdad project distributed under MIT Licence (see LICENSE.txt)

*/

#ifndef __ERR_H__
#define __ERR_H__

#define ERR_COUNT                   10
#define MAX_ERR_STR_SIZE            60

#define ERR_OK                      0
#define ERR_CFG_OPEN_FAILURE        1
#define ERR_MSG_STR_NOT_FOUND       2
#define ERR_CONFIG_FILE_BAD_IP      3
#define ERR_DEFAULT_DEV_NOT_FOUND   4
#define ERR_NETMASK_DEV             5
#define ERR_DEVICE_OPEN             6
#define ERR_DEV_NOT_ETHER           7
#define ERR_PARSE_FILTER            8
#define ERR_INSTALL_FILTER          9



int getErrString(unsigned int err, char *errStr);
int printErr(unsigned int err);

#endif //__ERR_H__
