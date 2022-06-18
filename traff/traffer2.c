
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


#include <png.h>
#include <glob.h>

#include "ln_tab.h"

static unsigned int ln_tab2[65536];

static int save_png_to_file(int width,int height,unsigned char* pixels, FILE * fp){
    png_structp png_ptr = NULL;
    png_infop info_ptr = NULL;
    png_colorp palette;
    size_t y;
    png_byte ** row_pointers = NULL;
    /* "status" contains the return value of this function. At first
       it is set to a value which means 'failure'. When the routine
       has finished its work, it is set to a value which means
       'success'. */
    int status = -1;
    /* The following number is set by trial and error only. I cannot
       see where it it is documented in the libpng manual.
    */
    int pixel_size = 1;
    int depth = 8;
    
    png_ptr = png_create_write_struct (PNG_LIBPNG_VER_STRING, NULL, NULL, NULL);
    if (png_ptr == NULL) {
        goto png_create_write_struct_failed;
    }
    
    info_ptr = png_create_info_struct (png_ptr);
    if (info_ptr == NULL) {
        goto png_create_info_struct_failed;
    }
    
    /* Set up error handling. */

    if (setjmp (png_jmpbuf (png_ptr))) {
        goto png_failure;
    }
    
    /* Set image attributes. */

    png_set_IHDR (png_ptr,
                  info_ptr,
                  width,
                  height,
                  depth,
                  PNG_COLOR_TYPE_PALETTE,
                  PNG_INTERLACE_NONE,
                  PNG_COMPRESSION_TYPE_BASE,
                  PNG_FILTER_TYPE_BASE);

   /* set the palette if there is one.  REQUIRED for indexed-color images */
   palette = (png_colorp)png_malloc(png_ptr, PNG_MAX_PALETTE_LENGTH * sizeof (png_color));
   int c;
   for(c=0;c<16;c++){
	palette[c].red  =(c>>3)*63 + (c&1)*192;
	palette[c].green=(c>>3)*63 + (c&2)*96;
	palette[c].blue =(c>>3)*63 + (c&4)*48;
   }
   png_set_PLTE(png_ptr, info_ptr, palette, PNG_MAX_PALETTE_LENGTH);
             
    
    /* Initialize rows of PNG. */

    row_pointers = png_malloc (png_ptr, height * sizeof (png_byte *));
    for (y = 0; y < height; ++y) {
        png_byte *row = pixels + y*width*pixel_size;
        row_pointers[y] = row;
    }
    
    /* Write the image data to "fp". */

    png_init_io (png_ptr, fp);
    png_set_rows (png_ptr, info_ptr, row_pointers);
    png_write_png (png_ptr, info_ptr, PNG_TRANSFORM_IDENTITY, NULL);

    png_free (png_ptr, row_pointers);

    /* The routine has successfully written the file, so we set
       "status" to a value which indicates success. */

    status = 0;
    
 png_failure:
 png_create_info_struct_failed:
    png_destroy_write_struct (&png_ptr, &info_ptr);
 png_create_write_struct_failed:
// fopen_failed:
    return status;
}


//unsigned char font[8192];
//int font_h=16;
#include "font.h"

void font_writech(int c,int color,int bgcolor,unsigned char* p,int w){
    int x,y;
    if(c<32 || c>127) return;
    c-=32;
    for(y=0;y<font_h;y++){
	unsigned char m=font[font_h*c+y];
	for(x=0;x<9;x++){
	    if(m&128) p[x]=color;
	    else if(bgcolor>=0) p[x]=bgcolor;
	    m<<=1;
	}
	p+=w;
    }
}

void font_write(char* str,int color,int bgcolor,unsigned char* p,int w){
    while(*str){
	font_writech(*str,color,bgcolor,p,w);
	p+=9;++str;
    }
}

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
//    void* mmap;
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
//    data->mmap=p;
    data->data=p+8;
    return data;
}

