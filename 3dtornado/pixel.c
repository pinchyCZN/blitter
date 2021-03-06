#include <windows.h>
#include <stdio.h>
#include <math.h>


#define BPP 3
#define BSIZE 32
#define WIDTH (640+BSIZE)
#define HEIGHT (480+BSIZE)
BYTE screen1[WIDTH*HEIGHT*BPP];
BYTE screen2[WIDTH*HEIGHT*BPP];
BYTE screen_out[WIDTH*HEIGHT*BPP];

char shift1=0,shift2=0;

int shift_rand=FALSE;
int switch_shift(){
	shift_rand=!shift_rand;
	return shift_rand;
}
BYTE valueshift[]={
	0,16,8,24,4,20,12,28,2,18,10,26,
	6,22,14,30,1,17,9,25,5,21,13,29,
	3,19,11,27,7,23,15,31
};
int compute_shift()
{
	static int i=0;
	shift2=shift1;
#ifdef _DEBUG
//	shift2=shift1=0;
//	return;
#endif
	if(shift_rand){
		static sign=-1;
		shift1=8*sign;
		sign=-sign;
		//shift1=rand()%BSIZE;
	}
	else
		shift1=valueshift[i];
	i++;
	i&=31;
	return 0;
}


int set_pix(char *dst,int x,int y,int color)
{
	if(x<0 || x>=WIDTH || y<0 || y>=HEIGHT)
		return 0;
	dst[x*BPP+y*WIDTH*BPP]=color;
	if(BPP>1)
		dst[x*BPP+1+y*WIDTH*BPP]=color>>8;
	if(BPP>2)
		dst[x*BPP+2+y*WIDTH*BPP]=color>>16;
	return 0;

}
int move_pixel(unsigned char *src,char *dst,int sx,int sy,int dx,int dy)
{
	int src_good=TRUE;
	int color=0;
	if(sx<0 || sx>=WIDTH || sy<0 || sy>=HEIGHT)
		src_good=FALSE;
	if(dx<0 || dx>=WIDTH || dy<0 || dy>=HEIGHT)
		return 0;
	if(src_good){
		color=src[sx*BPP+sy*WIDTH*BPP];
		if(BPP>1)
			color|=src[sx*BPP+sy*WIDTH*BPP+1]<<8;
		if(BPP>2)
			color|=src[sx*BPP+sy*WIDTH*BPP+2]<<16;
	}
	dst[dx*BPP+dy*WIDTH*BPP]=color;
	if(BPP>1)
		dst[dx*BPP+dy*WIDTH*BPP+1]=color>>8;
	if(BPP>2)
		dst[dx*BPP+dy*WIDTH*BPP+2]=color>>16;
	return 0;
}

int blit(char *src,char *dst,int sx,int sy,int dx,int dy,
		 int w,int h)
{
	int x,y;

	for(x=0;x<w;x++){
		for(y=0;y<h;y++){
			move_pixel(src,dst,sx+x,sy+y,dx+x,dy+y);
		}
	}
#ifndef _DEBUG
	if(FALSE)
#endif
	{
		int show_src=FALSE;
		int show_dst=FALSE;
		show_src=TRUE;
		//show_dst=TRUE;
		for(x=0;x<w;x++){
			if(show_src){
				set_pix(screen_out,sx+x,sy,0xFF0000);
				set_pix(screen_out,sx+x,sy+h,0xFF0000);
			}
			if(show_dst){
				set_pix(screen_out,dx+x,dy,0x7F70);
				set_pix(screen_out,dx+x,dy+h,0x7F70);
			}
		}
		for(y=0;y<h;y++){
			if(show_src){
				set_pix(screen_out,sx,sy+y,0xFF0000);
				set_pix(screen_out,sx+w,sy+y,0xFF0000);
			}
			if(show_dst){
				set_pix(screen_out,dx,dy+y,0x7F70);
				set_pix(screen_out,dx+w,dy+y,0x7F70);
			}
		}
		return 0;
	}
	return 0;
}
//-1,0,0,-1 out
//1,0,0,1 in
//0,-1,-1,0 rotate left no zoom
//0,1,1,0 rotate right no zoom
float direction[4]={-1,-1,-1,-1};
int int_mode=TRUE;
int change_mode()
{
	int_mode=!int_mode;
	return int_mode;
}
static int zpos=0;
int add_z_pos(int z)
{
	zpos+=z;
	printf("zpos=%i\n",zpos);
}

