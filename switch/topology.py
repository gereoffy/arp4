#! /usr/bin/python

import os
import sys
import time
import string
import re
import traceback

portnames={}

def read_portnames(fnev):
  f=open(fnev,"r")
  for line in f:
#    if line[0]!='C':
#      continue;
    line=line.strip()
    PORT=line.split(' ',1)[0]
    try:
	portnames[PORT]=line.split(' ',1)[1].replace("_"," ")
    except:
	pass
  f.close()

read_portnames("names.txt")

swnames={}
# FIXME

def read_swnames(fnev):
  f=open(fnev,"r")
  for line in f:
#    if line[0]!='C':
#      continue;
    try:
	line=line.strip()
	x=line.split()
#	print x
	swnames[x[1]]=x[0]
    except:
	pass
  f.close()

read_swnames("switch.conf")
print swnames


def port2swno(port):
    return port.split("/")[0]


swports={}

#GATEWAY A0:36:9F:63:F7:9C D253/2 D251/16 D245/24 D244/24 D243/24 D242/24 Cbartok/48 Cserver/Po2 Ckliens/24 Csplit/11 Cstack/Po1 U218/8 U230/1 Ubartok/1 Umuzeum/1 Uporta/2
#D251/ 54:B8:0A:7D:8C:59 Ckliens/13 Cstack/Gi2/0/47

gwswno=None

f=open("topology.txt","r")
for line in f:
    x=line.strip().split(" ")
    swno=port2swno(x[0]) #int(x[0].split(".",3)[3])
    mac=x[1]
#    print(mac)
    if len(x)==2:
	gwswno=swno
    for port in x[2:]:
	try:
	    swports[port].append(swno)
	except:
	    swports[port]=[swno]

#gwswno="Cstack"

swmap={}
uplink={}

for port in swports:
    print("----")
    print(port+"  <---> "+str(swports[port]))
    #swno=int(port.replace("HP","").replace("Cuj","").replace("S","").split("-",1)[0])
    swno=port2swno(port)

    if "GATEWAY" in swports[port]:
	if swno==gwswno:
	    print port+": Gateway!"
	else:
	    print port+": Uplink!"
	    if swno in uplink:
		print "Multiple UPLINK??? old: "+uplink[swno]
	uplink[swno]=port
    else:
	print port+": "+str(swports[port])
	for i in swports[port]:
	    if not swno in swmap:
		swmap[swno]={}
	    swmap[swno][i]=port
	

print "======================================================="
print swmap.keys()
print gwswno
print "======================================================="

def swcleanup(swno):
#  print "======================================================="
#  print "  Switch "+str(swno)
  swlist=swmap[swno].keys()
#  print swlist
  for i in swlist:
#  print "  Switch "+str(i)
    if i in swmap:
      for j in swmap[i]:
        try:
	  del swmap[swno][j]
#	  print "    del  "+str(j)
        except:
          pass
  swlist=swmap[swno].keys()
#  print swlist
  for i in swlist:
    if i in swmap:
      swcleanup(i)  # recursive!!!!

swcleanup(gwswno)

print "======================================================="


def swprint(i,prefix=""):
    for j in swmap[i]:
#	print "i=%d j=%d p=%s" %(i,j,swmap[i][j])
	port=swmap[i][j]
	try:
	    pname=portnames[port]
	except:
	    pname="undefined"
#	try:
#	    swname=swnames[port.split("-",1)[0]]
#	except:
#	    swname="10.200.4.%d"%(i)
	swname=i
	
	port2="%s/???" %(j)
	try:
	    port2=uplink[j]
	    pname2=portnames[port2]
	except:
	    pname2="undefined"
	swname2=port2swno(port2)
#	try:
#	    swname2=swnames[port2.split("-",1)[0]]
#	except:
#	    swname2="10.200.4.%d"%(j)
	    
	print prefix
	print prefix+"%s (%s / %s)" %(port,swname,pname)
	print prefix+" ---> %s (%s / %s)" %(port2,swname2,pname2)
	
	if j in swmap:
	    swprint(j,prefix+"      ")

#	print "%s (%s) -> %s (%s)" %(port,pname,port2,pname2)
	
#	try:
#	    print "%s -> %s" %(swmap[i][j],uplink[j])
#	except:
#	    print "%s -> %d/???" %(swmap[i][j],j)


swprint(gwswno)
