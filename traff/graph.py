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

devlist=""
for devline in os.popen("ls /var/traffmon/*-TRAFF2-*.dat|cut -d '-' -f 3|cut -d . -f 1-|sort|uniq"):
    if devlist:
        devlist=devlist+","
    devlist=devlist+devline.strip().replace(".dat","")

# Get form data
form = cgi.FieldStorage()
try:
    timep=form["time"].value
except:
    timep="auto"
try:
    zoom =int(form["zoom"].value)
except:
    zoom = 6 #72
try:
    w =int(form["w"].value)
except:
    w = 1200
try:
    h =int(form["h"].value)
except:
    h = 256
try:
    ip=form["ip"].value
except:
    ip=iplist
try:
    dev=form["dev"].value
except:
    dev=devlist
try:
    scale =int(form["scale"].value)
except:
    scale = -1
try:
    smooth =int(form["smooth"].value)
except:
    smooth = 3600

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
print("""<style type="text/css">
body {
  background-color:#CCC;
}
#tooltip {
  display: none;
  position: absolute;
  text-align: center;
  color: white;
  padding: 8px;
  margin-top: 16px;
  margin-left: 16px;
  font: 13px sans-serif;
  font-weight: 100;
  background: #333;
  border-radius: 4px;
  pointer-events: none;
  -moz-box-shadow: 4px  4px  4px #000000;
  -webkit-box-shadow: 4px  4px  4px #000000;
  box-shadow: 4px  4px  4px #000;
}
</style>""")
print("</HEAD><BODY>")

print("<br>")

print('<b>X:</b>')
print('pos: <button onclick="setTime(0)">AUTO</button>')
print('<button onclick="setTime(-400)">Back</button>')
print('<button onclick="setTime(400)">Fwd</button>')
print('scale: <button onclick="setZoom(-2)">Zoom-</button>')
print('<button onclick="setZoom(-0.5)">Zoom+</button>')
print('<button onclick="setZoom(%d)">year</button>'%(365*3600*24/w))
print('<button onclick="setZoom(%d)">month</button>'%(31*3600*24/w))
print('<button onclick="setZoom(%d)">week</button>'%(7*3600*24/w))
print('<button onclick="setZoom(%d)">day</button>'%(3600*24/w))
print('<button onclick="setZoom(%d)">6hr</button>'%(3600*6/w))
print('<button onclick="setZoom(%d)">2hr</button>'%(3600*2/w))
print('<button onclick="setZoom(%d)">1hr</button>'%(3600/w))
print('<button onclick="setZoom(1)">RT</button>')

print("<br>")

print('<b>Y:</b> log')
print('<button onclick="setScale(-100)">1G</button>')
print('<button onclick="setScale(-10)">100M</button>')
print('<button onclick="setScale(-1)">10M</button>')
print('lin')
print('<button onclick="setScale(102400)">100M</button>')
print('<button onclick="setScale(10240)">10M</button>')
print('<button onclick="setScale(1024)">1M</button>')
print('<button onclick="setScale(100)">100k</button>')
print('<button onclick="setScale(10)">10k</button>')
print('<button onclick="setScale(1)">1k</button>')
print('avg:')
print('<button onclick="setSmooth(0)">OFF</button>')
print('<button onclick="setSmooth(-5)">ON</button>')
print('<button onclick="setSmooth(10)">10s</button>')
print('<button onclick="setSmooth(60)">1m</button>')
print('<button onclick="setSmooth(300)">5m</button>')
print('<button onclick="setSmooth(3600)">1h</button>')

print("<br>")

canvasdata=""
canvascnt=0

if ip:
  for ip1 in ip.split(","):
    canvascnt+=1
#    print("PING "+ip1+" ("+timew+", %d secs/pixel)"%(zoom))
    print('<br><canvas id="canvas%d" width="%dpx" height="%dpx" onmousemove="canvasCursor(event,this,0)" onclick="canvasCursor(event,this,1)" onmouseover="mouseOver()" onmouseout="mouseOut()" style="cursor:crosshair"></canvas><br>'%(canvascnt,w,h))
    canvasdata+='updateCanvasPING("canvas%d","%s",%d);\n'%(canvascnt,ip1,w)

if dev:
  for dev1 in dev.split(","):
    canvascnt+=1
    print('<br><canvas id="canvas%drx" width="%dpx" height="%dpx" onmousemove="canvasCursor(event,this,0)" onclick="canvasCursor(event,this,1)" onmouseover="mouseOver()" onmouseout="mouseOut()" style="cursor:crosshair"></canvas><br>'%(canvascnt,w,h))
    print('<canvas id="canvas%dtx" width="%dpx" height="%dpx" onmousemove="canvasCursor(event,this,0)" onclick="canvasCursor(event,this,1)" onmouseover="mouseOver()" onmouseout="mouseOut()" style="cursor:crosshair"></canvas><br>'%(canvascnt,w,h))
    canvasdata+='updateCanvasTRAFF("canvas%d","%s",%d);\n'%(canvascnt,dev1,w)

