
// apt install libnetfilter-log1
// gcc -Wall -Wno-pointer-sign -Os dhcparser-NFLOG.c -o dhcparser-NFLOG -lnetfilter_log

#include <stdio.h>
#include <string.h>
#include <strings.h>
#include <malloc.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <errno.h>
#include <time.h>

#include <stdint.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/socket.h>
#include <asm/types.h>
#include <linux/netlink.h>
#include <linux/if.h>

//#include <linux/netfilter_ipv4/ipt_ULOG.h>
#include <libnetfilter_log/libnetfilter_log.h>


//#include "intsizes.h"
#include "dhcparser.h"

void hexDump(void *addr, int len){
    int i;
    unsigned char buff[17];
    unsigned char *pc = (unsigned char*)addr;
    if (len == 0) {
        printf("  ZERO LENGTH\n");
        return;
    }
    if (len < 0) {
        printf("  NEGATIVE LENGTH: %i\n",len);
        return;
    }
    // Process every byte in the data.
    for (i = 0; i < len; i++) {
        // Multiple of 16 means new line (with line offset).
        if ((i % 16) == 0) {
            // Just don't print ASCII for the zeroth line.
            if (i != 0)
                printf ("  %s\n", buff);

            // Output the offset.
            printf ("  %04x ", i);
        }
        // Now the hex code for the specific character.
        printf (" %02x", pc[i]);
        // And store a printable ASCII character for later.
        if ((pc[i] < 0x20) || (pc[i] > 0x7e))
            buff[i % 16] = '.';
        else
            buff[i % 16] = pc[i];
        buff[(i % 16) + 1] = '\0';
    }
    // Pad out last line if not exactly 16 characters.
    while ((i % 16) != 0) {
        printf("   ");
        i++;
    }
    // And print the final ASCII bit.
    printf("  %s\n", buff);
}


void fptoa(char *fingerprint, u_int8_t *rawfp, u_int8_t rawfplen){
	int i,sptr=0;
	for (i=0;i<rawfplen;i++)
		sptr+=sprintf((fingerprint+sptr),"%d,",rawfp[i]);
	if(sptr>0) fingerprint[sptr-1]=0;
}


char base_dir_name[]="info/";

void dumpInfo(char *mac,char *hn,char *ven,u_int8_t ttl,char *fingerprint, char *all){
	printf("MAC: %s HOSTNAME: %s VENDOR: %s TTL: %d FINGERPRINT: %s ALLOPTS: %s\n",mac,hn,ven,ttl,fingerprint,all);
	char dirname[256]; dirname[0]=0;
	strcpy(dirname,base_dir_name);
	strcat(dirname,mac);
	//printf("going to mkdir: %s\n",dirname);
	int ret=mkdir(dirname,0777);
	if (ret!=0) {
			if (errno!=EEXIST) {
				fprintf(stderr,"ERROR: mkdir(%s) %s\n",dirname,strerror(errno));
				return;
			}
	} else
	chmod(dirname,0777);
	char filename[256]; filename[0]=0;
	strcpy(filename,dirname);
	strcat(filename,"/dhcp");
	//printf("going to write to: %s\n",filename);
	FILE *f=fopen(filename,"w");
	if (!f) {
		fprintf(stderr,"ERROR: fopen(%s,\"w\") %s\n",filename,strerror(errno));
		return;
	}
	if (strlen(hn)>0) {
		fprintf(f,"dhcp-name: %s\n",hn);
	}
	if (strlen(ven)>0) {
		fprintf(f,"dhcp-vendor: %s\n",ven);
	}
	if (strlen(fingerprint)>0) {
		fprintf(f,"dhcp-fp: %s\n",fingerprint);
	}
	if (strlen(all)>0) {
		fprintf(f,"dhcp-fp2: %s\n",all);
	}
	fprintf(f,"dhcp-ttl: %d\n",ttl);
	fclose(f);
}

