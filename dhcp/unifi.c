
#define _GNU_SOURCE

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

#include <pcap.h>
#include "dhcparser.h"

//#include <linux/netfilter_ipv4/ipt_ULOG.h>
//#include <libnetfilter_log/libnetfilter_log.h>



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

// IP packet
void analyze(u_int8_t *packet, u_int32_t len, u_int8_t *hwaddr){
	//printf("---------------- Packet: %d Length: %d ----------------\n",numpkt,len);

        u_int8_t* p=packet+0x34;
	if(!memcmp(p,"POST /inform",12)){
	    unsigned char crlf[]={13,10,13,10};
	    p=memmem(p,len-0x34,crlf,4);
	    if(!p) return;
	    p+=4;
	}

//	hexDump(packet,len);

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

	if (ip->ip_p==IPPROTO_TCP){
	    tcp_hdr_t *tcp=(tcp_hdr_t*)(packet+size_ip);
	    u_int16_t dst_port=ntohs(tcp->dst_port);
	    u_int16_t src_port=ntohs(tcp->src_port);
	    u_int32_t seq=ntohl(tcp->seq);
	    int tcppayloadlen=len-size_ip-4*(tcp->offs>>4);
//	    printf("analyze():  %s->%s  proto=%d  ",ip_src,ip_dst,ip->ip_p);
//	    printf("TCP  %d->%d  %s%s  len=%d  seq=%u  fl=%02X\n",src_port,dst_port,(tcp->flags&TH_SYN)?"SYN":"",(tcp->flags&TH_FIN)?"FIN":"",tcppayloadlen,seq,tcp->flags);
	    if(dst_port!=8080) return;
        } else
            return;

	len-=p-packet;
	if(len<4) return;

//	hexDump(packet,p-packet);
//	hexDump(p,16);
	
	// https://github.com/mcrute/ubntmfi/blob/master/inform_protocol.md
	
	if(memcmp(p,"TNBU",4)) return;
	printf("UBNT v=%d f=%0X mac=%02X:%02X:%02X:%02X:%02X:%02X %s\n",*((int*)(p+4)),*((short*)(p+14)), p[8],p[9],p[10],p[11],p[12],p[13] ,ip_src);
	fflush(stdout);
}


void got_packet(u_char *args, const struct pcap_pkthdr *header, const u_char *p){
//	hexDump(p,header->caplen);
	analyze(p+14,header->caplen-14,p);
}

/* default snap length (maximum bytes per packet to capture) */
#define SNAP_LEN 16384

int main(int argc, char **argv)
{

	char errbuf[PCAP_ERRBUF_SIZE];		/* error buffer */
	pcap_t *handle;				/* packet capture handle */

	char filter_exp[] = "port 8080";	/* filter expression [3] */
	struct bpf_program fp;			/* compiled filter program (expression) */
	bpf_u_int32 mask;			/* subnet mask */
	bpf_u_int32 net;			/* ip */
//	int num_packets = 10;			/* number of packets to capture */
        char* dev=NULL;

	/* check for capture device name on command-line */
	if (argc == 2) {
		dev = argv[1];
	}
	else {
		/* find a capture device if not specified on command-line */
		dev = pcap_lookupdev(errbuf);
		if (dev == NULL) {
			fprintf(stderr, "Couldn't find default device: %s\n",
			    errbuf);
			exit(EXIT_FAILURE);
		}
	}
	
	/* get network number and mask associated with capture device */
	if (pcap_lookupnet(dev, &net, &mask, errbuf) == -1) {
		fprintf(stderr, "Couldn't get netmask for device %s: %s\n",
		    dev, errbuf);
		net = 0;
		mask = 0;
	}

	/* print capture info */
	printf("Device: %s\n", dev);
//	printf("Number of packets: %d\n", num_packets);
	printf("Filter expression: %s\n", filter_exp);

	/* open capture device */
	handle = pcap_open_live(dev, SNAP_LEN, 1, 1000, errbuf);
	if (handle == NULL) {
		fprintf(stderr, "Couldn't open device %s: %s\n", dev, errbuf);
		exit(EXIT_FAILURE);
	}

	/* make sure we're capturing on an Ethernet device [2] */
	if (pcap_datalink(handle) != DLT_EN10MB) {
		fprintf(stderr, "%s is not an Ethernet\n", dev);
		exit(EXIT_FAILURE);
	}

	/* compile the filter expression */
	if (pcap_compile(handle, &fp, filter_exp, 0, net) == -1) {
		fprintf(stderr, "Couldn't parse filter %s: %s\n",
		    filter_exp, pcap_geterr(handle));
		exit(EXIT_FAILURE);
	}

	/* apply the compiled filter */
	if (pcap_setfilter(handle, &fp) == -1) {
		fprintf(stderr, "Couldn't install filter %s: %s\n",
		    filter_exp, pcap_geterr(handle));
		exit(EXIT_FAILURE);
	}

	/* now we can set our callback function */
	pcap_loop(handle, 0, got_packet, NULL);

	/* cleanup */
	pcap_freecode(&fp);
	pcap_close(handle);

	printf("\nCapture complete.\n");

return 0;
}

