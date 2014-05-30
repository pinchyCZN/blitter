#define _WIN32_WINNT 0x400
#include <windows.h>
#include <commctrl.h>
#include <stdio.h>
#include <conio.h>
#include <fcntl.h>
#include "resource.h"

HWND	hWindow;
HINSTANCE	ghInstance;


int scale=256;
float rx=0,ry=0,rz=0;

DWORD tick=0;
RECT client_rect;
int screen_updated=TRUE;
#define BUF_WIDTH 640
#define BUF_HEIGHT 480
int BUF_SIZE=BUF_WIDTH*BUF_HEIGHT*3;

int stretch=0;
BYTE *buffer=0;


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
	offset=x*3+BUF_SIZE-BUF_WIDTH*3-y*BUF_WIDTH*3;
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
	float x1=0,y1=0;
	if(z!=0){
		x1=(*x)*scale/(*z);
		y1=(*y)*scale/(*z);
		set_pixel(buf,x1,y1,R,G,B);
	}
	return 0;
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
int drawbuffer(BYTE *buffer)
{
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

LRESULT CALLBACK MainDlg(HWND hwnd,UINT message,WPARAM wparam,LPARAM lparam)
{
	HDC hdc,hdcMem;
	PAINTSTRUCT ps;
	BITMAPINFO bmi;
	char str[255];
	static int timer=FALSE;
	static int xpos=0,ypos=0,LMB=FALSE,dx=0,dy=0;
	int i;

#ifdef _DEBUG
//	if(message!=0x200&&message!=0x84&&message!=0x20&&message!=WM_ENTERIDLE)
//		debug_printf("message=%08X wparam=%08X lparam=%08X\n",message,wparam,lparam);
#endif	
	switch(message)
	{
	case WM_INITDIALOG:
		create_grippy(hwnd);
		BringWindowToTop(hwnd);
		GetClientRect(hwnd,&client_rect);
		memset(buffer,0,BUF_SIZE);
		BringWindowToTop(hwnd);
		update_title(hwnd);
		SendMessage(hwnd,WM_KEYDOWN,VK_TAB,0);
		SendMessage(hwnd,WM_LBUTTONDOWN,0,0);
		break;
	case WM_SIZE:
		client_rect.right=LOWORD(lparam);
		client_rect.bottom=HIWORD(lparam);
		grippy_move(hwnd);
		InvalidateRect(hwnd,NULL,TRUE);
		break;
	case WM_TIMER:
		if(LMB)
			handle_click(xpos,ypos);
		memset(buffer,0,BUF_SIZE);
		do_3d(buffer,&rx,&ry,&rz);
		InvalidateRect(hwnd,NULL,TRUE);
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
	case WM_ERASEBKGND:
		return TRUE;
		break;
	case WM_PAINT:
//		TIME1
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
		EndPaint(hwnd,&ps);
		screen_updated=TRUE;
//		TIME2
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
				rx=x;
				y=ypos-y;
				ry=y;
				printf("%f %f\n",rx,ry);
			}
			if(key&MK_RBUTTON){
				y=ypos-y;
				rz=y;
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
		drawbuffer(buffer);
		//InvalidateRect(hwnd,NULL,TRUE);
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
		case 'O':
			set_offsetx(1);
			break;
		case 'P':
			set_offsetx(-1);
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
			break;
		case 'Z':
			if(shift)
				scale-=10;
			else
				scale--;
			break;
		case 0xC0:
			change_direction(0);
			break;
		case '0':
		case '1':
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
		InvalidateRect(hwnd,NULL,TRUE);   // force redraw
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
	buffer=malloc(BUF_SIZE);
	if(buffer==0){
		MessageBox(0,"cant allocate image buffer!","barcode",MB_ICONERROR|MB_SYSTEMMODAL);
		return -1;
	}
	
	hWindow=CreateDialog(ghInstance,MAKEINTRESOURCE(IDD_DIALOG1),NULL,MainDlg);
	if(!hWindow){
		
		MessageBox(NULL,"Could not create main dialog","ERROR",MB_ICONERROR | MB_OK);
		return 0;
	}

	ShowWindow(hWindow,iCmdShow);
	UpdateWindow(hWindow);

	memset(buffer,0,BUF_SIZE);

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
	free(buffer);
	return msg.wParam;
}