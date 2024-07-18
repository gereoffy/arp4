#! /usr/bin/python3

# code based on https://github.com/PinkInk/upylib/tree/master/usnmp
# changes:  py2 support (bytes->bytearray), ID generator, easysnmp api wrapper, disabled octetstr->utf8 decoding,

# conversion config parameters:
def snmp_config(int2str=False,oidsplit=False,octet2str=1):
    global config_int2str    # convert int to str? (easysnmp does that)
    global config_oidsplit   # split oid to base+index, aka easysnmp with use_numeric=True
    global config_octet2str  # 0=always bytes  1=bytes or ascii   2=python2 str   3=python3 str  8=utf8
    config_int2str=int2str
    config_oidsplit=oidsplit
    config_octet2str=octet2str
snmp_config() # set defaults


_SNMP_PROPS = ('ver', 'community')
_SNMP_TRAP_PROPS = ('enterprise', 'agent_addr', 'generic_trap', 'specific_trap', 'timestamp')
_SNMP_GETSET_PROPS = ('id', 'err_status', 'err_id')
#packet templates, refer utils/template.py
_SNMP_GETSET_TEMPL = b'0\x14\x02\x00\x04\x06public\xa0\x08\x02\x00\x02\x00\x02\x000\x00'
_SNMP_TRAP_TEMPL = b'0%\x02\x01\x00\x04\x06public\xa4\x18\x06\x05+\x06\x01\x04\x01@\x04\x7f\x00\x00\x01\x02\x01\x00\x02\x01\x00C\x01\x000\x00' 


try:
    const(1)
except:
    def const(v): return v

SNMP_VER1 = const(0x00)

#ASN1_NONE = const(0x00)  # ???
ASN1_BOOL = const(0x01)
ASN1_INT = const(0x02)
ASN1_BITSTR = const(0x03)
ASN1_OCTSTR = const(0x04)
ASN1_NULL = const(0x05)
ASN1_OID = const(0x06)
ASN1_SEQ = const(0x30)

# https://cdpstudio.com/manual/cdp/snmpio/about-snmp.html
SNMP_IPADDR = const(0x40)
SNMP_COUNTER = const(0x41)
SNMP_GUAGE = const(0x42)
SNMP_TIMETICKS = const(0x43)
SNMP_OPAQUE = const(0x44)
SNMP_NSAPADDR = const(0x45)
SNMP_COUNTER64 = const(0x46)
SNMP_UINTEGER32 = const(0x47)

# from https://github.com/Cistern/snmp/blob/master/types.go
SNMP_NoSuchObject   = const(0x80)
SNMP_NoSuchInstance = const(0x81)
SNMP_EndOfMIBView   = const(0x82)

SNMP_GETREQUEST = const(0xa0)
SNMP_GETNEXTREQUEST = const(0xa1)
SNMP_GETRESPONSE = const(0xa2)
SNMP_SETREQUEST = const(0xa3)
SNMP_TRAP = const(0xa4)
SNMP_BULKGETREQUEST = const(0xa5)
SNMP_REPORT = const(0xa8)

snmp_typ2str={
    1:"BOOL",
    2:"INTEGER",
    3:"BITS",
    4:"OCTETSTR",
    5:"NULL",
    6:"OBJECTID",
    0x40:"IPADDR",
    0x41:"COUNTER",
    0x42:"GAUGE",
    0x43:"TICKS",
    0x44:"OPAQUE",
    0x46:"COUNTER64",
    0x47:"UNSIGNED32",
    0x81:"NOSUCHINSTANCE",
    0x80:"NOSUCHOBJECT",
    0x82:"ENDOFMIBVIEW",
}

SNMP_ERR_NOERROR = const(0x00)
SNMP_ERR_TOOBIG = const(0x01)
SNMP_ERR_NOSUCHNAME = const(0x02)
SNMP_ERR_BADVALUE = const(0x03)
SNMP_ERR_READONLY = const(0x04)
SNMP_ERR_GENERR = const(0x05)

SNMP_TRAP_COLDSTART = const(0x0)
SNMP_TRAP_WARMSTART = const(0x10)
SNMP_TRAP_LINKDOWN = const(0x2)
SNMP_TRAP_LINKUP = const(0x3)
SNMP_TRAP_AUTHFAIL = const(0x4)
SNMP_TRAP_EGPNEIGHLOSS = const(0x5)

#ASN.1 sequence and SNMP derivatives
_SNMP_SEQs = bytearray([ASN1_SEQ,
                    SNMP_GETREQUEST,
                    SNMP_GETRESPONSE,
                    SNMP_GETNEXTREQUEST,
                    SNMP_SETREQUEST,
                    SNMP_TRAP,
                    SNMP_BULKGETREQUEST
                  ])

