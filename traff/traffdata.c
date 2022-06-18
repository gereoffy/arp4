
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

static unsigned int ln_tab2[65536];


typedef struct {
    unsigned short rx;
    unsigned short tx;
} entry_t;

typedef struct {
    unsigned int time;
    unsigned int endtime;
    int step;
    int len;
    int fd;
    entry_t* data;
} data_t;


data_t* load_data(char* name,unsigned int t1,unsigned int te){
    int fd=open(name, O_RDONLY);
    if(fd<0) return NULL;
    //
    unsigned int t=0;
    int step=1;
    read(fd, &t, 4);
    read(fd, &step, 4);
    off_t len=lseek(fd, 0, SEEK_END);
    if(len<=4){
	close(fd);
	return NULL;
    }
    int datalen=(len-8)/sizeof(entry_t);
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
    data->fd=fd;
    data->data=p+8;
    return data;
}

char* format(char* buffer,long long x){
//    int sign=1;
//    if(x<0){ sign=-1;x=-x;}
    char* m=" kMGT????";
    while(x>=1000*1024){
	++m;x=x>>10;
    }
    if(x>=256){
	if(x%1024==0)
	    sprintf(buffer,"%d%c",(int)(x/1024),m[1]);
	else if((1000*x/1024)%100==0)
	    sprintf(buffer,"%3.1f%c",x/1024.0,m[1]);
	else
	    sprintf(buffer,"%3.2f%c",x/1024.0,m[1]);
    } else
	sprintf(buffer,"%d%c",(int)x,m[0]);
    return buffer;
}


int data_num;
data_t** data_list;

int main(){
    int i;

    for(i=0;i<65536;i++){
        unsigned int x=exp2f(i/(65536.0/22.0)+10)-1024;
        ln_tab2[i]=x;
    }

    // PARAMS
    int w=1200;
    int zoom=24*3600/w; // 6 ora => 18x zoom
    int smooth=0;
    int tx=0;
    unsigned int time1=time(NULL);
    char* dev="enp1s0";
    int maxstep=1;

// QUERY_STRING=time=12345678&zoom=32
    char* query=getenv("QUERY_STRING");
//    printf("Content-type: text/plain\n\n%s\n",query);
    while(query){
	char* q=strchr(query,'&');
	if(q){ *q=0;++q; }
	if(!strncmp(query,"w=",2)) w=atoi(query+2);
	if(!strncmp(query,"dev=",3)) dev=strdup(query+4);
	if(!strncmp(query,"time=",5)) time1=atoi(query+5);
	if(!strncmp(query,"zoom=",5)) zoom=atoi(query+5);
	if(!strncmp(query,"smooth=",7)) smooth=atoi(query+7);
	query=q;
    }

    if(smooth<0)
	smooth=(-smooth)*zoom;
    else
	smooth=(smooth-zoom)/2;
    if(smooth<0) smooth=0;

    time1-=w*zoom;

    glob_t pglob;
    char maszk[128];
    char* node=strchr(dev,'@');
    if(node){
	*node=0;
	snprintf(maszk,128,"/var/traffmon/%s/*-TRAFF2-%s.dat",node+1,dev);
    } else
	snprintf(maszk,128,"/var/traffmon/*-TRAFF2-%s.dat",dev);
    glob(maszk,0,NULL,&pglob);
//    printf("glob->%d  %d\n",ret,pglob.gl_pathc);
    
    data_list=malloc(pglob.gl_pathc*sizeof(data_t*));
    data_num=0;
    for(i=0;i<pglob.gl_pathc;i++){
//	printf("GLOB[%d]: %s\n",i,pglob.gl_pathv[i]);
//	    if(time1+zoom+smooth<=data->time || time1-smooth>=data->endtime) continue; //SKIP!
	data_list[data_num]=load_data(pglob.gl_pathv[i],time1-smooth,time1+w*zoom+smooth);
	if(data_list[data_num]){
	    if(data_list[data_num]->step>maxstep) maxstep=data_list[data_num]->step;
	    ++data_num;
	}
    }

//    zoom/=maxstep; if(zoom<1) zoom=1; zoom*=maxstep;
    smooth/=maxstep; smooth*=maxstep;

printf("Content-Type: application/json\n\n");
printf("{\n");

for(tx=0;tx<=1;tx++){

    long long avg=0;
    int avgnum=0;
    int totmin=0;
    int totmax=0;
    int tot0=0;

    int x,z,d;

    printf("%s\": {\n",tx?",\"tx":"\"rx");

    for(x=0;x<w;x++){

	long long cc=0;
	long long ccma=0;
	int cmin=0;
	int cmax=0;
	int cnum=0;
	int cmanum=0;
	int c0=0;
	
	for(d=0;d<data_num;d++){
	    data_t* data=data_list[d];
	    if(time1+zoom+smooth<=data->time || time1-smooth>=data->endtime) continue; //SKIP!
	    int t=(time1 - data->time)/data->step;
//	    printf("d=%d time1=%d t=%d len=%d data=%p\n",d,time1,t,data->len,data->data);

	    t-=smooth/data->step;
	    for(z=0;z<smooth;z+=data->step){
		if(t>=data->len) break;
		if(t>=0){
		    int c=ln_tab2[tx ? data->data[t].tx : data->data[t].rx];
		    ccma+=c;cmanum+=data->step;
		}
		++t;
	    }

	    for(z=0;z<zoom;z+=data->step){
		if(t>=data->len) break;
		if(t>=0){
		    int c=ln_tab2[tx ? data->data[t].tx : data->data[t].rx];
		    if(c==0) c0+=data->step;
		    else {
			int cs=c/data->step;
			if(cmin==0 || cs<cmin) cmin=cs;
			if(cs>cmax) cmax=cs;
			cc+=c;cnum+=data->step;
		    }
		}
		++t;
	    }

	    for(z=0;z<smooth;z+=data->step){
		if(t>=data->len) break;
		if(t>=0){
		    int c=ln_tab2[tx ? data->data[t].tx : data->data[t].rx];
		    ccma+=c;cmanum+=data->step;
		}
		++t;
	    }

	}

	if(c0){
	    tot0+=c0;
	    cnum+=c0;
	}
	if(cnum){
	    if(smooth){
		ccma+=cc;cmanum+=cnum;
	    }
	    printf("\"%d\":[%d,%d,%d,%d],\n",x,(int)(cc/cnum),cmin,cmax,smooth?(int)(ccma/cmanum):0);
	    if(!avgnum || cmin<totmin) totmin=cmin;
	    if(!avgnum || cmax>totmax) totmax=cmax;
	    avg+=cc;avgnum+=cnum;
	};
	
	time1+=zoom;
    }
    time1-=zoom*w;

    printf("\"-1\":[0,0,0,0]}\n");

#if 1
    if(avgnum){
	char status[256];
	char buffer[50];
	char buffer1[50];
	char buffer2[50];
	char buffer3[50];
	sprintf(status,"%s %s: TOTAL=%sbytes MIN=%sbps MAX=%sbps AVG=%sbps LOSS=%d/%d (%4.2f%%)",
	    dev, tx?"TX":"RX",
	    format(buffer,avg),
	    format(buffer1,totmin),
	    format(buffer2,totmax),
	    format(buffer3,avg/avgnum),
	    tot0,avgnum,100.0*(float)tot0/(float)(avgnum));
	printf(",\"%sstat\": \"%s\"\n",tx?"tx":"rx",status);
    }
#endif
//    printf("[-1]]}");

}

printf("}\n");

    return 0;
}