// parse DHCP:
void parse_dhcp(unsigned char* udppayload, int udppayloadlen,unsigned char ttl){
	dhcp_hdr_t *dhcp=(dhcp_hdr_t*)(udppayload);
	if (dhcp->op!=DHCP_BOOTREQUEST){
		printf("not DHCP request pkg\n");
		return;
	}
	if (dhcp->htype!=DHCP_HTYPE_ETH)
		return;

	char client_mac[18];
	sprintf(client_mac,"%02X:%02X:%02X:%02X:%02X:%02X",(u_int8_t)dhcp->chaddr[0],(u_int8_t)dhcp->chaddr[1],(u_int8_t)dhcp->chaddr[2],(u_int8_t)dhcp->chaddr[3],(u_int8_t)dhcp->chaddr[4],(u_int8_t)dhcp->chaddr[5]);

	u_int8_t *opts=(u_int8_t*)(udppayload+sizeof(dhcp_hdr_t));
	u_int32_t optslen=udppayloadlen-sizeof(dhcp_hdr_t);
	if (optslen<5) {
		fprintf(stderr,"No DHCP options requested by %s\n",client_mac);
		return;
	}
	if (bcmp(opts,rfc_magic,RFC_MAGIC_SIZE))
		return;

	opts+=RFC_MAGIC_SIZE;
	optslen-=RFC_MAGIC_SIZE;

	u_int8_t *scan=(u_int8_t*)opts;
	u_int16_t scanlen=optslen;

	char hostname[256]; hostname[0]=0;
	char vendor[256]; vendor[0]=0;
	u_int8_t alltags[1024]; memset(alltags,0,1024); u_int16_t palltags=0;
	u_int8_t rawfp[256]; u_int8_t rawfplen=0;
	u_int8_t tag=dhcptag_pad_e;
	for (scan=(u_int8_t*)opts; tag!=dhcptag_end_e && scanlen>0;) {
		tag=scan[DHCP_TAG_OFFSET];
		palltags+=sprintf((alltags+palltags),"%u,",tag);
		switch (tag) {
			case dhcptag_end_e: scan++; scanlen--; break;
			case dhcptag_pad_e: scan++; scanlen--; break;
			case dhcptag_parameter_request_list_e: {
				u_int8_t thisoptlen=scan[DHCP_LEN_OFFSET];
				memset(rawfp,0,255);
				memcpy(rawfp,&scan[DHCP_OPTION_OFFSET],thisoptlen);
				rawfplen=thisoptlen;
				scanlen-=(thisoptlen+2);
				scan+=(thisoptlen+2);
				break;
			}
			case dhcptag_host_name_e: {
				u_int8_t thisoptlen=scan[DHCP_LEN_OFFSET];
				memset(hostname,0,255);
				strncpy(hostname,&scan[DHCP_OPTION_OFFSET],thisoptlen);
				scanlen-=(thisoptlen+2);
				scan+=(thisoptlen+2);
				break;
			}
			case dhcptag_vendor_class_identifier_e: {
				u_int8_t thisoptlen=scan[DHCP_LEN_OFFSET];
				memset(vendor,0,255);
				strncpy(vendor,&scan[DHCP_OPTION_OFFSET],thisoptlen);
				scanlen-=(thisoptlen+2);
				scan+=(thisoptlen+2);
				break;
			}
			default: {
				u_int8_t thisoptlen=scan[DHCP_LEN_OFFSET];
				scanlen-=(thisoptlen+2);
				scan+=(thisoptlen+2);
				break;
			}
		}
	}
	alltags[strlen(alltags)-1]=0;
	char fingerprint[1024]; // maxlen=255tag*3szamjegy+254vesszo=1019
	memset(fingerprint,0,1024);
	if(rawfplen>0) fptoa(fingerprint,rawfp,rawfplen);
	dumpInfo(client_mac,hostname,vendor,ttl,fingerprint,alltags);
}


