#include <arpa/inet.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <unistd.h>
#include <sys/time.h>
#include <signal.h>
#include <time.h>
#include <sys/ioctl.h>

#include <netinet/in.h>
#include <netinet/ether.h>
#include <netinet/ip.h>
#include <netinet/ip_icmp.h>

// https://wiki.gentoo.org/wiki/Project:Toolchain/Linux_headers_5.2_porting_notes/SIOCGSTAMP
#include <linux/sockios.h>

#include "ln_tab.h"

#define BUFLEN 2048

/* From Stevens, UNP2ev1 */
static unsigned short in_cksum(unsigned char *addr, int len){
    int nleft = len;
    int sum = 0;
    unsigned short *w = (unsigned short *)addr;
    unsigned short answer = 0;

    while (nleft > 1) {
        sum += *w++;
        nleft -= 2;
    }

    if (nleft == 1) {
        *(unsigned char *)(&answer) = *(unsigned char *)w;
        sum += answer;
    }

    sum = (sum >> 16) + (sum & 0xffff);
    sum += (sum >> 16);
    answer = ~sum;
    return (answer);
}

// Returns current time in microseconds                                         
unsigned int GetTimer(void){
  struct timeval tv;
  //  float s;
  gettimeofday(&tv,NULL);
  //  s=tv.tv_usec;s*=0.000001;s+=tv.tv_sec;
  return tv.tv_sec * 1000000 + tv.tv_usec;
}

long long int GetTimerLL(void){
  struct timeval tv;
  //  float s;
  gettimeofday(&tv,NULL);
  //  s=tv.tv_usec;s*=0.000001;s+=tv.tv_sec;
  return tv.tv_sec * 1000000LL + tv.tv_usec;
}

typedef struct {
    unsigned int ipaddr;
    unsigned int secret;
    unsigned int time; // time()
    unsigned int time_sent; // GetTimer()
    //
    int seq;
    unsigned int time1; // logfile elso bejegyzese
    int logpos; // eddig van kiirva a file
    int datalen;
    char* IP;
    FILE* file;
    unsigned char data[64];
} ping_t;

ping_t* create(char* IP){
    ping_t* p=malloc(sizeof(ping_t));
    if(!p) return NULL;
    p->ipaddr=inet_addr(IP);
    p->IP=IP; //strdup(IP);
    p->secret=(getpid()^rand())^GetTimer();
    p->time=0;
    p->time_sent=0;
    p->seq=0;
    p->file=NULL;
    p->logpos=0;
    p->datalen=64; // buffer
    memset(p->data,123,p->datalen);
    return p;
}

ping_t* ping_open(ping_t* p,unsigned int time1){
    p->time1=time1;
    char fname[100];
    sprintf(fname,"%d-PING-%s.dat",p->time1,p->IP);
    p->file=fopen(fname,"wb");
    if(!p->file){
	printf("Cannot create logfile: %s\n",fname);
	free(p);
	return NULL;
    }
    fwrite(&p->time1,4,1,p->file);fflush(p->file);
    return p;
}

