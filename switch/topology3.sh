#! /bin/bash

for MAC in $(ip addr|grep -o 'link/ether ..:..:..:..:..:.. '|cut -d ' ' -f 2|tr a-f A-F|sort|uniq); do
    echo GATEWAY $MAC $(grep "^$MAC" ports.all|cut -d " " -f 2|uniq|xargs)
done

cat switch.conf|while read line; do
    IP=$(echo $line|cut -d ' ' -f 1|cut -d '@' -f 2)
    MAC=$(arp -a -n $IP |cut -d ' ' -f 4 |tr a-z A-Z)
    NEV=$(echo $line|cut -d ' ' -f 2)
    echo $NEV $MAC $(grep "^$MAC" ports.all|cut -d " " -f 2|grep -v "^$NEV" | xargs)
done