// 16+6+2 byte=24
typedef struct tcpconn_s {
    u_int32_t   sip,dip;
//    u_int32_t   t;
    u_int32_t   seq;
    u_int16_t   sport,dport;
    u_int16_t   pos;
    unsigned char state,ssl;
} tcpconn_t;

static tcpconn_t tcpconn_table[1024];
static int tcpconn_idx=0;
static int16_t tcpconn_hash[65536];

tcpconn_t* new_tcpconn(u_int32_t sip,u_int32_t dip,u_int16_t sport,u_int16_t dport){
    unsigned int hash=(sip^dip)>>16; hash^=3*sport+7*dport; hash&=65535;
    tcpconn_t* conn=&tcpconn_table[tcpconn_idx];
    tcpconn_hash[hash]=tcpconn_idx;
    conn->sip=sip;
    conn->dip=dip;
    conn->sport=sport;
    conn->dport=dport;
//    printf("TCPCONN: new #%d hash=%04X  %08X:%d->%08X:%d\n",tcpconn_idx,hash,sip,sport,dip,dport);
    tcpconn_idx=(tcpconn_idx+1)&1023;
    return conn;
}

tcpconn_t* find_tcpconn(u_int32_t sip,u_int32_t dip,u_int16_t sport,u_int16_t dport){
    unsigned int hash=(sip^dip)>>16; hash^=3*sport+7*dport; hash&=65535;
    tcpconn_t* conn=&tcpconn_table[tcpconn_hash[hash]];
    if(conn->sip==sip && conn->dip==dip && conn->sport==sport && conn->dport==dport) return conn; // megvan!!!
    int i;
    for(i=0;i<256;i++){ // utolso 32 connection vizsgalata
	conn=&tcpconn_table[(1024+tcpconn_idx-i)&1023];
	if(conn->sip==sip && conn->dip==dip && conn->sport==sport && conn->dport==dport){
	    printf("TCPCONN: miss #%d-%d hash=%04X  %08X:%d->%08X:%d\n",tcpconn_idx,i,hash,sip,sport,dip,dport);
	    return conn; // megvan!!!
	}
    }
    printf("TCPCONN: notfound #%d hash=%04X  %08X:%d->%08X:%d\n",tcpconn_idx,hash,sip,sport,dip,dport);
    return NULL;
}


static FILE* logf=NULL;
static time_t logf_time=0;

static void log_http(char* conn,char* url, char* ua){
    struct tm tm;
    time_t t=time(NULL);
    localtime_r(&t,&tm);
    if(!logf){
	logf=fopen("/home/log/http.log","a");
	logf_time=t;
    }
    if(logf){
	fprintf(logf,"%4d.%02d.%02d-%02d:%02d:%02d %s %s %s\n",1900+tm.tm_year,1+tm.tm_mon,tm.tm_mday,tm.tm_hour,tm.tm_min,tm.tm_sec,conn,url,ua);
	if(t>logf_time+60){ // reopen every 600 sec
	    fclose(logf); logf=NULL;
	}
    }
    printf("!!! conn='%s' url='%s' ua='%s'\n",conn,url,ua);
}


