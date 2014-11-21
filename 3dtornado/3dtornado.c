#define _WIN32_WINNT 0x400
#include <windows.h>
#include <commctrl.h>
#include <stdio.h>
#include <string.h>
#include <conio.h>
#include <fcntl.h>
#include <math.h>
#include <GL/gl.h>
#include <GL/glu.h>
#include "resource.h"

HWND	hWindow;
HINSTANCE	ghInstance;


int scale=256;
float gx=0,gy=0,gz=-100;
float grx=0,gry=0,grz=0;
float rx=0,ry=0,rz=0;

DWORD tick=0;
int screen_updated=TRUE;
#define BUF_WIDTH 640
#define BUF_HEIGHT 480
int BUF_SIZE=BUF_WIDTH*BUF_HEIGHT*3;

int stretch=0;
BYTE *buffer=0;
BYTE *bufA,*bufB;
#define SIZE_MATRIX 56
int bwidth=SIZE_MATRIX,bheight=SIZE_MATRIX,bdepth=SIZE_MATRIX;
int swap=0;
int frame_step=0;
int jitter=0;

#define TIME1 tick=GetTickCount();
#define TIME2 debug_printf("time=%u\n",GetTickCount()-tick);
#define ZOOM_IN_KEY 0xDD
#define ZOOM_OUT_KEY 0xDB
#define RGBMODE_KEY_UP '1'
#define RGBMODE_KEY_DOWN '2'

void debug_printf(char *fmt,...)
{
	va_list ap;
	char s[255];
	va_start(ap,fmt);
	_vsnprintf(s,sizeof(s),fmt,ap);
	OutputDebugString(s);
}
int move_console(int x,int y,int w,int h)
{
	char title[MAX_PATH]={0}; 
	HWND hcon; 
	GetConsoleTitle(title,sizeof(title));
	if(title[0]!=0){
		hcon=FindWindow(NULL,title);
		SetWindowPos(hcon,0,x,y,w,h,SWP_NOZORDER);
	}
	return 0;
}
void open_console()
{
	BYTE Title[200]; 
	BYTE ClassName[200]; 
	LPTSTR  lpClassName=ClassName; 
	HANDLE hConWnd; 
	FILE *hf;
	static BYTE consolecreated=FALSE;
	static int hCrt=0;
	
	if(consolecreated==TRUE)
	{

		GetConsoleTitle(Title,sizeof(Title));
		hConWnd=FindWindow(NULL,Title);
		GetClassName(hConWnd,lpClassName,120);
		ShowWindow(hConWnd,SW_SHOW);
		SetForegroundWindow(hConWnd);
		hConWnd=GetStdHandle(STD_INPUT_HANDLE);
		FlushConsoleInputBuffer(hConWnd);
		return;
	}
	AllocConsole(); 
	hCrt=_open_osfhandle((long)GetStdHandle(STD_OUTPUT_HANDLE),_O_TEXT);

	fflush(stdin);
	hf=_fdopen(hCrt,"w"); 
	*stdout=*hf; 
	setvbuf(stdout,NULL,_IONBF,0);

	GetConsoleTitle(Title,sizeof(Title));
	hConWnd=FindWindow(NULL,Title);
	GetClassName(hConWnd,lpClassName,120);
	ShowWindow(hConWnd,SW_SHOW); 
	SetForegroundWindow(hConWnd);
	consolecreated=TRUE;
}
int adjust_windows(HWND hwnd)
{
	RECT rect={0},desk={0};
	GetWindowRect(hwnd,&rect);
	GetClientRect(GetDesktopWindow(),&desk);
	move_console(rect.left,rect.bottom,rect.right-rect.left+20,desk.bottom-(rect.bottom-rect.top)-GetSystemMetrics(SM_CYCAPTION)*2);
	SetFocus(hwnd);
	
}



#define GRIPPIE_SQUARE_SIZE 15
static HANDLE grippy;
static HWND grip_hwnd=0;

int create_grippy(HWND hwnd)
{
	RECT client_rect;
	GetClientRect(hwnd,&client_rect);
	
	grip_hwnd=CreateWindow("Scrollbar",NULL,WS_CHILD|WS_VISIBLE|SBS_SIZEGRIP,
		client_rect.right-GRIPPIE_SQUARE_SIZE,
		client_rect.bottom-GRIPPIE_SQUARE_SIZE,
		GRIPPIE_SQUARE_SIZE,GRIPPIE_SQUARE_SIZE,
		hwnd,NULL,NULL,NULL);

	return 0;
}

