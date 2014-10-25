/* wake.h -- header file for wake.c

Copyright (c) <2014> Ernest Takada <takada@gmx.com>

Part of psdad project distributed under MIT Licence (see LICENSE.txt)

*/

#ifndef __WAKE_H__
#define __WAKE_H__
int send_wol(const struct cfgData *data, char *ifname);
#endif //__WAKE_H__