static int clienthello(unsigned char* p,int len,char* conn){
//    hexDump(p,len);
    int van_nev=0;
    char pstr[1024];
    char ua[1024];

    printf("    ClientHello version %d.%d\n",p[0],p[1]);
    int ualen=sprintf(ua,"%d.%d;",p[0],p[1]);
    int off=2+32; // skip random
    off+=1+p[off]; // skip sessionid
    if(off>=len) return -1; // error
    int clen=p[off]*256+p[off+1];
    off+=2;
//    off+=clen; // skip ciphersuites
    while(clen>0){
	ualen+=sprintf(ua+ualen,"%d,",p[off]*256+p[off+1]);
	off+=2;clen-=2;
    }
    ua[ualen-1]=';';
    if(off>=len) return -2; // error
    off+=1+p[off]; // skip compression
    if(off>len) return -3; // error
    if(off==len) return 0; // no extensions (ssl3?)
//    int extlen=p[off]*256+p[off+1];
    off+=2;
//    printf("!!! Extensions off=%d len=%d (left: %d)\n",off,extlen,len-(off+extlen));
    while(off+4<=len){
//        extension type= 00 00  len=  00 22
	int exttype=p[off]*256+p[off+1];
	off+=2;
	int extlen=p[off]*256+p[off+1];
	off+=2;
//	printf("!!! EXT type=%d len=%d\n",exttype,extlen);
	ualen+=sprintf(ua+ualen,"%d,",exttype);
	if(exttype==0){
	    // SNI / Hostnames!!!
//        len2= 00 20 
//        NameType= 00
//        HostName:  len= 00 1d   data=76 6f  ....?...". ...vo
//	int extlen2=p[off]*256+p[off+1];
	int namelen=p[off+3]*256+p[off+4];
//	printf("Hostname len=%d\n",namelen); // name: off+5
	if(namelen>0 && off+5+namelen<=len){
	    snprintf(pstr,1020,"SSL/%d.%d https://%.*s",p[0],p[1],namelen,p+off+5);
	    printf("Hostname='%.*s'\n",namelen,p+off+5);
	    ++van_nev;
	}
	  
	}
	off+=extlen;
    }

    //if(!van_nev) printf("Hostname=MISSING (v%d.%d)\n",p[0],p[1]);
    if(van_nev){
//	printf("HTTPS '%s'\n",pstr);
	log_http(conn,pstr,ua);
    }
    return van_nev;
}

static char* ssl_msgtype(unsigned char x){
    switch(x){
    case 0: return "HelloRequest";
    case 1: return "ClientHello";
    case 2: return "ServerHello";
    case 4: return "NewSessionTicket";
    case 11: return "Certificate";
    case 12: return "ServerKeyExchange";
    case 13: return "CertificateRequest";
    case 14: return "ServerHelloDone";
    case 15: return "CertificateVerify";
    case 16: return "ClientKeyExchange";
    case 20: return "Finished";
    }
    return "???";
}



void analyze_tls(unsigned char* pkt, int len, char* conn,unsigned char* state){
//    int len=pkt[2]*256+pkt[3];
//    printf("==============================\npkt len=%d   ip len=%d\n",pktlen,len);
//    hexDump(pkt,pktlen);

//data:
//  0028  16=Handshake
//  0029  version=03.03=TLS1.2 length=0038 
//data:
//  0028  16=Handshake
//  0029  version=03.03=TLS1.2 length=0038 
//msgtype=02=ServerHello  len=000034
//       03 03 c9 1f d0 3d 3c 8d 01 21 0f 0e b3 dd b8  4.....=<..!.....
//  0040  46 2c 71 23 9b f5 7f 31 23 9d 37 a7 e5 f4 25 55  F,q#...1#.7...%U
//  0050  44 d8 d3 00 c0 13 00 00 0c 00 0b 00 04 03 00 01  D...............
//  0060  02 00 23 00 00 

  while(len>=5){
    if(pkt[0]<0x14 || pkt[0]>0x18 || pkt[1]!=0x3 || pkt[2]>4){
//	printf("BAD  %02X %02X %02X\n",pkt[0],pkt[1],pkt[2]);
	return;  // not ssl handshake
    }
    unsigned char sslprot=pkt[0];
    int ssllen=pkt[3]*256+pkt[4];
    
    char* protstr[]={"ChangeCipherSpec","Alert","Handshake","Application","Heartbeat"};
    
    printf("    SSL proto=%d/%s ver=%d len=%d  (iplen=%d)\n",sslprot,protstr[sslprot-0x14],pkt[2],ssllen,len);
    if(ssllen+5>len){
	if(sslprot!=23) printf("    FRAGMENTED!\n"); // Application eseten nem erdekes, es altalaban csak az nagy
//	hexDump(pkt,msglen);
	return; // fragmented
    }
//    if(sslprot==20) ++crypted;
    pkt+=5;len-=5;
    if(sslprot==20){
      // ChangeCipherSpec -> innentol titkositav van minden :(
      *state=255;
    } else
    if(sslprot==22 && *state!=255){
      // handshake:
//      hexDump(pkt,ssllen);
      int ssloff=0;
      while(ssloff+4<=ssllen){
//	printf(">>> %d (of %d)\n",ssloff,ssllen);
	unsigned char* p=pkt+ssloff;
	unsigned char msgtype=p[0];
	int msglen=p[1]*65536+p[2]*256+p[3];
	ssloff+=4;
	printf("    MSG type=%d/%s  len=%d\n",msgtype,ssl_msgtype(msgtype),msglen);
//	if(msgtype==1 && ssloff+msglen<=ssllen) hexDump(p+4,msglen);
	if(msgtype==1 && ssloff+msglen<=ssllen){
	    if(*state!=1) printf("error: invalid state: %d\n",*state);
	    int ret=clienthello(p+4,msglen,conn);
	    if(ret<1) printf("ClientHello error=%d\n",ret);
	    else *state=2;
	}
	ssloff+=msglen;
      }
      if(ssllen!=ssloff) printf("<<< %d left\n",ssllen-ssloff);
    }
    len-=ssllen;
    pkt+=ssllen;
  }
}