int grippy_move(HWND hwnd)
{
	RECT client_rect;
	GetClientRect(hwnd,&client_rect);
	if(grip_hwnd!=0)
	{
		SetWindowPos(grip_hwnd,NULL,
			client_rect.right-GRIPPIE_SQUARE_SIZE,
			client_rect.bottom-GRIPPIE_SQUARE_SIZE,
			GRIPPIE_SQUARE_SIZE,GRIPPIE_SQUARE_SIZE,
			SWP_NOZORDER|SWP_SHOWWINDOW);
	}
	return 0;
}


int set_pixel(BYTE *buf,int x,int y,BYTE R,BYTE G,BYTE B)
{
	int offset;
	if(x>=BUF_WIDTH)
		return 0;
	if(y>=BUF_HEIGHT)
		return 0;
	if(x<0)
		return 0;
	if(y<0)
		return 0;
	//offset=x*3+BUF_SIZE-BUF_WIDTH*3-y*BUF_WIDTH*3;
	offset=x*3+y*BUF_WIDTH*3;
	if(offset<0)
		return 0;
	if((offset+2)>=BUF_SIZE)
		return 0;
	else{
		buf[offset]=R;
		buf[offset+1]=G;
		buf[offset+2]=B;
	}
	return 0;
}

int set_3dpixel(BYTE *buf,float *x,float *y,float *z,BYTE R,BYTE G,BYTE B)
{
	float x1=0,y1=0,z1;
	z1=*z;
	z1+=100;
	if(z1>0){
		x1=(*x)*scale/(z1);
		y1=(*y)*scale/(z1);
		x1+=BUF_WIDTH/2;
		y1+=BUF_HEIGHT/2;
		{
			int i,j,max;
			max=floor(sqrt(abs(z1)));
			if(max<=0)
				max=1;
			for(i=0;i<max;i++)
				for(j=0;j<max;j++)
					set_pixel(buf,x1+i,y1+j,R,G,B);
		}
	}
	return 0;
}
int move_point(BYTE src,BYTE dst,int x1,int y1,int z1,int x2,int y2,int z2)
{
}
int blit_3d(char *src,char *dst,int x,int y,int z,int dx,int dy,int dz,int w,int h,int d,int size)
{
	int i,j,k;
	for(i=0;i<w;i++){
		for(j=0;j<h;j++){
			for(k=0;k<d;k++){
				int t=0;
				if((x+i)>=0 && (x+i)<size && (y+j)>=0 && (y+j)<size && (z+k)>=0 && (z+k)<size)
					t=src[x+i+(y+j)*size+(z+k)*size*size];
				else
					t=t;
				if((dx+i)>=0 && (dx+i)<size && (dy+j)>=0 && (dy+j)<size && (dz+k)>=0 && (dz+k)<size)
					dst[dx+i+(dy+j)*size+(dz+k)*size*size]=t;
				else
					t=t;
			}
		}
	}
}
int get3d_dst(int *x,int *y,int *z,int shift,int size)
{
	int cx,cy,cz;
	int nx=*x,ny=*y,nz=*z;
	cx=size/2;
	cy=size/2;
	cz=size/2;
	if(*x>=cx && *y<cy)
		nx--;
	if(*x<cx && *y>=cy)
		nx++;
	if(*y<cy && *x<cx)
		ny++;
	if(*y>=cy && *x>=cx)
		ny--;
	//if(*z<cz)
	//	(*z)--;
	//else
	//	(*z)++;
	*x=nx;
	*y=ny;
	*z=nz;
	(*x)+=shift;
	(*y)+=shift;
	(*z)+=shift;
}
int do_3d_tornado(BYTE *src,BYTE *dst,int size)
{
	int x,y,z;
	int bsize=size/2;
	jitter=(rand()%17)-8;
	//jitter=0;
	printf("jitter=%i\n",jitter);
	for(x=0;x<size;x+=bsize){
		for(y=0;y<size;y+=bsize){
			for(z=0;z<size;z+=bsize){
				int dx=x,dy=y,dz=z;
				int w,h,d;
				w=h=d=bsize;
				get3d_dst(&x,&y,&z,0,size);
				blit_3d(src,dst,x+jitter,y+jitter,z+jitter,dx+jitter,dy+jitter,dz+jitter,w,h,d,size);
			}
		}
	}
	memset(src,0,size*size*size);

}
int print_text(char *str,char *buf,int x,int y)
{
	extern char bitmapfonts[];
	int i,j,k;
	int R,G,B;
	BYTE a;
	for(i=0;i<100;i++){
		a=str[i];
		if(a==0)
			break;
		for(j=0;j<8;j++){
			for(k=0;k<12;k++){
				if(bitmapfonts[a*12+k]&(1<<(7-j)))
					R=G=B=0;
				else
					R=G=B=0xFF;
				set_pixel(buf,x+j+i*8,y+k,R,G,B);
			}
		}
	}
	return 0;
}