#ASN.1 int and SNMP derivatives
_SNMP_INTs = bytearray([ASN1_INT, ASN1_BOOL,
                    SNMP_COUNTER,
                    SNMP_GUAGE,
                    SNMP_TIMETICKS,
                    SNMP_COUNTER64,
                    SNMP_UINTEGER32
                  ])

_SNMP_ERRs = bytearray([ #ASN1_NONE,
                    SNMP_NoSuchObject,
                    SNMP_NoSuchInstance,
                    SNMP_EndOfMIBView
                  ])


def tobytes_tv(t, v=None):
    if t in _SNMP_SEQs:
        b = v
    elif t == ASN1_OCTSTR:
#        print(type(v), v)
        if type(v) in (bytes, bytearray):
            b = v
        else:
            b = bytearray(v,'utf-8')
#            raise ValueError('string or buffer required')
    elif t in _SNMP_INTs:
        v=int(v)
        if v < 0:
            raise ValueError('ASN.1 ints must be >=0')
        else:
            b = bytearray() if v!=0 else bytearray(1)
            while v > 0:
                b = bytearray([v & 0xff]) + b
                v //= 0x100
            if len(b)>0 and b[0]&0x80 == 0x80:
                b = bytearray([0x0]) + b
    elif t == ASN1_NULL:
        b = bytearray()
    elif t == ASN1_OID:
        oid = [int(x) for x in v.split('.')]
#        if len(oid)==1 or oid[1]>=40:
        if len(oid)==1:
            b = bytearray([oid[0]*40]) ; oid=oid[1:]
        else:  #first two indexes are encoded in single byte:
            b = bytearray([oid[0]*40 + oid[1]]) ; oid=oid[2:]
        for id in oid:
            ob = bytearray() if id>0 else bytearray([0])
            while id > 0:
                ob = bytearray([id&0x7f if len(ob)==0 else 0x80+(id&0x7f)]) + ob
                id //= 0x80
            b += ob
    elif t == SNMP_IPADDR:
        b = bytearray()
        for octet in v.split('.'):
            octet = int(octet)
            b = b + bytearray([octet])
    elif t in (SNMP_OPAQUE, SNMP_NSAPADDR):
        raise Exception('not implemented', t)
    else:
        raise TypeError('invalid type', t)
    return bytearray([t]) + tobytes_len(len(b)) + b

def tobytes_len(l):
    if l < 0x80:
        return bytearray([l])
    else:
        b = bytearray()
        while l > 0:
            b = bytearray([l&0xff]) + b
            l //= 0x100
        return bytearray([0x80+len(b)]) + b

def frombytes_tvat(b, ptr):
    # frombytes_tvat bytearray(b'\x04\x82\x01\x00\xff\xff\xff\xff\xff\xff\xff\xff\xff\xff\xff\xff')
    t = b[ptr]
    l, l_incr = frombytes_lenat(b, ptr)
    ptr+= 1+l_incr
    end = ptr+l
#    print("From: t=",t,l,l_incr,b[ptr:end],b[end:end+8])
    if t in _SNMP_SEQs:
        v = bytearray(b[ptr:end])
    elif t == ASN1_OCTSTR:
        v = bytearray(b[ptr:end])
        if config_octet2str==1 and sum([(x<32 or x>127) for x in v])==0: # plain ascii, no control chars/unicodes, convert to string!
            v = "".join(map(chr, v)) # py2/py3 compatible bytes->str
        if config_octet2str==2: v = str(v) # python2
        if config_octet2str==3: v = v.decode("latin-1") # python3
        if config_octet2str==8: v = v.decode("utf-8")

#        try:
#            v = str(b[ptr:end], 'utf-8')
#            v = b[ptr:end].decode("us-ascii")
#            v = b[ptr:end].decode("utf-8")
#        except: #UnicodeDecodeError:
#            v = bytearray(b[ptr:end])
    elif t in _SNMP_INTs:
        v=0
        while ptr < end:
            v = v*0x100 + b[ptr]
            ptr += 1
#        v=str(v) # :(
    elif t == ASN1_NULL:
        v=None
    elif t == ASN1_OID:
        #first 2 indexes are encoded in single byte
