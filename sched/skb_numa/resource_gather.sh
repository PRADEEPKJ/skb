#!/bin/bash

#start skb with the support of script, which applications can access for resources

#cd ../skb/sched/skb_numa

SERVICE="skb"
RESULT=`ps -a | sed -n /${SERVICE}/p`

#if [ "${RESULT:-null}" = null ];then
#	echo " SKB is not running start skb"
#	exit 1
#fi


COUNTER=0
while [  1 ]; do
     #echo The counter is $COUNTER
	./addsysinfo $1 $2 > addinfo.txt 2>&1
        sleep 5
    done


