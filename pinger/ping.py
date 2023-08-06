#! /usr/bin/python3

import os
import sys

import time
import socket
import select
import struct

def checksum(data):  # Calc ICMP checksum as in RFC 1071
    csum = sum( ((data[i] << 8) + data[i+1]) for i in range(0, len(data)-1, 2) )
    if len(data)&1: csum += (data[len(data)-1] << 8)
    while csum>0xFFFF: csum= (csum & 0xFFFF) + (csum>>16)
    return (~csum) & 0xFFFF

# pure python parallel pinger
def pppping(iplist,cnt=5,interval=1,deadline=0,source=None,verbose=False):
    if not deadline: deadline=cnt*interval+2
    try:
        sock = socket.socket(socket.AF_INET, socket.SOCK_RAW, socket.getprotobyname("icmp"))
        offset=20
    except PermissionError:
        sock = socket.socket(socket.AF_INET, socket.SOCK_DGRAM, socket.getprotobyname("icmp"))
        offset=0
    if source: sock.bind((source, 0))

    p_id=os.getpid()&0x7FFF
    p_id2=None
    p_seq=1
#    payload=bytes(1400)
    payload=bytes([0xAA,0x55]*700)
    sent={}
    result={ip:0 for ip in iplist}

    t0=time.time()
    tlast=0
    while time.time()<t0+deadline:
        if p_seq<=cnt and time.time()>tlast+interval:
            # send packets!
            packet=struct.pack("!BBHHH",8,0,    0           ,p_id,p_seq)+payload
            packet=struct.pack("!BBHHH",8,0,checksum(packet),p_id,p_seq)+payload
            for target in iplist:
                sock.sendto(packet, (target, 0))
                sent[(target,p_id,p_seq)]=time.time()
            tlast=time.time()
            if verbose: print("Sent seq=%d at %5.3f"%(p_seq,tlast-t0))
            p_seq+=1
        # receive packets!
        time_left=max(tlast+interval-time.time(), 0.1)
        data_ready = select.select([sock], [], [], time_left)
        if not data_ready[0]: continue
        packet, source = sock.recvfrom(2048)
        if not packet: continue
#        print(source, packet[:64].hex(' '))
        t=time.time()
        ip=source[0]
        resp=struct.unpack("!BBHHH", packet[offset:offset+8]) # msg_type, msg_code, checksum, id, seq   (0, 0, 48513, 17021, 1)
        # Unknown reply: (0, 0, 15807, 2, 1)
        csum=checksum(packet[offset:])
#        print(source, len(packet), resp, csum) # ('193.224.177.1', 0) 1428 (0, 0, 38736, 26798, 1) 0
        if csum==0:
            if resp[0]==0:
                key=(ip,resp[3],resp[4])
                if not key in sent:  # ID mismatch, OK for non-root users...
                    key=(ip,p_id,resp[4])
                    if key in sent:
                        if p_id2==None: p_id2=resp[3] # learn new ID  (IP & seq matched)
                        elif p_id2!=resp[3] and verbose: print("ID mismatch! %d != %d  (sent %d)"%(p_id2,resp[3],p_id))
                if key in sent:
                    p_id2=resp[3]
                    if verbose: print("Received ping from %s time %5.3f ms"%(ip,1000.0*(t-sent[key])))
                    result[ip]+=1
                    del sent[key]
                elif verbose: print("BAD reply:",key, sent.keys())
            elif verbose: print("Unknown reply:",resp)
        elif verbose: print("Bad checksum!", resp)
        if p_seq>cnt and len(sent)==0:
            if verbose: print("All OK!!!")
            break # done
    return result

if __name__ == "__main__":
    res=pppping(sys.argv[1:] if len(sys.argv)>1 else ["193.224.177.1","1.1.1.1","8.8.8.8"], 10, 0.3, verbose=True)
    print(res)