int ping1(int s,ping_t* p,int id,unsigned int time1){
    struct sockaddr_in si_other;
    memset((char *) &si_other, 0, sizeof(si_other));
    si_other.sin_family = AF_INET;
    si_other.sin_addr.s_addr = p->ipaddr;
    
//    printf("sizeof icmp = %d\n",sizeof(struct icmp));

    int pay=1400-4;
    int len=sizeof(struct icmp)+16+pay;
    unsigned char buf[len];
    memset(buf,0,len);
    int i;
    unsigned int x=rand();
    for(i=0;i<pay;i++){
	buf[sizeof(struct icmp)+16+i]=x&255;
	x=(x*123)^(x>>9);
    }
    struct icmp* icmp_hdr=(struct icmp*)buf;
    /* Prepare outgoing ICMP header. */
    icmp_hdr->icmp_type  = ICMP_ECHO;
//    icmp_hdr->icmp_code  = 0;
//    icmp_hdr->icmp_cksum = 0;
    icmp_hdr->icmp_id    = htons(id);
    icmp_hdr->icmp_seq   = htons(++p->seq);
    p->time=time1;
    p->time_sent=GetTimer();
    memcpy(buf+sizeof(struct icmp),p,16);
    icmp_hdr->icmp_cksum = in_cksum(buf,len);
//    int i; for(i=0;i<len;i++) printf(" %02X",buf[i]); printf("\n");
    len=sendto(s, buf, len, 0, (const struct sockaddr *)(&si_other), sizeof(si_other));
    p->time_sent=GetTimer(); // update timer

// xxx
    unsigned int off=p->time - p->time1;
    p->data[off % p->datalen]=0;


#if 0
    struct timeval tv_ioctl;
    int ret=ioctl(s, SIOCGSTAMP, &tv_ioctl);
    unsigned int sent3=tv_ioctl.tv_usec+tv_ioctl.tv_sec*1000000;
    printf("sent3->%d  delay=%d\n",ret,sent3-p->time_sent);
#endif
//    if(len<40 || sent2>100) printf("send->%d  time: %d\n",len,sent2);
    
}

ping_t* pinglist[1024];
int ping_num=0;

void ping(int s,int verbose,int id){
  struct sockaddr_in si_me, si_other;
  struct sockaddr si_from;
  int i;
  unsigned char buf[BUFLEN];
  unsigned int time_sent, time_start, time_start0;

//  int s=socket(AF_INET, SOCK_RAW, IPPROTO_ICMP);
//  if(dev) if(setsockopt(s, SOL_SOCKET, SO_BINDTODEVICE, dev, strlen(dev)+1)) return -1;

  time_start=GetTimer();
  time_start0=time_start;
  
//  while(1){
    int slen=sizeof(si_from);
    unsigned int rcvd_time,rcvd_time2;
    int len;
    while((len=recvfrom(s, buf, BUFLEN, 0, &si_from, &slen))>0){
	rcvd_time = GetTimer();
	struct timeval tv_ioctl;
	int ret=ioctl(s, SIOCGSTAMP, &tv_ioctl);
	if(ret){
	    printf("timestamp ioctl err: %d\n",ret);
	    rcvd_time2=rcvd_time;
	} else {
	    rcvd_time2=tv_ioctl.tv_usec+tv_ioctl.tv_sec*1000000;
	    int rcvd_delta=rcvd_time-rcvd_time2;
	    if(rcvd_delta<0 || rcvd_delta>=1000) printf("rcvd time delta: %d\n",rcvd_delta);
	}
	
	struct ip *ip_hdr     = (struct ip *)buf;
	int hlen = ip_hdr->ip_hl << 2;
#ifdef DEBUG
	printf("Received packet from %s  size=%d (%d) ttl=%d proto=%d  hlen=%d\n", 
    	   inet_ntoa(ip_hdr->ip_src), len,
    	    ntohs(ip_hdr->ip_len), ip_hdr->ip_ttl, ip_hdr->ip_p, hlen);
//        int i; for(i=0;i<len;i++) printf(" %02X",buf[i]); printf("\n");
#endif
	if(ip_hdr->ip_p==IPPROTO_ICMP){
	    struct icmp *icmp_hdr = (struct icmp *)(buf + sizeof(struct ip));
#ifdef DEBUG2
	    printf("ICMP: type=%d code=%d id=%d (%d?) seq=%d\n",
		icmp_hdr->icmp_type, icmp_hdr->icmp_code,
		ntohs(icmp_hdr->icmp_id), id, ntohs(icmp_hdr->icmp_seq));
#endif
	    if(icmp_hdr->icmp_type==0 && ntohs(icmp_hdr->icmp_id)==id
		    && len>=16+sizeof(struct ip)+sizeof(struct icmp)){
		ping_t* p2=(ping_t*)(buf+sizeof(struct ip)+sizeof(struct icmp));
		int pi;
		for(pi=0;pi<ping_num;pi++){
		  ping_t* p=pinglist[pi];
		  if(p->secret==p2->secret && p->ipaddr==p2->ipaddr){
		    int reply_time=rcvd_time2 - p2->time_sent;
		    if(p->time==p2->time) reply_time=rcvd_time2 - p->time_sent; // correction
//		    printf("PING reply! %d us  (sent @ %d)\n",reply_time,p2->time);
		    int off=p2->time - p->time1;
		    if(off<0){
			printf("invalid timestamp!");
		    } else {
			// update DB
			p->data[off % p->datalen]=ln_search(reply_time);
			while(p->logpos<=off){
			    unsigned char x=p->data[p->logpos % p->datalen];
			    if(x==0 && p->logpos>=off-10) break;
			    fwrite(&x,1,1,p->file);
			    ++p->logpos;
			}
			fflush(p->file);
		    }
		  }
		}
//		  else printf("secret: %X != %X || ipaddr: %X != %X\n",p->secret,p2->secret,p->ipaddr,p2->ipaddr);
//		if(verbose>=2) printf("PING reply time %d us   (sent %d)\n",reply_time,time_sent-time_start0);
//		else if(verbose==1){ putchar('*'); fflush(stdout); }
////		break;
	    }
	}
    }
//  }
}