print('<div id="tooltip"></div>')


print("""<script type="text/javascript">
// <![CDATA[ =================================== JAVASCRIPT BEGINS ==============================================

var cursorx=0;

function unix2str(t){
var date = new Date(t*1000);
var ev = date.getFullYear();
var honap = 1+date.getMonth();
var nap = date.getDate();
// Hours part from the timestamp
var hours = date.getHours();
// Minutes part from the timestamp
var minutes = "0" + date.getMinutes();
// Seconds part from the timestamp
var seconds = "0" + date.getSeconds();
// Will display time in 10:30:23 format
return ev+'.'+honap+'.'+nap+' / '+ hours + ':' + minutes.substr(-2) + ':' + seconds.substr(-2);
}

function renderCanvas(canvObj,data,zoom,scale,stat,stat2){ // ha scale==0 akkor ping
    var canvas = document.getElementById(canvObj)
    var ctx = canvas.getContext("2d");
    let rect = canvas.getBoundingClientRect();
    var w=rect.width;
    var h=rect.height;
//    var w=canvas.width;
//    var h=canvas.height;

    var devicePixelRatio = window.devicePixelRatio || 1;
//    console.log(rect);
//    console.log(devicePixelRatio);
    // increase the actual size of our canvas
    canvas.width = rect.width * devicePixelRatio;
    canvas.height = rect.height * devicePixelRatio;
    ctx.scale(devicePixelRatio, devicePixelRatio);
    // ensure all drawing operations are scaled
    canvas.style.width = rect.width + 'px';
    canvas.style.height = rect.height + 'px';

    // clear
    ctx.fillStyle = "#ffffff";
    ctx.fillRect(0,0,w,h);

    function transform(x){
        if(scale<0)
            x=(Math.log10(x/(-scale)+1000)-3.0)*h/4;
        else if(scale>0)
            x=(x/scale)*h/1024;
        if(x<0) x=0;
        if(x>=h) x=h-1;
        return h-x;
    }

//     DRAW MIN-MAX

    if(zoom>1){
        ctx.fillStyle = "#00FF00";
        for (const [xx, value] of Object.entries(data)){
          y=transform(value[1]);
          ctx.fillRect(xx,y,1,transform(value[2])-y);
        }
    }

//     DRAW AXIS

    ctx.textBaseline = "top";
    ctx.textAlign = "start";
    ctx.font = "16px Helvetica";
    ctx.fillStyle = "#c0c0c0";
  if(scale<0){
    // log scale
    var y1=100*(-scale);
    while(y1<=10000000*(-scale)){
	var y2;
	for(y2=3;y2>0;y2--){
	    y=y1*((y2<3)?y2:5);
	    y=transform(y);
	    if(y<1) y=1;
	    ctx.fillStyle = "#c0c0c0";
	    ctx.fillRect(0,y,w,1);
	    if(y2==1){
		--y;
		ctx.fillRect(0,y,w,1);
		ctx.fillStyle = "#000000";
		ctx.textBaseline = (y>h/3) ? "bottom" : "top";
		if(y1>=1000000) ctx.fillText((y1/1000000)+"M",0,y);
		else if(y1>=1000) ctx.fillText((y1/1000)+"k",0,y);
		else ctx.fillText(y1,0,y);
	    }
	}
	y1*=10;
    }
  } else if(scale>0){
    // linear scale
	var y2;
	for(y2=0;y2<=20;y2++){
//	for(y2=20;y2>=0;y2--){
	    y=h-1-y2*h/20;
	    if(y<1) y=1;
	    ctx.fillRect(0,y,w,1);
	    if(y2%5==0){
		--y;
		ctx.fillRect(0,y,w,1);
		y1=(y2*1024/20)*scale;
//		if(y>=h/3) y-=14;
		ctx.textBaseline = (y>h/3) ? "bottom" : "top";
		if(y1>=1024*1024) ctx.fillText((y1/(1024*1024))+"M",0,y);
		else if(y1>=1024) ctx.fillText((y1/1024)+"k",0,y);
		else ctx.fillText(y1,0,y);
	    }
	}
  } else {
    // no scale (PING)
    var y1=100;
    while(y1<=10000000){
	var y2;
	for(y2=3;y2>0;y2--){
	    y=y1*((y2<3)?y2:5);
	    //y=transform(y);
	    y=h-(Math.log10(y+1000)-3.0)*h/4;
	    if(y<1) y=1;
	    ctx.fillStyle = "#c0c0c0";
	    ctx.fillRect(0,y,w,1);
	    if(y2==1){
		--y;
		ctx.fillRect(0,y,w,1);
		ctx.textBaseline = (y>h/3) ? "bottom" : "top";
		ctx.textAlign = "start";
		ctx.fillStyle = "#000000";
		if(y1>=1000000) ctx.fillText((y1/1000000)+"s",0,y);
		else if(y1>=1000) ctx.fillText((y1/1000)+"ms",0,y);
		else ctx.fillText(y1,0,y)+"us";
		if(y1<=1000000){ // packet-loss scale
		    ctx.textAlign = "end";
		    ctx.fillStyle = "#ff0000";
		    ctx.fillText((y1/10000)+"%",w,y);
		}
	    }
	}
	y1*=10;
    }

  }

//      DRAW DATA

    ctx.fillStyle = "#000000";
    ctx.strokeStyle = "#FF0000";
    ctx.lineWidth = 1;
    ctx.beginPath();
    var lastx=0;
    for (const [xx, value] of Object.entries(data)){
    //  console.log(x, value);
      x=parseInt(xx);
      ctx.fillRect(x-0.5,transform(value[0])-0.5,2,2);
      if(x!=(lastx+1))
        ctx.moveTo(x,transform(value[3]));
      else
        ctx.lineTo(x,transform(value[3]));
      lastx=x;
    }
    ctx.stroke();

//      DRAW STATS

    ctx.fillStyle = "#0000FF";
    ctx.textBaseline = "top";
    ctx.textAlign = "center";
    ctx.font = "16px Helvetica";
    ctx.fillText(stat,w/2,4);
    ctx.font = "12px Helvetica";
    ctx.fillText(stat2,w/2,22);

}


function updatelabel(zoom,start,w){

var X=start+cursorx*zoom;

document.getElementById('xcoord').textContent=unix2str(X);
tooltip.innerHTML=unix2str(X);

if(w>0){
    document.getElementById('xend').textContent=unix2str(start+w*zoom);
    document.getElementById('xstart').textContent=unix2str(start);
}

//document.getElementById('xcoord').textContent=X.toString();
//document.getElementById('ycoord').textContent=Y.toString();
//kep.title=X.toString()+' '+Y.toString();
}

function egerakepen(e,zoom){
    var kep=document.getElementById("grafikon");
//    var start = parseInt((kep.src.split("time=")[1]).split("&")[0]);
    var start = parseInt(kep.src.split("&time=")[1]);
    cursorx=e.clientX-kep.offsetLeft+window.pageXOffset;
    updatelabel(zoom,start,0);

//  tooltip.innerHTML=e.pageX+";"+e.pageY;
  tooltip.style.left=e.pageX+"px";
  tooltip.style.top=e.pageY+"px";

}

function mouseOver(){tooltip.style.display="inline";}
function mouseOut(){tooltip.style.display="none";}


//window.onload = function() {
function autorefresh(zoom,width){
    var image = document.getElementById("grafikon");
    var image2 = document.getElementById("grafikon2");
    function updateImage() {
	var t=new Date().getTime();
	t=t/1000-zoom*width;
        image.src = image.src.split("&time=")[0] + "&time=" + t;
	if(image2!=null) image2.src = image2.src.split("&time=")[0] + "&time=" + t;
//	var start = parseInt(image.src.split("&time=")[1]);
	updatelabel(zoom,t,image.width);
    }
    setInterval(updateImage, zoom*1000);
}


//var canvasIntervals={};
var glob_time=0;
var glob_time1=0;
var glob_zoom=1;
var glob_scale=-1;
var glob_smooth=0;
var glob_refresh=0;
var glob_data={};


function setupAll(func){
//    var t=new Date().getTime();

//    window.location.href=window.location.href.split("?")[0]+"?zoom="+glob_zoom;
//    window.location.search=window.location.href.split("?")[0]+"?zoom="+glob_zoom;
//    console.log(window.location.pathname);
    url=window.location.pathname+"?zoom="+glob_zoom;
    if(glob_time) url+="&time="+parseInt(glob_time);
    console.log(url);
    history.pushState({}, null, url);
//    window.location.search=url;

    func();
    if(glob_refresh){ clearInterval(glob_refresh); glob_refresh=0; console.log("setInterval OFF"); }
    if(!glob_time){
      glob_refresh=setInterval(func, glob_zoom*1000);
      console.log("setInterval "+glob_zoom+" -> "+glob_refresh);
    }
}

function get_stat2(w){
    var timew=glob_zoom*w;
    if(timew<300)
        timew=timew+" seconds"
    else if(timew<8*3600 && (timew%3600)!=0)
        timew=parseInt(timew/60)+" minutes";
    else if(timew<5*24*3600 && (timew%(3600*24))!=0)
        timew=parseInt(timew/3600)+" hours";
    else
        timew=parseInt(timew/(24*3600))+" days";
    var t1=unix2str(glob_time1-w*glob_zoom);
    var t2=unix2str(glob_time1);
//    console.log(t1.split(" / ")[0]);
    if(t1.split(" / ")[0]==t2.split(" / ")[0]) t2=t2.split(" / ")[1];
    return t1+" - "+t2+"  ("+timew+", "+glob_zoom+" secs/pixel)";
}

function updateCanvasPING(canvObj,ip,w){
      const start = Date.now();
      fetch('pingdata?time='+glob_time1+'&ip='+ip+'&zoom='+glob_zoom+'&w='+w).then(function(response) {
        if (!response.ok) { throw Error(response.statusText); }
        response.json().then(function(jsondata) {
            renderCanvas(canvObj,jsondata["ping"],glob_zoom,0,jsondata["pingstat"],get_stat2(w));
            glob_data[canvObj]=jsondata["ping"];
//            console.log("time:",Date.now() - start);
        });
      }).catch(function(error) {
            console.log('EXCEPTION: '+error);
      });
}

function updateCanvasTRAFF(canvObj,dev,w){
      const start = Date.now();
      fetch('traffdata?time='+glob_time1+'&dev='+dev+'&zoom='+glob_zoom+'&w='+w+'&smooth='+glob_smooth).then(function(response) {
        if (!response.ok) { throw Error(response.statusText); }
        response.json().then(function(jsondata) {
            renderCanvas(canvObj+"rx",jsondata["rx"],glob_zoom,glob_scale,jsondata["rxstat"],get_stat2(w));
            glob_data[canvObj+"rx"]=jsondata["rx"];
            renderCanvas(canvObj+"tx",jsondata["tx"],glob_zoom,glob_scale,jsondata["txstat"],get_stat2(w));
            glob_data[canvObj+"tx"]=jsondata["tx"];
//            console.log("time:",Date.now() - start);
        });
      }).catch(function(error) {
            console.log('EXCEPTION: '+error);
      });
}


function setZoom(chg){
    if(chg<0)
        glob_zoom=parseInt(-glob_zoom*chg);
    else
        glob_zoom=chg;
    if(glob_zoom<1) glob_zoom=1;
    setupAll(updateAll);
}

function setTime(chg){
    if(chg)
        glob_time=glob_time1+chg*glob_zoom;
    else
        glob_time=0;
    setupAll(updateAll);
}

function setScale(chg){
    glob_scale=chg;
    setupAll(updateAll);
}
function setSmooth(chg){
    glob_smooth=chg;
    setupAll(updateAll);
}


function canvasCursor(e,canvas,click){
//    console.log(click);
//    console.log(cid.id);
//    var canvas=document.getElementById("canvas"+cid);
//    var canvas=document.getElementById(cid.id);  // https://stackoverflow.com/questions/6186065/element-id-on-mouse-over
//    var canvas=cid; //document.getElementById(cid.id);  // https://stackoverflow.com/questions/6186065/element-id-on-mouse-over
    cursorx=e.clientX-canvas.offsetLeft+window.pageXOffset;
    let rect = canvas.getBoundingClientRect();
    var t=glob_time1-(rect.width-cursorx)*glob_zoom;
    if(click){
        console.log(cursorx,click,unix2str(t));
        // zoom vagy move
        if(cursorx>rect.width/3 && cursorx<2*rect.width/3){
            glob_zoom=parseInt(glob_zoom/2)
            if(glob_zoom<1) glob_zoom=1;
        }
        glob_time=t+(rect.width/2)*glob_zoom;
        setupAll(updateAll);
    } else {
        tooltip.style.left=e.pageX+"px";
        tooltip.style.top=e.pageY+"px";
        tooltip.innerHTML=unix2str(t);
        tooltip.innerHTML=unix2str(t)+"<br>"+glob_data[canvas.id][cursorx];
    }
}""")

print('glob_url=window.location.pathname+"?ip=%s&dev=%s";'%(ip,dev))
print('function updateAll(){')
print('  glob_time1=glob_time ? glob_time : Date.now()/1000; // auto time')
print('  console.log(glob_time1);')
print(canvasdata)
print('}')
print('glob_time=%d;'%(0 if timep=="auto" else time1))
print('glob_zoom=%d;'%(zoom))
print('glob_scale=%d;'%(scale))
print('glob_smooth=%d;'%(smooth))
print('setupAll(updateAll);')

print("// =================================== JAVASCRIPT ENDS ================================== ]]>")
print('</script>')
print("</BODY></HTML>")

