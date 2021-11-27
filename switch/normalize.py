#! /usr/bin/python

import os
import sys
import time
import string
import re
import traceback

######################################################################

portcnt={}
maclist={}
swmacs={}
trunks={}
maybetrunk={}

f1=open("sw-macs.txt","r")
for line in f1:
    mac=line.strip()
    swmacs[mac]="???"

try:
  f1=open("sw-trunks.txt","r")
  for line in f1:
    mac,port=line.strip().split(" ",1)
    trunks[port]=mac
except:
  pass

#f=open("ports.txt","r")
f=sys.stdin
for line in f:
    try:
	mac,port=line.strip().split(" ",1)
    except:
	continue
    try:
	maclist[mac]=maclist[mac]+","+port
    except:
	maclist[mac]=port
    try:
	portcnt[port]+=1
    except:
	portcnt[port]=1

    if mac in swmacs:
#	print "UPLINK Port: "+port
	portcnt[port]+=1000
	swmacs[mac]=port
	trunks[port]=mac
    else:
	m2list=swmacs.keys()
	for m2 in m2list:
	    if mac[0:15]==m2[0:15]:
		portcnt[port]+=100
		swmacs[mac]=port
		trunks[port]=mac

#for port in portcnt:
#    print port+": %d"%(portcnt[port])

for mac in maclist:
    bestport=""
    bestcnt=0
    for port in maclist[mac].split(","):
	if bestport=="" or portcnt[port]<bestcnt:
	    bestport=port
	    bestcnt=portcnt[port]
    if not bestport in trunks:
	# csak akkor irjuk ki, ha nem trunk port. mert lehet hogy az uplink sw tovabb emlexik ra :(
	print mac+" "+bestport
#
    for port in maclist[mac].split(","):
	if port!=bestport:
	    try:
		maybetrunk[port]+=1
	    except:
		maybetrunk[port]=1

f1=open("sw-trunks.txt","w")
for port in trunks:
    f1.write(trunks[port]+" "+port+"\n")

f1=open("sw-trunks.maybe","w")
for port in maybetrunk:
    if not port in trunks:
        f1.write(str(maybetrunk[port])+" "+port+"\n")