void updatelog(ping_t* p,unsigned int time1){
    int off=time1 - p->time1; // hol kene most tartson a logfile
    while(p->logpos<off-10){  // le van maradva?
	unsigned char x=p->data[p->logpos % p->datalen];
	fwrite(&x,1,1,p->file);
	fflush(p->file);
	++p->logpos;
    }
}

int main(int argc,char* argv[]){

    signal(SIGALRM,SIG_IGN);
    srand(time(NULL));

    int cc;
    for(cc=1;cc<argc;cc++)
	pinglist[ping_num++]=create(argv[cc]);

    int s=socket(AF_INET, SOCK_RAW, IPPROTO_ICMP);
//    char* dev="eth0";
//    if(dev) if(setsockopt(s, SOL_SOCKET, SO_BINDTODEVICE, dev, strlen(dev)+1)) return -1;

//#include <linux/if_packet.h>
//    struct sockaddr_ll sock_address;
//    memset(&sock_address, 0, sizeof(sock_address));
//    sock_address.sll_family = PF_PACKET;
//    sock_address.sll_protocol = htons(ETH_P_ALL);
//    sock_address.sll_ifindex = if_nametoindex("eth0");

#if 0
// BIND to IP address!!!!!!!!!!!
    struct sockaddr_in serveraddr;
    memset(&serveraddr, 0x00, sizeof(struct sockaddr_in));
    serveraddr.sin_family = AF_INET;
//    serveraddr.sin_port = htons(port);
    serveraddr.sin_addr.s_addr = inet_addr("192.168.30.1");
    if (bind(s, (struct sockaddr*) &serveraddr, sizeof(serveraddr)) < 0) {
      perror("bind failed\n");
      close(s);
      return 1;
    }
#endif

#if 1
  /** Set transfer timeouts */
  struct timeval to;
  to.tv_sec = 0;
  to.tv_usec = 100000;
//  if(setsockopt(s, SOL_SOCKET, SO_SNDTIMEO, &to, sizeof(to)) != 0) return -1;
  if(setsockopt(s, SOL_SOCKET, SO_RCVTIMEO, &to, sizeof(to)) != 0) return -1;
#endif

    int id=getpid()&0x7FFF;
    unsigned int time1=time(NULL)+2;
    int pi;
    for(pi=0;pi<ping_num;pi++) ping_open(pinglist[pi],time1);

    while(1){
//	int pi;
	while(time(NULL)<time1){
	    ping(s,3,id);
	    //usleep(100000);
	}
	//printf("*** %lld *** (%d)\n",GetTimerLL(),time1);
	printf("*** %lld ***\n",GetTimerLL()-(long long int)time1*1000000LL);
	for(pi=0;pi<ping_num;pi++){
	    ping1(s,pinglist[pi],id,time1);
//	    ping(s,3,id);//	    usleep(200000);
	    updatelog(pinglist[pi],time1);
	}
	++time1;
//	sleep(1);
    }

}

