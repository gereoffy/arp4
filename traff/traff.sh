#! /bin/bash

mkdir /var/traffmon
cd /var/traffmon
/var/arp4/traff/traffmon2 $(cd /sys/class/net/ ; ls | grep -e ^eth -e ^enp -e ^vlan -e ^bond0)
# &>/dev/null

