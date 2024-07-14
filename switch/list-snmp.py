#! /usr/bin/python3

####################################################
# Excample:  ./list-snmp.py 172.17.20.18 NG18- 1/g
####################################################


# snmpwalk -On -C c -v2c -c public 10.90.90.102 1.3.6.1.4.1
# 1.3.6.1.4.1 enterprises
# 1.3.6.1.4.1.9.2    cisco enterprise
# 1.3.6.1.4.1.9.6    cisco 300/350
# 1.3.6.1.4.1.9.9    cdp, vtp... (hp es cisco enterprise)
# 1.3.6.1.4.1.11     hp 2530
# 1.3.6.1.4.1.171    dlink
# 1.3.6.1.4.1.171.10 dlink 1210
# 1.3.6.1.4.1.171.11 dlink 3xxx
# 1.3.6.1.4.1.171.12 dlink core
# 1.3.6.1.4.1.4526   netgear
# 1.3.6.1.4.1.11863  tplink 3210


import os
import sys
import time
import string
import re
import traceback

try:
    from easysnmp import snmp_get,snmp_walk
except:
    from usnmp import snmp_get,snmp_walk

######################################################################

# core switch params:
#swip="172.18.98.240"  # HP
#swip="172.17.20.20" # HP 
#swip="172.17.15.5" # tplink TL-SG3424
#swip="172.17.20.18" # netgear
#swip="172.18.11.246" # cisco 350
#swip="172.18.11.248" # cisco 300 uj fw
#swip="172.17.20.4" # dlink DGS-1210-24
#swip="10.191.50.1"  # hp 2650
#swip="10.200.5.102" # cisco nagy
#swip="10.200.6.10"   # dlink 3xxx
swip="10.200.5.1"   # dlink core
#swip="10.90.90.103"   # dlink vision regi
#swip="192.168.1.66"    # hp 2524 regi
#swip="192.168.1.45"    # mikrotik rb2011
#swip="10.100.86.232"  # regi 3com
#swip="10.100.86.231"   # cicko sg200

#swip="192.168.68.250"   # cicko zti
#swip="192.168.68.253"   # dlink 1500
#swip="192.168.68.251"   # dlink 1100 szar
#swip="192.168.68.243"   # dlink 1100 - kell hozza a mib order patch!

swc="public"
#portcut="1/g" # a port nevebol levagni (hogy csak a port szama maradjon)
#portcut="Slot0/" # dlink
portcut="port" # tplink
debug=True


if len(sys.argv)>1:
    debug=False # production use
    swip=sys.argv[1]
    if '@' in swip:
        swc,swip=swip.split('@')
    swname=sys.argv[2]
    try:
        portcut=sys.argv[3]
    except:
        portcut=None


# detect SNMP version & availability
swver=2
try:
    osi=int(snmp_get("1.3.6.1.2.1.1.7.0", hostname=swip, community=swc, version=swver, timeout=2, retries=1, use_numeric=True).value)
    if debug:
        desc=snmp_get("1.3.6.1.2.1.1.1.0", hostname=swip, community=swc, version=swver, timeout=2, retries=1, use_numeric=True).value #.encode('ascii', 'ignore')
        name=snmp_get("1.3.6.1.2.1.1.5.0", hostname=swip, community=swc, version=swver, timeout=2, retries=1, use_numeric=True).value #.encode('ascii', 'ignore')
except:
    swver=1
    try:
        osi=int(snmp_get("1.3.6.1.2.1.1.7.0", hostname=swip, community=swc, version=swver, timeout=2, retries=1, use_numeric=True).value)
        if debug:
            desc=snmp_get("1.3.6.1.2.1.1.1.0", hostname=swip, community=swc, version=swver, timeout=2, retries=1, use_numeric=True).value #.encode('ascii', 'ignore')
            name=snmp_get("1.3.6.1.2.1.1.5.0", hostname=swip, community=swc, version=swver, timeout=2, retries=1, use_numeric=True).value #.encode('ascii', 'ignore')
    except:
        print("No SNMP support / bad password")
        sys.exit()

if debug:
    for i in [1,2,3,4,5,6,7,8,9]:
        if osi&(1<<(i-1)): print("OSI layer %d supported"%(i))
    print(desc)
    print(name)


##def myget(oid):
##    return snmp_get(oid, hostname=swip, community=swc, version=1, timeout=2, retries=1).value.encode('ascii', 'ignore')

