/* sniffer.h -- header file for sniffer.c

Copyright (c) <2014> Ernest Takada <takada@gmx.com>

Part of psdad project distributed under MIT Licence (see LICENSE.txt)

*/

#ifndef __SNIFFER_H__
#define __SNIFFER_H__

#include "config.h"

int init_sniffer(char *conf_dev);
int start_loop();
//int inject_packet(struct cfgData *data);

#endif