int change_direction(int i)
{
	switch(i){
	case 0:
		add_z_pos(-zpos);
		break;
	case 1:
		add_z_pos(1);
		break;
	case 2:
		add_z_pos(-1);
		break;
	}
	return 0;
}
int centerx=8; //WIDTH/(BSIZE*3); //WIDTH/2;
int centery=8; //HEIGHT/(BSIZE*2); //HEIGHT/2; 
int move_center(int i)
{
	switch(i){
	default:
	case 1:
		centerx--;
		break;
	case 2:
		centerx++;
		break;
	case 3:
		centery--;
		break;
	case 4:
		centery++;
		break;
	}
	return 0;
}
static int swap=TRUE;
int swap_buffer(char **src,char **dst)
{
	if(swap){
		*src=screen1;
		*dst=screen2;
	}
	else{
		*src=screen2;
		*dst=screen1;
	}
	swap=!swap;
	return swap;
}
int get_buffer()
{
	if(!swap)
		return screen1;
	else
		return screen2;
}
int init_t(int x,int y,char *buf)
{
	static int a=FALSE;
	int i,j;
	int w=384;
	int h=288;
	char str[20];
	FILE *f=0;

	f=fopen("untitled.bmp","rb");
	//f=fopen("PICT0022.bmp","rb");
	if(f!=0){
		int origw,origh;
		w=h=0;
		memset(buf,0,WIDTH*HEIGHT*BPP);
		fseek(f,0x12,SEEK_SET);
		fread(&w,1,4,f);
		fread(&h,1,4,f);
		origw=w;
		origh=h;
		if(h>HEIGHT)
			h=HEIGHT;
		if(w>WIDTH)
			w=WIDTH;

		fseek(f,0x36,SEEK_SET);
		for(i=0;i<h;i++){
			fread(buf+i*WIDTH*BPP,1,w*BPP,f);
			fseek(f,0x36+origw*3*i,SEEK_SET);
		}
		fclose(f);
		return;
	}
	memset(buf,0,WIDTH*HEIGHT*BPP);
	for(i=0;i<HEIGHT;i+=16){
		int k=0;
		int color;
		for(j=0;j<WIDTH;j++){
			if(k==0){
				k=rand()%30;
				color=((rand()%255)<<16)|((rand()%255)<<8)|(rand()%255);
			}
			set_pix(buf,j+x%16,i+y%16,color);
			k--;
		}
	}
	for(i=0;i<HEIGHT;i++){
		int k=0;
		int color;
		for(j=0;j<WIDTH;j+=1){
			if(k==0){
				k=rand()%30;
				color=((rand()%255)<<16)|((rand()%255)<<8)|(rand()%255);
			}
			set_pix(buf,j+x%16,i+y%16,color);
			k--;
		}
	}

	return 0;
}
int clear_screen()
{
	memset(screen1,0,sizeof(screen1));
	memset(screen2,0,sizeof(screen2));
	return 0;
}
int offsetx=-5;
int offsety=-36;
int set_offsetx(int i)
{
	offsetx+=i;
	return offsetx;
}
int set_offsety(int i)
{
	offsety+=i;
	return offsety;
}
/*

-5,-36 CCW
28,-4 CW
*/
int create_seed()
{
	char *p1,*p2;
	int i,j,color;
	p1=screen1;
	p2=screen2;
	/*
	if(direction[1]<0){
		offsetx=-5;
		offsety=-36;
	}
	else{
		offsetx=28;
		offsety=-4;
	}
	*/
	for(i=0;i<10;i+=2){
		for(j=0;j<10;j+=2){
			color=(((rand()%255) )<<16)|(((rand()%255) )<<8)|((rand()%255) );
			//set_pix(p1,i+(centerx+offsetx)*BSIZE+shift1,j+(centery+offsety)*BSIZE+shift1,color);
			//set_pix(p2,i+(centerx+offsetx)*BSIZE+shift1,j+(centery+offsety)*BSIZE+shift1,color);
			set_pix(p1,i+centerx*BSIZE+offsetx+shift1,j+centery*BSIZE+offsety+shift1,color);
			set_pix(p2,i+centerx*BSIZE+offsetx+shift1,j+centery*BSIZE+offsety+shift1,color);
		}
	}
	return 0;
}
int handle_click(int x,int y)
{
	offsetx=(x-640/2)/3;
	offsety=(480/2-y)/3;
	create_seed();
	return 0;
}
int get_xpos(int x,int y,int k)
{
	if(int_mode)
		return x+(int)direction[0]*(x/BSIZE-centery)-(int)direction[1]*(y/BSIZE-centery)+k;
	else
		return x+direction[0]*(x/BSIZE-centery)-direction[1]*(y/BSIZE-centery)+k;
}
int get_ypos(int x,int y,int k)
{
	if(int_mode)
		return y+(int)direction[2]*(x/BSIZE-centerx)+(int)direction[3]*(y/BSIZE-centerx)+k;
	else
		return y+direction[2]*(x/BSIZE-centerx)+direction[3]*(y/BSIZE-centerx)+k;
}
int tornado(char *buf,int init)
{

	static int p=0;
	int i,k,x,y;
	char *src,*dst;

	swap_buffer(&src,&dst);

#ifdef _DEBUG
	memset(screen_out,0,sizeof(screen1));
#endif

	if(init){
		shift2=shift1=0;
		k=0;
//		blit(dst,buf,k,k,0,0,WIDTH,HEIGHT);
		return 0;
		memset(src,0,sizeof(screen1));
		init_t(0,0,src);
	}

	/*
	compute_shift();
	k=shift2-shift1;
	for(y=0;y<HEIGHT;y+=BSIZE){
		i=0;
		for(x=0;x<WIDTH;x+=BSIZE){
			blit(src,dst,
				//x+direction[0]*(x/BSIZE-6)-direction[1]*(y/BSIZE-7)+k,y+direction[2]*(x/BSIZE-6)+direction[3]*(y/BSIZE-6)+k,
				//x+direction[0]*(x/BSIZE-centery)-direction[1]*(y/BSIZE-centery)+k,
				//y+direction[2]*(x/BSIZE-centerx)+direction[3]*(y/BSIZE-centerx)+k,
				get_xpos(x,y,k),get_ypos(x,y,k),
				x,y,BSIZE,BSIZE);
		}
	}
	*/
	printf("%i ",k);
	k=shift1;
	i=31;
	copy_to_main(dst,buf,k,k);
//	blit(dst,buf,k,k,0,0,640,480);
	//blit(dst,buf,0,0,0,0,WIDTH,HEIGHT);
	//memcpy(buf,dst,WIDTH*HEIGHT*BPP);

	return 0;
}
int move_screen(int x,int y)
{
	char *src,*dst;
	swap_buffer(&src,&dst);
	blit(src,dst,x,y,0,0,WIDTH,HEIGHT);
	return 0;
}
int copy_to_main(char *src,char *dst,int x,int y)
{
	int i,w,h;
	int srcx=0;

	w=WIDTH-x;
	if(w>640)
		w=640;
	h=HEIGHT-y;
	if(h>480)
		h=480;
	else if(h<0)
		h=0;

	for(i=0;i<h;i++){
		int j;
		memcpy(dst+i*640*3,src+i*WIDTH*BPP+x*BPP+y*WIDTH*BPP,w*BPP);
#ifdef _DEBUG
		for(j=0;j<w*BPP;j++){
			if(screen_out[j+i*WIDTH*BPP+x*BPP+y*WIDTH*BPP]!=0)
				dst[i*640*3+j]=screen_out[j+i*WIDTH*BPP+x*BPP+y*WIDTH*BPP];
		}
#endif
	}
	return 0;
}
int save_image(char *buf)
{
	static int count=1;
	FILE *f;
	char str[255];
	sprintf(str,"PIC%08i.BMP",count++);
	f=fopen(str,"wb");
	if(f!=0){
		extern char bitmapfile[];
		fwrite(bitmapfile,1,0x36,f);
		fwrite(buf,1,WIDTH*HEIGHT*BPP,f);
		fclose(f);
	}
}
/*
cx=1
cy=1
cz=cos(*rz/(57.2957795));

sx=0
sy=0
sz=sin(*rz/(57.2957795));

tx=(*x)*cz+(*y)*sz;
ty=(*x)*(-sz)+(*y)*(cz);
tz=(*z);
*/
static int rotate_3d(float *x,float *y,float *z,float *rx,float *ry,float *rz)
{
	float cx,cy,cz,sx,sy,sz;
	float tx,ty,tz;
#define RID 57.2957795
	cx=cos(*rx/(RID));
	cy=cos(*ry/(RID));
	cz=cos(*rz/(RID));
	sx=sin(*rx/(RID));
	sy=sin(*ry/(RID));
	sz=sin(*rz/(RID));
	tx=(*x)*cy*cz+(*y)*cy*sz-(*z)*sy;
	ty=(*x)*(sx*sy*cz-cx*sz)+(*y)*(sx*sy*sz+cx*cz)+(*z)*sx*cy;
	tz=(*x)*(cx*sy*cz+sx*sz)+(*y)*(cx*sy*sz-sx*cz)+(*z)*cx*cy;
	*x=tx;
	*y=ty;
	*z=tz;
	return TRUE;
}

