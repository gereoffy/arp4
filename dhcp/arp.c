#include <time.h>
#include <pcap.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <ctype.h>
#include <errno.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>

/* default snap length (maximum bytes per packet to capture) */
#define SNAP_LEN 1518

/* ethernet headers are always exactly 14 bytes [1] */
#define SIZE_ETHERNET 14

/* Ethernet addresses are 6 bytes */
#define ETHER_ADDR_LEN	6


typedef struct hash_s {
	struct hash_s* next;
	int cnt,len;
	time_t time;
	unsigned short vlan;
	unsigned char data[];
} hash_t;

#define HASH_SIZE 1024
static hash_t* hash_table[HASH_SIZE];
static char *dev = NULL;			/* capture device name */

static int hash_add(int cnt,int vlan,int len,const unsigned char* data){
	unsigned int crc=0;
	int i;
	for(i=0;i<len;i++) crc^=(crc<<3)^data[i];
	crc&=HASH_SIZE-1;
	hash_t* p=hash_table[crc];
//	printf("CRC=0x%X  p=%p\n",crc,p);
	while(p){
//	  printf("  p=%p  len=%d cnt=%d\n",p,p->len,p->cnt);
	  if(p->len==len)
	    if(memcmp(p->data,data,len)==0){
	    	// megvan!
	    	p->cnt+=cnt;
	    	p->vlan=vlan; //update
	    	time_t old=p->time;
	    	p->time=time(NULL);
	    	return old;
	    }
	  p=p->next;
	}
	// nincs meg
	p=malloc(sizeof(hash_t)+len);
	memcpy(p->data,data,len);
	p->len=len; p->cnt=cnt; p->vlan=vlan;
	p->time=time(NULL);
	p->next=hash_table[crc];hash_table[crc]=p;
	return 0;
}

static time_t hash_list(FILE* f){
    int crc;
    int cnt=0;
    for(crc=0;crc<HASH_SIZE;crc++){
	hash_t* p=hash_table[crc];
	while(p){
//	  printf("CRC=0x%X  p=%p  len=%d cnt=%d\n",crc,p,p->len,p->cnt);
//	  fprintf(f,"%7d (%d) %.*s",p->cnt,p->len,p->len,p->data);
//	  fprintf(f,"%7d %.*s",p->cnt,p->len,p->data);
//	  fprintf(f,"DUMP %7d %u %02X:%02X:%02X:%02X:%02X:%02X %d.%d.%d.%d\n",p->cnt,(unsigned int)(p->time),
//	    p->data[0],p->data[1],p->data[2],p->data[3],p->data[4],p->data[5],
//	    p->data[6],p->data[7],p->data[8],p->data[9]);
	  char dev2[16];
	  sprintf(dev2,"%s.%d",dev,p->vlan);
	  if(p->time>time(NULL)-900) // csak ha lattuk az elmult 15 percben
	  fprintf(f,"%d,%u,%d.%d.%d.%d,%02X:%02X:%02X:%02X:%02X:%02X,%s\n",
	    p->cnt,(unsigned int)(p->time),
	    p->data[6],p->data[7],p->data[8],p->data[9],
	    p->data[0],p->data[1],p->data[2],p->data[3],p->data[4],p->data[5],
	    p->vlan ? dev2 : dev);
	  cnt+=p->cnt;
	  p=p->next;
	}
    }
    return cnt;
}

static time_t dumptime=0;

void got_packet(u_char *args, const struct pcap_pkthdr *header, const u_char *p)
{
// 0                 6                  12           16
//EC EB B8 AF FD 40  68 05 CA 2B CF 77  81 00 00 62  08 06 00 01 08 00 06 04 00 01 68 05 CA 2B CF 77 AC 12 62 FC 00 00 00 00???
//68 05 CA 2B CF 77  EC EB B8 AF FD 40  81 00 00 62  08 06 00 01 08 00 06 04 00 02 EC EB B8 AF FD 40 AC 12 62 F0 68 05 CA 2B???
//FF FF FF FF FF FF  00 18 AE 5C C0 DD  81 00 00 0A  08 06 00 01 08 00 06 04 00 01 00 18 AE 5C C0 DD 00 00 00 00 00 00 00 00???
//FF FF FF FF FF FF  00 57 47 62 7D 70  81 00 00 62  08 06 00 01 08 00 06 04 00 01 00 57 47 62 7D 70 AC 12 62 A2 00 00 00 00???
//FF FF FF FF FF FF  00 18 AE 81 CB A5  81 00 00 0A  08 06 00 01 08 00 06 04 00 01 00 18 AE 81 CB A5 00 00 00 00 00 00 00 00???
//24 A4 3C DE 96 A7  00 30 48 F9 EF C6               08 06 00 01 08 00 06 04|00 01|00 30 48 F9 EF C6|0A 41 00 01|00 00 00 00 00 00|0A 41 00 66
	int vlan=0;
	unsigned char hdr[]={8,6,0,1,8,0,6,4};
	if(memcmp(p+12,hdr,8)){
//	    int i;
//	    for(i=0;i<40;i++) printf(" %02X",p[i]);
//	    printf("???\n");
	    if(p[12]==0x81 && p[13]==0){
		vlan=(p[14]&15)*256+p[15];
		p+=4; // skip vlan tag
		if(memcmp(p+12,hdr,8)) return; // vlan de megse jo
	    } else
		return; // invalid format
	}

//	for(i=0;i<6+6+2;i++) printf(" %02X",packet[i]);
//	for(i=0;i<header->caplen;i++) printf(" %02X",packet[i]);
//	printf("\n");fflush(stdout);
// 0                 6                 12    14    16    18    20    22    24    26    28    30    32                38
// 24 A4 3C DE 96 A7 00 30 48 F9 EF C6 08 06 00 01 08 00 06 04|00 01|00 30 48 F9 EF C6|0A 41 00 01|00 00 00 00 00 00|0A 41 00 66
	if(p[28]==0 || p[28]==255 || p[22]==0xFF) return; // missing IP/MAC
	time_t t=hash_add(1,vlan,6+4,p+22);
	if(!t) printf("NEW!!! "); else if(t<(time(NULL)-300)) printf("Return! ");
	printf("%d Vlan%d ARP[%02X%02X] %02X:%02X:%02X:%02X:%02X:%02X-%d.%d.%d.%d -> %02X:%02X:%02X:%02X:%02X:%02X-%d.%d.%d.%d\n",
	    (int)(t ? (time(NULL)-t) : 0), vlan,
	    p[20],p[21],
	    p[22],p[23],p[24],p[25],p[26],p[27],  p[28],p[29],p[30],p[31],
	    p[32],p[33],p[34],p[35],p[36],p[37],  p[38],p[39],p[40],p[41]);
	if(!t || dumptime+300<time(NULL)){
//	    hash_list(stdout);
    	  	FILE* f=fopen("arptablec.new","wb");
    	  	hash_list(f);
    	  	fclose(f);
		rename("arptablec.new","arptablec.dump");
	    dumptime=time(NULL);
	}
}

int main(int argc, char **argv)
{

	char errbuf[PCAP_ERRBUF_SIZE];		/* error buffer */
	pcap_t *handle;				/* packet capture handle */

	char filter_exp[] = "arp";		/* filter expression [3] */
	struct bpf_program fp;			/* compiled filter program (expression) */
	bpf_u_int32 mask;			/* subnet mask */
	bpf_u_int32 net;			/* ip */
//	int num_packets = 10;			/* number of packets to capture */

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