void analyze_http(unsigned char* pkt, int len, char* conn){
//    hexDump(pkt,len);
    int p=0;
    int h=-1;
    int ua=-1;
    while(p<len){
	int s=p;
	while(p<len && pkt[p]!=0x0a && pkt[p]!=0x0d) ++p;
	if(p>=len) break;
	int l=p-s;
	printf("hdr='%.*s'\n",l,pkt+s);
	if(l>5 && !strncasecmp(pkt+s,"Host:",5)) h=s+5;
	if(l>11 && !strncasecmp(pkt+s,"User-Agent:",11)) ua=s+11;
	if(pkt[p]==0x0a || pkt[p]==0x0d) ++p;
	if(p<len && (pkt[p]==0x0a || pkt[p]==0x0d) && pkt[p]!=pkt[p-1]) ++p;
	if(p<len && (pkt[p]==0x0a || pkt[p]==0x0d)){
	    printf("hdr-end\n");
	    break;
	}
    }
    if(h>0){
	// parse host
	while(h<len && pkt[h]==32) ++h;
	p=h;
	while(p<len && pkt[p]!=0x0a && pkt[p]!=0x0d) ++p;
	int hlen=p-h;
	// parse command
	p=0;
	while(p<len && pkt[p]!=32) ++p;
	int path=p+1;
	int prot=0;
	int ver=0;
	while(p<len && pkt[p]!=0x0a && pkt[p]!=0x0d){
	    if(pkt[p]==32) prot=p;
	    if(pkt[p]=='/') ver=p+1;
	    ++p;
	}
	if(prot){
	    char pstr[1024];
	    char uastr[256];
	    snprintf(pstr,1020,"%.*s/%.*s http://%.*s%.*s",path-1,pkt, p-ver,pkt+ver, hlen,pkt+h, prot-path,pkt+path);
//	    printf("HTTP '%s'\n",pstr);
	    while(ua<len && pkt[ua]==32) ++ua;
	    p=ua;while(p<len && pkt[p]!=0x0a && pkt[p]!=0x0d) ++p;
	    snprintf(uastr,250,"%.*s",p-ua,pkt+ua);
	    log_http(conn,pstr,uastr);
//	    printf("HTTP  version='%.*s'\n",p-ver,pkt+ver);
//	    printf("HTTP  proto='%.*s'\n",path-1,pkt);
//	    printf("HTTP  path='%.*s'\n",prot-path,pkt+path);
	}
    }
}