void display_help(HWND hwnd)
{
	MessageBox(hwnd,
		"tab=animate\r\n"
		"space=one frame\r\n"
		"left click=restart\n"
		"right click=switch directions\r\n"
		"0-9,cursor,home keys\r\n",
		"HELP",MB_OK);
}

int update_title(HWND hwnd)
{
	extern int centerx,centery,shift_rand,int_mode;
	extern int offsetx,offsety;
	extern float direction[];
	char str[255];
	memset(str,0,sizeof(str));
	sprintf(str,"scale=%i",scale);
	SetWindowText(hwnd,str);
	return 0;
}
int print_globs()
{
	printf("%3.2f %3.2f %3.2f , %3.2f %3.2f %3.2f\n",grx,gry,grz,gx,gy,gz);
}
LRESULT CALLBACK win_view1_proc(HWND hwnd,UINT msg,WPARAM wparam,LPARAM lparam)
{
	static POINT mpoint;
	switch(msg){
	case WM_CREATE:
		return 0;
	case WM_MBUTTONDOWN:
	case WM_RBUTTONDOWN:
	case WM_LBUTTONDOWN:
		mpoint.x=LOWORD(lparam);
		mpoint.y=HIWORD(lparam);
		SetFocus(hwnd);
		print_globs();
		break;
	case WM_MOUSEWHEEL:
		{
			short delta=HIWORD(wparam);
			int mod=LOWORD(wparam);
			int scale=1;
			if(mod&(MK_RBUTTON|MK_CONTROL))
				scale=10;
			else if(mod&MK_SHIFT)
				scale=100;
			delta/=120;
			delta*=scale;
			gz+=delta;
			print_globs();
		}
		break;
	case WM_MOUSEMOVE:
		{
			POINT p;
			int deltax,deltay;
			int mod=LOWORD(wparam);
			p.x=LOWORD(lparam);
			p.y=HIWORD(lparam);
			deltax=p.x-mpoint.x;
			deltay=p.y-mpoint.y;

			if(wparam&MK_LBUTTON){
				gry-=deltax;
				grx-=deltay;
				print_globs();
			}
			if(wparam&MK_RBUTTON){
				float scale=4;
				if(mod&MK_SHIFT)
					scale=.99;
				else if(mod&MK_CONTROL)
					scale=8;

				gx+=(float)deltax/scale;
				gy-=(float)deltay/scale;
				print_globs();
			}
			mpoint=p;
		}
		break;
	}
	return DefWindowProc(hwnd,msg,wparam,lparam);
}
GLfloat ambient[]={1.0,1.0,1.0,0.0};
GLfloat light_position[]={-5.0,-10.0,10.0,0.0};
GLfloat white_light[]={1.0,1.0,1.0,1.0};
void gl_init(void)
{
//	return;
	glClearColor(0.0,0.0,0.0,0.0);
//	glShadeModel(GL_FLAT);
//	glShadeModel(GL_SMOOTH);
	glLightfv(GL_LIGHT0,GL_POSITION,light_position);
	glLightfv(GL_LIGHT0,GL_DIFFUSE,white_light);
	glLightfv(GL_LIGHT0,GL_SPECULAR,white_light);
	glLightfv(GL_LIGHT0,GL_AMBIENT,ambient);
    glMatrixMode(GL_PROJECTION);
    glFrustum(-0.08, 0.08F, -0.06F, 0.06F, 0.1F, 1000.0F);

	glEnable(GL_LIGHTING);
	glEnable(GL_LIGHT0);
	glEnable(GL_DEPTH_TEST);
//		glLightfv(GL_LIGHT0,GL_POSITION,light_position);

/*
    glMatrixMode(GL_PROJECTION);
    glFrustum(-0.5F, 0.5F, -0.5F, 0.5F, 1.0F, 3.0F);

    glMatrixMode(GL_MODELVIEW);
    glTranslatef(0.0F, 0.0F, -2.0F);

    glRotatef(30.0F, 1.0F, 0.0F, 0.0F);
    glRotatef(30.0F, 0.0F, 1.0F, 0.0F);

    glEnable(GL_DEPTH_TEST);
    glEnable(GL_LIGHTING);
    glEnable(GL_LIGHT0);
*/
}
int setupPixelFormat(HDC hDC)
{
    PIXELFORMATDESCRIPTOR pfd;
    int pixelFormat;
	memset(&pfd,0,sizeof(pfd));
    pfd.nSize = sizeof(pfd);
    pfd.nVersion = 1;
    pfd.dwFlags = PFD_DRAW_TO_WINDOW | PFD_SUPPORT_OPENGL | PFD_DOUBLEBUFFER;
    pfd.iPixelType = PFD_TYPE_RGBA;
    pfd.cColorBits = 24;
    pfd.cDepthBits = 16;
    pfd.iLayerType = PFD_MAIN_PLANE;

    pixelFormat = ChoosePixelFormat(hDC, &pfd);
    if (pixelFormat == 0) {
        MessageBox(WindowFromDC(hDC), "ChoosePixelFormat failed.", "Error",
                MB_ICONERROR | MB_OK);
        exit(1);
    }

    if (SetPixelFormat(hDC, pixelFormat, &pfd) != TRUE) {
        MessageBox(WindowFromDC(hDC), "SetPixelFormat failed.", "Error",
                MB_ICONERROR | MB_OK);
        exit(1);
    }
}
int create_view_windows(HWND hwnd,HWND *hview)
{
    WNDCLASS wnd;
	RECT rect={0};
	memset(&wnd,0,sizeof(wnd));
	wnd.style=CS_OWNDC;
	wnd.cbClsExtra=0;
	wnd.cbWndExtra=0;
	wnd.hInstance=ghInstance;
	wnd.hIcon=LoadIcon(NULL,IDI_APPLICATION);
	wnd.hCursor=LoadCursor(NULL,IDC_ARROW);
	wnd.hbrBackground=GetStockObject(BLACK_BRUSH);
	wnd.lpszMenuName=NULL;
	wnd.lpszClassName="VIEW1";
	wnd.lpfnWndProc=win_view1_proc;
	if(hview){
		HWND hv;
		RegisterClass(&wnd);
		GetClientRect(hwnd,&rect);
		hv=CreateWindow(wnd.lpszClassName,"view1",WS_CHILD|WS_VISIBLE,
			0,0,0,0,hwnd,NULL,ghInstance,NULL);
		*hview=hv;
	}
}
int init_ogl(HWND hwnd,HGLRC *hglrc,HDC *hdc)
{
	if(hwnd && hglrc && hdc){
		HDC hDC;
		HGLRC hGLRC;
		hDC=GetDC(hwnd);
		if(hDC){
			setupPixelFormat(hDC);
			hGLRC=wglCreateContext(hDC);
			if(hGLRC){
				wglMakeCurrent(hDC,hGLRC);
				if(hglrc)
					*hglrc=hGLRC;
				gl_init();
			}
			//SelectObject(hDC,GetStockObject(SYSTEM_FONT));
			//wglUseFontBitmaps(hDC,0,255,1000);
			//ReleaseDC(hv,hDC);
			if(hdc)
				*hdc=hDC;
		}
	}

}
void perspectiveGL( GLdouble fovY, GLdouble aspect, GLdouble zNear, GLdouble zFar )
{
	const GLdouble pi = 3.1415926535897932384626433832795;
	GLdouble fW, fH;
	fH = tan( fovY / 360 * pi ) * zNear;
	fW = fH * aspect;
	glFrustum( -fW, fW, -fH, fH, zNear, zFar );
}
void reshape(int w, int h)
{
	glPushMatrix();
	glMatrixMode(GL_PROJECTION);
	glLoadIdentity();
	perspectiveGL(25.0,(GLfloat)w/(GLfloat)h,.1,10000.0);
	glMatrixMode(GL_MODELVIEW);
	glLoadIdentity();
	glViewport(0,0,(GLsizei)w,(GLsizei)h);
	glPopMatrix();
}
int render_rect(float *rot,float *trans)
{
	static float theta=0;
	int i;
	unsigned char indices[] = { 0, 1, 2, 0, 2, 3 };
	float vertices[] = { 
		0, 0, 0,
		1, 0, 0,
		1, 1, 0,
		0, 1, 0
	};
	float normals[] = { 
		0, 0, 1,
		0, 0, 1,
		0, 0, 1,
		0, 0, 1
	};
	for(i=0;i<4;i++){
		//vertices[ 3 * i + 0 ] *= size[0];
		//vertices[ 3 * i + 1 ] *= size[1];
	}
	glMatrixMode(GL_MODELVIEW);
	glPushMatrix();
	glTranslatef(trans[0], trans[1], trans[2]);
	glRotatef(rot[0], 1.0f, 0.0f, 0.0f);
	glRotatef(rot[1], 0.0f, 1.0f, 0.0f);
	glRotatef(rot[2], 0.0f, 0.0f, 1.0f);
	//glRotatef(theta, 0.0f, 1.0f, 0.0f);
	glEnableClientState(GL_VERTEX_ARRAY);
	glEnableClientState(GL_NORMAL_ARRAY);
	glDisable(GL_TEXTURE_2D);
	glVertexPointer(3,GL_FLOAT,0,vertices);
	glNormalPointer(GL_FLOAT,0,normals);
	glDrawElements(GL_TRIANGLES,6,GL_UNSIGNED_BYTE,indices);
	glMatrixMode(GL_MODELVIEW);
	glPopMatrix();
	theta+=1;

}


