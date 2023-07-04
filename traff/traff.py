#! /usr/bin/python3

import os
import sys
import cgi
import time

devlist=" ".join([ipline.strip()[:-4] for ipline in os.popen("ls /var/traffmon/*-TRAFF2-*.dat|cut -d '-' -f 3|sort|uniq")])

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
    scale =int(form["scale"].value)
except:
    scale = -1
try:
    smooth =int(form["smooth"].value)
except:
    smooth = 3600
try:
    w =int(form["w"].value)
except:
    w = 1200
try:
    h =int(form["h"].value)
except:
    h = 256
try:
    dev =form["dev"].value
except:
    dev=devlist

try:
    time1=int(timep)
except:
    timep="auto"
    time1=int(time.time()-(w-10)*zoom)


try:
    click =form["click"].value.lstrip('?').split(',')
    click_x=int(click[0])
    click_y=int(click[1])
#    print "Content-type: text/plain"
#    print ""
#    print "X=%d Y=%d" % (click_x,click_y)
    click_t=time1+click_x*zoom
    if click_x>w/3 and click_x<2*w/3:
        zoom=zoom/2
    if zoom<1:
        zoom=1
    time1=click_t-(w/2)*zoom
except:
    pass


print("Content-type: text/html")
print("")
print("<HTML><HEAD>")
if timep=="auto":
    print('<meta http-equiv="refresh" content="%d">' % (zoom))
print("</HEAD><BODY>")

for dev1 in dev.split(" "):
    print('<A HREF="/cgi-bin/traff.py?time=%s&zoom=%d&smooth=%d&scale=%d&w=%d&dev=%s">[%s]</A>'%(timep,zoom,smooth,scale,w,dev1,dev1))
    if dev.find(dev1)<0:
        print('<A HREF="/cgi-bin/traff.py?time=%s&zoom=%d&smooth=%d&scale=%d&w=%d&dev=%s">+</A>'%(timep,zoom,smooth,scale,w,dev+","+dev1))

for dev1 in dev.split(" "):
    print("<br><br>")

#    print '<A href="/cgi-bin/test-cgi?hello=123">'
    print('<A HREF="/cgi-bin/traff.py?time=%d&zoom=%d&smooth=%d&scale=%d&w=%d&dev=%s&click=">'%(time1,zoom,smooth,scale,w,dev))
    print('<IMG SRC="/cgi-bin/traffer2?zoom=%d&smooth=%d&scale=%d&w=%d&h=%d&dev=%s&rx&time=%d" width=%d height=%d ismap onmousemove="egerakepen(event,%d)" id="grafikon" alt="target">'%(zoom,smooth,scale,w,h,dev1,time1,w,h,zoom))

    print("</A>")
#    print "<br>"

    lt=time.localtime(time1)
    print('<table width=%d><tr><td align=left>|&lt;---'%(w))
    print('<span id="xstart">%04d.%02d.%02d/%02d:%02d:%02d</span>' % (lt[0],lt[1],lt[2],lt[3],lt[4],lt[5]))
#    print '%04d.%02d.%02d/%02d:%02d:%02d' % (lt[0],lt[1],lt[2],lt[3],lt[4],lt[5])
    timew=zoom*w
    lt=time.localtime(time1+timew)
    if timew<300:
        timew="%d seconds"%(timew)
    elif timew<8*3600 and (timew%3600)!=0:
        timew="%d minutes"%(timew/60)
    elif timew<5*24*3600 and (timew%(3600*24))!=0:
        timew="%d hours"%(timew/3600)
    else:
        timew="%d days"%(timew/(24*3600))
    print('</td><td align=center><b>')
    print("TRAFFIC "+dev1+" ("+timew+", %d secs/pixel)"%(zoom))
    print('</b></td><td align=right>')