// IP packet
void analyze(u_int8_t *packet, u_int32_t len, u_int8_t *hwaddr){
	//printf("---------------- Packet: %d Length: %d ----------------\n",numpkt,len);
	//hexDump(packet,len);

//TODO: minimum packet len check!

	// IP packet
	ip_hdr_t *ip=(ip_hdr_t*)(packet);

	int size_ip=IP_HL(ip)*4;
	if (size_ip<20) {
		fprintf(stderr,"ERROR: Invalid IP header length: %d bytes\n",size_ip);
		return;
	}
	if (size_ip>20) {
		fprintf(stderr,"WARNING: IP header length: %d bytes\n",size_ip);
	}
	
	u_int16_t iplen=ntohs(ip->ip_len);
	if(iplen>len){
		fprintf(stderr,"WARNING: IP packet truncated!!! iplen=%d pktlen=%d bytes\n",iplen,len);
	} else
	if(iplen<len){
		fprintf(stderr,"WARNING: IP len too small??? iplen=%d pktlen=%d bytes\n",iplen,len);
		len=iplen;
	}

	char ip_src[16],ip_dst[16];
	strncpy(ip_src,inet_ntoa(ip->ip_src),15);
	strncpy(ip_dst,inet_ntoa(ip->ip_dst),15);
	printf("analyze():  %s->%s  proto=%d  ",ip_src,ip_dst,ip->ip_p);

	if (ip->ip_p==IPPROTO_UDP){
	    // UDP datagram
	    udp_hdr_t *udp=(udp_hdr_t*)(packet+size_ip);
	    u_int16_t dst_port=ntohs(udp->dst_port);
	    u_int16_t src_port=ntohs(udp->src_port);
	    printf("UDP  %d->%d\n",src_port,dst_port);
	    //u_int16_t chksum=ntohs(udp->chksum);
	    if (udp->len<8) {
		fprintf(stderr,"ERROR: Invalid UDP header length: %d bytes\n",udp->len);
		return;
	    }
	    u_int8_t *udppayload=(u_int8_t*)(packet+size_ip+SIZE_UDP_HEADER);
	    int udppayloadlen=ntohs(udp->len)-SIZE_UDP_HEADER;
	    if (dst_port==67){
		parse_dhcp(udppayload,udppayloadlen,ip->ip_ttl);
	    }
	} else
	if (ip->ip_p==IPPROTO_TCP){
	    tcp_hdr_t *tcp=(tcp_hdr_t*)(packet+size_ip);
	    u_int16_t dst_port=ntohs(tcp->dst_port);
	    u_int16_t src_port=ntohs(tcp->src_port);
	    u_int32_t seq=ntohl(tcp->seq);
	    int tcppayloadlen=len-size_ip-4*(tcp->offs>>4);
	    printf("TCP  %d->%d  %s%s  len=%d  seq=%u  fl=%02X\n",src_port,dst_port,(tcp->flags&TH_SYN)?"SYN":"",(tcp->flags&TH_FIN)?"FIN":"",tcppayloadlen,seq,tcp->flags);

	    if(tcp->flags&TH_SYN){
		tcpconn_t* conn=new_tcpconn(ip->ip_src.s_addr,ip->ip_dst.s_addr,src_port,dst_port);
		conn->seq=seq+1; conn->pos=0;
		conn->state=1; conn->ssl=0;
	    } else {
		tcpconn_t* conn=find_tcpconn(ip->ip_src.s_addr,ip->ip_dst.s_addr,src_port,dst_port);
		if(!conn){
		    // re-create conn
		    conn=new_tcpconn(ip->ip_src.s_addr,ip->ip_dst.s_addr,src_port,dst_port);
		    conn->seq=seq+tcppayloadlen; conn->pos=tcppayloadlen;
		    conn->state=13; conn->ssl=255;
		} else {
		    char connstr[256];
		    if(conn->seq!=seq){
			int diff=conn->seq-seq; // +=retrans -=missing
			if(tcppayloadlen==1 && diff==1)
			    printf("TCPCONN: keepalive!\n");
			else
			    printf("TCPCONN: seq mismatch!  %u/%u  %d (%s)\n",conn->seq,seq,diff,(diff>0)?"retrans":"missing");
		    } else
		    if(tcppayloadlen>0){
			sprintf(connstr,"%02X:%02X:%02X:%02X:%02X:%02X %s:%d-%s:%d %d",hwaddr[6],hwaddr[7],hwaddr[8],hwaddr[9],hwaddr[10],hwaddr[11],ip_src,src_port,ip_dst,dst_port,ip->ip_ttl);
			printf("TCPCONN: data: len=%d start=%d conn=%s\n",tcppayloadlen,conn->pos,connstr);
			if(conn->state==1){
			    if(dst_port==443) analyze_tls(packet+size_ip+4*(tcp->offs>>4),tcppayloadlen,connstr,&conn->ssl);
			    else if(conn->pos==0) analyze_http(packet+size_ip+4*(tcp->offs>>4),tcppayloadlen,connstr);
			}
			conn->seq+=tcppayloadlen;
			conn->pos+=tcppayloadlen;
			conn->state=2;
		    }
		    if(tcp->flags&TH_FIN){ ++conn->seq;conn->state=0; }
		}
	    }
	} else
	printf("\n");

}