int render_cube()
{
	float rot[]={0,0,0};
	float trans[]={0,0,0};
	static theta=0;

	trans[0]=-.5;
	trans[1]=-.5;
	trans[2]=.5;
	render_rect(rot,trans); //front

	trans[0]=-.5;
	trans[1]=.5;
	trans[2]=-.5;
	rot[0]=180;
	render_rect(rot,trans); //back
	theta++;

	trans[0]=-.5;
	trans[1]=.5;
	trans[2]=.5;
	rot[0]=-90;
	render_rect(rot,trans); //top

	trans[0]=-.5;
	trans[1]=-.5;
	trans[2]=-.5;
	rot[0]=90;
	render_rect(rot,trans); //bottom

	trans[0]=.5;
	trans[1]=-.5;
	trans[2]=.5;
	rot[0]=0;
	rot[1]=90;
	render_rect(rot,trans); //right

	trans[0]=-.5;
	trans[1]=-.5;
	trans[2]=-.5;
	rot[0]=0;
	rot[1]=-90;
	render_rect(rot,trans); //left
}
int do_matrix()
{
	int i,j,k;
	float cx,cy,cz;
	cx=-bwidth/2;
	cy=-bheight/2;
	cz=-bdepth/2;
	for(i=0;i<bwidth;i++){
		for(j=0;j<bheight;j++){
			for(k=0;k<bdepth;k++){
				if(buffer[i+j*bwidth+(k*bwidth*bheight)]){
					int move=0;
					glPushMatrix();
					glTranslatef(cx+i+move,cy+j+move,cz+k+move);
					render_cube();
					glPopMatrix();
				}
			}
		}
	}
}
int do_triangle()
{
	static float theta=0;
	glClearColor(0.0f, 0.0f, 0.0f, 0.0f);
	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
//	glColor3f(1.0,0.0,0.0);

	glMatrixMode(GL_MODELVIEW);
	glPushMatrix();
//	glMatrixMode(GL_PROJECTION);
	
	glLoadIdentity();
	glTranslatef(gx,gy,gz);
	glRotatef(grx,1,0,0);
	glRotatef(gry,0,1,0);
	glRotatef(grz,0,0,1);


	glRotatef(theta, 0.0f, 1.0f, 1.0f);
//	render_cube();
	do_matrix();
	if(0){
	glBegin(GL_TRIANGLES);

		glColor3f(1.0f, 0.0f, 0.0f);   glVertex2f(0.0f,   1.0f);
		glColor3f(0.0f, 1.0f, 0.0f);   glVertex2f(0.87f,  -0.5f);
		glColor3f(0.0f, 0.0f, 1.0f);   glVertex2f(-0.87f, -0.5f);

	glEnd();
	}

	glPopMatrix();

//	theta += 1.0f;
	Sleep (1);
}
int display_view1(HWND hwnd,HGLRC hglrc)
{
	HDC hdc;
	static int slow=0;
	if(hwnd==0 || hglrc==0)
		return FALSE;
	hdc=GetDC(hwnd);
	if(hdc){
		wglMakeCurrent(hdc,hglrc);
		if(swap)
			buffer=bufB;
		else
			buffer=bufA;
		do_triangle();
		SwapBuffers(hdc);
		slow++;
		//if(slow>4)
		{
			slow=0;
			if(frame_step){
				do_3d_tornado(buffer,swap?bufA:bufB,SIZE_MATRIX);
				swap=!swap;
				frame_step=0;
			}
		}
	}
}
int resize_view(HWND hwnd,HWND hview)
{
	RECT rect={0};
	GetClientRect(hwnd,&rect);
	reshape(rect.right,rect.bottom);
	return MoveWindow(hview,0,0,rect.right,rect.bottom,FALSE);
}
int rand_fill(unsigned char *buf,unsigned int size)
{
	int i;
	srand(GetTickCount());
	memset(buf,0,size);
	for(i=0;i<size;i++){
		if(rand()&1)
			buf[i]=1;
	}
}
LRESULT CALLBACK MainDlg(HWND hwnd,UINT message,WPARAM wparam,LPARAM lparam)
{
	PAINTSTRUCT ps;
	BITMAPINFO bmi;
	char str[255];
	static int timer=FALSE;
	static int xpos=0,ypos=0,LMB=FALSE,dx=0,dy=0;
	static HWND hview=0;
	static HGLRC hglrc=0;
	static HDC hdc=0;
	int i;

#ifdef _DEBUG
//	if(message!=0x200&&message!=0x84&&message!=0x20&&message!=WM_ENTERIDLE)
//		debug_printf("message=%08X wparam=%08X lparam=%08X\n",message,wparam,lparam);
#endif	
	switch(message)
	{
	case WM_INITDIALOG:
		bufA=malloc(bwidth*bheight*bdepth);
		bufB=malloc(bwidth*bheight*bdepth);
		if(bufA==0 || bufB==0)
			MessageBox(hwnd,"malloc failed","error",MB_OK);
		else{
			memset(bufB,0,bwidth*bheight*bdepth);
			rand_fill(bufA,bwidth*bheight*bdepth);
		}
		create_grippy(hwnd);
		BringWindowToTop(hwnd);
		BringWindowToTop(hwnd);
		update_title(hwnd);
		SendMessage(hwnd,WM_KEYDOWN,VK_TAB,0);
		SendMessage(hwnd,WM_LBUTTONDOWN,0,0);
		create_view_windows(hwnd,&hview);
		init_ogl(hview,&hglrc,&hdc);
		resize_view(hwnd,hview);
		break;
	case WM_SIZE:
		{
			int w,h;
			w=LOWORD(lparam);
			h=HIWORD(lparam);
			grippy_move(hwnd);
			resize_view(hwnd,hview);
			reshape(w,h);
		}
		break;
	case WM_TIMER:
		if(LMB)
			handle_click(xpos,ypos);
		display_view1(hview,hglrc);
		break;
	case WM_COMMAND:

		switch(LOWORD(wparam))
		{
		case IDC_ONTOP:
			SetWindowPos(hwnd,IsDlgButtonChecked(hwnd,LOWORD(wparam))? HWND_TOPMOST : HWND_NOTOPMOST, 0, 0, 0, 0, SWP_NOMOVE | SWP_NOSIZE );
			break;
		case WM_DESTROY:
		#ifndef _DEBUG
			if(MessageBox(hwnd,"Sure you want to quit?","QUIT",MB_OKCANCEL)!=IDOK)
				break;
		#endif
			PostQuitMessage(0);
			break;
		}
		break;
	case WM_PAINT:
	/*
		hdc=BeginPaint(hwnd,&ps);
		memset(&bmi,0,sizeof(BITMAPINFO));
		bmi.bmiHeader.biBitCount=24;
		bmi.bmiHeader.biWidth=BUF_WIDTH;
		bmi.bmiHeader.biHeight=BUF_HEIGHT;
		bmi.bmiHeader.biPlanes=1;
		bmi.bmiHeader.biSize=40;
		if(stretch)
			StretchDIBits(hdc,0,0,client_rect.right,client_rect.bottom,0,0,BUF_WIDTH,BUF_HEIGHT,buffer,&bmi,DIB_RGB_COLORS,SRCCOPY);
		else
			SetDIBitsToDevice(hdc,0,0,BUF_WIDTH,BUF_HEIGHT,0,0,0,BUF_WIDTH,buffer,&bmi,DIB_RGB_COLORS);
		screen_updated=TRUE;
		EndPaint(hwnd,&ps);
		*/
		break;
	case WM_CLOSE:
	case WM_QUIT:
		PostQuitMessage(0);
		break;
	case WM_DROPFILES:
		break;
	case WM_RBUTTONDOWN:
	case WM_LBUTTONDOWN:
	case WM_MBUTTONDOWN:
		xpos=LOWORD(lparam);
		ypos=HIWORD(lparam);
		break;
	case WM_MOUSEMOVE:
		{
			int x,y;
			int key=wparam;
			x=LOWORD(lparam);
			y=HIWORD(lparam);
			if(key&MK_LBUTTON){
				x=xpos-x;
				ry=x;
				y=ypos-y;
				rx=y;
				printf("rz=%.1f ry=%.1f\n",rx,ry);
			}
			if(key&MK_RBUTTON){
				x=xpos-x;
				rz=-x;
				printf("z=%.1f\n",rz);
			}
			update_title(hwnd);
		}
		break;
	case WM_MOUSEWHEEL:
		if(wparam&0x80000000)
			SendMessage(hwnd,WM_KEYDOWN,VK_NEXT,0);
		else
			SendMessage(hwnd,WM_KEYDOWN,VK_PRIOR,0);
		break;
	case WM_KEYUP:
		break;
	case WM_KEYDOWN:
		{
			int ctrl=GetKeyState(VK_CONTROL)&0x8000;
			int shift=GetKeyState(VK_SHIFT)&0x8000;

#ifdef _DEBUG
//		debug_printf("message=%08X wparam=%08X lparam=%08X\n",message,wparam,lparam);
#endif
		switch(wparam)
		{
		case VK_INSERT:
			clear_screen();
			break;
		case 0xBD:
		case 0xBB:
			break;
		case 'Q':
			rx=ry=rz=0;
			printf("angles reset\n");
			break;
		case 'W':
			break;
		case 0xC0:
			rand_fill(swap?bufB:bufA,bwidth*bheight*bdepth);
			break;
		case 'K':
			set_offsety(1);
			break;
		case 'L':
			set_offsety(-1);
			break;
		case 'X':
			if(shift)
				scale+=10;
			else
				scale++;
			printf("scale=%i\n",scale);
			break;
		case 'Z':
			if(shift)
				scale-=10;
			else
				scale--;
			printf("scale=%i\n",scale);
			break;
		//case 0xC0:
			change_direction(0);
			rx=ry=rz=0;
			break;
		case '0':
		case '1':
			//tube();
			frame_step=1;
			break;
		case '2':
		case '3':
		case '4':
		case '5':
		case '6':
		case '7':
		case '8':
		case '9':
			change_direction(wparam-'0');
			break;

		case VK_SPACE:
			break;
		case VK_TAB:
			if(timer)
				KillTimer(hwnd,1);
			else
				SetTimer(hwnd,1,60,NULL);
			timer=!timer;
			break;
		case VK_F1:
			display_help(hwnd);
			break;
/*		case VK_F2:
			i=CreateDialog(ghInstance,MAKEINTRESOURCE(IDD_DIALOG2),hwnd,request_value);
			ShowWindow(i,SW_SHOWNORMAL);
			debug_printf("return =%i\r\n",i);
			break;*/
		case VK_F5:
			break;
		case VK_F9:
			stretch=!stretch;
			break;
		case 0xDEADBEEF:
			break;
		case VK_DOWN:
			if(GetKeyState(VK_CONTROL)&0x8000)
				move_center(3);
			else if(GetKeyState(VK_SHIFT)&0x8000)
				change_direction(12);
			else
				change_direction(2);
			break;
		case VK_UP:
			if(GetKeyState(VK_CONTROL)&0x8000)
				move_center(4);
			else if(GetKeyState(VK_SHIFT)&0x8000)
				change_direction(11);
			else
				change_direction(1);
			break;
		case VK_LEFT:
			if(GetKeyState(VK_CONTROL)&0x8000)
				move_center(1);
			else if(GetKeyState(VK_SHIFT)&0x8000)
				change_direction(14);
			else
				change_direction(4);
			break;
		case VK_RIGHT:
			if(GetKeyState(VK_CONTROL)&0x8000)
				move_center(2);
			else if(GetKeyState(VK_SHIFT)&0x8000)
				change_direction(13);
			else
				change_direction(3);
			break;
		case VK_ADD:
			break;
		case VK_SUBTRACT:
			break;
		case VK_NEXT: //page key
			if(GetKeyState(VK_CONTROL)&0x8000)
				;
			else if(GetKeyState(VK_SHIFT)&0x8000)
				;
			change_direction(100);
			break;
		case VK_PRIOR: //page key
			if(GetKeyState(VK_CONTROL)&0x8000)
				;
			else if(GetKeyState(VK_SHIFT)&0x8000)
				;
			change_direction(100);
			break;
		case VK_HOME:
			change_direction(5);
			break;
		case VK_END:
			;
			break;
		case ZOOM_IN_KEY: //[
			if(GetKeyState(VK_SHIFT)&0x8000){
				if(GetKeyState(VK_CONTROL)&0x8000)
					;
				else
					;
			}
			else
				;
			break;
		case ZOOM_OUT_KEY: //]
			if(GetKeyState(VK_SHIFT)&0x8000){
				if(GetKeyState(VK_CONTROL)&0x8000)
					;
				else
					;
			}
			else
				;
			break;
		case 0xBE:  //>
			if(GetKeyState(VK_SHIFT)&0x8000){
				if(GetKeyState(VK_CONTROL)&0x8000)
					;
				else
					;
			}
			else
				;
			break;
		case 0xBC:  //<
			if(GetKeyState(VK_SHIFT)&0x8000){
				if(GetKeyState(VK_CONTROL)&0x8000)
					;
				else
					;
			}
			else
				;
			break;
		case 'V':
			if(GetKeyState(VK_CONTROL)&0x8000){
				if(OpenClipboard(NULL)){
					char *p=GetClipboardData(CF_TEXT);
					if(p!=0){
						strncpy(str,p,sizeof(str));
						SetDlgItemText(hwnd,IDC_EDIT1,str);
					}
					CloseClipboard();
				}
			}
			break;
		case VK_ESCAPE:
			if(MessageBox(hwnd,"Sure you want to quit?","QUIT",MB_OKCANCEL)==IDOK)
				PostQuitMessage(0);
			break;
		}
		update_title(hwnd);
		}
		break;
	}
	return 0;
}


