
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <math.h>

// open+mmap-hoz:
#include <sys/mman.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>

#include <glob.h>

#include "ln_tab.h"


typedef struct {
    unsigned int time;
    unsigned int endtime;
    int step;
    int len;
    unsigned char* data;
} data_t;


data_t* load_data(char* name,unsigned int t1,unsigned int te){
    int fd=open(name, O_RDONLY);
    if(fd<0) return NULL;
    //
    unsigned int t=0;
    int step=1;
    read(fd, &t, 4);
//    read(fd, &step, 4);
    off_t len=lseek(fd, 0, SEEK_END);
    if(len<=4){
	close(fd);
	return NULL;
    }
    int datalen=len-4;
    if(t>te || t+datalen*step<t1){
	close(fd);
	return NULL;  // ez nem kell
    }
    // map it!
    void* p=mmap(NULL,len,PROT_READ,MAP_PRIVATE,fd,0);
    if(p==MAP_FAILED){
	close(fd);
	return NULL;
    }
    data_t* data=malloc(sizeof(data_t));
    if(!data) return NULL;
    data->time=t;
    data->step=step;
    data->len=datalen;
    data->endtime=data->time+data->len*data->step;
    data->data=p+4;
    return data;
}



int data_num;
data_t** data_list;

int main(){
    int i;

    // PARAMS
    int w=1200;
    int zoom=3000; //30*24*3600/w; // 6 ora => 18x zoom
    unsigned int time1=time(NULL); //-w*zoom;//1357348000;
    char* ip=strdup("8.8.8.8");

// QUERY_STRING=time=12345678&zoom=32
    char* query=getenv("QUERY_STRING");
//    query=strdup("time=1495477755&zoom=3&w=1200&h=256&ip=86.101.142.213@arp");
//    printf("Content-type: text/plain\n\n%s\n",query);
    while(query){
	char* q=strchr(query,'&');
	if(q){ *q=0;++q; }
	if(!strncmp(query,"w=",2)) w=atoi(query+2);
	if(!strncmp(query,"ip=",3)) ip=strdup(query+3);
	if(!strncmp(query,"time=",5)) time1=atoi(query+5);
	if(!strncmp(query,"zoom=",5)) zoom=atoi(query+5);
	query=q;
    }

    time1-=w*zoom;

    glob_t pglob;
    char maszk[128];
    char* node=strchr(ip,'@');
    if(node){
	*node=0;
	snprintf(maszk,128,"/var/pinger/%s/*-PING-%s.dat",node+1,ip);
    } else
	snprintf(maszk,128,"/var/pinger/*-PING-%s.dat",ip);
//    printf("mask='%s'\n",maszk);
    glob(maszk,0,NULL,&pglob);
//    printf("glob->%d  %d\n",ret,pglob.gl_pathc);
    
    data_list=malloc(pglob.gl_pathc*sizeof(data_t*));
    data_num=0;
    for(i=0;i<pglob.gl_pathc;i++){
//	printf("GLOB[%d]: %s\n",i,pglob.gl_pathv[i]);
	data_list[data_num]=load_data(pglob.gl_pathv[i],time1,time1+w*zoom);
	if(data_list[data_num]) ++data_num;
    }

printf("Content-Type: application/json\n\n");
printf("{\"ping\":{\n");

    long long avg=0;
    int avgnum=0;
    int totmin=0;
    int totmax=0;
    int tot0=0;

    int x,z,d;

    for(x=0;x<w;x++){

	int cc=0;
	int cmin=256; //h-1;
	int cmax=0;
	int cnum=0;
	int c0=0;
	
	for(d=0;d<data_num;d++){
	    data_t* data=data_list[d];
	    if(time1+zoom<=data->time || time1>=data->endtime) continue; //SKIP!
	    int t=(time1 - data->time)/data->step;
//	    printf("d=%d time1=%d t=%d len=%d data=%p\n",d,time1,t,data->len,data->data);
	    for(z=0;z<zoom;z+=data->step){
		if(t>=data->len) break;
		if(t>=0){
		    int c=data->data[t];
		    if(c==0) ++c0;
		    else {
			if(c<cmin) cmin=c;
			if(c>cmax) cmax=c;
			cc+=c;++cnum;
		    }
		}
		++t;
	    }
	}

	if(cnum){
	    if(!avgnum || cmin<totmin) totmin=cmin;
	    if(!avgnum || cmax>totmax) totmax=cmax;
	    avg+=cc;avgnum+=cnum;
	}

	tot0+=c0;

        if(c0+cnum>0) printf("\"%d\":[%d,%d,%d,%d],\n",x,cnum ? cc/cnum : -1,cnum ? cmin : 0,cmax,c0 ? ln_search(1000000.0*c0/(c0+cnum)) : 0);
//	if(!cnum) printf("\"%d\":[%d,%d,%d,%d],\n",x,cnum ? cc/cnum : -1,cmin,cmax,c0 ? (int)(1000000.0*c0/(c0+cnum)) : 0);
	
	time1+=zoom;
    }

    printf("\"-1\":[0,0,0,0]}\n");

    if(avgnum){
	char status[256];
	sprintf(status,"PING %s: MIN=%4.2fms MAX=%4.2fms AVG=%5.3fms LOSS=%d/%d (%4.2f%%)",
	    ip,
	    ln_calc(totmin)/1000.0,
	    ln_calc(totmax)/1000.0,
	    ln_calc((avg/(float)avgnum))/1000.0,
	    tot0,tot0+avgnum,100.0*(float)tot0/(float)(avgnum+tot0));
	    printf(",\"pingstat\": \"%s\"\n",status);
    }
    printf("}\n");

    return 0;
}