int old_main(int argc, char **argv)
{
	FILE *f=fopen(argv[1],"rb");
	if (!f) {
		fprintf(stderr,"fopen %s failed, exiting\n", argv[1]);
		return 1;
	}

	pcap_hdr_t gheader;
	fread(&gheader,sizeof(pcap_hdr_t),1,f);
	// should be a1b2c3d4 if endianness is correct
	char magic[16]; memset(magic,0,16);
	snprintf(magic,9,"%02x",gheader.magic_number);
	if (strncmp(magic,"a1b2c3d4",8)) {
		fprintf(stderr,"FATAL ERROR: Wrong Endianness!\nMagic is read as: %s, it shoult be a1b2c3d4\n",magic);
		fclose(f);
		return 1;
	}
	// should be 2.4
	if ((gheader.version_major!= 2) || (gheader.version_minor!=4)) {
		fprintf(stderr,"pcap version is: %d.%d, only v2.4 supported.\n",gheader.version_major,gheader.version_minor);
		fclose(f);
		return 1;
	}
	// 0: GMT, -3600:GMT+1 etc. but in practice its always in GMT, so always 0
	//printf("timezone correction: %i\n",gheader.thiszone);
	//printf("snaplen: %d\n",gheader.snaplen); // max len of captured packets in octets
	//printf("network: %d\n",gheader.network); // 1: ethernet

	u_int8_t *packet=NULL;
	packet=(u_int8_t*)malloc(gheader.snaplen);
	if (packet==NULL) {
		fprintf(stderr,"failed to allocate packet buffer.\n");
		fclose(f);
		return 1;
	}

	while (!feof(f)) {
		pcaprec_hdr_t hdr;
		fread(&hdr,sizeof(hdr),1,f);
		if (hdr.incl_len>gheader.snaplen) {
			fprintf(stderr,"packet len > snaplen!\n");
			fclose(f);
			return 1;
		}
		//printf("reading packet: %d (len: %d)\n",numpkt,hdr.incl_len);
		memset(packet,0,gheader.snaplen);
		fread(packet,hdr.incl_len,1,f);
//		analyze(packet,hdr.incl_len);  // FIXME
		analyze(packet+14,hdr.incl_len-14,packet);
	}

	free(packet);
	fclose(f);

	return 0;
}


static int cb(struct nflog_g_handle *gh, struct nfgenmsg *nfmsg,
		struct nflog_data *ldata, void *data)
{
	struct nfulnl_msg_packet_hdr *ph = nflog_get_msg_packet_hdr(ldata);
	u_int32_t mark = nflog_get_nfmark(ldata);
	u_int32_t indev = nflog_get_indev(ldata);
	u_int32_t outdev = nflog_get_outdev(ldata);
	char *prefix = nflog_get_prefix(ldata);
	char *payload;
	int payload_len = nflog_get_payload(ldata, &payload);
	int hwlen=nflog_get_msg_packet_hwhdrlen(ldata);
	char* hwaddr=nflog_get_msg_packet_hwhdr(ldata);
	