#        print("OID:",b[ptr],b[ptr+1])
        v = str( b[ptr]//0x28 )
        if l>1 or b[ptr]%0x28 != 0:
            v += '.' + str( b[ptr]%0x28 )
#        v = str( b[ptr]//0x28 ) + '.' + str( b[ptr]%0x28 ) # example: 1.0.8802.1.1.1.1.2.1.1.1
        ptr += 1
        ob = 0
        while ptr < end:
            if b[ptr]&0x80 == 0x80:
                ob = ob*0x80 + (b[ptr]&0x7f)
            else:
                v += '.' + str((ob*0x80)+b[ptr])
                ob = 0
            ptr += 1
#        print(v)
    elif t == SNMP_IPADDR:
        v = ''
        while ptr < end:
            v += '.' + str(b[ptr]) if v!='' else str(b[ptr])
            ptr += 1
    elif t in _SNMP_ERRs:
#        print("ERR type found:",t,"len=",l)
        v = None # bulk get returns type=0x82 at the end!
    elif t in (SNMP_OPAQUE, SNMP_NSAPADDR):
        raise Exception('not implemented', t)
    else:
        raise TypeError('invalid type', t)
    return t, v

def frombytes_lenat(b, ptr):
    if b[ptr+1]&0x80 == 0x80:
        lol=b[ptr+1]&0x7f # len of len
        l = 0
        for i in b[ptr+2 : ptr+2+lol]:
            l = l*0x100 + i
        return l, 1 + lol
    else:
        return b[ptr+1], 1




class SNMPVariable:
    def __init__(self, oid, tv=None, oidsplit=None):
        if tv == None: t,v = ASN1_NULL, None
        else:          t,v = tv
        self.raw_oid=oid
        self.raw_type=t
        self.snmp_type=snmp_typ2str[t]
        self.value=str(v) if config_int2str and t in _SNMP_INTs else v
        # OID split?
        try: oid_base,oid_index=oid.rsplit(".",1)
        except: oid_base,oid_index="",oid
        if oidsplit==None: oidsplit=config_oidsplit
        self.oid,self.oid_index = (oid_base,oid_index) if oidsplit else (oid,"")
        self.idx = int(oid_index)
    def get(self): return self.raw_oid, self.raw_type, self.value
    def __repr__(self):  #  from easysnmp
        return (
            "<{0} value={1} (oid={2}, oid_index={3}, snmp_type={4})>".format(
                self.__class__.__name__,
                repr(self.value), repr(self.oid),
                repr(self.oid_index), repr(self.snmp_type)
            )
        )


class SnmpPacket:

    def __init__(self, *args, **kwargs):
        if len(args) == 1 and type(args[0]) in (bytes, bytearray, memoryview):
            b = bytearray(args[0])
        elif 'type' in kwargs:
            if kwargs['type'] == SNMP_TRAP:
                b = bytearray(_SNMP_TRAP_TEMPL)
            else:
                b = bytearray(_SNMP_GETSET_TEMPL)
        else:
            raise ValueError('buffer or type=x required')
        oidsplit = kwargs.get("oidsplit",None)
        ptr = 1 + frombytes_lenat(b, 0)[1]
        ptr = self._frombytes_props(b, ptr, _SNMP_PROPS)
        self.type = b[ptr]
        ptr += 1 + frombytes_lenat(b, ptr)[1] #step into seq
        if self.type == SNMP_TRAP:
            ptr = self._frombytes_props(b, ptr, _SNMP_TRAP_PROPS)
        else:
            ptr = self._frombytes_props(b, ptr, _SNMP_GETSET_PROPS)
        ptr += 1 + frombytes_lenat(b, ptr)[1]
        self.varbinds = [] # array of SNMPVariable
        while ptr < len(b):
            ptr += 1 + frombytes_lenat(b, ptr)[1] #step into seq
            oid = frombytes_tvat(b, ptr)[1]
            if not oid: break # EOF?
            ptr += 1 + sum(frombytes_lenat(b, ptr))
            tv = frombytes_tvat(b, ptr)
            ptr += 1 + sum(frombytes_lenat(b, ptr))
            self.varbinds.append(SNMPVariable(oid,tv,oidsplit=oidsplit))
        for arg in kwargs:
            if hasattr(self, arg):
                setattr(self, arg, kwargs[arg])

    def tobytes(self):
        b = bytearray()
        for var in self.varbinds:
            oid,t,v = var.get()
            b += tobytes_tv(ASN1_SEQ, tobytes_tv(ASN1_OID, oid) + tobytes_tv(t,v))
        b = tobytes_tv(ASN1_SEQ, b)
        if self.type == SNMP_TRAP:
            b = tobytes_tv(ASN1_OID, self.enterprise) \
                + tobytes_tv(SNMP_IPADDR, self.agent_addr) \
                + tobytes_tv(ASN1_INT, self.generic_trap) \
                + tobytes_tv(ASN1_INT, self.specific_trap) \
                + tobytes_tv(SNMP_TIMETICKS, self.timestamp) \
                + b
        elif self.type == SNMP_BULKGETREQUEST:
            b = tobytes_tv(ASN1_INT, self.id) \
                + tobytes_tv(ASN1_INT, 0) \
                + tobytes_tv(ASN1_INT, 32) \
                + b
        else:
            b = tobytes_tv(ASN1_INT, self.id) \
                + tobytes_tv(ASN1_INT, self.err_status) \
                + tobytes_tv(ASN1_INT, self.err_id) \
                + b
        return tobytes_tv(ASN1_SEQ, tobytes_tv(ASN1_INT, self.ver) \
                + tobytes_tv(ASN1_OCTSTR, self.community) \
                + tobytes_tv(self.type, b))

    def _frombytes_props(self, b, ptr, properties):
        for prop in properties:
            setattr(self, prop, frombytes_tvat(b, ptr)[1])
            ptr += 1 + sum(frombytes_lenat(b, ptr))
        return ptr


# high-level easysnmp-compatible API:

def snmp_id():
    from time import time
    return (int(time()*1000000) & 0x3FFFFFFF)|0x40000000



import socket
from contextlib import closing  # python2 socket() doesnt support context manager :(

def snmp_get(oid, hostname, community="public", version=2, timeout=2, retries=1, use_numeric=None, debug=False, port=161):
    with closing(socket.socket(socket.AF_INET,socket.SOCK_DGRAM)) as s:
        s.settimeout(timeout)
        # send a getrequest packet:
        greq = SnmpPacket(type=SNMP_GETREQUEST, community=community, ver=version-1, id=snmp_id())
        greq.varbinds = [SNMPVariable(oid)]
        data=greq.tobytes()
        if debug: print("Send:",data)
        s.sendto(data, (hostname, port))
        d = s.recvfrom(1024)
        if debug: print("Rcvd:",d[0])
        # decode the response:
        gresp = SnmpPacket(d[0],oidsplit=use_numeric)
        if debug: print(gresp.varbinds)
        if len(gresp.varbinds)!=1: return None # WTF
        return gresp.varbinds[0]

def snmp_walk(oid, hostname, community="public", version=2, timeout=2, retries=1, use_numeric=None, debug=False, port=161, bulk=True):
    s=socket.socket(socket.AF_INET,socket.SOCK_DGRAM)
    s.settimeout(timeout)
    #build a getnextrequest packet
    greq = SnmpPacket(type=(SNMP_BULKGETREQUEST if (bulk and version==2) else SNMP_GETNEXTREQUEST), community=community, ver=version-1, id=snmp_id())
    o=oid
    while True:
        #send getnextrequest:
        greq.varbinds = [SNMPVariable(o)]
        data=greq.tobytes()
        if debug: print("Send:",data)
        s.sendto(data, (hostname, port))
        d = s.recvfrom(16384) # jumbo frame support ;)
        if debug: print("Rcvd:",d[0])
        #decode the response:
        gresp = SnmpPacket(d[0],oidsplit=use_numeric)
#        print("Sent %d -> Rcvd %d bytes, %d oids"%(len(data),len(d[0]),len(gresp.varbinds)))
        if debug: print(gresp.varbinds)
        for var in gresp.varbinds:
            o,t,v=var.get()
            if not o or not o.startswith(oid) or t in _SNMP_ERRs: s.close() ; return  # itt a vege fuss el vele
            yield var
        greq.id+=1


if __name__ == '__main__':

    swver=2
    swip="10.200.6.21"
#    swip="10.200.5.1"
    swc="public"
    oid = "1.3.6.1.2.1.1.3.0" # uptime

#    from easysnmp import snmp_get,snmp_walk
#    for item in snmp_walk('1.3.6.1.2.1.31.1.1.1.1',hostname=swip, community=swc, version=2, use_numeric=True): print(item)
#    for item in snmp_walk('1.3.6.1.2.1.17.4.3.1.1',hostname=swip, community=swc, version=2, use_numeric=True): print(item)
# <SNMPVariable value='3:40' (oid='.1.3.6.1.2.1.31.1.1.1.1', oid_index='168', snmp_type='OCTETSTR')>
# <SNMPVariable value=997977330 (oid='1.3.6.1.2.1.1.3', oid_index='0', snmp_type=67)>

#    res=snmp_get("1.3.6.1.2.1.1.7.0", hostname=swip, community=swc, version=swver, timeout=2, retries=1, use_numeric=True)
#    print(res)
#    osi=int(res.value)

# 1.0 	standard, std
# 1.1 	registration-authority
# 1.2 	member-body
# 1.3 	identified-organization, org, iso-identified-organization
    for item in snmp_walk('1',hostname=swip, community=swc, version=2, use_numeric=True, timeout=10): print(item)

#    res=usnmp_get(oid, hostname=swip, community=swc, version=swver, timeout=2, retries=1, use_numeric=True)
#    print(res)
#    osi=int(res.value)