char* format(char* buffer,long long x){
    int sign=1;
    if(x<0){ sign=-1;x=-x;}
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
//    printf("%3d: %d\n",i,x);
//    printf("%u, ",x); if((i&15)==15) printf("\n");
    ln_tab2[i]=x;
}

    // PARAMS
    int w=1200;
    int h=512;
    int zoom=18; //24*3600/w; // 6 ora => 18x zoom
    int smooth=0;
    int scale=-1;  // negativ: logaritmikus
    int tx=0;
    unsigned int time1=time(NULL)-w*zoom;
    char* dev="eth1";
    int maxstep=1;

// QUERY_STRING=time=12345678&zoom=32
    char* query=getenv("QUERY_STRING");
//    printf("Content-type: text/plain\n\n%s\n",query);
    while(query){
	char* q=strchr(query,'&');
	if(q){ *q=0;++q; }
	if(!strncmp(query,"w=",2)) w=atoi(query+2);
	if(!strncmp(query,"h=",2)) h=atoi(query+2);
	if(!strncmp(query,"dev=",3)) dev=strdup(query+4);
	if(!strncmp(query,"time=",5)) time1=atoi(query+5);
	if(!strncmp(query,"zoom=",5)) zoom=atoi(query+5);
	if(!strncmp(query,"scale=",6)) scale=atoi(query+6);
	if(!strncmp(query,"smooth=",7)) smooth=atoi(query+7);
	if(!strncmp(query,"tx",2)) tx=1;
	query=q;
    }
//    printf("%d x %d   time1=%d zoom=%d\n",w,h,time1,zoom); return 0;
    unsigned int time0=time(NULL)-time1;
    float hscale=h/1024.0;
    if(smooth<0)
	smooth=(-smooth)*zoom;
    else
	smooth=(smooth-zoom)/2;
    if(smooth<0) smooth=0;


inline int transform(int x){
    if(scale<0)
	x=ln_search(x/(-scale))*4*hscale;
    else
	x=(x/scale)*hscale;   // hscale 1024-rol meretez kepernyore, tehat 1024 magas a skala
    if(x<0) x=0;
    if(x>=h) x=h-1;
    return x;
}


    glob_t pglob;
    char maszk[128];
    char* node=strchr(dev,'@');
    if(node){
	*node=0;
	snprintf(maszk,128,"/var/traffmon/%s/*-TRAFF2-%s.dat",node+1,dev);
    } else
	snprintf(maszk,128,"/var/traffmon/*-TRAFF2-%s.dat",dev);
    int ret=glob(maszk,0,NULL,&pglob);
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

//    data_t* data=load_data("1357387074-PING-91.82.126.82.dat");
//    data_t* data=load_data("1357380799-PING-195.228.219.23.dat");
//    printf("data: %d\n",data->len);
    
//    FILE* f=fopen("/var/pinger/VGA-ROM.F16","rb");
//    fread(font,8192,1,f);
//    fclose(f);


    unsigned char* pixels=malloc(w*h);
    memset(pixels,15,w*h);
    int x,y,z,d;