#    print '%04d.%02d.%02d/%02d:%02d:%02d' % (lt[0],lt[1],lt[2],lt[3],lt[4],lt[5])
    print('<span id="xend">%04d.%02d.%02d/%02d:%02d:%02d</span>' % (lt[0],lt[1],lt[2],lt[3],lt[4],lt[5]))
    print('---&gt;|</td></tr></table>')
    #print " --- %04d.%02d.%02d/%02d:%02d:%02d" % (lt[0],lt[1],lt[2],lt[3],lt[4],lt[5])

    print('<A HREF="/cgi-bin/traff.py?time=%d&zoom=%d&smooth=%d&scale=%d&w=%d&dev=%s&click=">'%(time1,zoom,smooth,scale,w,dev))
    print('<IMG SRC="/cgi-bin/traffer2?zoom=%d&smooth=%d&scale=%d&w=%d&h=%d&dev=%s&tx&time=%d" width=%d height=%d ismap onmousemove="egerakepen(event,%d)" id="grafikon2" alt="target">'%(zoom,smooth,scale,w,h,dev1,time1,w,h,zoom))
    print("</A>")

    print("<br>")
    print('Cursor: <span id="xcoord"></span><br/>')

    print("<br>")
    print('<A HREF="/cgi-bin/traff.py?time=%d&zoom=%d&smooth=%d&scale=%d&w=%d&dev=%s">[BACK]</A>'%(time1-zoom*w/4,zoom,smooth,scale,w,dev))
    print('<A HREF="/cgi-bin/traff.py?time=%s&zoom=%d&smooth=%d&scale=%d&w=%d&dev=%s">[auto]</A>'%("auto",zoom,smooth,scale,w,dev))
    print('<A HREF="/cgi-bin/traff.py?time=%d&zoom=%d&smooth=%d&scale=%d&w=%d&dev=%s">[stop]</A>'%(time1,zoom,smooth,scale,w,dev))
    print('<A HREF="/cgi-bin/traff.py?time=%d&zoom=%d&smooth=%d&scale=%d&w=%d&dev=%s">[FWD]</A>'%(time1+zoom*w/4,zoom,smooth,scale,w,dev))


    if timep=="auto":
        print('<A HREF="/cgi-bin/traff.py?time=%s&zoom=%d&smooth=%d&scale=%d&w=%d&dev=%s">---</A>'%(timep,2*zoom,smooth,scale,w,dev))
        if zoom>1:
            print('<A HREF="/cgi-bin/traff.py?time=%s&zoom=%d&smooth=%d&scale=%d&w=%d&dev=%s">+++</A>'%(timep,zoom/2,smooth,scale,w,dev))
    else:
        time2=time1+(w/2)*zoom
        print('<A HREF="/cgi-bin/traff.py?time=%d&zoom=%d&smooth=%d&scale=%d&w=%d&dev=%s">---</A>'%(time2-w*zoom,2*zoom,smooth,scale,w,dev))
        if zoom>1:
            print('<A HREF="/cgi-bin/traff.py?time=%d&zoom=%d&smooth=%d&scale=%d&w=%d&dev=%s">+++</A>'%(time2-w*zoom/4,zoom/2,smooth,scale,w,dev))

    print('<A HREF="/cgi-bin/traff.py?time=%s&zoom=%d&smooth=%d&scale=%d&w=%d&dev=%s">[year]</A>'%(timep,365*24*3600/w,smooth,scale,w,dev))
    print('<A HREF="/cgi-bin/traff.py?time=%s&zoom=%d&smooth=%d&scale=%d&w=%d&dev=%s">[100d]</A>'%(timep,100*24*3600/w,smooth,scale,w,dev))
    print('<A HREF="/cgi-bin/traff.py?time=%s&zoom=%d&smooth=%d&scale=%d&w=%d&dev=%s">[month]</A>'%(timep,31*24*3600/w,smooth,scale,w,dev))
    print('<A HREF="/cgi-bin/traff.py?time=%s&zoom=%d&smooth=%d&scale=%d&w=%d&dev=%s">[week]</A>'%(timep,7*24*3600/w,smooth,scale,w,dev))
    print('<A HREF="/cgi-bin/traff.py?time=%s&zoom=%d&smooth=%d&scale=%d&w=%d&dev=%s">[day]</A>'%(timep,24*3600/w,smooth,scale,w,dev))
    print('<A HREF="/cgi-bin/traff.py?time=%s&zoom=%d&smooth=%d&scale=%d&w=%d&dev=%s">[6hr]</A>'%(timep,6*3600/w,smooth,scale,w,dev))
    print('<A HREF="/cgi-bin/traff.py?time=%s&zoom=%d&smooth=%d&scale=%d&w=%d&dev=%s">[2hr]</A>'%(timep,2*3600/w,smooth,scale,w,dev))
    print('<A HREF="/cgi-bin/traff.py?time=%s&zoom=%d&smooth=%d&scale=%d&w=%d&dev=%s">[1hr]</A>'%(timep,3600/w,smooth,scale,w,dev))
    print('<A HREF="/cgi-bin/traff.py?time=%s&zoom=%d&smooth=%d&scale=%d&w=%d&dev=%s">[RT]</A>'%(timep,1,smooth,scale,w,dev))

    print('Scale: <A HREF="/cgi-bin/traff.py?time=%s&zoom=%d&smooth=%d&scale=%d&w=%d&dev=%s">1G</A>'%(timep,zoom,smooth,-100,w,dev))
    print('<A HREF="/cgi-bin/traff.py?time=%s&zoom=%d&smooth=%d&scale=%d&w=%d&dev=%s">100M</A>'%(timep,zoom,smooth,-10,w,dev))
    print('<A HREF="/cgi-bin/traff.py?time=%s&zoom=%d&smooth=%d&scale=%d&w=%d&dev=%s">10M</A>'%(timep,zoom,smooth,-1,w,dev))
    print('<A HREF="/cgi-bin/traff.py?time=%s&zoom=%d&smooth=%d&scale=%d&w=%d&dev=%s">+++</A>'%(timep,zoom,smooth,scale/2,w,dev))
    print('<A HREF="/cgi-bin/traff.py?time=%s&zoom=%d&smooth=%d&scale=%d&w=%d&dev=%s">---</A>'%(timep,zoom,smooth,scale*2,w,dev))
    print('<A HREF="/cgi-bin/traff.py?time=%s&zoom=%d&smooth=%d&scale=%d&w=%d&dev=%s">100M</A>'%(timep,zoom,smooth,102400,w,dev))
    print('<A HREF="/cgi-bin/traff.py?time=%s&zoom=%d&smooth=%d&scale=%d&w=%d&dev=%s">10M</A>'%(timep,zoom,smooth,10240,w,dev))
    print('<A HREF="/cgi-bin/traff.py?time=%s&zoom=%d&smooth=%d&scale=%d&w=%d&dev=%s">1M</A>'%(timep,zoom,smooth,1024,w,dev))
    print('<A HREF="/cgi-bin/traff.py?time=%s&zoom=%d&smooth=%d&scale=%d&w=%d&dev=%s">100k</A>'%(timep,zoom,smooth,100,w,dev))
    print('<A HREF="/cgi-bin/traff.py?time=%s&zoom=%d&smooth=%d&scale=%d&w=%d&dev=%s">10k</A>'%(timep,zoom,smooth,10,w,dev))
    print('<A HREF="/cgi-bin/traff.py?time=%s&zoom=%d&smooth=%d&scale=%d&w=%d&dev=%s">1k</A>'%(timep,zoom,smooth,1,w,dev))

    print('Smooth: <A HREF="/cgi-bin/traff.py?time=%s&zoom=%d&smooth=%d&scale=%d&w=%d&dev=%s">OFF</A>'%(timep,zoom,0,scale,w,dev))
    print('/ <A HREF="/cgi-bin/traff.py?time=%s&zoom=%d&smooth=%d&scale=%d&w=%d&dev=%s">ON</A>'%(timep,zoom,-5,scale,w,dev))
    print('/ <A HREF="/cgi-bin/traff.py?time=%s&zoom=%d&smooth=%d&scale=%d&w=%d&dev=%s">10s</A>'%(timep,zoom,10,scale,w,dev))
    print('/ <A HREF="/cgi-bin/traff.py?time=%s&zoom=%d&smooth=%d&scale=%d&w=%d&dev=%s">1min</A>'%(timep,zoom,60,scale,w,dev))
    print('/ <A HREF="/cgi-bin/traff.py?time=%s&zoom=%d&smooth=%d&scale=%d&w=%d&dev=%s">5min</A>'%(timep,zoom,300,scale,w,dev))
    print('/ <A HREF="/cgi-bin/traff.py?time=%s&zoom=%d&smooth=%d&scale=%d&w=%d&dev=%s">1hr</A>'%(timep,zoom,3600,scale,w,dev))

    print("<br>")
    print("<br>")
    print("<br>")

print("</BODY></HTML>")