static int stars[3*3];

int do_3d(char *buffer,float *rx,float *ry,float *rz)
{
	static int init=TRUE;
	int i=0,j=0,k=0;
	if(init){
		for (i=0;i<sizeof(stars)/(3*sizeof(int));i++){
			stars[i*3]=-50+rand()%100;
			stars[i*3+1]=-50+rand()%100;
			stars[i*3+2]=-50+rand()%100;
		}
		init=FALSE;
	}
	for (i=0;i<sizeof(stars)/(3*sizeof(int));i++){
		float x,y,z;
		unsigned char c;
		float _c;
		x=stars[i*3];
		y=stars[i*3+1];
		z=stars[i*3+2]+zpos;
		//x=10;
		//y=10;
		//z=10;

		rotate_3d(&x,&y,&z,rx,ry,rz);
		
		_c=255-z*10;
		if(_c>255)
			_c=255;
		else if(_c<0)
			_c=0;
		c=_c;
		set_3dpixel(buffer,&x,&y,&z,c,c,c);

	}

	/*
	for(i=0;i<20;i++){
		for(j=0;j<20;j++)
		{
			for(k=0;k<20;k++)
			{
				float x,y,z;
				x=i*10;
				y=j*10;
				z=k*10+zpos;
				rotate_3d(&x,&y,&z,rx,ry,rz);
				set_3dpixel(buffer,&x,&y,&z,0x55,0x55,0x55);
			}
		}
	}
	*/
}