if(scale<0){
    // log scale
    int y1=100*(-scale),y2;
    while(y1<=10000000*(-scale)){
	for(y2=1;y2<4;y2++){
	    y=(y1/(-scale))*((y2<3)?y2:5);
	    y=h-1-hscale*4*ln_search(y);
	    if(y<1) y=1;
	    for(x=0;x<w;x++) pixels[w*y+x]&=~8;
	    if(y2==1){
		--y;
		for(x=0;x<w;x++) pixels[w*y+x]&=~8;
		if(y>h/3) y-=14;
		switch(y1){
		case 100: font_write("100byte/s",7,-1,pixels+w*y+x,w); break;
		case 1000: font_write("1kbyte/s",7,-1,pixels+w*y+x,w); break;
		case 10000: font_write("10kb/s",7,-1,pixels+w*y+x,w); break;
		case 100000: font_write("100kb/s",7,-1,pixels+w*y+x,w); break;
		case 1000000: font_write("1Mb/s",7,-1,pixels+w*y+x,w); break;
		case 10000000: font_write("10Mb/s",7,-1,pixels+w*y+x,w); break;
		case 100000000: font_write("100Mb/s",7,-1,pixels+w*y+x,w); break;
		case 1000000000: font_write("1Gb/s",7,-1,pixels+w*y+x,w); break;
		}
	    }
    	}
	y1*=10;
    }
} else {
#if 1
    // linear scale
	int y2;
	for(y2=0;y2<=20;y2++){
	    y=h-1-y2*h/20;
	    if(y<1) y=1;
	    for(x=0;x<w;x++) pixels[w*y+x]&=~8;
	    if(y2%5==0){
		--y;
		for(x=0;x<w;x++) pixels[w*y+x]&=~8;
		char temp[64];
		int bps=(y2*1024/20)*scale;
#if 1
		format(temp,bps);
#else
		if(bps>=1024*1024){
		    bps/=1024;
		    if(bps%1024==0)
			sprintf(temp,"%dMbps",bps/1024);
		    else
			sprintf(temp,"%3.2fMbps",bps/1024.0);
		}
		else if(bps>=1024){
		    if(bps%1024==0)
			sprintf(temp,"%dkbps",bps/1024);
		    else
			sprintf(temp,"%3.2fkbps",bps/1024.0);
		}
		else
		    sprintf(temp,"%dbps",bps);
#endif
		if(y>=h/3) y-=14;
		font_write(temp,7,-1,pixels+w*y+x,w);
	    }
	}
#endif
}



    long long avg=0;
    int avgnum=0;
    int totmin=0;
    int totmax=0;
    int tot0=0;
    int lastcma=-1;

    for(x=0;x<w;x++){
	unsigned char* p=pixels+x+(h-1)*w;

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
//	    for(y=0;y<h;y++) p[-y*w]=(c0>1)?1:(1+8);
	    tot0+=c0;
	    cnum+=c0;
	}
	if(cnum){
	    int c=transform(cmax);// ha cnum==1 akkor c==cmax
	    if(cnum>1){
		for(y=transform(cmin);y<=c;y++)
		    p[-y*w]=2+8;
		c=transform(cc/cnum);
	    }
	    p[-c*w]=0;
	    if(c+1<h) p[-c*w-w]=0;
	    //
	    if(smooth){
		ccma+=cc;cmanum+=cnum;
		c=transform(ccma/cmanum);
		if(lastcma>=0){
		    y=transform(lastcma);
		    if(y<c)
			for(;y<=c;y++) p[-y*w]=9;
		    else
			for(;y>=c;y--) p[-y*w]=9;
		} else
		    p[-c*w]=9;
//		if(c+1<h) p[-c*w-w]=9;
		lastcma=ccma/cmanum;
	    }
	    if(!avgnum || cmin<totmin) totmin=cmin;
	    if(!avgnum || cmax>totmax) totmax=cmax;
	    avg+=cc;avgnum+=cnum;
	} else lastcma=-1;
	
	time1+=zoom;
    }


    // idotengely    
    x=time0/zoom;
//    x=w/3;
//    printf("Content-type: text/plain\n\n"); printf("x=%d  t=%d\n",x,time1);
    if(x>=0 && x+1<w)
    {
	unsigned char* p=pixels+x;
	for(y=0;y<h;y++){
	    p[0]=0;
	    p[1]=0;
	    p+=w;
	}
    }

    if(avgnum){
	char status[256];
	char buffer[50];
	char buffer1[50];
	char buffer2[50];
	char buffer3[50];
	sprintf(status,"%s: TOTAL=%sbytes MIN=%sbps MAX=%sbps AVG=%sbps LOSS=%d/%d (%4.2f%%)",
	    tx?"TX":"RX",
	    format(buffer,avg),
	    format(buffer1,totmin),
	    format(buffer2,totmax),
	    format(buffer3,avg/avgnum),
	    tot0,avgnum,100.0*(float)tot0/(float)(avgnum));
	font_write(status,4,15,pixels+(w-strlen(status)*9)/2,w);
    }
    
    
    printf("Content-Type: image/png\n\n");
    save_png_to_file(w,h,pixels,stdout);
    
    return 0;
}