#/usr/bin/snmpwalk -v 1 -c public $IP .1.3.6.1.2.1.31.1.1.1.1  # if_name
#/usr/bin/snmpwalk -v 1 -c public $IP .1.3.6.1.2.1.31.1.1.1.18 # if_alias

#/usr/bin/snmpwalk -v 1 -c public $IP .1.3.6.1.2.1.17.4.3.1.1  # mac_list
#/usr/bin/snmpwalk -v 1 -c public $IP .1.3.6.1.2.1.17.4.3.1.2  # mac_port
#/usr/bin/snmpwalk -v 1 -c public $IP .1.3.6.1.2.1.17.1.4.1.2  # mac2port

portname={}
# 1.3.6.1.2.1.17.1.4.1.2     port index->phisical?
# 1.3.6.1.2.1.2.2.1.2 ifDescr   "STRING: unit 1 port 17 Gigabit - Level"
# 1.3.6.1.2.1.31.1.1.1.1 ifName "STRING: 1/g17"
# 1.3.6.1.2.1.31.1.1.1.18 if_alias  "szerver_sw_D13/24"
for item in snmp_walk('1.3.6.1.2.1.31.1.1.1.1',hostname=swip, community=swc, version=2, use_numeric=True):
#    oid=item.oid+"."+item.oid_index if item.oid_index else item.oid
#    print(oid)
    i=int(item.oid_index) if item.oid_index else int(item.oid.rsplit('.',1)[1])
#    print(i)
#    print(item.value)
    if debug: print("PORT %d -> %s"%(i,item.value))
    x=str(item.value).replace(": Gigabit Copper","").replace(" ","")
    portname[i]=x[len(portcut):] if portcut and x.startswith(portcut) else x

portmap={}  #    dot1dBasePortIfIndex
#if debug:
for item in snmp_walk('1.3.6.1.2.1.17.1.4.1.2',hostname=swip, community=swc, version=2, use_numeric=True):
    i=int(item.oid_index) if item.oid_index else int(item.oid.rsplit('.',1)[1])
    j=int(item.value)
    if i!=j:
        if debug: print("PORT-MAP %d = %d"%(i,j))
        portmap[i]=j

# port description
for item in snmp_walk('1.3.6.1.2.1.31.1.1.1.18',hostname=swip, community=swc, version=2, use_numeric=True):
    port=int(item.oid_index) if item.oid_index else int(item.oid.rsplit('.',1)[1])
    port=portmap.get(port,port)
    port=portname.get(port,port)
    if debug:
        print("NAME %s -> %s"%(port,item.value))
    elif item.value:
        sys.stderr.write(swname+str(port)+" "+str(item.value)+"\n")
#    x=str(item.value).replace(": Gigabit Copper","").replace(" ","")
#    portname[i]=x[len(portcut):] if portcut and x.startswith(portcut) else x


# http://oidref.com/1.3.6.1.2.1.17.7.1.4
# 1.3.6.1.2.1.17.7.1.4.2.1.3 dot1qVlanFdbId
# 1.3.6.1.2.1.17.7.1.4.3.1.1 vlanok nevei
# SNMPv2-SMI::mib-2.17.7.1.4.2.1.3.0.98 = Gauge32: 7
# Q-BRIDGE MIB, a legtobb vlanos switchen ez mukodik: (kiveve persze a cisco es dlink :))
vlanmap={}
for item in snmp_walk('1.3.6.1.2.1.17.7.1.4.2.1.3',hostname=swip, community=swc, version=2, use_numeric=True):
    i=int(item.oid_index) if item.oid_index else int(item.oid.rsplit('.',1)[1])
    j=int(item.value)
    if debug: print("VLAN %d -> %d"%(j,i))
    vlanmap[j]=i

# ha a dot1qVlanFdbId nem talalt semmit, akkor vagy cisco ios vagy dlink :)
vlanname={}
cisco=False
if len(vlanmap)==0:
  # CISCO vlan lista
  for item in snmp_walk('1.3.6.1.4.1.9.9.46.1.3.1.1.4.1',hostname=swip, community=swc, version=2, use_numeric=True):
    i=int(item.oid_index) if item.oid_index else int(item.oid.rsplit('.',1)[1])
    if debug: print("CISCO VLAN %d: %s"%(i,item.value))
    vlanname[i]=item.value
    cisco=True
