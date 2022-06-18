#! /bin/bash

mkdir /var/pinger
cd /var/pinger

/var/arp4/traff/pingstat3 $(cat /var/arp4/traff/ping.list|cut -d ' ' -f 1)
