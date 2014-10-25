/* main.c -- main program

Copyright (c) <2014> Ernest Takada <takada@gmx.com>

Part of psdad project distributed under MIT Licence (see LICENSE.txt)

*/


#include <stdio.h>

#include "defines.h"
#include "err.h"
#include "config.h"
#include "sniffer.h"


int main(int argc, char **argv)
{
    char *dev=NULL;
    if (argc == 2)
        dev = argv[1];
    int err=ERR_OK;
    err = read_config ("cfg/sample.cfg");
    if (err != ERR_OK)
    {
        printErr(err);
        return err;
    }
    err = init_sniffer(dev);
    if (err != ERR_OK)
    {
        printErr(err);
        return err;
    }
    err = set_ifaces(dev);
    if (err != ERR_OK)
    {
        printErr(err);
    }
    err = start_loop();
    if (err != ERR_OK)
    {
        printErr(err);
        return err;
    }
    return (err); 
}