else:
  # RFC dot1q VLAN lista:
  for item in snmp_walk('1.3.6.1.2.1.17.7.1.4.3.1.1',hostname=swip, community=swc, version=2, use_numeric=True):
    i=int(item.oid_index) if item.oid_index else int(item.oid.rsplit('.',1)[1])
    if debug: print("VLAN %d: %s"%(i,item.value))
    vlanname[i]=item.value

num=0
if cisco:
    # vegig kell menni egyesevel a vlanokon...
    #portmap={} # cache!!!  de lehet, hogy vlanonkent kulon kene? de ugy tunik minden vlanban ugyanaz...
    for vlan in vlanname:
        for item in snmp_walk('1.3.6.1.2.1.17.4.3.1.2',hostname=swip, community=swc+"@"+str(vlan), version=2, use_numeric=True):
            oid=item.oid+"."+item.oid_index if item.oid_index else item.oid
            x=oid.rsplit('.',7)
            try:
                port=portmap[int(item.value)]
            except:
                port=snmp_get("1.3.6.1.2.1.17.1.4.1.2."+str(item.value), hostname=swip, community=swc+"@"+str(vlan), version=2, timeout=2, retries=1).value #.encode('ascii', 'ignore')
#                print(port)
                portmap[int(item.value)]=int(port)
            port=portname.get(int(port),str(port))
            mac="%02X:%02X:%02X:%02X:%02X:%02X"%(int(x[2]),int(x[3]),int(x[4]),int(x[5]),int(x[6]),int(x[7]))
            num+=1
            if debug:
                print("%s %s (Vlan%d)  cisco!"%(mac,port,vlan))
            else:
                print("%s %s%s"%(mac,swname,port))

# 1.3.6.1.2.1.17.4.3.1.1     dot1dTpFdbAddress        mac cimek HEX-ben, nincs sok ertelme
# 1.3.6.1.2.1.17.4.3.1.2     dot1dTpFdbPort   802.1D  vlan nelkul!!!
# 1.3.6.1.2.1.17.4.3.1.3     dot1dTpFdbStatus
# 1.3.6.1.2.1.17.4.3.1.5     dot1dTpFdbAge    Age of this entry in seconds.
# http://oidref.com/1.3.6.1.2.1.17.7.1.2.2.1  - nagy ciscon nincs!!!!
# 1.3.6.1.2.1.17.7.1.2.2.1.2 dot1qTpFdbPort   802.1q  vlan is         RFC 4363 (2006)  https://tools.ietf.org/html/rfc4363#page-12 
for item in snmp_walk('1.3.6.1.2.1.17.7.1.2.2.1.2',hostname=swip, community=swc, version=2, use_numeric=True):
    oid=item.oid+"."+item.oid_index if item.oid_index else item.oid
#    print(oid)
    x=oid.rsplit('.',7)
    vlan=int(x[1])
    if vlan in vlanmap: vlan=vlanmap[vlan]  # HP-hoz kell, netgear-hez nem, cisconal mindegy
    port=int(item.value)
    port=portmap.get(port,port)
    port=portname.get(port,port)
    mac="%02X:%02X:%02X:%02X:%02X:%02X"%(int(x[2]),int(x[3]),int(x[4]),int(x[5]),int(x[6]),int(x[7]))
    num+=1
    if debug:
        print("%s %s (Vlan%d)  {%s}"%(mac,port,vlan,item.value))
    else:
        print("%s %s%s"%(mac,swname,port))
#    print(item)
#    print(item.oid)
#    print(x)

# ha nincs eredmeny, akkor megnezzuk a gyarto-specifikus MIB-eket:
if num==0:
# TPlink?     ehhez kell patchelni az easysnmp-t, hogy a nem sorbarendezett OID-ekre ne adjon hibat!
  for item in snmp_walk('1.3.6.1.4.1.11863.1.1.5.2.3.2.2.1.3', hostname=swip, community=swc, version=2, use_numeric=True):
    oid=item.oid+"."+item.oid_index if item.oid_index else item.oid
    x=oid.rsplit('.',7)
    vlan=int(x[7])
    port=str(item.value).replace("PORT","")
#    port=item.value[len(portcut):] if portcut and item.value.startswith(portcut) else item.value
    mac="%02X:%02X:%02X:%02X:%02X:%02X"%(int(x[1]),int(x[2]),int(x[3]),int(x[4]),int(x[5]),int(x[6]))
    num+=1
    if debug:
        print("%s %s (Vlan%d)  tplink!"%(mac,port,vlan))
    else:
        print("%s %s%s"%(mac,swname,port))

if debug: print("Total: %d"%(num))
