
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
//    data->fd=fd;
//    data->mmap=p;
    data->data=p+4;
    return data;
}



int data_num;
data_t** data_list;

int main(){
    int i;

    // PARAMS
    int w=1200;
    int h=512;
    int zoom=24*3600/w; // 6 ora => 18x zoom
    unsigned int time1=1357348000;
    char* ip=strdup("192.190.173.40@dell-fw.ganteline.hu");

// QUERY_STRING=time=12345678&zoom=32
    char* query=getenv("QUERY_STRING");
//    query=strdup("time=1495477755&zoom=3&w=1200&h=256&ip=86.101.142.213@arp");
//    printf("Content-type: text/plain\n\n%s\n",query);
    while(query){
	char* q=strchr(query,'&');
	if(q){ *q=0;++q; }
	if(!strncmp(query,"w=",2)) w=atoi(query+2);
	if(!strncmp(query,"h=",2)) h=atoi(query+2);
	if(!strncmp(query,"ip=",3)) ip=strdup(query+3);
	if(!strncmp(query,"time=",5)) time1=atoi(query+5);
	if(!strncmp(query,"zoom=",5)) zoom=atoi(query+5);
	query=q;
    }
//    printf("%d x %d   time1=%d zoom=%d\n",w,h,time1,zoom); return 0;
    unsigned int time0=time(NULL)-time1;
    float hscale=h/256.0;
//    int hh=h+128;

    glob_t pglob;
    char maszk[128];
    char* node=strchr(ip,'@');
    if(node){
	*node=0;
	snprintf(maszk,128,"/var/pinger/%s/*-PING-%s.dat",node+1,ip);
    } else
	snprintf(maszk,128,"/var/pinger/*-PING-%s.dat",ip);
//    printf("mask='%s'\n",maszk);
    int ret=glob(maszk,0,NULL,&pglob);
//    printf("glob->%d  %d\n",ret,pglob.gl_pathc);
    
    data_list=malloc(pglob.gl_pathc*sizeof(data_t*));
    data_num=0;
    for(i=0;i<pglob.gl_pathc;i++){
//	printf("GLOB[%d]: %s\n",i,pglob.gl_pathv[i]);
	data_list[data_num]=load_data(pglob.gl_pathv[i],time1,time1+w*zoom);
	if(data_list[data_num]) ++data_num;
    }

//    data_t* data=load_data("1357387074-PING-91.82.126.82.dat");
//    data_t* data=load_data("1357380799-PING-195.228.219.23.dat");
//    printf("data: %d\n",data->len);
    
//    FILE* f=fopen("/var/pinger/VGA-ROM.F16","rb");
//    fread(font,8192,1,f);
//    fclose(f);


    unsigned char* pixels=malloc(w*h);
    memset(pixels,15,w*h);
//    memset(pixels+w*h,7,w*28);
    int x,y,z,d;



    int y1=100,y2;
    while(y1<10000000){
	for(y2=1;y2<4;y2++){
	    y=y1*((y2<3)?y2:5);  //  1/2/5
	    y=h-1-hscale*ln_search(y);
	    for(x=0;x<w;x++) pixels[w*y+x]&=~8;
	    if(y2==1){
		--y;
		for(x=0;x<w;x++) pixels[w*y+x]&=~8;
		y-=14;
		switch(y1){
		case 100: if(h>128) font_write("100us",8,-1,pixels+w*y+x,w); break;
		case 1000: font_write("1ms",8,-1,pixels+w*y+x,w); font_write("0.1%",1,-1,pixels+w*y+x+w-48,w); break;
		case 10000: font_write("10ms",8,-1,pixels+w*y+x,w); font_write("  1%",1,-1,pixels+w*y+x+w-48,w); break;
		case 100000: font_write("100ms",8,-1,pixels+w*y+x,w); font_write(" 10%",1,-1,pixels+w*y+x+w-48,w); break;
		case 1000000: font_write("1sec",8,-1,pixels+w*y+x,w); font_write("100%",1,-1,pixels+w*y+x+w-48,w); break;
		}
	    }
    	}
	y1*=10;
    }







    long long avg=0;
    int avgnum=0;
    int totmin=0;
    int totmax=0;
    int tot0=0;

    for(x=0;x<w;x++){
	unsigned char* p=pixels+x+(h-1)*w;

	int cc=0;
	int cmin=h-1;
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
	    int c=hscale*cmax;// ha cnum==1 akkor c==cmax
	    if(cnum>1){
		for(y=hscale*cmin;y<=c;y++)
		    p[-y*w]=2+8;
		c=(hscale*cc)/cnum;
	    }
	    p[-c*w]=0;
	    if((c+1)<h) p[-c*w-w]=0;
	    //
	    if(!avgnum || cmin<totmin) totmin=cmin;
	    if(!avgnum || cmax>totmax) totmax=cmax;
	    avg+=cc;avgnum+=cnum;
	}

	if(c0){
	    int yh=hscale*ln_search(1000000*c0/(c0+cnum));
	    if(yh<0) yh=0;
	    if(yh>h) yh=h;
	    for(y=0;y<yh;y++)
		p[-y*w]=(c0>1)?1:(1+8);
	    tot0+=c0;
	}


	
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
	sprintf(status,"MIN=%4.2fms MAX=%4.2fms AVG=%5.3fms LOSS=%d/%d (%4.2f%%)",
	    ln_calc(totmin)/1000.0,
	    ln_calc(totmax)/1000.0,
	    ln_calc((avg/(float)avgnum))/1000.0,
	    tot0,tot0+avgnum,100.0*(float)tot0/(float)(avgnum+tot0));
	font_write(status,4,15,pixels+(w-strlen(status)*9)/2,w);
    }
    
    
    printf("Content-Type: image/png\n\n");
    save_png_to_file(w,h,pixels,stdout);
    
    return 0;
}
