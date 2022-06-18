// ./traffmon /sys/class/net/*

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <time.h>
#include <sys/time.h>
#include <math.h>

static unsigned int ln_tab[65536];

static unsigned int ln_search6(unsigned int x){
    if(!x) return 0;

    int min=0;
    if(x>=ln_tab[32768]) min+=32768;
    if(x>=ln_tab[min+16384]) min+=16384;
    if(x>=ln_tab[min+8192]) min+=8192;
    if(x>=ln_tab[min+4096]) min+=4096;
    //
    if(x>=ln_tab[min+2048]) min+=2048;
    if(x>=ln_tab[min+1024]) min+=1024;
    if(x>=ln_tab[min+512]) min+=512;
    if(x>=ln_tab[min+256]) min+=256;
    //
    if(x>=ln_tab[min+128]) min+=128;
    if(x>=ln_tab[min+64]) min+=64;
    if(x>=ln_tab[min+32]) min+=32;
    if(x>=ln_tab[min+16]) min+=16;
    // max-min==16
    if(x>=ln_tab[min+8]) min+=8;
    if(x>=ln_tab[min+4]) min+=4;
    if(x>=ln_tab[min+2]) min+=2;
    if(x>=ln_tab[min+1]) min+=1;
    return min;
}

unsigned int readvalue(char* dev,char* file){
    unsigned char buffer[1024];
    snprintf(buffer,1024,"/sys/class/net/%s%s",dev,file);
    FILE* f=fopen(buffer,"rt");
    if(!f) return 0;
    memset(buffer,0,1024);
    fread(buffer,1024,1,f);
    fclose(f);
    return (unsigned int)strtoll(buffer,NULL,10);
}

// Returns current time in microseconds
unsigned int GetTimer(void){
  struct timeval tv;
  gettimeofday(&tv,NULL);
  return (tv.tv_sec*1000000+tv.tv_usec);
}

typedef struct {
    unsigned int rx;
    unsigned int tx;
    unsigned int time;
    unsigned int step;
    unsigned int time1; // logfile elso bejegyzese
    int logpos; // eddig van kiirva a file
    char* dev;
    FILE* file;
} traff_t;

traff_t* create(char* dev,unsigned int time1){
    traff_t* t=malloc(sizeof(traff_t));
    t->logpos=-1;
    t->dev=dev;
    t->time1=time1;
    t->step=10;
    char fname[100];
    sprintf(fname,"%d-TRAFF2-%s.dat",t->time1,t->dev);
    t->file=fopen(fname,"wb");
    if(!t->file){
	printf("Cannot create logfile: %s\n",fname);
	free(t);
	return NULL;
    }
    fwrite(&t->time1,4,1,t->file);fflush(t->file);
    fwrite(&t->step,4,1,t->file);fflush(t->file);
    return t;
}


void update(traff_t* t,unsigned int time1){
    unsigned int new_rx=readvalue(t->dev,"/statistics/rx_bytes");
    unsigned int new_tx=readvalue(t->dev,"/statistics/tx_bytes");
    unsigned int tim=GetTimer();
    if(t->logpos<0 || time1<t->time1){
	// first entry
	t->rx=new_rx;
	t->tx=new_tx;
	t->time=tim;
	t->logpos=0;
	return;
    }
    // diff!
    unsigned short data[2];
    int dt=tim - t->time;
    if(dt<=9000000 || dt>=15000000){
	data[0]=ln_search6(new_rx-t->rx);
	data[1]=ln_search6(new_tx-t->tx);
    } else {
	// small correction...
	data[0]=ln_search6( ((new_rx-t->rx)*10000000LL)/dt );
	data[1]=ln_search6( ((new_tx-t->tx)*10000000LL)/dt );
    }
    t->rx=new_rx;
    t->tx=new_tx;
    t->time=tim;
    printf("%d  %d/%d      dt=%d\n",time1,data[0],data[1],dt);

    fwrite(data,4,1,t->file);fflush(t->file);

}


traff_t* traff[256];
int traffnum=0;

int main(int argc,char* argv[]){
int i;

for(i=0;i<65536;i++){
    unsigned int x=exp2f(i/(65536.0/22.0)+10)-1024;
//    printf("%3d: %d\n",i,x);
//    printf("%u, ",x); if((i&15)==15) printf("\n");
    ln_tab[i]=x;
}

unsigned int time1=time(NULL);
for(i=1;i<argc;i++) traff[traffnum++]=create(argv[i],time1+2);

while(1){
    int d;
    while( (d=time1-time(NULL))>0 ){
	if(d>1){
	  sleep(d-1);
	} else {
	  struct timeval tv;
	  gettimeofday(&tv,NULL);
	  d=1000000-tv.tv_usec-20000;
	  if(d>200000) d=200000;
	  if(d>1000) usleep(d);
	}
    }
    for(i=0;i<traffnum;i++) update(traff[i],time1);
    time1+=10;
}


}