	if (ph) {
		printf("hw_protocol=0x%04x hook=%u ", 
			ntohs(ph->hw_protocol), ph->hook);
	}

	printf("hwlen=%u ", hwlen);
	printf("mark=%u ", mark);

	if (indev > 0)
		printf("indev=%u ", indev);

	if (outdev > 0)
		printf("outdev=%u ", outdev);


	if (prefix) {
		printf("prefix=\"%s\" ", prefix);
	}
	if (payload_len >= 0)
		printf("payload_len=%d ", payload_len);

	fputc('\n', stdout);


//    ulog_packet_msg_t* hdr=buffer+16;
#if DEBUG
    int iplen=(hdr->payload[2]<<8)+hdr->payload[3];
    printf("%4d (%d/%d)  %.8s -> %.8s   %d  MAC:",nr,hdr->data_len,iplen,hdr->indev_name,hdr->outdev_name,hdr->mac_len);
    int i;
    for(i=0;i<hdr->mac_len;i++) printf(" %02X",hdr->mac[i]);    printf("\n");
//    for(i=0;i<hdr->data_len;i++) printf(" %02X",hdr->payload[i]);    printf("\n");
    printf("  %d.%d.%d.%d",hdr->payload[12],hdr->payload[13],hdr->payload[14],hdr->payload[15]);
    printf("->  %d.%d.%d.%d\n",hdr->payload[16],hdr->payload[17],hdr->payload[18],hdr->payload[19]);
//    printf("[%d]\n",sizeof(ulog_packet_msg_t));
#endif


//	memcpy(packet,hwaddr,hwlen);
//	memcpy(packet+14,payload,payload_len);
//	analyze(packet,payload_len+14);
	analyze(payload,payload_len,hwaddr);

	return 0;
}


int main(int argc, char **argv)
{
	struct nflog_handle *h;
	struct nflog_g_handle *qh;
	int rv, fd;
	char buf[4096];

	h = nflog_open();
	if (!h) {
		fprintf(stderr, "error during nflog_open()\n");
		exit(1);
	}

	printf("unbinding existing nf_log handler for AF_INET (if any)\n");
	if (nflog_unbind_pf(h, AF_INET) < 0) {
		fprintf(stderr, "error nflog_unbind_pf()\n");
		exit(1);
	}

	printf("binding nfnetlink_log to AF_INET\n");
	if (nflog_bind_pf(h, AF_INET) < 0) {
		fprintf(stderr, "error during nflog_bind_pf()\n");
		exit(1);
	}
	printf("binding this socket to group 100\n");
	qh = nflog_bind_group(h, 100);
	if (!qh) {
		perror("wtf");
		fprintf(stderr, "no handle for group 100\n");
		exit(1);
	}

	printf("setting copy_packet mode\n");
	if (nflog_set_mode(qh, NFULNL_COPY_PACKET, 0xffff) < 0) {
		fprintf(stderr, "can't set packet copy mode\n");
		exit(1);
	}

	fd = nflog_fd(h);

	printf("registering callback for group\n");
	nflog_callback_register(qh, &cb, NULL);

	printf("going into main loop\n");
	while ((rv = recv(fd, buf, sizeof(buf), 0)) && rv >= 0) {
		printf("pkt received (len=%u)\n", rv);

		/* handle messages in just-received packet */
		nflog_handle_packet(h, buf, rv);
	}

	printf("unbinding from group 100\n");
	nflog_unbind_group(qh);

#ifdef INSANE
	/* norally, applications SHOULD NOT issue this command,
	 * since it detaches other programs/sockets from AF_INET, too ! */
	printf("unbinding from AF_INET\n");
	nflog_unbind_pf(h, AF_INET);
#endif

	printf("closing handle\n");
	nflog_close(h);

	return EXIT_SUCCESS;
}


