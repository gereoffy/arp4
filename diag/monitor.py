#! /usr/bin/python3

import sys,time,os,glob,stat

pingresult={}
ping_cnt=10

stattab={}
statkey=[]

def test_ping(path):
    ip=open(path).readline().strip()
    res=ip #"ping "+ip

    if ip in pingresult:
        # use cached result!
        out=pingresult[ip]
        if out>=ping_cnt-1: return 0,res # (almost) all replied -> OK
        if out==0: return 2,res  # 0 replies -> ERR
        if out<0: return out,res # system error
        return 1,res # WARN

#    print ip
#    out = subprocess.Popen(["/bin/ping", "-c1", "-w5", ip], stdout=subprocess.PIPE).stdout.read()
#    out = os.popen("/bin/ping -c3 -w5 "+ip,"r").read()
#    out = pyping.ping(ip)
    out = os.system("/bin/ping -q -c1 -w2 "+ip+" >/dev/null")
    if out==0:
        # host up!
        return 0,res
    if out!=256:
        # system error
        return -1,res
    # try again
    out = os.system("/bin/ping -q -c3 -w5 "+ip+" >/dev/null")
    if out==0:
        return 0,res   # ha 3 ping mind megjon akkor OK
#    out = os.system("/bin/ping -q -c30 "+ip+" >/dev/null")
    out = os.system("/bin/ping -q -c10 "+ip+" >/dev/null")
    if out==0:
        return 1,res   # ha 10 pingbol legalabb 1 megjott akkor WARN, kulonben ERR
#        return 0   # ha 10 pingbol legalabb 1 megjott akkor WARN, kulonben ERR
    return 2,res

# return:  -1=error 0=ok 1=warning 2=critical 3=unknown
def run_test(path):
    test=path.rsplit("/",1)[1]
    if test=="ping":
        return test_ping(path)
    return -1,"unknown:"+test

def run_tree(path,dotest):
    lista=glob.glob(path+'*')
    lista.sort()
    ok=0
    err=0
    for f in lista:
        if stat.S_ISREG(os.stat(f).st_mode):
            if dotest:
                ret,res=run_test(f)
                if ret==0:
                    ok+=1
                if ret==2:
                    err+=1
            else:
                ret=3
                res="skipping"
#            print f+"="+str(ret)
            stattab[f]=(ret,res)
            statkey.append(f)
    if err>0 and ok==0:
        dotest=0
    for f in lista:
        if stat.S_ISDIR(os.stat(f).st_mode):
            run_tree(f+"/",dotest)


def ping_tree(path,dotest):
    lista=glob.glob(path+'*')
    lista.sort()
    ok=0
    err=0
    pinglist=[]
    for f in lista:
        if stat.S_ISREG(os.stat(f).st_mode):
            ip=open(f).readline().strip()
            pinglist.append(ip)
    for f in lista:
        if stat.S_ISDIR(os.stat(f).st_mode):
            pinglist+=ping_tree(f+"/",dotest)
    return pinglist


######### MAIN ############

try:
    from ping import pppping
    pingresult=pppping(ping_tree("tree/",1),cnt=ping_cnt,interval=1,deadline=0,source=None,verbose=False)
    #print(pingresult)
except:
    pass

run_tree("tree/",1)

#print run_test("htk-switches/CORE/","ping")

ss={}
ss[0]="OK"
ss[1]="WARN"
ss[2]="ERR"
ss[3]="???"
l=max(len(res) for ret,res in stattab.values())

for line in open("status.dat","r"):
    path,ret=line.strip().rsplit("=",1)
    try:
        ret=int(ret)&3
        newret,newres=stattab[path]
        if ret!=newret:
            if newret==3:
                stattab[path]=4|ret;
                res=path+": ? (%s)"%(ss[ret])
            else:
                res=path+": %s->%s  CHANGED!"%(ss[ret],ss[newret])
        else:
            res=path+": %s"%(ss[newret])
#        print("%-50s  (%s)"%(res,newres))
        print("%*s  %s"%(l,newres,res))
    except:
        print(path+": ???")

f=open("status.dat","w")
for line in statkey:
    f.write(line+"="+str(stattab[line][0])+"\n")
