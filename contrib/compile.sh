#!/bin/bash

ARCH=`(uname -m) 2>/dev/null`
ARCH+="_linux"
echo $ARCH

gdbus-codegen \
    --interface-prefix org.freedesktop \
    --generate-c-code Skb \
    org.freedesktop.Skb.xml \
&& \
gcc -std=c99 -o service-skb \
    `pkg-config --cflags --libs gio-2.0 gio-unix-2.0` -I/home/user/Eclipse/include/$ARCH  \
    -L/home/user/Eclipse/lib/$ARCH  -I$PWD -lreadline -leclipse\
    service-skb.c Skb.h Skb.c skb_main.c skb_service.c skb_functions.c