int WINAPI WinMain(HINSTANCE hInstance,HINSTANCE hPrevInstance,PSTR szCmdLine,int iCmdShow)
{
	MSG msg;
	ghInstance=hInstance;
#ifdef _DEBUG
	open_console();
#endif
	
	hWindow=CreateDialog(ghInstance,MAKEINTRESOURCE(IDD_DIALOG1),NULL,MainDlg);
	if(!hWindow){
		
		MessageBox(NULL,"Could not create main dialog","ERROR",MB_ICONERROR | MB_OK);
		return 0;
	}

	ShowWindow(hWindow,iCmdShow);
	UpdateWindow(hWindow);
#ifdef _DEBUG
	adjust_windows(hWindow);
#endif

	while(GetMessage(&msg,NULL,0,0))
	{
		if(!IsDialogMessage(hWindow,&msg)){		// Translate messages for the dialog
			TranslateMessage(&msg);
			DispatchMessage(&msg);
		}
		else{
		//	debug_printf("msg.message=%08X msg.lparam=%08X msg.wparam=%08X\n",msg.message,msg.lparam,msg.wparam);
		//	DispatchMessage(&msg);
			//if(msg.message == WM_KEYDOWN && msg.wparam!=VK_ESCAPE){
			if((msg.message == WM_KEYDOWN && msg.wParam!=VK_ESCAPE && msg.wParam!=VK_SHIFT && msg.wParam!=VK_CONTROL)
				|| (msg.message==WM_KEYUP && msg.wParam!=VK_ESCAPE && msg.wParam!=VK_SHIFT && msg.wParam!=VK_CONTROL)){
				static DWORD time=0;
				//if((GetTickCount()-time)>100){
				{
					//if(screen_updated)
					{					
						screen_updated=FALSE;
						SendMessage(hWindow,msg.message,msg.wParam,msg.lParam);
						time=GetTickCount();
					}
				}
			}
		}
	}
	return msg.wParam;
}