int tube()
{
	static char PIXBUF[320*240*4];
	int i[3]={100,99,8};
	static int TEXUV[320*240*4];
	static char screenbuf[320*240*4];
	static char pal[320*240*4];
	static char texture[0x10000*2];
	int pindex=0;
	char *buffer;
	buffer=get_buffer();
#define EYE -2
#define SCREEN 160

	sprintf(TEXUV,"baze");

	__asm{


	xor	ecx,ecx
PAL1:	mov	edx,3C8h
	mov	ax,cx
	lea edi,pal
	push eax
	and ax,0xFF
	mov bl,3
	mul bl
	xor ebx,ebx
	mov	bx,ax
	add edi,ebx
	pop eax
//	out	dx,al
	inc	edx
	sar	al,1
	js	PAL2
//	out	dx,al
	mov [edi],al
	inc edi
	mul	al
	shr	ax,6
//	out	dx,al
	mov [edi],al
	inc edi

PAL2:	mov	al,0
//	out	dx,al
	mov [edi],al
	inc edi

	jns	PAL3
	sub	al,cl
	shr	al,1
//	out	dx,al
	mov [edi],al
	inc edi

	shr	al,1
//	out	dx,al
	mov [edi],al
	inc edi

PAL3:	mov	ebx,ecx
//	mov	[fs:bx],bl
	dec cx
	jnz PAL1

	lea esi,texture
TEX:	mov	bx,cx
	add	ax,cx
	rol	ax,cl
	mov	dh,al
	sar	dh,5
	adc	dl,dh

	and ebx,0xFFFF
	adc dl,[esi+ebx+255]
//	adc	dl,[fs:bx+255]
	shr	dl,1
//	mov	[fs:bx],dl
	and ebx,0xFFFF;
	mov [esi+ebx],dl
	not	bh
	and ebx,0xFFFF
	mov [esi+ebx],dl
//	mov	[fs:bx],dl
	dec cx
	jnz TEX

	fninit
	fldz

MAIN:
	lea	edi,TEXUV-4
	fadd	dword ptr[edi]
	lea	edi,PIXBUF
	push	edi


	mov	edx,-80
TUBEY:	mov	ebp,-160
TUBEX:	
	lea	esi,TEXUV
	fild	word ptr[esi+EYE]


	mov	[esi],ebp
	fild	[esi]
	mov	[esi],edx
	fild	[esi]

	mov	ecx,2
ROTATE:	
	fld	st(3)
	fsincos
	fld	st(2)
	fmul	st(0),st(1)
	fld	st(4)

	fmul	st(0),st(3)
	fsubp	st(1),st(0)
	fxch	st(3)
	fmulp	st(2),st(0)
	fmulp	st(3),st(0)
	faddp	st(2),st(0)
	fxch	st(2)
	loop	ROTATE

	fld	st(1)
	fmul	st(0),st(0)
	fld	st(1)
	fmul	st(0),st(0)
	faddp	st(1),st(0)
	fsqrt
	fdivp	st(3),st(0)
	fpatan
	fimul	word ptr[esi-4]
	fistp	word ptr[esi]
	fimul	word ptr[esi-4]
	fistp	word ptr[esi+1]
	mov	esi,[esi]

	lea	eax,[ebx+esi]
	add	al,ah
	and	al,64
	mov	al,-5
	jz	STORE

	shl	esi,2
	lea	eax,[ebx+esi]
	sub	al,ah
	mov	al,-16
	jns	STORE

	shl	esi,1
	mov	al,-48
STORE:	add	al,[ebx+esi]
	add	[edi],al
	inc	edi

	inc	ebp
	cmp	ebp,160

	jnz	TUBEX

	inc	edx
	cmp	edx,80
	jnz	TUBEY

	lea esi,TEXUV
	lea edi,screenbuf
	add	edi,(100-SCREEN/2)*320
	mov	ecx,(SCREEN/2)*320/256
	rep	movsw

	mov	ecx,SCREEN*320/256
BLUR:	dec	esi
	sar	byte ptr[esi],2
	loop	BLUR


EXIT:
	};

}