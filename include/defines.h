/* defines.h -- global definitions

Copyright (c) <2014> Ernest Takada <takada@gmx.com>

Part of psdad project distributed under MIT Licence (see LICENSE.txt)

*/


#ifndef __DEFINES_H__
#define __DEFINES_H__

//#define DEBUG
//#undef DEBUG
/* default snap length (maximum bytes per packet to capture) */
#define SNAP_LEN 1518
/* ethernet headers are always exactly 14 bytes [1] */
#define SIZE_ETHERNET 14

#define IP_HL(ip)               (((ip)->ip_vhl) & 0x0f)
#define IP_V(ip)                (((ip)->ip_vhl) >> 4)
#define SERVER_PORT 2020
#define WAKEUP_MESSAGE          "Here comes the magic"
#define SUSPEND_MESSAGE         "Node suspending"
#ifdef DEBUG_ALL
    #define debug_printf(...) fprintf( stderr, __VA_ARGS__ ); 
#else
    #define debug_printf(...) 
#endif

#endif //__DEFINES_H__
