#! /usr/bin/python3

import os
import sys
import cgi
import time

iplist=""
for ipline in os.popen("ls /var/pinger/*-PING-*.dat|cut -d '-' -f 3|cut -d . -f 1-4|sort|uniq"):
    if iplist:
        iplist=iplist+","
    iplist=iplist+ipline.strip()

# Get form data
form = cgi.FieldStorage()
try:
    timep=form["time"].value
except:
    timep="auto"
try:
    zoom =int(form["zoom"].value)
except:
    zoom = 72
try:
    w =int(form["w"].value)
except:
    w = 1200
try:
    h =int(form["h"].value)
except:
    h = 128
try:
    ip =form["ip"].value
except:
    ip=iplist

try:
    time1=int(timep)
except:
    timep="auto"
    time1=int(time.time()-(w-10)*zoom)


print("Content-type: text/html")
print("")
print("<HTML><HEAD>")
if timep=="auto":
    print('<meta http-equiv="refresh" content="%d">' % (zoom))
print("</HEAD><BODY>")

for ip1 in iplist.split(","):
    print('<A HREF="/cgi-bin/ping.py?time=%s&zoom=%d&w=%d&ip=%s">[%s]</A>'%(timep,zoom,w,ip1,ip1))
    if ip.find(ip1)<0:
        print('<A HREF="/cgi-bin/ping.py?time=%s&zoom=%d&w=%d&ip=%s">+</A>'%(timep,zoom,w,ip+","+ip1))

for ip1 in ip.split(","):
    print("<br><br>")

    lt=time.localtime(time1)
    print('<table width=%d><tr><td align=left>|&lt;---'%(w))
    print('%04d.%02d.%02d/%02d:%02d:%02d' % (lt[0],lt[1],lt[2],lt[3],lt[4],lt[5]))
    timew=zoom*w
    lt=time.localtime(time1+timew)
    if timew<300:
        timew="%d seconds"%(timew)
    elif timew<3600:    
        timew="%d minutes"%(timew/60)
    elif timew<24*3600:
        timew="%d hours"%(timew/3600)
    else:
        timew="%d days"%(timew/(24*3600))
    print('</td><td align=center>')
    print("PING "+ip1+" ("+timew+", %d secs/pixel)"%(zoom))
    print('</td><td align=right>')
    print('%04d.%02d.%02d/%02d:%02d:%02d' % (lt[0],lt[1],lt[2],lt[3],lt[4],lt[5]))
    print('---&gt;|</td></tr></table>')
    #print " --- %04d.%02d.%02d/%02d:%02d:%02d" % (lt[0],lt[1],lt[2],lt[3],lt[4],lt[5])

    print('<IMG SRC="/cgi-bin/pinger?time=%d&zoom=%d&w=%d&h=%d&ip=%s" width=%d height=%d>'%(time1,zoom,w,h,ip1,w,h))
    print("<br>")

    print('<A HREF="/cgi-bin/ping.py?time=%d&zoom=%d&w=%d&ip=%s">[BACK]</A>'%(time1-zoom*w/4,zoom,w,ip))
    print('<A HREF="/cgi-bin/ping.py?time=%s&zoom=%d&w=%d&ip=%s">[auto]</A>'%("auto",zoom,w,ip))
    print('<A HREF="/cgi-bin/ping.py?time=%d&zoom=%d&w=%d&ip=%s">[stop]</A>'%(time1,zoom,w,ip))
    print('<A HREF="/cgi-bin/ping.py?time=%d&zoom=%d&w=%d&ip=%s">[FWD]</A>'%(time1+zoom*w/4,zoom,w,ip))

    if zoom>1:
        print('<A HREF="/cgi-bin/ping.py?time=%s&zoom=%d&w=%d&ip=%s">+++</A>'%(timep,zoom/2,w,ip))
    print('<A HREF="/cgi-bin/ping.py?time=%s&zoom=%d&w=%d&ip=%s">---</A>'%(timep,2*zoom,w,ip))
    print('<A HREF="/cgi-bin/ping.py?time=%s&zoom=%d&w=%d&ip=%s">[year]</A>'%(timep,365*24*3600/w,w,ip))
    print('<A HREF="/cgi-bin/ping.py?time=%s&zoom=%d&w=%d&ip=%s">[100d]</A>'%(timep,100*24*3600/w,w,ip))
    print('<A HREF="/cgi-bin/ping.py?time=%s&zoom=%d&w=%d&ip=%s">[month]</A>'%(timep,31*24*3600/w,w,ip))
    print('<A HREF="/cgi-bin/ping.py?time=%s&zoom=%d&w=%d&ip=%s">[week]</A>'%(timep,7*24*3600/w,w,ip))
    print('<A HREF="/cgi-bin/ping.py?time=%s&zoom=%d&w=%d&ip=%s">[day]</A>'%(timep,24*3600/w,w,ip))
    print('<A HREF="/cgi-bin/ping.py?time=%s&zoom=%d&w=%d&ip=%s">[6hr]</A>'%(timep,6*3600/w,w,ip))
    print('<A HREF="/cgi-bin/ping.py?time=%s&zoom=%d&w=%d&ip=%s">[2hr]</A>'%(timep,2*3600/w,w,ip))
    print('<A HREF="/cgi-bin/ping.py?time=%s&zoom=%d&w=%d&ip=%s">[1hr]</A>'%(timep,3600/w,w,ip))
    print('<A HREF="/cgi-bin/ping.py?time=%s&zoom=%d&w=%d&ip=%s">[RT]</A>'%(timep,1,w,ip))


print("</BODY></HTML>")

