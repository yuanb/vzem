#include "stdafx.h"
#include "resource.h"
#include <stdio.h>
#include <shellapi.h>
#include <stdarg.h>
#include <windows.h>
#include <atlstr.h>
#include <commctrl.h>
#include <Mmsystem.h>
#include <commdlg.h>
#include <ddraw.h>
#include <mmreg.h>
#include <dsound.h>
#include <math.h>
#include "vzem.h"
#include "osdepend.h"


#define MAX_LOADSTRING 100
#define max_path 512

static int g_playpos = 0;
static int g_writepos = 0;

enum {
  IDC_MAIN_STATUS=200,
  IDC_MAIN_TOOL=201
};
int statwidths[] = {50,100, -1};
HWND	hStatus;
HWND	hTool;
HMENU	hMenu;
LONG	lStyle;
HBRUSH	hBrush; 
RECT	statusr;
RECT	menubuttonsr;
char	D1_fname[90];
char	D1[100];
char	D2_fname[90];
char	D2[100];
char	Cass[100];
char	Cass_fname[90];

char	vzDir[100];

#define SAMPLELENGTH 8*1024
#define NUMSTREAMINGBUFFERS 5
#define DXAUDIO_BUFFER_LEN 882*5

VZFILE loadedFile;
int	   g_fileLen;
bool   g_soundEnabled = true;

typedef struct
{
	int		playpos;
	int		writepos;
	DWORD	dwLastEndWritePos;		
	short	samples[SAMPLELENGTH];
	float	sinpos;
} bufferstruct;

bufferstruct	audioBuffers[NUMSTREAMINGBUFFERS];	

extern PREFS	prefs;
extern bool	g_internalMenu;					// internal menu display 

static const byte pal_square_fontdata8x12[] =
{
	// text characters
	0x00, 0x00, 0x00, 0x1C, 0x22, 0x02, 0x1A, 0x2A, 0x2A, 0x1C, 0x00, 0x00, 
	0x00, 0x00, 0x00, 0x08, 0x14, 0x22, 0x22, 0x3E, 0x22, 0x22, 0x00, 0x00, 
	0x00, 0x00, 0x00, 0x3C, 0x12, 0x12, 0x1C, 0x12, 0x12, 0x3C, 0x00, 0x00, 
	0x00, 0x00, 0x00, 0x1C, 0x22, 0x20, 0x20, 0x20, 0x22, 0x1C, 0x00, 0x00, 
	0x00, 0x00, 0x00, 0x3C, 0x12, 0x12, 0x12, 0x12, 0x12, 0x3C, 0x00, 0x00, 
	0x00, 0x00, 0x00, 0x3E, 0x20, 0x20, 0x3C, 0x20, 0x20, 0x3E, 0x00, 0x00, 
	0x00, 0x00, 0x00, 0x3E, 0x20, 0x20, 0x3C, 0x20, 0x20, 0x20, 0x00, 0x00, 
	0x00, 0x00, 0x00, 0x1E, 0x20, 0x20, 0x26, 0x22, 0x22, 0x1E, 0x00, 0x00, 
	0x00, 0x00, 0x00, 0x22, 0x22, 0x22, 0x3E, 0x22, 0x22, 0x22, 0x00, 0x00, 
	0x00, 0x00, 0x00, 0x1C, 0x08, 0x08, 0x08, 0x08, 0x08, 0x1C, 0x00, 0x00, 
	0x00, 0x00, 0x00, 0x02, 0x02, 0x02, 0x02, 0x22, 0x22, 0x1C, 0x00, 0x00, 
	0x00, 0x00, 0x00, 0x22, 0x24, 0x28, 0x30, 0x28, 0x24, 0x22, 0x00, 0x00, 
	0x00, 0x00, 0x00, 0x20, 0x20, 0x20, 0x20, 0x20, 0x20, 0x3E, 0x00, 0x00, 
	0x00, 0x00, 0x00, 0x22, 0x36, 0x2A, 0x2A, 0x22, 0x22, 0x22, 0x00, 0x00, 
	0x00, 0x00, 0x00, 0x22, 0x32, 0x2A, 0x26, 0x22, 0x22, 0x22, 0x00, 0x00, 
	0x00, 0x00, 0x00, 0x3E, 0x22, 0x22, 0x22, 0x22, 0x22, 0x3E, 0x00, 0x00, 
	0x00, 0x00, 0x00, 0x3C, 0x22, 0x22, 0x3C, 0x20, 0x20, 0x20, 0x00, 0x00, 
	0x00, 0x00, 0x00, 0x1C, 0x22, 0x22, 0x22, 0x2A, 0x24, 0x1A, 0x00, 0x00, 
	0x00, 0x00, 0x00, 0x3C, 0x22, 0x22, 0x3C, 0x28, 0x24, 0x22, 0x00, 0x00, 
	0x00, 0x00, 0x00, 0x1C, 0x22, 0x10, 0x08, 0x04, 0x22, 0x1C, 0x00, 0x00, 
	0x00, 0x00, 0x00, 0x3E, 0x08, 0x08, 0x08, 0x08, 0x08, 0x08, 0x00, 0x00, 
	0x00, 0x00, 0x00, 0x22, 0x22, 0x22, 0x22, 0x22, 0x22, 0x1C, 0x00, 0x00, 
	0x00, 0x00, 0x00, 0x22, 0x22, 0x22, 0x14, 0x14, 0x08, 0x08, 0x00, 0x00, 
	0x00, 0x00, 0x00, 0x22, 0x22, 0x22, 0x2A, 0x2A, 0x36, 0x22, 0x00, 0x00, 
	0x00, 0x00, 0x00, 0x22, 0x22, 0x14, 0x08, 0x14, 0x22, 0x22, 0x00, 0x00, 
	0x00, 0x00, 0x00, 0x22, 0x22, 0x14, 0x08, 0x08, 0x08, 0x08, 0x00, 0x00, 
	0x00, 0x00, 0x00, 0x3E, 0x02, 0x04, 0x08, 0x10, 0x20, 0x3E, 0x00, 0x00, 
	0x00, 0x00, 0x00, 0x38, 0x20, 0x20, 0x20, 0x20, 0x20, 0x38, 0x00, 0x00, 
	0x00, 0x00, 0x00, 0x20, 0x20, 0x10, 0x08, 0x04, 0x02, 0x02, 0x00, 0x00, 
	0x00, 0x00, 0x00, 0x0E, 0x02, 0x02, 0x02, 0x02, 0x02, 0x0E, 0x00, 0x00, 
	0x00, 0x00, 0x00, 0x08, 0x1C, 0x2A, 0x08, 0x08, 0x08, 0x08, 0x00, 0x00, 
	0x00, 0x00, 0x00, 0x00, 0x08, 0x10, 0x3E, 0x10, 0x08, 0x00, 0x00, 0x00, 
	0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 
	0x00, 0x00, 0x00, 0x08, 0x08, 0x08, 0x08, 0x08, 0x00, 0x08, 0x00, 0x00, 
	0x00, 0x00, 0x00, 0x14, 0x14, 0x14, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 
	0x00, 0x00, 0x00, 0x14, 0x14, 0x36, 0x00, 0x36, 0x14, 0x14, 0x00, 0x00, 
	0x00, 0x00, 0x00, 0x08, 0x1E, 0x20, 0x1C, 0x02, 0x3C, 0x08, 0x00, 0x00, 
	0x00, 0x00, 0x00, 0x32, 0x32, 0x04, 0x08, 0x10, 0x26, 0x26, 0x00, 0x00, 
	0x00, 0x00, 0x00, 0x10, 0x28, 0x28, 0x10, 0x2A, 0x24, 0x1A, 0x00, 0x00, 
	0x00, 0x00, 0x00, 0x18, 0x18, 0x18, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 
	0x00, 0x00, 0x00, 0x08, 0x10, 0x20, 0x20, 0x20, 0x10, 0x08, 0x00, 0x00, 
	0x00, 0x00, 0x00, 0x08, 0x04, 0x02, 0x02, 0x02, 0x04, 0x08, 0x00, 0x00, 
	0x00, 0x00, 0x00, 0x00, 0x08, 0x1C, 0x3E, 0x1C, 0x08, 0x00, 0x00, 0x00, 
	0x00, 0x00, 0x00, 0x00, 0x08, 0x08, 0x3E, 0x08, 0x08, 0x00, 0x00, 0x00, 
	0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x30, 0x30, 0x10, 0x20, 0x00, 0x00, 
	0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x3E, 0x00, 0x00, 0x00, 0x00, 0x00, 
	0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x30, 0x30, 0x00, 0x00, 
	0x00, 0x00, 0x00, 0x02, 0x02, 0x04, 0x08, 0x10, 0x20, 0x20, 0x00, 0x00, 
	0x00, 0x00, 0x00, 0x18, 0x24, 0x24, 0x24, 0x24, 0x24, 0x18, 0x00, 0x00, 
	0x00, 0x00, 0x00, 0x08, 0x18, 0x08, 0x08, 0x08, 0x08, 0x1C, 0x00, 0x00, 
	0x00, 0x00, 0x00, 0x1C, 0x22, 0x02, 0x1C, 0x20, 0x20, 0x3E, 0x00, 0x00, 
	0x00, 0x00, 0x00, 0x1C, 0x22, 0x02, 0x0C, 0x02, 0x22, 0x1C, 0x00, 0x00, 
	0x00, 0x00, 0x00, 0x04, 0x0C, 0x14, 0x3E, 0x04, 0x04, 0x04, 0x00, 0x00, 
	0x00, 0x00, 0x00, 0x3E, 0x20, 0x3C, 0x02, 0x02, 0x22, 0x1C, 0x00, 0x00, 
	0x00, 0x00, 0x00, 0x1C, 0x20, 0x20, 0x3C, 0x22, 0x22, 0x1C, 0x00, 0x00, 
	0x00, 0x00, 0x00, 0x3E, 0x02, 0x04, 0x08, 0x10, 0x20, 0x20, 0x00, 0x00, 
	0x00, 0x00, 0x00, 0x1C, 0x22, 0x22, 0x1C, 0x22, 0x22, 0x1C, 0x00, 0x00, 
	0x00, 0x00, 0x00, 0x1C, 0x22, 0x22, 0x1E, 0x02, 0x02, 0x1C, 0x00, 0x00, 
	0x00, 0x00, 0x00, 0x00, 0x18, 0x18, 0x00, 0x18, 0x18, 0x00, 0x00, 0x00, 
	0x00, 0x00, 0x00, 0x18, 0x18, 0x00, 0x18, 0x18, 0x08, 0x10, 0x00, 0x00, 
	0x00, 0x00, 0x00, 0x04, 0x08, 0x10, 0x20, 0x10, 0x08, 0x04, 0x00, 0x00, 
	0x00, 0x00, 0x00, 0x00, 0x00, 0x3E, 0x00, 0x3E, 0x00, 0x00, 0x00, 0x00, 
	0x00, 0x00, 0x00, 0x10, 0x08, 0x04, 0x02, 0x04, 0x08, 0x10, 0x00, 0x00, 
	0x00, 0x00, 0x00, 0x18, 0x24, 0x04, 0x08, 0x08, 0x00, 0x08, 0x00, 0x00,
	// semigraphics
	0x00,0x00,0x00,0x00,0x00,0x00, 0x00,0x00,0x00,0x00,0x00,0x00,
	0x00,0x00,0x00,0x00,0x00,0x00, 0x0f,0x0f,0x0f,0x0f,0x0f,0x0f,
	0x00,0x00,0x00,0x00,0x00,0x00, 0xf0,0xf0,0xf0,0xf0,0xf0,0xf0,
	0x00,0x00,0x00,0x00,0x00,0x00, 0xff,0xff,0xff,0xff,0xff,0xff,
	0x0f,0x0f,0x0f,0x0f,0x0f,0x0f, 0x00,0x00,0x00,0x00,0x00,0x00,
	0x0f,0x0f,0x0f,0x0f,0x0f,0x0f, 0x0f,0x0f,0x0f,0x0f,0x0f,0x0f,
	0x0f,0x0f,0x0f,0x0f,0x0f,0x0f, 0xf0,0xf0,0xf0,0xf0,0xf0,0xf0,
	0x0f,0x0f,0x0f,0x0f,0x0f,0x0f, 0xff,0xff,0xff,0xff,0xff,0xff,
	0xf0,0xf0,0xf0,0xf0,0xf0,0xf0, 0x00,0x00,0x00,0x00,0x00,0x00,
	0xf0,0xf0,0xf0,0xf0,0xf0,0xf0, 0x0f,0x0f,0x0f,0x0f,0x0f,0x0f,
	0xf0,0xf0,0xf0,0xf0,0xf0,0xf0, 0xf0,0xf0,0xf0,0xf0,0xf0,0xf0,
	0xf0,0xf0,0xf0,0xf0,0xf0,0xf0, 0xff,0xff,0xff,0xff,0xff,0xff,
	0xff,0xff,0xff,0xff,0xff,0xff, 0x00,0x00,0x00,0x00,0x00,0x00,
	0xff,0xff,0xff,0xff,0xff,0xff, 0x0f,0x0f,0x0f,0x0f,0x0f,0x0f,
	0xff,0xff,0xff,0xff,0xff,0xff, 0xf0,0xf0,0xf0,0xf0,0xf0,0xf0,
	0xff,0xff,0xff,0xff,0xff,0xff, 0xff,0xff,0xff,0xff,0xff,0xff
};


//-----------------------------------------------------------------------------
// Global Variables:
//-----------------------------------------------------------------------------

int		g_fileOp;
int		g_saveOp = 0;
int		g_fileType;					// 0 = VZ, 1 = WAV
float	g_averageWaveLength;		// Average wavelength of cycles in wav file
int		g_maxWavByte, g_minWavByte;

HINSTANCE hInst;								// current instance
TCHAR szTitle[MAX_LOADSTRING];					// The title bar text
TCHAR szWindowClass[MAX_LOADSTRING];			// The title bar text

HWND						g_hWnd;				// Global copy of window handle

// Direct Draw

LPDIRECTDRAW7               g_pDD = NULL;			// DirectDraw object
LPDIRECTDRAWSURFACE7        g_pDDSPrimary = NULL;	// DirectDraw primary surface
LPDIRECTDRAWSURFACE7        g_pDDSBuffer = NULL;	// DirectDraw double buffer
LPDIRECTDRAWCLIPPER         g_pClipper = NULL;		// Clipper for primary
DDSURFACEDESC2				g_bdds;					// Surface description of double buffer
RECT						g_rcWindow;             // Saves the window size & pos.
RECT						g_rcViewport;           // Pos. & size to blt from
RECT						g_rcScreen;             // Screen pos. for blt
BOOL                        g_bActive = FALSE;		// Is application active?
BOOL						g_bWindowed = TRUE;		// App running in window
int							BytesPerPixel;

// Direct Sound

#define SAFE_DELETE(p)  { if(p) { delete (p);     (p)=NULL; } }
#define SAFE_RELEASE(p) { if(p) { (p)->Release(); (p)=NULL; } }

LPDIRECTSOUND       g_pDS            = NULL;
LPDIRECTSOUNDBUFFER g_pDSBuffer[5];				// sound buffers for vz speaker & sound chip 

LPDIRECTSOUNDBUFFER g_pDSSector      = NULL;
LPDIRECTSOUNDBUFFER g_pDSStepper     = NULL;

// Timing globals
LONGLONG					g_next_time=0;		// time to render next frame
LONGLONG					g_last_time=0;	    // time of previous frame
LONGLONG					g_cur_time = 0;		// current time
DWORD						g_time_count=20;    // ms per frame 
LONGLONG					g_perf_cnt;			// performance timer frequency
BOOL						g_perf_flag=FALSE;  // flag determining which timer to use

double						g_time_elapsed;	    // time since previous frame
double						g_time_scale;		// scaling factor for time
bool						g_fPause = false;	// paused or not
bool						g_finished = false;	// quit emulation
bool						g_sound = true;
bool						g_joystick = FALSE;
bool						g_pasteclipboard = FALSE;
char						*g_clipboard;


UINT	color_set[16];
byte	RGB32BIT[16][4];
byte	RGB24BIT[16][3];
USHORT	RGB16BIT[16];


//-----------------------------------------------------------------------------
// Local definitions
//-----------------------------------------------------------------------------
#define NAME                "VZEM"
#define TITLE               "  VZEM - VZ200/300 Emulator"

//-----------------------------------------------------------------------------
// Default settings
//-----------------------------------------------------------------------------
#define SIZEX               320
#define SIZEY               240

//-----------------------------------------------------------------------------
// import direct draw, direct input and direct sound functions
// Should really compile these seperately but just including for simplicity
//-----------------------------------------------------------------------------

#include "ddraw.cpp"
#include "dsound.cpp"

void removeFilePath(char *s, char *d)
{
	int len = strlen(s);
	int pos = len;
	char ch = s[pos];
	while (ch != 92)
	{
		pos--;
		ch = s[pos];
	}
	for (int i=0;i<len;i++)
	{
		pos++;
		ch = s[pos];
		d[i] = ch;
		if (ch == 0x00) break;
	}
}


/*
 * Copies the given string TO the clipboard.
 */
int copyStringToClipboard(char * source)
{
   int ok = OpenClipboard(NULL);
   
   if (!ok) return -1;
   /* else */

	HGLOBAL clipbuffer;
	char * buffer;
	EmptyClipboard();
	clipbuffer = GlobalAlloc(GMEM_DDESHARE, strlen(source)+1);
	buffer = (char*)GlobalLock(clipbuffer);
	strcpy(buffer, source);
	GlobalUnlock(clipbuffer);
	SetClipboardData(CF_TEXT,clipbuffer);
	CloseClipboard();
}

void copyClipboardToKeyboard()
{
	HANDLE clip;
	if(OpenClipboard(NULL))
	{

		clip = GetClipboardData(CF_TEXT);
		g_clipboard = (char*)clip;
		g_pasteclipboard = TRUE;
	}
}

void pasteClipboardChar(byte *kbrd)
{
	// Called from read keyboard
	char	c;
	static int delay = 0;

	if (++delay < 25) return;

	delay = 0;

	c = *g_clipboard;

	if (c == 0)
	{
		CloseClipboard();
		g_pasteclipboard = FALSE;
	}
	g_clipboard++;

	// Skip any graphics characters

	if (c > 127) return;

	// convert to uppercase if necessary

	if ((c > 96) && (c < 123))
	{
		c -= 32;
	}

	// now push the keys

	switch (c)
	{
	case '1': kbrd[3] &= 0xEF; break;		
   	case '2': kbrd[3] &= 0xFD; break;		
   	case '3': kbrd[3] &= 0xF7; break;		
   	case '4': kbrd[3] &= 0xDF; break;		
   	case '5': kbrd[3] &= 0xFE; break;		
   	case '6': kbrd[5] &= 0xFE; break;		
   	case '7': kbrd[5] &= 0xDF; break;		
   	case '8': kbrd[5] &= 0xF7; break;		
   	case '9': kbrd[5] &= 0xFD; break;	
	case '0': kbrd[5] &= 0xEF; break;		
	case '-': kbrd[5] &= 0xFB; break;

	case '!': kbrd[3] &= 0xEF; kbrd[2] &= 0xFB; break;		
   	case '"': kbrd[3] &= 0xFD; kbrd[2] &= 0xFB; break;		
	case '#': kbrd[3] &= 0xF7; kbrd[2] &= 0xFB; break;		
   	case '$': kbrd[3] &= 0xDF; kbrd[2] &= 0xFB; break;		
   	case '%': kbrd[3] &= 0xFE; kbrd[2] &= 0xFB; break;		
   	case '&': kbrd[5] &= 0xFE; kbrd[2] &= 0xFB; break;		
   	case  39: kbrd[5] &= 0xDF; kbrd[2] &= 0xFB; break;		
   	case '(': kbrd[5] &= 0xF7; kbrd[2] &= 0xFB; break;		
   	case ')': kbrd[5] &= 0xFD; kbrd[2] &= 0xFB; break;	
	case '@': kbrd[5] &= 0xEF; kbrd[2] &= 0xFB; break;		
	case '=': kbrd[5] &= 0xFB; kbrd[2] &= 0xFB; break;

	case 'Q': kbrd[0] &= 0xEF; break;		
	case 'W': kbrd[0] &= 0xFD; break;		
	case 'E': kbrd[0] &= 0xF7; break;		
	case 'R': kbrd[0] &= 0xDF; break;		
	case 'T': kbrd[0] &= 0xFE; break;		
	case 'Y': kbrd[6] &= 0xFE; break;		
	case 'U': kbrd[6] &= 0xDF; break;		
	case 'I': kbrd[6] &= 0xF7; break;		
	case 'O': kbrd[6] &= 0xFD; break;		
	case 'P': kbrd[6] &= 0xEF; break;		
	case  13: kbrd[6] &= 0xFB; break;		

	case '[': kbrd[6] &= 0xFD; kbrd[2] &= 0xFB; break;		
	case ']': kbrd[6] &= 0xEF; kbrd[2] &= 0xFB; break;		

	case 'A': kbrd[1] &= 0xEF; break;		
	case 'S': kbrd[1] &= 0xFD; break;		
	case 'D': kbrd[1] &= 0xF7; break;		
	case 'F': kbrd[1] &= 0xDF; break;		
	case 'G': kbrd[1] &= 0xFE; break;		
	case 'H': kbrd[7] &= 0xFE; break;		
	case 'J': kbrd[7] &= 0xDF; break;		
	case 'K': kbrd[7] &= 0xF7; break;		
	case 'L': kbrd[7] &= 0xFD; break;		
	case ';': kbrd[7] &= 0xEF; break;		
	case ':': kbrd[7] &= 0xFB; break;		

	case '/': kbrd[7] &= 0xF7; kbrd[2] &= 0xFB; break;		
	case '?': kbrd[7] &= 0xFD; kbrd[2] &= 0xFB; break;		
	case '+': kbrd[7] &= 0xEF; kbrd[2] &= 0xFB; break;		
	case '*': kbrd[7] &= 0xFB; kbrd[2] &= 0xFB; break;		

	case 'Z': kbrd[2] &= 0xEF; break;		
	case 'X': kbrd[2] &= 0xFD; break;		
	case 'C': kbrd[2] &= 0xF7; break;		
	case 'V': kbrd[2] &= 0xDF; break;		
	case 'B': kbrd[2] &= 0xFE; break;		
	case 'N': kbrd[4] &= 0xFE; break;		
	case 'M': kbrd[4] &= 0xDF; break;		
	case ',': kbrd[4] &= 0xF7; break;		
	case '.': kbrd[4] &= 0xFD; break;		
	case ' ': kbrd[4] &= 0xEF; break;		

	case '^': kbrd[4] &= 0xFE; kbrd[2] &= 0xFB; break;		
	case  92: kbrd[4] &= 0xDF; kbrd[2] &= 0xFB; break;		
	case '<': kbrd[4] &= 0xF7; kbrd[2] &= 0xFB; break;		
	case '>': kbrd[4] &= 0xFD; kbrd[2] &= 0xFB; break;		
	}

}


int hex2int(char *s)
{
	int		n,m,v;
	char	ch;
			
	v=0; m=4096;
	for (n=0;n<4;n++)
	{
		ch = s[n];
		if (ch > 96) ch -= 32;		// convert from lower case to upper
		if (ch > 64) ch -= 55;
		if (ch > 47) ch -= 48;
		v += m*ch;
		m = m/16;
	}
	return v;
}

byte hex2byte(char *s)
{
	int		n,m,v;
	char	ch;
			
	v=0; m=16;
	for (n=0;n<2;n++)
	{
		ch = s[n];
		if (ch > 96) ch -= 32;		// convert from lower case to upper
		if (ch > 64) ch -= 55;
		if (ch > 47) ch -= 48;
		v += m*ch;
		m = m/16;
	}
	return v;
}



//-----------------------------------------------------------------------------
// Windows specific 
//-----------------------------------------------------------------------------



void osd_ScanKbrd(BYTE *kbrd)
{

	//
	// This will be called every 20 ms from vzem
   byte			k;
   HRESULT		hRet;
   BYTE         diks[256]; // keyboard state buffer
   extern		byte g_joystickByte;
   extern int	g_render;

   if (!g_bActive) return;

   // Poll the device before reading the current state. This is required
   // for some devices (joysticks) but has no effect for others (keyboard
   // and mice). Note: this uses a DIDevice2 interface for the device.
   if (!GetKeyboardState(diks))
	   return;

   for (int i = 0; i < sizeof(diks); i++)
	   diks[i] = diks[i] & 0x80;
 
   static bool lastframe = false;
   if (diks[VK_F11])					// Toggle window & full screen
   {
	    if (lastframe == false)
		{
			lastframe = true;
            g_bWindowed = !g_bWindowed;
		    if (g_bWindowed)
			{
				PostMessage(g_hWnd, WM_COMMAND, ID_OPTIONS_SIZE_X2, 0);
				SetWindowLong(g_hWnd, GWL_STYLE, lStyle);
				SetWindowPos(g_hWnd, HWND_TOP, 0, 0, 0, 0, SWP_FRAMECHANGED);

				SetMenu(g_hWnd, hMenu);										// show menu
				ShowWindow(hTool, SW_SHOW);									// show toolbar
				ShowWindow(hStatus, SW_SHOW);								// show status bar 
				UpdateWindow(g_hWnd); 
			}
			else			// Simulate full screen
			{	
				PostMessage(g_hWnd, WM_SYSCOMMAND, SC_MAXIMIZE, 0);			// maximise screen 
				SetMenu(g_hWnd, NULL);										// hide menu
				ShowWindow(hTool, SW_HIDE);									// hide toolbar
				ShowWindow(hStatus, SW_HIDE);								// hide status bar 

				int w = GetSystemMetrics(SM_CXSCREEN);						// hide task bar 
				int h = GetSystemMetrics(SM_CYSCREEN);
				SetWindowLongPtr(g_hWnd, GWL_STYLE, WS_VISIBLE | WS_POPUP);
				SetWindowPos(g_hWnd, HWND_TOP, 0, 0, w, h, SWP_FRAMECHANGED);
				UpdateWindow(g_hWnd);
			}
		}
   }
   else
   {
	   lastframe = false;
   }

   //BY:They following definitions use US standard keyboard layout
   // keyboard line 0
   k = kbrd[0];
   if (diks['R']) k = (k & 0xDF);
   if (diks['Q']) k = (k & 0xEF);
   if (diks['E']) k = (k & 0xF7);
   if (diks['W']) k = (k & 0xFD);
   if (diks['T']) k = (k & 0xFE);
   kbrd[0] = k;

   // diksboard line 1
   k = kbrd[1];
   if (diks['F']) k = (k & 0xDF);
   if (diks['A']) k = (k & 0xEF);
   if (diks['D']) k = (k & 0xF7);
   if (diks[VK_LCONTROL]) k = (k & 0xFB);
   if (diks[VK_RCONTROL]) k = (k & 0xFB);
   if (diks['S']) k = (k & 0xFD);
   if (diks['G']) k = (k & 0xFE);
   kbrd[1] = k;

   // diksboard line 2
   k = kbrd[2];
   if (diks['V']) k = (k & 0xDF);
   if (diks['Z']) k = (k & 0xEF);
   if (diks['C']) k = (k & 0xF7);
   if (diks[VK_LSHIFT] ) k = (k & 0xFB);
   if (diks[VK_RSHIFT] ) k = (k & 0xFB);
   if (diks['X']) k = (k & 0xFD);
   if (diks['B']) k = (k & 0xFE);
   kbrd[2] = k;

   // diksboard line 3
   k = kbrd[3];
   if (diks['4']) k = (k & 0xDF); 
   if (diks['1']) k = (k & 0xEF); 
   if (diks['3']) k = (k & 0xF7); 
   if (diks['2']) k = (k & 0xFD); 
   if (diks['5']) k = (k & 0xFE); 
   kbrd[3] = k;

   // diksboard line 4
   k = kbrd[4];
   if (diks['M']) k = (k & 0xDF);
   if (diks[VK_SPACE]) k = (k & 0xEF);
   if (diks[VK_OEM_COMMA]) k = (k & 0xF7);
   if (diks[VK_OEM_PERIOD]) k = (k & 0xFD);
   if (diks['N']) k = (k & 0xFE);
   kbrd[4] = k;

   // diksboard line 5
   k = kbrd[5];
   if (diks['7']) k = (k & 0xDF); 
   if (diks['0']) k = (k & 0xEF); 
   if (diks['8']) k = (k & 0xF7); 
   if (diks[VK_OEM_MINUS]) k = (k & 0xFB);
   //if (diks[DIK_EQUALS]) k = (k & 0xFB);	//BY:This key is not present on the VZ300 keyboard. Use VK_OEM_PLUS if this is needed.
   if (diks['9']) k = (k & 0xFD); 
   if (diks['6']) k = (k & 0xFE); 
   kbrd[5] = k;

   // diksboard line 6
   k = kbrd[6];
   if (diks['U']) k = (k & 0xDF); else k = (k | 0x20);
   if (diks['P']) k = (k & 0xEF); else k = (k | 0x10);
   if (diks['I']) k = (k & 0xF7); else k = (k | 0x08);
   if (diks[VK_RETURN]) k = (k & 0xFB); else k = (k | 0x04);
   if (diks['O']) k = (k & 0xFD); else k = (k | 0x02);
   if (diks['Y']) k = (k & 0xFE); else k = (k | 0x01);
   kbrd[6] = k;

   // diksboard line 7
   k = kbrd[7];
   if (diks['J']) k = (k & 0xDF);
   if (diks[VK_OEM_1]) k = (k & 0xEF);
   if (diks['K']) k = (k & 0xF7);
   if (diks[VK_OEM_7]) k = (k & 0xFB);
   if (diks['L']) k = (k & 0xFD);
   if (diks['H']) k = (k & 0xFE);
   kbrd[7] = k;


   g_joystickByte = 0xFF;
   if (diks[VK_NUMPAD4]) g_joystickByte &= 0xFB;
   if (diks[VK_NUMPAD6]) g_joystickByte &= 0xF7;
   if (diks[VK_NUMPAD8]) g_joystickByte &= 0xFE;
   if (diks[VK_NUMPAD2]) g_joystickByte &= 0xFD;
   if (diks[VK_NUMPAD5]) g_joystickByte &= 0xEF;
   if (diks[VK_NUMPAD0]) g_joystickByte &= 0x7F; // this is a hack to fit status in 1 byte

   // extra dikss
   if (diks[VK_BACK])
   {
	kbrd[4] &= 0xDF;              // press M
	kbrd[1] &= 0xFB;          // press ctrl
   }

   if (diks[VK_LEFT]) 
   {
	kbrd[4] &= 0xDF;              // press M
	kbrd[1] &= 0xFB;			  // press ctrl
   }

   if (diks[VK_RIGHT])
   {
	   kbrd[4] &= 0xF7;              // press comma
	   kbrd[1] &= 0xFB;          // press ctrl
   }

   if (diks[VK_UP])
   {
	   kbrd[4] &= 0xFD;              // press period
	   kbrd[1] &= 0xFB;          // press ctrl
   }

   if (diks[VK_DOWN])      
   {
	   kbrd[4] &= 0xEF;              // press space
	   kbrd[1] &= 0xFB;          // press ctrl
   }

   if (diks[VK_INSERT])
   {
       kbrd[7] &= 0xFD;              // press L
       kbrd[1] &= 0xFB;          // press ctrl
   }

   if (diks[VK_DELETE])       
   {
	kbrd[7] &= 0xEF;              // press semicolon
	kbrd[1] &= 0xFB;          // press ctrl
   }

   if (diks[VK_END])
   {
	kbrd[7] &= 0xFB;              // press apostrophe
	kbrd[1] &= 0xFB;          // press ctrl
   }

   if (diks[VK_ESCAPE])
   {
	   g_internalMenu = true; 
   }

   if (g_pasteclipboard)
   {
		pasteClipboardChar(kbrd);
   }
}


/*----------------------------------------------------------------*\
   Function: InitTimer;
   Purpose:  Check for performance counter, otherwise use clock
\*----------------------------------------------------------------*/

void osd_InitTimer()
{
	// is there a performance counter available? 

	if (QueryPerformanceFrequency((LARGE_INTEGER *) &g_perf_cnt)) { 
		// yes, set g_time_count and timer choice flag 
		g_perf_flag = TRUE;
		g_time_count = (DWORD) g_perf_cnt/50;
		QueryPerformanceCounter((LARGE_INTEGER *) &g_next_time); 
		g_time_scale = 1.0/g_perf_cnt;
	} else { 
		g_finished = 1;
	} 
	// save time of last frame
	g_last_time=g_next_time;
}

/*----------------------------------------------------------------*\
   Function: osd_SynchVZ();
   Purpose:  Time the emulation to run at vz speed
\*----------------------------------------------------------------*/

bool osd_SynchVZ()
{

	while (g_cur_time < g_next_time)
	{
		QueryPerformanceCounter((LARGE_INTEGER *) &g_cur_time); 
		g_last_time=g_cur_time;
	}
	g_next_time = g_cur_time + g_time_count; 
	return true;
}

void LoadBitMap()
{
	long		lDataLen = 1000;					// Maximum size for the signature.
	DWORD		dwFileLen, dwFileLen1;
	byte		fileBuffer[65535];
	byte		vzbitmap[8192];
	byte		tempPixelData[8192];
	extern		byte VZMEM[65536];
	
	HANDLE		hFile;
	CHAR		szFile[MAX_PATH] = TEXT(".bmp\0");
	OPENFILENAME ofn;

	ZeroMemory(&ofn, sizeof(ofn));
	ofn.lStructSize = sizeof(ofn);
	ofn.hwndOwner = g_hWnd;
	ofn.lpstrFile = szFile;

	ofn.lpstrFile[0] = '\0';
	ofn.nMaxFile = sizeof(szFile);
	ofn.lpstrFilter =   "BMP files\0*.bmp"; //sets the file filter
	ofn.nFilterIndex = 1;  //sets the filter that will be initially selected
	ofn.lpstrFileTitle = NULL;
	ofn.nMaxFileTitle = 0;
	ofn.lpstrInitialDir = NULL;
	ofn.Flags = OFN_PATHMUSTEXIST | OFN_FILEMUSTEXIST;


	if (GetOpenFileName(&ofn) == TRUE)
	{
				//Open the selected file for reading ink data.
				hFile = CreateFile(
						ofn.lpstrFile,					// file to open
						GENERIC_READ,					// we just need read access
						FILE_SHARE_READ,				// allow read access for others
						NULL,							// security attributes
						OPEN_EXISTING,					// the file needs to exist
						0,								// file attribute
						NULL );							// handle to a template file
				if (hFile) 
				{
					//Get the size of the file.
					dwFileLen = GetFileSize(hFile, NULL);
					ReadFile(hFile,fileBuffer,dwFileLen,&dwFileLen1,0);
					CloseHandle(hFile);

					// Get the bitmap info

					extern byte	VZMEM[65536];
					BITMAPFILEHEADER bmfh;
					BITMAPINFOHEADER bmih;

					memcpy(&bmfh, &fileBuffer[0], sizeof(BITMAPFILEHEADER));	// 14 bytes
					memcpy(&bmih, &fileBuffer[14], sizeof(BITMAPINFOHEADER));	// 40 bytes

					if ((bmih.biWidth != 256)   ||
						(bmih.biHeight != 192)  ||
						(bmih.biSizeImage != 6144))
					{
						MessageBox( g_hWnd, "Imported bitmap must be 256x192 2 color", 
						"VZEM", MB_OK | MB_ICONERROR );
					}


					memcpy(tempPixelData, &fileBuffer[62], dwFileLen1 - 62);
					//memcpy(vzbitmap, &fileBuffer[54], dwFileLen1 - 54);


					LONG byteWidth,padWidth;
					byteWidth=padWidth=(LONG)((float)bmih.biWidth*(float)bmih.biBitCount/8.0);
					while(padWidth%4!=0)
					{
					   padWidth++;
					}

					DWORD diff;
					int offset;
					LONG height;

					height=bmih.biHeight;
					diff=height*byteWidth;

					DWORD size;
					size=bmfh.bfSize-bmfh.bfOffBits;

					offset=0;
					do
					{
						memcpy((vzbitmap+(offset*byteWidth)),
							   (tempPixelData+((height-1-offset)*padWidth)),
								byteWidth);
						offset++;
					} while(offset<height);
						 
					memcpy(&VZMEM[0xC000],vzbitmap, 6144);
					// invert the bitmap
					for (int i=0;i<6144;i++)
					{
						byte b = VZMEM[0xC000+i];
						b = ~b;
						VZMEM[0xC000+i] = b;
					}

			 }
		}
}

void LoadTxt()
{
	long		lDataLen = 1000;					// Maximum size for the signature.
	DWORD		dwFileLen, dwFileLen1;
	byte		fileBuffer[65535];

	HANDLE		hFile;
	CHAR		szFile[MAX_PATH] = TEXT(".txt\0");
	OPENFILENAME ofn;


	ZeroMemory(&ofn, sizeof(ofn));
	ofn.lStructSize = sizeof(ofn);
	ofn.hwndOwner = g_hWnd;
	ofn.lpstrFile = szFile;

	ofn.lpstrFile[0] = '\0';
	ofn.nMaxFile = sizeof(szFile);
	ofn.lpstrFilter =   "TXT files\0*.txt"; //sets the file filter
	ofn.nFilterIndex = 1;  //sets the filter that will be initially selected
	ofn.lpstrFileTitle = NULL;
	ofn.nMaxFileTitle = 0;
	ofn.lpstrInitialDir = NULL;
	ofn.Flags = OFN_PATHMUSTEXIST | OFN_FILEMUSTEXIST;


	if (GetOpenFileName(&ofn) == TRUE)
	{
				//Open the selected file for reading ink data.
				hFile = CreateFile(
						ofn.lpstrFile,					// file to open
						GENERIC_READ,					// we just need read access
						FILE_SHARE_READ,				// allow read access for others
						NULL,							// security attributes
						OPEN_EXISTING,					// the file needs to exist
						0,								// file attribute
						NULL );							// handle to a template file
				if (hFile) 
				{
					//Get the size of the file.
					dwFileLen = GetFileSize(hFile, NULL);
					ReadFile(hFile,fileBuffer,dwFileLen,&dwFileLen1,0);
					CloseHandle(hFile);
					g_clipboard = (char*)fileBuffer;
					g_pasteclipboard = TRUE;
				}
	}

}

void osd_Load_ROM(short offset)
{
/*----------------------------------------------------------------*\
   Function: LoadRom();
   Purpose:  Open a dialog box & allow selection of a file
\*----------------------------------------------------------------*/

	long		lDataLen = 1000;					// Maximum size for the signature.
	DWORD		dwFileLen, dwFileLen1;

	HANDLE		hFile;
	CHAR		szFile[MAX_PATH] = TEXT(".rom\0");
	OPENFILENAME ofn;
	extern byte	VZMEM[65536];

	ZeroMemory(&ofn, sizeof(ofn));
	ofn.lStructSize = sizeof(ofn);
	ofn.hwndOwner = g_hWnd;
	ofn.lpstrFile = szFile;

	ofn.lpstrFile[0] = '\0';
	ofn.nMaxFile = sizeof(szFile);
	ofn.lpstrFilter =   "All files\0*.*"; //sets the file filter
	ofn.nFilterIndex = 1;  //sets the filter that will be initially selected
	ofn.lpstrFileTitle = NULL;
	ofn.nMaxFileTitle = 0;
	ofn.lpstrInitialDir = NULL;
	ofn.Flags = OFN_PATHMUSTEXIST | OFN_FILEMUSTEXIST;


	if (GetOpenFileName(&ofn) == TRUE)
	{
				//Open the selected file for reading ink data.
				hFile = CreateFile(
						ofn.lpstrFile,					// file to open
						GENERIC_READ,					// we just need read access
						FILE_SHARE_READ,				// allow read access for others
						NULL,							// security attributes
						OPEN_EXISTING,					// the file needs to exist
						0,								// file attribute
						NULL );							// handle to a template file
				if (hFile) 
				{
					//Get the size of the file.
					dwFileLen = GetFileSize(hFile, NULL);
					ReadFile(hFile,&VZMEM[offset],dwFileLen,&dwFileLen1,0);
					CloseHandle(hFile);
				}
				if (offset == 0)
					strcpy(prefs.romFile, ofn.lpstrFile);
				else
					strcpy(prefs.cartridgeFile, ofn.lpstrFile);

	}

}




void osd_LoadFont()
{
/*----------------------------------------------------------------*\
   Function: LoadVZFont();
   Purpose:  Open a dialog box & allow selection of a font
\*----------------------------------------------------------------*/

	long		lDataLen = 1000;					// Maximum size for the signature.
	DWORD		dwFileLen, dwFileLen1;
	byte		fileBuffer[32768];

	HANDLE		hFile;
	CHAR		szFile[MAX_PATH] = TEXT(".fnt\0");
	OPENFILENAME ofn;

	ZeroMemory(&ofn, sizeof(ofn));
	ofn.lStructSize = sizeof(ofn);
	ofn.hwndOwner = g_hWnd;
	ofn.lpstrFile = szFile;

	ofn.lpstrFile[0] = '\0';
	ofn.nMaxFile = sizeof(szFile);
	ofn.lpstrFilter =   "Font files\0*.fnt"; //sets the file filter
	ofn.nFilterIndex = 1;  //sets the filter that will be initially selected
	ofn.lpstrFileTitle = NULL;
	ofn.nMaxFileTitle = 0;
	ofn.lpstrInitialDir = NULL;
	ofn.Flags = OFN_PATHMUSTEXIST | OFN_FILEMUSTEXIST;


	if (GetOpenFileName(&ofn) == TRUE)
	{
				//Open the selected file for reading ink data.
				hFile = CreateFile(
						ofn.lpstrFile,					// file to open
						GENERIC_READ,					// we just need read access
						FILE_SHARE_READ,				// allow read access for others
						NULL,							// security attributes
						OPEN_EXISTING,					// the file needs to exist
						0,								// file attribute
						NULL );							// handle to a template file
				if (hFile) 
				{
					//Get the size of the file.
					dwFileLen = GetFileSize(hFile, NULL);
					ReadFile(hFile,fileBuffer,dwFileLen,&dwFileLen1,0);
					CloseHandle(hFile);
					LoadFont(fileBuffer);
				}
	}

}

void ReadWavByte(HANDLE *phFile, BYTE *wavByte, HEADER wavHeader) throw (...)
{
	HANDLE	hFile = *phFile;
	DWORD	numbytes;
	short	sample16;
	BOOL	result;
	

	if (wavHeader.bits_per_sample == 8)
	{
		for (int n=0; n<wavHeader.channels; n++) { 
			result = ReadFile(hFile,wavByte,1,&numbytes,0);
		}
	}

	if (wavHeader.bits_per_sample == 16)
	{
		for (int n=0; n<wavHeader.channels; n++) { 
			result = ReadFile(hFile,&sample16,2,&numbytes,0);
			if (sample16 > 0) {
				*wavByte = 128 + (sample16 >> 8);
			} else {
				sample16 = (short)abs(sample16);
				*wavByte = 128 - (sample16 >> 8);
			}
		}
	}

	if (numbytes == 0) {
		throw -1;
	}
}


BYTE FindCycle (HANDLE *phFile, int cycleNbr, HEADER wavHeader) throw (...)
{
	// count the number of WAV file Bytes between the +ve and -ve range
	// to determine if the next bit is a binary 1 or 0

	static int lastCycle = CYCLESHORT;
	static int lastSampleCount = 0;

	int		count; 
	int		numSamples, pSamples, nSamples;
	BYTE	cycle;
	HANDLE	hFile = *phFile;

	static BYTE wavByte = 0xFF;

	nSamples = pSamples = numSamples = 0;
	while (wavByte > 0x80) {				// high to low

		ReadWavByte(&hFile,&wavByte,wavHeader);
		nSamples++;
	}

	while (wavByte < g_maxWavByte) {				// low to high
		ReadWavByte(&hFile,&wavByte,wavHeader);
		pSamples++;
	}

	numSamples = pSamples+nSamples;

	while (wavByte >= g_maxWavByte) {				// point to end of current peak
		ReadWavByte(&hFile,&wavByte,wavHeader);
	}

	count = numSamples;
	cycle = CYCLENULL;

	if (count > (float)(g_averageWaveLength * 3) || 
		count < (float)(g_averageWaveLength / 3))
	{
		cycle = CYCLENULL;
	}
	else
	{	
		if ((lastCycle == CYCLELONG) || (lastCycle == CYCLENULL) || (cycleNbr == 1) || (cycleNbr == 3))
		{
			cycle = CYCLESHORT;
		}
		else
		{
			// last cycle was short. Need to check if this is short or long
			if ((lastSampleCount == 0) || (lastSampleCount > g_averageWaveLength * 3)) lastSampleCount = count;
			int diff = abs(count-lastSampleCount);
			if ((diff > (int)(g_averageWaveLength/4)) && (count > lastSampleCount) && cycleNbr == 2) 
			{
				cycle = CYCLELONG;
			}
			else
			{
				cycle = CYCLESHORT;
			}
		}
	}

	lastCycle = cycle;
	lastSampleCount = count;

	return (cycle);
}




void FindStartBit(HANDLE *phFile, HEADER wavHeader) throw (...)
{
	HANDLE hFile = *phFile;

	BYTE	c1, c2, c3;
	BOOL	found = false;

	while (found == false)
	{
		c1 = FindCycle(&hFile,2, wavHeader);	
		if (c1 == CYCLESHORT)
		{
			c2 = FindCycle(&hFile,2, wavHeader);
			if (c2 == CYCLESHORT) 
			{
				c3 = FindCycle(&hFile,2, wavHeader);
				if (c3 == CYCLESHORT)
				{
					found = true;
				}
			}
		}
	}
}

BYTE ReadVZBit(HANDLE *phFile, HEADER wavHeader) throw (...)
{
	// Decode the next VZ bit from the WAV file data

	BYTE		bit, c1, c2, c3;
	HANDLE hFile = *phFile;

	// A binary 1 is 3 short cycles
	// A binary 0 is 1 short and 1 long cycle

	bit = NULLBIT;				// assume error

	c1 = FindCycle(&hFile,1, wavHeader);
	if (c1 == CYCLESHORT)		// all bits start on a short cycle
	{
		c2 = FindCycle(&hFile,2, wavHeader);
		if (c2 == CYCLELONG)	// found binary 0 
		{
			bit = 0;
		}
		else
		{
			if (c2 == CYCLESHORT)
			{
				c3 = FindCycle(&hFile,3, wavHeader);
				if (c3 == CYCLESHORT)	// found binary 1
				{
					bit = 1;
				}
			}
		}
	}
	return bit;
}

BYTE ReadVZByte(HANDLE *phFile, HEADER wavHeader) throw (...)
{
	// Decode the next VZ Byte from the WAV file data

	int		n;
	int		bit, vzb;

	HANDLE hFile = *phFile;

	vzb = 0;
	for (n=0;n<8;n++)
	{
		bit = ReadVZBit(&hFile, wavHeader);
		if (bit != NULLBIT)
		{
			vzb = (vzb << 1) | bit;
		}
	}
	return vzb;
}


VZFILE LoadWAVFile(HANDLE *phFile)
{
	HANDLE	hFile = *phFile;
	HEADER	wavHeader;
	VZFILE	vzf;
	DWORD	numbytes;
	byte	wavByte;
	byte	vzByte;
	int		n;
	unsigned short	startAddress, endAddress, fileSize, chk, checksum;
	extern		byte VZMEM[65536];
	byte		fileBuffer[32768];

	try {

	if (ReadFile(hFile,&wavHeader,sizeof(wavHeader),&numbytes,0) == FALSE) {
		throw("Error reading WAV file");
	}

	// checking for "RIFF" and "data" in header
	CString FORMAT = CString(wavHeader.format).Left(4);
	CString WAVE = CString(wavHeader.wave_fmt).Left(7);
	if ((FORMAT.Compare("RIFF") == 0) && (WAVE.Compare("WAVEfmt") == 0))
	{
		// valid header
	}
	else
	{
		throw("WAV file is invalid");
	}

	// Seek to midpoint of file
	SetFilePointer(hFile,GetFileSize(hFile,NULL)/3,NULL,FILE_BEGIN);
	
	// 600 bits per second. 1 bit = 3 cycles
	float wavBytesPerCycle = (float)1.0/1800 * wavHeader.sampleRate;
	g_averageWaveLength = wavBytesPerCycle;

	g_maxWavByte = 0;
	g_minWavByte = 0xFF;
	// Find maximum and minimum values
	for (n=0; n < 30; n++)
	{
		ReadWavByte(&hFile,&wavByte,wavHeader);
		if (wavByte > g_maxWavByte) g_maxWavByte = wavByte;
		if (wavByte < g_minWavByte) g_minWavByte = wavByte;
	}

	// trim the max and min wav bytes

	g_maxWavByte = g_maxWavByte - (g_maxWavByte - 128) * 0.15;
	g_minWavByte = g_minWavByte + (128 - g_minWavByte) * 0.15;

	// Seek to start of wave data
	SetFilePointer(hFile,sizeof(wavHeader),NULL,FILE_BEGIN);

	// navigate to start of first peak
	ReadWavByte(&hFile,&wavByte,wavHeader);
	while (wavByte < g_maxWavByte) {
		ReadWavByte(&hFile,&wavByte,wavHeader);
	}

	FindStartBit(&hFile, wavHeader);
	for (int n=0;n<7;n++)
	{
		if (ReadVZBit(&hFile, wavHeader) != 0) throw "Could not find start bit. Try increasing recording volume";
	}


	vzByte = ReadVZByte(&hFile, wavHeader);
	while (vzByte != 0x80)
	{
		vzByte = ReadVZByte(&hFile, wavHeader);
	}


	// Search for preamble Bytes
	while (vzByte == 0x80) 
	{
		vzByte = ReadVZByte(&hFile, wavHeader);
	}
	if (vzByte != 0xFE)
	{
		vzf.ftype = 0x00;
		return vzf;
	}

	// 4 more preamble bytes, 5 in total
	for (int n=0; n<4;n++) 
	{
		vzByte = ReadVZByte(&hFile, wavHeader);
		if (vzByte != 0xFE)
		{
			vzf.ftype = 0x00;
			return vzf;
		}
	}

	// Get File type, eg F0 (Basic), F1 (M/C)
	vzByte = ReadVZByte(&hFile, wavHeader);
	vzf.ftype = vzByte;

	for (int n=0; n<17; n++)
	{
		vzByte = ReadVZByte(&hFile, wavHeader);
		vzf.filename[n] = vzByte;
		if (vzByte == 0) break;
	}

	checksum = 0;	// start checksum

	// Read start address
	vzByte = ReadVZByte(&hFile, wavHeader);
	checksum += vzByte;
	vzf.start_addrl = vzByte;
	vzByte = ReadVZByte(&hFile, wavHeader);
	checksum += vzByte;
	vzf.start_addrh = vzByte;
	startAddress = vzf.start_addrl + 256 * vzf.start_addrh;


	// Read end address
	vzByte = ReadVZByte(&hFile, wavHeader);
	checksum += vzByte;
	endAddress = vzByte;
	vzByte = ReadVZByte(&hFile, wavHeader);
	checksum += vzByte;
	endAddress += 256 * vzByte;

	fileSize = endAddress - startAddress;
	g_fileLen = fileSize;


	for (int x=0; x< fileSize; x++)
	{
		vzByte = ReadVZByte(&hFile, wavHeader);
		checksum += vzByte;
		fileBuffer[24+x] = vzByte;
	}

	// Compare checksums to verify file converted properly

	vzByte = ReadVZByte(&hFile, wavHeader);
	chk = vzByte;
	vzByte = ReadVZByte(&hFile, wavHeader);
	chk += 256 * vzByte;
	if (checksum != chk) {
		vzf.ftype = 0x00;
		throw "Checksum error. Try adjusting recording volume";
	}

	LoadVZFile(fileBuffer, fileSize, &vzf);

	} 
	catch (int errorCode) {
        MessageBox( g_hWnd, "Could not decode WAV file. Try adjusting recording volume", 
                    "VZEM", MB_OK | MB_ICONERROR );
        EndDialog( g_hWnd, IDABORT );
		vzf.ftype = 0x00;
	}
	catch (char *str) {
       MessageBox( g_hWnd, str, 
                    "VZEM", MB_OK | MB_ICONERROR );
       EndDialog( g_hWnd, IDABORT );
	   vzf.ftype = 0x00;
	}
	
	CloseHandle(hFile);
	return vzf;
}


VZFILE osd_LoadVZFile()
{
/*----------------------------------------------------------------*\
   Function: LoadVZFile();
   Purpose:  Open a dialog box & allow selection of a file
\*----------------------------------------------------------------*/

	long		lDataLen = 1000;					// Maximum size for the signature.
	DWORD		dwFileLen, dwFileLen1;
	byte		fileBuffer[32768];

	HANDLE		hFile;
	CHAR		szFile[MAX_PATH] = TEXT(".vz\0");
	OPENFILENAME ofn;


	VZFILE vzf;

	ZeroMemory(&ofn, sizeof(ofn));
	ofn.lStructSize = sizeof(ofn);
	ofn.hwndOwner = g_hWnd;
	ofn.lpstrFile = szFile;

	ofn.lpstrFile[0] = '\0';
	ofn.nMaxFile = sizeof(szFile);
	ofn.lpstrFilter =   "VZ files\0*.vz;*.wav"; //sets the file filter
	ofn.nFilterIndex = 1;  //sets the filter that will be initially selected
	ofn.lpstrFileTitle = NULL;
	ofn.nMaxFileTitle = 0;
	ofn.lpstrInitialDir = NULL;
	ofn.Flags = OFN_PATHMUSTEXIST | OFN_FILEMUSTEXIST;

	vzf.ftype = 0x00;		// if user clicks cancel this will stay zero
	if (GetOpenFileName(&ofn) == TRUE)
	{
				//Open the selected file for reading ink data.
				hFile = CreateFile(
						ofn.lpstrFile,					// file to open
						GENERIC_READ,					// we just need read access
						FILE_SHARE_READ,				// allow read access for others
						NULL,							// security attributes
						OPEN_EXISTING,					// the file needs to exist
						0,								// file attribute
						NULL );							// handle to a template file
				if (hFile) 
				{
					char c = ofn.lpstrFile[strlen(ofn.lpstrFile)-1];
					if ((c == 'z') || (c == 'Z'))		// extension of vz or VZ
					{
						dwFileLen = GetFileSize(hFile, NULL);
						ReadFile(hFile,fileBuffer,dwFileLen,&dwFileLen1,0);
						memcpy(&vzf,fileBuffer,24);
						LoadVZFile(fileBuffer, dwFileLen-24, &vzf);
						g_fileType = 0;					// VZ snapshot
						CloseHandle(hFile);
						g_fileLen = dwFileLen;
					} else 
					{
						vzf = LoadWAVFile(&hFile);
						g_fileType = 1;					// WAV file
					}
				}
	}
	return vzf;

}

void osd_SaveROM(short offset)
{
/*----------------------------------------------------------------*\
   Function: SaveROM();
   Purpose:  Open a dialog box & allow creation of a file
\*----------------------------------------------------------------*/

	long		lDataLen = 1000;					// Maximum size for the signature.
	DWORD		dwFileLen, dwFileLen1;
	byte		fileBuffer[32768];

	HANDLE		hFile;
	CHAR		szFile[MAX_PATH] = TEXT(".rom\0");
	OPENFILENAME ofn;
	extern byte	VZMEM[65536];


	ZeroMemory(&ofn, sizeof(ofn));
	ofn.lStructSize = sizeof(ofn);
	ofn.hwndOwner = g_hWnd;
	ofn.lpstrFile = szFile;

	ofn.lpstrFile[0] = '\0';
	ofn.nMaxFile = sizeof(szFile);
	ofn.lpstrFilter =   "All files\0*.*"; //sets the file filter
	ofn.nFilterIndex = 1;  //sets the filter that will be initially selected
	ofn.lpstrFileTitle = NULL;
	ofn.nMaxFileTitle = 0;
	ofn.lpstrInitialDir = NULL;
	ofn.Flags = OFN_PATHMUSTEXIST;


	if (GetSaveFileName(&ofn) == TRUE)
	{
				//Open the selected file for reading ink data.
				hFile = CreateFile(
						ofn.lpstrFile,					// file to open
						GENERIC_WRITE,					// we just need read access
						0,								// 
						NULL,							// security attributes
						CREATE_ALWAYS,					// the file needs to exist
						FILE_ATTRIBUTE_NORMAL,			// file attribute
						NULL );							// handle to a template file
				if (hFile) 
				{
					if (offset == 0)
					{
						dwFileLen = 16384;
						memcpy(fileBuffer, &VZMEM[0], dwFileLen);
					}
					else
					{
						dwFileLen = 10240;
						memcpy(fileBuffer, &VZMEM[16384], dwFileLen);
					}
					WriteFile(hFile,fileBuffer,dwFileLen,&dwFileLen1,0); 
					CloseHandle(hFile);
				}
	}

}

int WriteVZByte(HANDLE *phFile, BYTE vzByte)
{
	static BYTE BIT1[36] =	{	0x9D, 0xB1, 0xBC, 0xBD, 0xC1, 0xAC, 0x70, 0x54, 0x49, 0x43, 0x41, 0x6D,
								0x9D, 0xB1, 0xBC, 0xBD, 0xC1, 0xAC, 0x70, 0x54, 0x49, 0x43, 0x41, 0x6D,
								0x9D, 0xB1, 0xBC, 0xBD, 0xC1, 0xAC, 0x70, 0x54, 0x49, 0x43, 0x41, 0x6D
							};	// 3 short pulses

	static BYTE BIT0[36] =	{	0x9D, 0xB1, 0xBC, 0xBD, 0xC1, 0xAC, 0x70, 0x54, 0x49, 0x43, 0x41, 0x6D,
								0x8B, 0xAA, 0xB7, 0xBC, 0xBE, 0xBE, 0xBD, 0xBC, 0xBB, 0xBB, 0xB8, 0xBA,
								0x59, 0x48, 0x3E, 0x3C, 0x3B, 0x3B, 0x3C, 0x3D, 0x3E, 0x3F, 0x41, 0x42
							};	// 1 short 1 long pulse

	BYTE	mask;
	DWORD	numBytes;
	int		total = 0;
	HANDLE	hFile = *phFile;

	mask = 0x80;
	for (int n=0; n<8; n++)
	{
		if ((vzByte & mask) == mask)	// binary 1
		{
			WriteFile(hFile,&BIT1,36,&numBytes,0); 
		}
		else
		{
			WriteFile(hFile,&BIT0,36,&numBytes,0); 
		}
		mask = (mask >> 1);
		total += 36;
	}
	return total; 
}


void SaveWavFile(HANDLE *phFile,byte *fileBuffer, DWORD dwFileLen)
{
	VZFILE	vzf;
	HEADER			wavHeader = {0x52,0x49,0x46,0x46,
					0, 0x57,0x41,0x56,0x45,0x66,0x6D,0x74,0x20,
					16,1,1,22050,22050,1,8,
					0x64,0x61,0x74,0x61,0};
	DWORD	numBytes;
	byte	wavByte, vzByte;
	int		n;
	unsigned short		chk;
	unsigned short		startAddress, endAddress;
	HANDLE	hFile = *phFile;

	// write the wave header information to the wave file
	WriteFile(hFile,&wavHeader,sizeof(wavHeader),&numBytes,0); 
	
	// get the vz snapshot header info
	memcpy(&vzf,fileBuffer,24);


	// Write some silent bytes
	wavByte = 0x80;
	wavHeader.data_len = 0;
	for (n=0; n<5000; n++)
	{
		WriteFile(hFile,&wavByte,1,&numBytes,0);
		wavHeader.data_len++;
	}
	
	// Write 255 leader bytes
	for (n=0; n<255; n++)
	{
		wavHeader.data_len += WriteVZByte(&hFile, 0x80);
	}
	
	// Write 5 preamble bytes
	for (n=0; n<5; n++)
	{
		wavHeader.data_len += WriteVZByte(&hFile, 0xFE);
	}

	// Write file type
	wavHeader.data_len += WriteVZByte(&hFile, vzf.ftype);

	// Write file name
	for (n=0; n<17; n++)
	{
		vzByte = vzf.filename[n];
		wavHeader.data_len += WriteVZByte(&hFile, vzByte);
		if (vzByte == 0) break;
	}
	

	// there is a 1 bit length gap between header and start address
	wavByte = 0x00;
	for (n=0;n<36;n++)
	{
		WriteFile(hFile,&wavByte,1,&numBytes,0);
		wavHeader.data_len++;
	}


	chk = 0;

	// Write start address
	wavHeader.data_len += WriteVZByte(&hFile, vzf.start_addrl); chk += vzf.start_addrl;
	wavHeader.data_len += WriteVZByte(&hFile, vzf.start_addrh); chk += vzf.start_addrh;
	startAddress = vzf.start_addrl + 256 * vzf.start_addrh;


	// Write end address
	endAddress = startAddress + dwFileLen - 24;		// end address = file length - vzheader
	wavHeader.data_len += WriteVZByte(&hFile, (endAddress & 0x00FF)); chk += (endAddress & 0x00FF);
	wavHeader.data_len += WriteVZByte(&hFile, (endAddress & 0xFF00) >> 8); chk += (endAddress & 0xFF00) >> 8;

	// Write the program

	unsigned long filesize = endAddress - startAddress;
	for (n=0; n<filesize; n++)
	{
		vzByte = fileBuffer[n+24];
		wavHeader.data_len += WriteVZByte(&hFile, vzByte);
		chk += vzByte;
	}

	// Write the checksum

	BYTE	hb, lb;

	hb = (BYTE)(chk/256);
	lb = (BYTE)(chk - 256*hb);
	wavHeader.data_len += WriteVZByte(&hFile, lb);
	wavHeader.data_len += WriteVZByte(&hFile, hb);

	// Write some silent bytes

	wavHeader.data_len += WriteVZByte(&hFile,0x00);
	wavByte = 0x80;
	for (n=0; n<5000; n++)
	{
		WriteFile(hFile,&wavByte,1,&numBytes,0);
		wavHeader.data_len++;
	}


	//write final wave header
	wavHeader.fileLength = wavHeader.data_len + 36;
	SetFilePointer(hFile,0,NULL,FILE_BEGIN);
	WriteFile(hFile,&wavHeader, sizeof(wavHeader),&numBytes,0);

}


void osd_SaveVZFile(VZFILE *vzf,int fileLength)
{
/*----------------------------------------------------------------*\
   Function: SaveVZFile();
   Purpose:  Open a dialog box & allow creation of a file
\*----------------------------------------------------------------*/

	long		lDataLen = 1000;					// Maximum size for the signature.
	DWORD		dwFileLen, dwFileLen1;
	byte		fileBuffer[32768];

	HANDLE		hFile;
	CHAR		szFile[MAX_PATH];	// = TEXT(".vz\0");
	OPENFILENAME ofn;

	if (g_saveOp == 0) strcpy(szFile,TEXT(".vz\0"));
	if (g_saveOp == 1) strcpy(szFile,TEXT(".wav\0"));

	ZeroMemory(&ofn, sizeof(ofn));
	ofn.lStructSize = sizeof(ofn);
	ofn.hwndOwner = g_hWnd;
	ofn.lpstrFile = szFile;

	ofn.lpstrFile[0] = '\0';
	ofn.nMaxFile = sizeof(szFile);
	if (g_saveOp == 0) ofn.lpstrFilter =   "VZ files\0*.vz"; //sets the file filter
	if (g_saveOp == 1) ofn.lpstrFilter =   "WAV files\0*.wav"; //sets the file filter
	ofn.nFilterIndex = 1;  //sets the filter that will be initially selected
	ofn.lpstrFileTitle = NULL;
	ofn.nMaxFileTitle = 0;
	ofn.lpstrInitialDir = NULL;
	ofn.Flags = OFN_PATHMUSTEXIST;


	if (GetSaveFileName(&ofn) == TRUE)
	{
				//Open the selected file for reading ink data.
				hFile = CreateFile(
						ofn.lpstrFile,					// file to open
						GENERIC_WRITE,					// we just need read access
						0,								// 
						NULL,							// security attributes
						CREATE_ALWAYS,					// the file needs to exist
						FILE_ATTRIBUTE_NORMAL,			// file attribute
						NULL );							// handle to a template file
				if (hFile) 
				{
					if (g_saveOp == 0)				// save to vz snapshot
					{
						dwFileLen = fileLength;
						SaveVZFile(fileBuffer, &dwFileLen,vzf);
						WriteFile(hFile,fileBuffer,dwFileLen,&dwFileLen1,0); 
						CloseHandle(hFile);
					}
					if (g_saveOp == 1)				// save to wav file
					{
						dwFileLen = fileLength;
						SaveVZFile(fileBuffer, &dwFileLen,vzf);
						SaveWavFile(&hFile,fileBuffer,dwFileLen);
						CloseHandle(hFile);
					}
				}
	}

}


void osd_MapDisk(int drive)
{
/*----------------------------------------------------------------*\
   Function: MapDisk();
   Purpose:  Open a dialog box & allow selection of a file
\*----------------------------------------------------------------*/

	long		lDataLen = 1000;					// Maximum size for the signature.

	CHAR		szFile[MAX_PATH] = TEXT(".dsk\0");
	OPENFILENAME ofn;

	ZeroMemory(&ofn, sizeof(ofn));
	ofn.lStructSize = sizeof(ofn);
	ofn.hwndOwner = g_hWnd;
	ofn.lpstrFile = szFile;

	ofn.lpstrFile[0] = '\0';
	ofn.nMaxFile = sizeof(szFile);
	ofn.lpstrFilter =   "VZ Disk files\0*.dsk"; //sets the file filter
	ofn.nFilterIndex = 1;  //sets the filter that will be initially selected
	ofn.lpstrFileTitle = NULL;
	ofn.nMaxFileTitle = 0;
	ofn.lpstrInitialDir = NULL;
	ofn.Flags = OFN_PATHMUSTEXIST | OFN_FILEMUSTEXIST;


	if (GetOpenFileName(&ofn) == TRUE)
	{
		vtech1_floppy_init(drive,ofn.lpstrFile);
		if (drive == 0)
		{
			strcpy(D1, "D1: ");
			removeFilePath(ofn.lpstrFile, D1_fname); 
			strcat(D1, D1_fname);
			SendDlgItemMessage(g_hWnd, IDC_MAIN_STATUS, SB_SETTEXT, 0, (LPARAM)D1);
		}
		if (drive == 1)
		{
			strcpy(D2, "D2: ");
			removeFilePath(ofn.lpstrFile, D2_fname); 
			strcat(D2, D2_fname);
			SendDlgItemMessage(g_hWnd, IDC_MAIN_STATUS, SB_SETTEXT, 1, (LPARAM)D2);
		}
	}
}

void osd_MapTape(int mode)
{
/*----------------------------------------------------------------*\
   Function: MapTape();
   Purpose:  Open a dialog box & allow selection of a file
\*----------------------------------------------------------------*/

	long		lDataLen = 1000;					// Maximum size for the signature.
	CHAR		szFile[MAX_PATH] = TEXT(".wav\0");
	OPENFILENAME ofn;

	ZeroMemory(&ofn, sizeof(ofn));
	ofn.lStructSize = sizeof(ofn);
	ofn.hwndOwner = g_hWnd;
	ofn.lpstrFile = szFile;

	ofn.lpstrFile[0] = '\0';
	ofn.nMaxFile = sizeof(szFile);
	ofn.lpstrFilter =   "wav files\0*.wav"; //sets the file filter
	ofn.nFilterIndex = 1;  //sets the filter that will be initially selected
	ofn.lpstrFileTitle = NULL;
	ofn.nMaxFileTitle = 0;
	ofn.lpstrInitialDir = NULL;
	ofn.Flags = OFN_PATHMUSTEXIST;

	if (mode == 1)
	{
		if (GetOpenFileName(&ofn) == TRUE)
		{
			MapTape(ofn.lpstrFile); 
			strcpy(Cass, "Cass: ");
			removeFilePath(ofn.lpstrFile, Cass_fname); 
			strcat(Cass, Cass_fname);
			SendDlgItemMessage(g_hWnd, IDC_MAIN_STATUS, SB_SETTEXT, 2, (LPARAM)Cass);

		}
	}

	if (mode == 2)
	{
		if (GetSaveFileName(&ofn) == TRUE)
		{
			MapTape(ofn.lpstrFile); 
			strcpy(Cass, "Cass: ");
			removeFilePath(ofn.lpstrFile, Cass_fname); 
			strcat(Cass, Cass_fname);
			SendDlgItemMessage(g_hWnd, IDC_MAIN_STATUS, SB_SETTEXT, 2, (LPARAM)Cass);
		}
	}
}

void osd_MapPrinter()
{
/*----------------------------------------------------------------*\
   Function: MapPrinter();
   Purpose:  Open a dialog box & allow selection of a file
\*----------------------------------------------------------------*/

	long		lDataLen = 1000;					// Maximum size for the signature.
	DWORD		dwFileLen, dwFileLen1;
	
	CHAR		szFile[MAX_PATH] = TEXT(".txt\0");
	OPENFILENAME ofn;

	ZeroMemory(&ofn, sizeof(ofn));
	ofn.lStructSize = sizeof(ofn);
	ofn.hwndOwner = g_hWnd;
	ofn.lpstrFile = szFile;

	ofn.lpstrFile[0] = '\0';
	ofn.nMaxFile = sizeof(szFile);
	ofn.lpstrFilter =   "txt files\0*.txt"; //sets the file filter
	ofn.nFilterIndex = 1;  //sets the filter that will be initially selected
	ofn.lpstrFileTitle = NULL;
	ofn.nMaxFileTitle = 0;
	ofn.lpstrInitialDir = NULL;
	ofn.Flags = OFN_PATHMUSTEXIST;

	if (GetSaveFileName(&ofn) == TRUE)
	{
		MapPrinter(ofn.lpstrFile); 
	}
}



// Mesage handler for about box.
LRESULT CALLBACK About(HWND hDlg, UINT message, WPARAM wParam, LPARAM lParam)
{
	switch (message)
	{
		case WM_INITDIALOG:
				return TRUE;

		case WM_COMMAND:
			if (LOWORD(wParam) == IDOK || LOWORD(wParam) == IDCANCEL) 
			{
				EndDialog(hDlg, LOWORD(wParam));
				return TRUE;
			}
			break;
	}
    return FALSE;
}


// Mesage handler for Snapshot 
LRESULT CALLBACK Snapshot(HWND hDlg, UINT message, WPARAM wParam, LPARAM lParam)
{
	int		startaddr;
	int		endaddr;
	int		filetype;
	char	szString[80]; 
	VZFILE	vzf;
	extern byte	VZMEM[65536];

	switch (message)
	{
		case WM_INITDIALOG:
			
			if (g_fileOp == 0)		// Saving
			{
				CheckRadioButton(hDlg,IDC_RADIO_BASIC,IDC_RADIO_BINARY,IDC_RADIO_BASIC);
				CheckRadioButton(hDlg,IDC_RADIO_VZ,IDC_RADIO_WAV,IDC_RADIO_VZ);

				startaddr = VZMEM[0x78A4] + 256 * VZMEM[0x78A5]; 
				endaddr   = VZMEM[0x78F9] + 256 * VZMEM[0x78FA]; 
				filetype = 0xF0;
				sprintf(szString,"%02X",filetype);
				SetDlgItemText(hDlg, IDC_EDIT_FILETYPE, szString);
				sprintf(szString,"%04X",startaddr);
				SetDlgItemText(hDlg, IDC_EDIT_STARTADDRESS, szString);
				sprintf(szString,"%04X",endaddr);
				SetDlgItemText(hDlg, IDC_EDIT_ENDADDRESS, szString);
	
				return TRUE;
			}

			if (g_fileOp == 1)		// Loading
			{
				if (loadedFile.ftype == 0xF0)
				{
					CheckRadioButton(hDlg,IDC_RADIO_BASIC,IDC_RADIO_BINARY,IDC_RADIO_BASIC);
				} else
				{
					CheckRadioButton(hDlg,IDC_RADIO_BASIC,IDC_RADIO_BINARY,IDC_RADIO_BINARY);
				}
				if (g_fileType == 0x00)
				{
					CheckRadioButton(hDlg,IDC_RADIO_VZ,IDC_RADIO_WAV,IDC_RADIO_VZ);
				} else
				{
					CheckRadioButton(hDlg,IDC_RADIO_VZ,IDC_RADIO_WAV,IDC_RADIO_WAV);
				}

				sprintf(szString,"%02X",loadedFile.ftype);
				SetDlgItemText(hDlg, IDC_EDIT_FILETYPE, szString);
				startaddr = loadedFile.start_addrh * 256 + loadedFile.start_addrl; 
				sprintf(szString,"%04X",startaddr);
				SetDlgItemText(hDlg, IDC_EDIT_STARTADDRESS, szString);
				endaddr = startaddr + g_fileLen - 24; 
				sprintf(szString,"%04X",endaddr);
				SetDlgItemText(hDlg, IDC_EDIT_ENDADDRESS, szString);
				memcpy(szString,loadedFile.filename,17); 
				SetDlgItemText(hDlg, IDC_EDIT_FILENAME, szString);

				return TRUE;
			}

		case WM_COMMAND:
			if (LOWORD(wParam) == IDOK) 
			{
				if (g_fileOp == 0)
				{
					GetDlgItemText(hDlg, IDC_EDIT_FILETYPE, szString, 3);
					vzf.ftype = hex2byte(szString);

					GetDlgItemText(hDlg, IDC_EDIT_STARTADDRESS, szString, 5);
					startaddr = hex2int(szString);
					vzf.start_addrh = (byte)(startaddr >> 8);
					vzf.start_addrl = (byte)(startaddr & 0x00FF);

					GetDlgItemText(hDlg, IDC_EDIT_ENDADDRESS, szString, 5);
					endaddr = hex2int(szString);

					GetDlgItemText(hDlg, IDC_EDIT_FILENAME, szString, 18);
					for (int i=0;i<18;i++) 
					{
						if (szString[i] > 96) szString[i] -= 32;		// convert from lower case to upper
					}
					
					memcpy(vzf.filename,szString,17); 
					memcpy(vzf.vzmagic,"VZF0",4);

					EndDialog(hDlg, LOWORD(wParam));
					osd_SaveVZFile(&vzf,endaddr-startaddr);
				}
				if (g_fileOp == 1)
				{
					EndDialog(hDlg, LOWORD(wParam));
				}
				return TRUE;
			}

			if (LOWORD(wParam) == IDCANCEL) 
			{
				// ANNOY JASON WITH A CANCEL BUTTON 
				// WHICH DOES SFA
				EndDialog(hDlg, LOWORD(wParam));
				return TRUE;
			}

			if (LOWORD(wParam) == IDC_RADIO_BASIC) 
			{
				if (g_fileOp == 0)
				{
					startaddr = VZMEM[0x78A4] + 256 * VZMEM[0x78A5]; 
					endaddr   = VZMEM[0x78F9] + 256 * VZMEM[0x78FA]; 
					filetype = 0xF0;

					sprintf(szString,"%02X",filetype);
					SetDlgItemText(hDlg, IDC_EDIT_FILETYPE, szString);
					sprintf(szString,"%04X",startaddr);
					SetDlgItemText(hDlg, IDC_EDIT_STARTADDRESS, szString);
					sprintf(szString,"%04X",endaddr);
					SetDlgItemText(hDlg, IDC_EDIT_ENDADDRESS, szString);
				}
			}

			if (LOWORD(wParam) == IDC_RADIO_BINARY) 
			{
				if (g_fileOp == 0)
				{
					startaddr = 0x8000; 
					endaddr   = 0xF000; 
					filetype = 0xF1;

					sprintf(szString,"%02X",filetype);
					SetDlgItemText(hDlg, IDC_EDIT_FILETYPE, szString);
					sprintf(szString,"%04X",startaddr);
					SetDlgItemText(hDlg, IDC_EDIT_STARTADDRESS, szString);
					sprintf(szString,"%04X",endaddr);
					SetDlgItemText(hDlg, IDC_EDIT_ENDADDRESS, szString);
				}
			}

			if (LOWORD(wParam) == IDC_RADIO_VZ)
			{
				g_saveOp = 0;
			}
			if (LOWORD(wParam) == IDC_RADIO_WAV)
			{
				g_saveOp = 1;
			}


			break;

	}
	return FALSE;
}

// Mesage handler for Debugger 
LRESULT CALLBACK Debugger(HWND hDlg, UINT message, WPARAM wParam, LPARAM lParam)
{
	HDC hdc = NULL;
	HDC hDC = NULL;
	HWND hPic;
	HWND edit;
	char disText[500000];
	char regText[1024];
	char ptrText[1024];
	char hwText[1024];
	char brkText[80];
	static int addr = 0x0000;

	int x,y;

	byte	*buffer;
	int		scanline;
	long	*palette;
	static boolean bRedraw = true;
	//LPSTR	szString;
	char	szString[80]; 

	switch (message)
	{
		case WM_INITDIALOG:
			edit = GetDlgItem(hDlg, IDC_DISASSEMBLY);
			GetDisassembly(disText, regText, ptrText, hwText, brkText);
			SetDlgItemText(hDlg, IDC_DISASSEMBLY, (LPCTSTR)disText);
			SetDlgItemText(hDlg, IDC_REGISTERS, (LPCTSTR)regText);
			SetDlgItemText(hDlg, IDC_POINTERS, (LPCTSTR)ptrText);
			SetDlgItemText(hDlg, IDC_HARDWARE, (LPCTSTR)hwText);
			SetDlgItemText(hDlg, IDC_BREAKPOINTS, (LPCTSTR)brkText);
			sprintf(szString,"%04X",addr);
			SetDlgItemText(hDlg, IDC_MEMSTART, szString);
			bRedraw = true;

			return TRUE;

		case WM_CLOSE:
			EndDialog(hDlg, LOWORD(wParam));
			return TRUE;

		case WM_COMMAND:
			if (LOWORD(wParam) == IDOK) 
			{
				EndDialog(hDlg, LOWORD(wParam));
				return TRUE;
			}

			if (LOWORD(wParam) == ID_SETBP) 
			{
				GetDlgItemText(hDlg, IDC_BREAKPOINTS, szString, 32);
				SetBreakPoints(szString);
				return TRUE;
			}


			if (LOWORD(wParam) == ID_STEPINTO) 
			{
				DoStep();
				GetDisassembly(disText, regText, ptrText, hwText, brkText);
				SetDlgItemText(hDlg, IDC_DISASSEMBLY, (LPCTSTR)disText);
				SetDlgItemText(hDlg, IDC_REGISTERS, (LPCTSTR)regText);
				SetDlgItemText(hDlg, IDC_POINTERS, (LPCTSTR)ptrText);
				SetDlgItemText(hDlg, IDC_HARDWARE, (LPCTSTR)hwText);

				edit = GetDlgItem(hDlg, IDC_DISASSEMBLY);
				SendMessage(edit, WM_PAINT, NULL, NULL);
				edit = GetDlgItem(hDlg, IDC_REGISTERS);
				SendMessage(edit, WM_PAINT, NULL, NULL);
				edit = GetDlgItem(hDlg, IDC_POINTERS);
				SendMessage(edit, WM_PAINT, NULL, NULL);
				edit = GetDlgItem(hDlg, IDC_HARDWARE);
				bRedraw = true;
				SendMessage(edit, WM_PAINT, NULL, NULL);

				return TRUE;
			}

			if (LOWORD(wParam) == ID_BUILD) 
			{
				int startAddr, endAddr;
				GetDlgItemText(hDlg, IDC_START, szString, 32);
				startAddr = hex2int(szString);
				GetDlgItemText(hDlg, IDC_END, szString, 32);
				endAddr = hex2int(szString);
				if ((endAddr - startAddr) > 16384)
				{
					endAddr = startAddr + 16384;
				}
				Disassemble(disText, startAddr, endAddr);
				SetDlgItemText(hDlg, IDC_DISASSEMBLY, (LPCTSTR)disText);
				edit = GetDlgItem(hDlg, IDC_DISASSEMBLY);
				SendMessage(edit, WM_PAINT, NULL, NULL);

				return TRUE;
			}

			if (LOWORD(wParam) == ID_COPY) 
			{
				GetDlgItemText(hDlg, IDC_DISASSEMBLY, disText, 500000);
				copyStringToClipboard(disText);

				return TRUE;
			}

			if (LOWORD(wParam) == ID_SETMEM)
			{
				bRedraw = true;
				edit = GetDlgItem(hDlg,IDC_PIC2);
				SendMessage(edit, WM_PAINT, NULL, NULL);
				return TRUE;
			}

			if (LOWORD(wParam) == ID_MEMUP)
			{
				if (addr > 6) addr -= 6;
				sprintf(szString,"%04X",addr);
				SetDlgItemText(hDlg, IDC_MEMSTART, szString);
				bRedraw = true;
				edit = GetDlgItem(hDlg,IDC_MEMSTART);
				SendMessage(edit, WM_PAINT, NULL, NULL);
				return TRUE;
			}
			if (LOWORD(wParam) == ID_MEMDN)
			{
				if (addr < 0xFFF9) addr += 6;
				sprintf(szString,"%04X",addr);
				SetDlgItemText(hDlg, IDC_MEMSTART, szString);
				bRedraw = true;
				edit = GetDlgItem(hDlg,IDC_MEMSTART);
				SendMessage(edit, WM_PAINT, NULL, NULL);
				return TRUE;
			}

			break;

		case WM_PAINT:
			
				byte r,g,b;
				long pixel;
				byte idx;
				static int frame = 0;
				hPic = GetDlgItem(hDlg,IDC_PIC);
				hdc = GetDC(hPic);
				GetVideoBuffer(&buffer, &scanline, &palette);

				if (bRedraw)
				{
				  for (y=24;y<216;y++)
				  {
					for (x=32;x<288;x++)
					{
						idx = buffer[320*y+x];
						pixel = palette[idx];
						r = (pixel & 0xFF0000) >> 16;
						g = (pixel & 0x00FF00) >> 8;
						b = (pixel & 0x0000FF);

						SetPixel(hdc, x-32, y-24, RGB(r,g,b)); 
				    }
				  }
				}

				//	 63-254		Main display	192 lines
				if ((scanline > 62) && (scanline <255)) 
				{
					for (x=32;x<288;x++)
					{
						idx = buffer[320*(scanline-38)+x];
						pixel = 0xFF0000;
						SetPixel(hdc, x-32, scanline-63, pixel); 
					}
				}

				ReleaseDC(hPic, hdc);
				DeleteDC(hdc);
			
				// Memory dump
				hPic = GetDlgItem(hDlg,IDC_PIC2);
				hdc = GetDC(hPic);
				byte fdata, mask, ch;
				char hex[80];
				char hva[4];
				int x,y,z,c, foffs;
				extern byte	VZMEM[65536];

				GetDlgItemText(hDlg, IDC_MEMSTART, szString, 5);
				addr = hex2int(szString);
				if (bRedraw)
				{
					for (int row=0;row<16;row++)
					{
						sprintf(hex, "%04X",addr+row*6);
						strcat(hex, "  ");
						for (z=0;z<6;z++)
						{
							sprintf(hva, "%02X",VZMEM[addr+row*6+z]);
							strcat(hex, hva);
							strcat(hex, " ");
						}
						strcat(hex, "  ");
						// Print address and byte values in hex
						for (c=0;c<26;c++)
						{
							foffs = hex[c];
							if (foffs > 64) foffs -= 64;
							for (y=0;y<12;y++)
							{
								fdata = pal_square_fontdata8x12[12*foffs+y];
								mask = 0x80;
								for (x=0;x<8;x++)
								{
									if (fdata & mask)
									{
										SetPixel(hdc, x+c*8,y+row*12, 0x00FF00); 
									}
									else
									{
										SetPixel(hdc, x+c*8,y+row*12, 0x000000); 
									}
									
									mask = mask >> 1;
								}
							}
						}
						// now print as ascii
						for (z=0;z<6;z++)
						{
							pixel = 0x00ff00;
							ch = VZMEM[addr+z+row*6];
							if ((ch > 63) && (ch < 128))			// Check for inverse characters
							{
								ch -= 64;
								pixel = 0x00ff00;
							}

							if ((ch > 127) && (ch < 144))							// alpha numeric characters
							{
								ch -= 64;
								pixel = 0x00ff00;
							}

							if ((ch > 143) && (ch < 160))							
							{
								ch -= 80;
								pixel = 0x00ffff;
							}

							if ((ch > 159) && (ch < 176))							
							{
								ch -= 96;
								pixel = 0xff0000;
							}

							if ((ch > 175) && (ch < 192))							
							{
								ch -= 112;
								pixel = 0x0000ff;
							}


							if ((ch > 191) && (ch < 208))							
							{
								ch -= 128;
								pixel = 0xffffff;
							}


							if ((ch > 207) && (ch < 224))							
							{
								ch -= 144;
								pixel = 0xffff00;
							}

							if ((ch > 223) && (ch < 240))							
							{
								ch -= 160;
								pixel = 0xff00ff;
							}

							if ((ch > 239) && (ch < 256))							
							{
								ch -= 176;
								pixel = 0x0080ff;
							}
							foffs = ch;
							for (y=0;y<12;y++)
							{
								fdata = pal_square_fontdata8x12[12*foffs+y];
								mask = 0x80;
								for (x=0;x<8;x++)
								{
									if (fdata & mask)
									{
										SetPixel(hdc, x+208 + z*8 ,y+row*12, pixel); 
									}
									else
									{
										SetPixel(hdc, x+208 + z*8 ,y+row*12, 0x000000); 
									}					
									mask = mask >> 1;
								}
							}
						}
					}
				}
				bRedraw = false;
				ReleaseDC(hPic, hdc);
				DeleteDC(hdc);

				return true;
	}
    return FALSE;
}



//
//  FUNCTION: WndProc(HWND, unsigned, WORD, LONG)
//
//  PURPOSE:  Processes messages for the main window.
//
//  WM_COMMAND	- process the application menu
//  WM_PAINT	- Paint the main window
//  WM_DESTROY	- post a quit message and return
//
//
LRESULT CALLBACK WndProc(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam)
{
	HDC		screenDC;
	int		wmId, wmEvent;
	TCHAR	szHello[MAX_LOADSTRING];
	LoadString(hInst, IDS_HELLO, szHello, MAX_LOADSTRING);
    RECT                        rc;
    int                         xRight = -1;
    int                         yBottom = -1;
    MINMAXINFO                 *pMinMax;
	HIMAGELIST himlToolbar;

	switch (message) 
	{
		case WM_CREATE:

TBBUTTON buttons[10];

/* Create image list from a custom bitmap */

 himlToolbar = ImageList_LoadImage(
	hInst,			/* App instance handle */
	(LPCTSTR)IDB_BITMAP3,/* ID of your custom bitmap in resources */
	16,					/* Width of each button image within the bitmap */
	1,					/* By how much toolbar can grow. I don't use this */
	RGB(255, 0, 255),	/* Transparent color used in button images. I use magenta */
	IMAGE_BITMAP,
	LR_CREATEDIBSECTION);

if(!himlToolbar)
{
	return 1;
}

/* Create toolbar */

hTool = CreateWindowEx(0, TOOLBARCLASSNAME, NULL, WS_CHILD | WS_VISIBLE, 0, 0, 1, 1, hWnd, NULL, GetModuleHandle(NULL), NULL);

if(!hTool)
{
	return 1;
}

/* Attach image list to the toolbar */

SendMessage(hTool, TB_SETIMAGELIST, 0, (LPARAM)himlToolbar);

/* Set common properties for all buttons */

for(int n = 0; n < 10; n++)
{
	buttons[n].iBitmap = n;
	buttons[n].fsState = TBSTATE_ENABLED;
	buttons[n].fsStyle = TBSTYLE_BUTTON;
	buttons[n].dwData = 0;
	buttons[n].iString = 0;
}

/* Set properties specific to each button */

buttons[0].idCommand = ID_FILE_LOAD;
buttons[1].idCommand = ID_FILE_SAVE;
buttons[2].idCommand = ID_EDIT_PASTE;
buttons[3].idCommand = ID_FILE_MAPPRINTER;
buttons[4].idCommand = ID_MOUNT_DRIVE1;
buttons[5].idCommand = ID_MOUNT_DRIVE2;
buttons[6].idCommand = ID_TAPEPLAY;
buttons[7].idCommand = ID_TAPE_RECORD;
buttons[8].idCommand = ID_TAPE_STOP;
buttons[9].idCommand = ID_UTILS_DEBUGGER;


/* Tell toolbar the size of button structure */

SendMessage(hTool, TB_BUTTONSTRUCTSIZE, (WPARAM)sizeof(TBBUTTON), 0);

/* Add 6 buttons to the toolbar that we initialized earlier */

SendMessage(hTool, TB_ADDBUTTONS, (WPARAM)10, (LPARAM)&buttons);

/* Auto-size the toolbar */

SendMessage(hTool, TB_AUTOSIZE, 0, 0);


			GetWindowRect (hTool, &menubuttonsr);

		break;

		case WM_SIZE:
			int nWidth;
			int nWidths[3];
			extern int		g_render;

			nWidth = LOWORD(lParam);
			nWidths[0] = 1 * nWidth / 3;
			nWidths[1] = 2 * nWidth / 3;
			nWidths[2] = 3 * nWidth / 3;
			SendMessage(hStatus, SB_SETPARTS, 3, (LPARAM) nWidths);
			SendMessage(hStatus, WM_SIZE, 0, 0);
			SendMessage(hTool, WM_SIZE, 0, 0);

			screenDC = GetDC(g_hWnd);
            SelectObject(screenDC, hBrush);
            Rectangle(screenDC, 0, 0, GetSystemMetrics(SM_CXSCREEN), GetSystemMetrics(SM_CYSCREEN));
            ReleaseDC(g_hWnd,screenDC);
            DeleteDC(screenDC);
			g_render = 4;	// force redraw if adjusting window size 

			return DefWindowProc(hWnd, message, wParam, lParam);
			break;


        case WM_ACTIVATE:
            // Pause if minimized
            g_bActive = !((BOOL)HIWORD(wParam));
            break;

		case WM_KILLFOCUS:
			g_bActive = false;
			break;

		case WM_COMMAND:
			wmId    = LOWORD(wParam); 
			wmEvent = HIWORD(wParam); 

			// Parse the menu selections:
			switch (wmId)
			{
				case ID_FILE_LOAD:
					g_fileOp = 1;
					loadedFile = osd_LoadVZFile();
					if ((prefs.fast_load == 0) && (loadedFile.ftype != 0x00))
					{
						DialogBox(hInst, (LPCTSTR)IDD_SNAPSHOT, hWnd, (DLGPROC)Snapshot);
					}
					break;

				case ID_FILE_SAVE:
					g_fileOp = 0;
				   DialogBox(hInst, (LPCTSTR)IDD_SNAPSHOT, hWnd, (DLGPROC)Snapshot);
				   break;

				case ID_FILE_LOAD_ROM:
					osd_Load_ROM(0);
				   break;

				case ID_FILE_LOADFONT:
					osd_LoadFont();
				   break;

			
				case ID_FILE_SAVE_ROM:
					osd_SaveROM(0);
				   break;

   				case ID_FILE_SAVECARTRIDGE:
					osd_SaveROM(16384);
				   break;


				case ID_FILE_LOAD_CARTRIDGE:
					osd_Load_ROM(16384);
				   break;


				case ID_FILE_LOADTXT:
					LoadTxt();
					break;

				case ID_MOUNT_DRIVE1:
					osd_MapDisk(0);
				   break;

				case ID_MOUNT_DRIVE2:
					osd_MapDisk(1);
				   break;

				case ID_REMOVE_DRIVE1:
					vtech1_floppy_remove(0);
					strcpy(D1, "D1: None");
					strcpy(D1_fname, D1);
					SendDlgItemMessage(g_hWnd, IDC_MAIN_STATUS, SB_SETTEXT, 0, (LPARAM)D1);
				   break;

				case ID_REMOVE_DRIVE2:
					vtech1_floppy_remove(1);
					strcpy(D2, "D2: None");
					strcpy(D2_fname, D2);
					SendDlgItemMessage(g_hWnd, IDC_MAIN_STATUS, SB_SETTEXT, 1, (LPARAM)D2);
				   break;


				case ID_FILE_MAPPRINTER:
					osd_MapPrinter();
				   break;

				case ID_FILE_RESET:
					InitVZ();
				   break;

				case ID_EDIT_PASTE:
					copyClipboardToKeyboard();
					break;

				case ID_OPTIONS_MODEL_VZ200:
					CheckMenuItem(GetMenu(hWnd), ID_OPTIONS_MODEL_VZ200, MF_CHECKED);
					CheckMenuItem(GetMenu(hWnd), ID_OPTIONS_MODEL_VZ300, MF_UNCHECKED);
					prefs.vzmodel = 0;
					break;

				case ID_OPTIONS_MODEL_VZ300:
					CheckMenuItem(GetMenu(hWnd), ID_OPTIONS_MODEL_VZ200, MF_UNCHECKED);
					CheckMenuItem(GetMenu(hWnd), ID_OPTIONS_MODEL_VZ300, MF_CHECKED);
					prefs.vzmodel = 1;
					break;

				case ID_OPTIONS_MODEL_PAL:
					CheckMenuItem(GetMenu(hWnd), ID_OPTIONS_MODEL_PAL, MF_CHECKED);
					CheckMenuItem(GetMenu(hWnd), ID_OPTIONS_MODEL_NTSC, MF_UNCHECKED);
					prefs.display_type = 0;
					break;

				case ID_OPTIONS_MODEL_NTSC:
					CheckMenuItem(GetMenu(hWnd), ID_OPTIONS_MODEL_PAL, MF_UNCHECKED);
					CheckMenuItem(GetMenu(hWnd), ID_OPTIONS_MODEL_NTSC, MF_CHECKED);
					prefs.display_type = 1;
					break;

				case ID_OPTIONS_SNOW_ON:
					CheckMenuItem(GetMenu(hWnd), ID_OPTIONS_SNOW_ON, MF_CHECKED);
					CheckMenuItem(GetMenu(hWnd), ID_OPTIONS_SNOW_OFF, MF_UNCHECKED);
					prefs.snow = 1;
					break;

				case ID_OPTIONS_SNOW_OFF:
					CheckMenuItem(GetMenu(hWnd), ID_OPTIONS_SNOW_ON, MF_UNCHECKED);
					CheckMenuItem(GetMenu(hWnd), ID_OPTIONS_SNOW_OFF, MF_CHECKED);
					prefs.snow = 0;
					break;

				case ID_OPTIONS_SPEAKER_ENABLED:
					for (int n=0;n<NUMSTREAMINGBUFFERS;n++) g_pDSBuffer[n]->Play(0, 0, DSBPLAY_LOOPING);
					CheckMenuItem(GetMenu(hWnd), ID_OPTIONS_SPEAKER_ENABLED, MF_CHECKED);
					CheckMenuItem(GetMenu(hWnd), ID_OPTIONS_SPEAKER_DISABLED, MF_UNCHECKED);
					break;

				case ID_OPTIONS_SPEAKER_DISABLED:
					StopBuffer(); 
					CheckMenuItem(GetMenu(hWnd), ID_OPTIONS_SPEAKER_ENABLED, MF_UNCHECKED);
					CheckMenuItem(GetMenu(hWnd), ID_OPTIONS_SPEAKER_DISABLED, MF_CHECKED);
					break;

				case ID_CASSETTEAUDIO_ENABLED:
					CheckMenuItem(GetMenu(hWnd), ID_CASSETTEAUDIO_ENABLED, MF_CHECKED);
					CheckMenuItem(GetMenu(hWnd), ID_CASSETTEAUDIO_DISABLED, MF_UNCHECKED);
					prefs.cassetteAudio = true;
					break;

				case ID_CASSETTEAUDIO_DISABLED:
					CheckMenuItem(GetMenu(hWnd), ID_CASSETTEAUDIO_ENABLED, MF_UNCHECKED);
					CheckMenuItem(GetMenu(hWnd), ID_CASSETTEAUDIO_DISABLED, MF_CHECKED);
					prefs.cassetteAudio = false;
					break;

				case ID_DISKAUDIO_ENABLED:
					CheckMenuItem(GetMenu(hWnd), ID_DISKAUDIO_ENABLED, MF_CHECKED);
					CheckMenuItem(GetMenu(hWnd), ID_DISKAUDIO_DISABLED, MF_UNCHECKED);
					prefs.diskAudio = true;
					break;

				case ID_DISKAUDIO_DISABLED:
					CheckMenuItem(GetMenu(hWnd), ID_DISKAUDIO_ENABLED, MF_UNCHECKED);
					CheckMenuItem(GetMenu(hWnd), ID_DISKAUDIO_DISABLED, MF_CHECKED);
					prefs.diskAudio = false;
					break;

				case ID_SNAPSHOTS_DISPLAYINFO:
					CheckMenuItem(GetMenu(hWnd), ID_SNAPSHOTS_DISPLAYINFO, MF_CHECKED);
					CheckMenuItem(GetMenu(hWnd), ID_SNAPSHOTS_FASTLOAD, MF_UNCHECKED);
					prefs.fast_load = false;
					break;

				case ID_SNAPSHOTS_FASTLOAD:
					CheckMenuItem(GetMenu(hWnd), ID_SNAPSHOTS_DISPLAYINFO, MF_UNCHECKED);
					CheckMenuItem(GetMenu(hWnd), ID_SNAPSHOTS_FASTLOAD, MF_CHECKED);
					prefs.fast_load = true;
					break;

				case ID_SOUNDCHIP_NONE:
					CheckMenuItem(GetMenu(hWnd), ID_SOUNDCHIP_SN76489, MF_UNCHECKED);
					CheckMenuItem(GetMenu(hWnd), ID_SOUNDCHIP_AY8910, MF_UNCHECKED);
					CheckMenuItem(GetMenu(hWnd), ID_SOUNDCHIP_NONE, MF_CHECKED);
					prefs.sound_card = 0;
					break;

				case ID_SOUNDCHIP_SN76489:
					CheckMenuItem(GetMenu(hWnd), ID_SOUNDCHIP_SN76489, MF_CHECKED);
					CheckMenuItem(GetMenu(hWnd), ID_SOUNDCHIP_NONE, MF_UNCHECKED);
					CheckMenuItem(GetMenu(hWnd), ID_SOUNDCHIP_AY8910, MF_UNCHECKED);
					prefs.sound_card = 1;
					break;

				case ID_SOUNDCHIP_AY8910:
					CheckMenuItem(GetMenu(hWnd), ID_SOUNDCHIP_AY8910, MF_CHECKED);
					CheckMenuItem(GetMenu(hWnd), ID_SOUNDCHIP_NONE, MF_UNCHECKED);
					CheckMenuItem(GetMenu(hWnd), ID_SOUNDCHIP_SN76489, MF_UNCHECKED);
					prefs.sound_card = 2;
					break;


				case ID_FRAMERATE_NORMAL:
					CheckMenuItem(GetMenu(hWnd), ID_FRAMERATE_NORMAL, MF_CHECKED);
					CheckMenuItem(GetMenu(hWnd), ID_FRAMERATE_MAXIMUM, MF_UNCHECKED);
					prefs.synchVZ = true;
					break;

				case ID_FRAMERATE_MAXIMUM:
					CheckMenuItem(GetMenu(hWnd), ID_FRAMERATE_NORMAL, MF_UNCHECKED);
					CheckMenuItem(GetMenu(hWnd), ID_FRAMERATE_MAXIMUM, MF_CHECKED);
					prefs.synchVZ = false;
					break;

				case ID_MONITOR_COLOUR:
					CheckMenuItem(GetMenu(hWnd), ID_MONITOR_COLOUR, MF_CHECKED);
					CheckMenuItem(GetMenu(hWnd), ID_MONITOR_MONOCHROME, MF_UNCHECKED);
					prefs.monitor = 1;
					setMonitor(prefs.monitor);
					break;
			
				case ID_MONITOR_MONOCHROME:
					CheckMenuItem(GetMenu(hWnd), ID_MONITOR_COLOUR, MF_UNCHECKED);
					CheckMenuItem(GetMenu(hWnd), ID_MONITOR_MONOCHROME, MF_CHECKED);
					prefs.monitor = 0;
					setMonitor(prefs.monitor);
					break;

				case ID_HIRES_NORMAL:
					CheckMenuItem(GetMenu(hWnd), ID_HIRES_NORMAL, MF_CHECKED);
					CheckMenuItem(GetMenu(hWnd), ID_HIRES_INVERSE, MF_UNCHECKED);
					prefs.inverse = 0;
					break;

				case ID_HIRES_INVERSE:
					CheckMenuItem(GetMenu(hWnd), ID_HIRES_NORMAL, MF_UNCHECKED);
					CheckMenuItem(GetMenu(hWnd), ID_HIRES_INVERSE, MF_CHECKED);
					prefs.inverse = 1;
					break;

				case ID_OPTIONS_JOYSTICK_ENABLED:
					CheckMenuItem(GetMenu(hWnd), ID_OPTIONS_JOYSTICK_ENABLED, MF_CHECKED);
					CheckMenuItem(GetMenu(hWnd), ID_OPTIONS_JOYSTICK_DISABLED, MF_UNCHECKED);
					prefs.joystick = 1;
					break;

				case ID_OPTIONS_JOYSTICK_DISABLED:
					CheckMenuItem(GetMenu(hWnd), ID_OPTIONS_JOYSTICK_DISABLED, MF_CHECKED);
					CheckMenuItem(GetMenu(hWnd), ID_OPTIONS_JOYSTICK_ENABLED, MF_UNCHECKED);
					prefs.joystick = 0;
					break;

				case ID_OPTIONS_MEMORY_8K:
					CheckMenuItem(GetMenu(hWnd), ID_OPTIONS_MEMORY_8K, MF_CHECKED);
					CheckMenuItem(GetMenu(hWnd), ID_OPTIONS_MEMORY_18K, MF_UNCHECKED);
					CheckMenuItem(GetMenu(hWnd), ID_OPTIONS_MEMORY_24K, MF_UNCHECKED);
					CheckMenuItem(GetMenu(hWnd), ID_OPTIONS_MEMORY_34K, MF_UNCHECKED);
					CheckMenuItem(GetMenu(hWnd), ID_OPTIONS_MEMORY_4MB, MF_UNCHECKED);
					prefs.top_of_memory = 36863;
					break;

				case ID_OPTIONS_MEMORY_18K:
					CheckMenuItem(GetMenu(hWnd), ID_OPTIONS_MEMORY_8K, MF_UNCHECKED);
					CheckMenuItem(GetMenu(hWnd), ID_OPTIONS_MEMORY_18K, MF_CHECKED);
					CheckMenuItem(GetMenu(hWnd), ID_OPTIONS_MEMORY_24K, MF_UNCHECKED);
					CheckMenuItem(GetMenu(hWnd), ID_OPTIONS_MEMORY_34K, MF_UNCHECKED);
					CheckMenuItem(GetMenu(hWnd), ID_OPTIONS_MEMORY_4MB, MF_UNCHECKED);
					prefs.top_of_memory = 47103;
					break;

				case ID_OPTIONS_MEMORY_24K:
					CheckMenuItem(GetMenu(hWnd), ID_OPTIONS_MEMORY_8K, MF_UNCHECKED);
					CheckMenuItem(GetMenu(hWnd), ID_OPTIONS_MEMORY_18K, MF_UNCHECKED);
					CheckMenuItem(GetMenu(hWnd), ID_OPTIONS_MEMORY_24K, MF_CHECKED);
					CheckMenuItem(GetMenu(hWnd), ID_OPTIONS_MEMORY_34K, MF_UNCHECKED);
					CheckMenuItem(GetMenu(hWnd), ID_OPTIONS_MEMORY_4MB, MF_UNCHECKED);
					prefs.top_of_memory = 53247;
					break;

				case ID_OPTIONS_MEMORY_34K:
					CheckMenuItem(GetMenu(hWnd), ID_OPTIONS_MEMORY_8K, MF_UNCHECKED);
					CheckMenuItem(GetMenu(hWnd), ID_OPTIONS_MEMORY_18K, MF_UNCHECKED);
					CheckMenuItem(GetMenu(hWnd), ID_OPTIONS_MEMORY_24K, MF_UNCHECKED);
					CheckMenuItem(GetMenu(hWnd), ID_OPTIONS_MEMORY_34K, MF_CHECKED);
					CheckMenuItem(GetMenu(hWnd), ID_OPTIONS_MEMORY_4MB, MF_UNCHECKED);
					prefs.top_of_memory = 63487;
					break;

				case ID_OPTIONS_MEMORY_4MB:
					CheckMenuItem(GetMenu(hWnd), ID_OPTIONS_MEMORY_8K, MF_UNCHECKED);
					CheckMenuItem(GetMenu(hWnd), ID_OPTIONS_MEMORY_18K, MF_UNCHECKED);
					CheckMenuItem(GetMenu(hWnd), ID_OPTIONS_MEMORY_24K, MF_UNCHECKED);
					CheckMenuItem(GetMenu(hWnd), ID_OPTIONS_MEMORY_34K, MF_UNCHECKED);
					CheckMenuItem(GetMenu(hWnd), ID_OPTIONS_MEMORY_4MB, MF_CHECKED);
					prefs.top_of_memory = 65535;
					break;

				case ID_OPTIONS_DISKDRIVE_ENABLED:
					CheckMenuItem(GetMenu(hWnd), ID_OPTIONS_DISKDRIVE_ENABLED, MF_CHECKED);
					CheckMenuItem(GetMenu(hWnd), ID_OPTIONS_DISKDRIVE_DISABLED, MF_UNCHECKED);
					prefs.disk_drives = 1;
					break;

				case ID_OPTIONS_DISKDRIVE_DISABLED:
					CheckMenuItem(GetMenu(hWnd), ID_OPTIONS_DISKDRIVE_ENABLED, MF_UNCHECKED);
					CheckMenuItem(GetMenu(hWnd), ID_OPTIONS_DISKDRIVE_DISABLED, MF_CHECKED);
					prefs.disk_drives = 0;
					break;

				case ID_OPTIONS_EXTENDEDGFX_AUS:
					CheckMenuItem(GetMenu(hWnd), ID_OPTIONS_EXTENDEDGFX_AUS, MF_CHECKED);
					CheckMenuItem(GetMenu(hWnd), ID_OPTIONS_EXTENDEDGFX_GERMAN, MF_UNCHECKED);
					CheckMenuItem(GetMenu(hWnd), ID_OPTIONS_EXTENDEDGFX_DISABLED, MF_UNCHECKED);
					prefs.gfx = 1;
					break;

				case ID_OPTIONS_EXTENDEDGFX_GERMAN:
					CheckMenuItem(GetMenu(hWnd), ID_OPTIONS_EXTENDEDGFX_AUS, MF_UNCHECKED);
					CheckMenuItem(GetMenu(hWnd), ID_OPTIONS_EXTENDEDGFX_GERMAN, MF_CHECKED);
					CheckMenuItem(GetMenu(hWnd), ID_OPTIONS_EXTENDEDGFX_DISABLED, MF_UNCHECKED);
					prefs.gfx = 2;
					break;


				case ID_OPTIONS_EXTENDEDGFX_DISABLED:
					CheckMenuItem(GetMenu(hWnd), ID_OPTIONS_EXTENDEDGFX_AUS, MF_UNCHECKED);
					CheckMenuItem(GetMenu(hWnd), ID_OPTIONS_EXTENDEDGFX_GERMAN, MF_UNCHECKED);
					CheckMenuItem(GetMenu(hWnd), ID_OPTIONS_EXTENDEDGFX_DISABLED, MF_CHECKED);
					prefs.gfx = 0;
					break;		

				case ID_WRITETOROM_ENABLED:
					CheckMenuItem(GetMenu(hWnd), ID_WRITETOROM_ENABLED, MF_CHECKED);
					CheckMenuItem(GetMenu(hWnd), ID_WRITETOROM_DISABLED, MF_UNCHECKED);
					prefs.rom_writes = 1;
					break;

				case ID_WRITETOROM_DISABLED:
					CheckMenuItem(GetMenu(hWnd), ID_WRITETOROM_ENABLED, MF_UNCHECKED);
					CheckMenuItem(GetMenu(hWnd), ID_WRITETOROM_DISABLED, MF_CHECKED);
					prefs.rom_writes = 0;
					break;

				case ID_WRITETOCARTRIDGE_ENABLED:
					CheckMenuItem(GetMenu(hWnd), ID_WRITETOCARTRIDGE_ENABLED, MF_CHECKED);
					CheckMenuItem(GetMenu(hWnd), ID_WRITETOCARTRIDGE_DISABLED, MF_UNCHECKED);
					prefs.cartridge_writes = 1;
					break;

				case ID_WRITETOCARTRIDGE_DISABLED:
					CheckMenuItem(GetMenu(hWnd), ID_WRITETOCARTRIDGE_ENABLED, MF_UNCHECKED);
					CheckMenuItem(GetMenu(hWnd), ID_WRITETOCARTRIDGE_DISABLED, MF_CHECKED);
					prefs.cartridge_writes = 0;
					break;


				case ID_TAPEPLAY:
					osd_MapTape(1);
					int nResult;
					nResult = PlayTape();
					if (nResult == 0)
					{
						EnableMenuItem(GetMenu(hWnd),ID_TAPEPLAY,MF_GRAYED);
						EnableMenuItem(GetMenu(hWnd),ID_TAPE_RECORD,MF_GRAYED);
						EnableMenuItem(GetMenu(hWnd),ID_TAPE_STOP,MF_ENABLED);
					}

					if (nResult == -1)
						MessageBox( g_hWnd, "To play a tape, first map the tape image to a wav file", 
                    "VZEM", MB_OK | MB_ICONERROR );
					if (nResult == -2)
						MessageBox( g_hWnd, "Could not open file. Please map to a valid image", 
                    "VZEM", MB_OK | MB_ICONERROR );
					if (nResult == -3)
						MessageBox( g_hWnd, "WAV file must be 22050hz 8 bit mono PCM. Please map to another image", 
                    "VZEM", MB_OK | MB_ICONERROR );

					break;				

				case ID_TAPE_RECORD:
					osd_MapTape(2);
					RecordTape();
					EnableMenuItem(GetMenu(hWnd),ID_TAPEPLAY,MF_GRAYED);
					EnableMenuItem(GetMenu(hWnd),ID_TAPE_RECORD,MF_GRAYED);
					EnableMenuItem(GetMenu(hWnd),ID_TAPE_STOP,MF_ENABLED);
					break;				

				case ID_TAPE_STOP:
					StopTape();
					EnableMenuItem(GetMenu(hWnd),ID_TAPEPLAY,MF_ENABLED);
					EnableMenuItem(GetMenu(hWnd),ID_TAPE_RECORD,MF_ENABLED);
					EnableMenuItem(GetMenu(hWnd),ID_TAPE_STOP,MF_GRAYED);
					break;				


				case IDM_ABOUT:
				   DialogBox(hInst, (LPCTSTR)IDD_ABOUTBOX, hWnd, (DLGPROC)About);
				   break;

				case ID_UTILS_DEBUGGER:
				   StopBuffer(); 
				   DialogBox(hInst, (LPCTSTR)IDD_DEBUGGER, hWnd, (DLGPROC)Debugger);
   				   for (int n=0;n<NUMSTREAMINGBUFFERS;n++) g_pDSBuffer[n]->Play(0, 0, DSBPLAY_LOOPING);
				   break;

				case ID_LOAD_BITMAP:
					LoadBitMap();
					break;


				case IDM_EXIT:
				   DestroyWindow(hWnd);
				   break;
                case ID_OPTIONS_SIZE_X1:
                    xRight = SIZEX * 1;
                    yBottom = SIZEY * 1;
					CheckMenuItem(GetMenu(hWnd), ID_OPTIONS_SIZE_X1, MF_CHECKED);
					CheckMenuItem(GetMenu(hWnd), ID_OPTIONS_SIZE_X2, MF_UNCHECKED);
					CheckMenuItem(GetMenu(hWnd), ID_OPTIONS_SIZE_X3, MF_UNCHECKED);
					CheckMenuItem(GetMenu(hWnd), ID_OPTIONS_SIZE_X4, MF_UNCHECKED);
                    break;

                case ID_OPTIONS_SIZE_X2:
                    xRight = SIZEX * 2;
                    yBottom = SIZEY * 2;
					CheckMenuItem(GetMenu(hWnd), ID_OPTIONS_SIZE_X1, MF_UNCHECKED);
					CheckMenuItem(GetMenu(hWnd), ID_OPTIONS_SIZE_X2, MF_CHECKED);
					CheckMenuItem(GetMenu(hWnd), ID_OPTIONS_SIZE_X3, MF_UNCHECKED);	
					CheckMenuItem(GetMenu(hWnd), ID_OPTIONS_SIZE_X4, MF_UNCHECKED);
					break;

               case ID_OPTIONS_SIZE_X3:
                    xRight = SIZEX * 3;
                    yBottom = SIZEY * 3;
					CheckMenuItem(GetMenu(hWnd), ID_OPTIONS_SIZE_X1, MF_UNCHECKED);
					CheckMenuItem(GetMenu(hWnd), ID_OPTIONS_SIZE_X2, MF_UNCHECKED);
					CheckMenuItem(GetMenu(hWnd), ID_OPTIONS_SIZE_X3, MF_CHECKED);
					CheckMenuItem(GetMenu(hWnd), ID_OPTIONS_SIZE_X4, MF_UNCHECKED);
                    break;

               case ID_OPTIONS_SIZE_X4:
                    xRight = SIZEX * 4;
                    yBottom = SIZEY * 4;
					CheckMenuItem(GetMenu(hWnd), ID_OPTIONS_SIZE_X1, MF_UNCHECKED);
					CheckMenuItem(GetMenu(hWnd), ID_OPTIONS_SIZE_X2, MF_UNCHECKED);
					CheckMenuItem(GetMenu(hWnd), ID_OPTIONS_SIZE_X3, MF_UNCHECKED);
					CheckMenuItem(GetMenu(hWnd), ID_OPTIONS_SIZE_X4, MF_CHECKED);
                    break;


			   case ID_OPTIONS_SCREENSIZE_FULLSCREEN:
                    if (g_bWindowed)
                          GetWindowRect(hWnd, &g_rcWindow);
                    g_bWindowed = FALSE;
                    ChangeCoopLevel(hWnd);
					break;

				default:
				   return DefWindowProc(hWnd, message, wParam, lParam);
			}
			if (xRight != -1)
            {
                // Change the window size if set
                SetRect(&rc, 0, 0, xRight, yBottom + statusr.bottom - statusr.top + menubuttonsr.bottom - menubuttonsr.top );
                AdjustWindowRectEx(&rc,
                                   GetWindowLong(hWnd, GWL_STYLE),
                                   GetMenu(hWnd) != NULL,
                                   GetWindowLong(hWnd, GWL_EXSTYLE));
                SetWindowPos(hWnd, NULL, 0, 0, rc.right - rc.left,
                             rc.bottom - rc.top,
                             SWP_NOZORDER | SWP_NOMOVE | SWP_NOACTIVATE);
                return 0L;
            }
 			break;

		case WM_DESTROY:
            ReleaseAllObjects();
			PostQuitMessage(0);
			break;

        case WM_GETMINMAXINFO:
            // Fix the minimum size of the window to SIZEX x SIZEY
            pMinMax = (MINMAXINFO *)lParam;
            pMinMax->ptMinTrackSize.x = SIZEX+GetSystemMetrics(SM_CXSIZEFRAME)*2;
            pMinMax->ptMinTrackSize.y = SIZEY +
                                        GetSystemMetrics(SM_CYSIZEFRAME) * 2 +
                                        GetSystemMetrics(SM_CYCAPTION) +
                                        GetSystemMetrics(SM_CYMENU);
            return 0L;


		default:
			return DefWindowProc(hWnd, message, wParam, lParam);
   }
   return 0;
}




//
//  FUNCTION: MyRegisterClass()
//
//  PURPOSE: Registers the window class.
//
//  COMMENTS:
//
//    This function and its usage is only necessary if you want this code
//    to be compatible with Win32 systems prior to the 'RegisterClassEx'
//    function that was added to Windows 95. It is important to call this function
//    so that the application will get 'well formed' small icons associated
//    with it.
//

ATOM MyRegisterClass(HINSTANCE hInstance)
{
	WNDCLASSEX wcex;

	wcex.cbSize = sizeof(WNDCLASSEX); 

	wcex.style			= CS_HREDRAW | CS_VREDRAW;
	wcex.lpfnWndProc	= (WNDPROC)WndProc;
	wcex.cbClsExtra		= 0;
	wcex.cbWndExtra		= 0;
	wcex.hInstance		= hInstance;
	wcex.hIcon			= LoadIcon(GetModuleHandle(NULL), MAKEINTRESOURCE(IDI_ICON1));
	wcex.hCursor		= LoadCursor(NULL, IDC_ARROW);
	wcex.hbrBackground	= (HBRUSH)(COLOR_WINDOW+1);
	wcex.lpszMenuName	= (LPCSTR)IDC_VZ;
	wcex.lpszClassName	= szWindowClass;

	return RegisterClassEx(&wcex);
}

//
//   FUNCTION: InitInstance(HANDLE, int)
//
//   PURPOSE: Saves instance handle and creates main window
//
//   COMMENTS:
//
//
BOOL InitInstance(HINSTANCE hInstance, int nCmdShow, HWND *phWnd)
{
    WNDCLASS                    wc;
    HRESULT                     hRet;
 

    // Set up and register window class
    wc.style = CS_HREDRAW | CS_VREDRAW;
    wc.lpfnWndProc = WndProc;
    wc.cbClsExtra = 0;
    wc.cbWndExtra = 0;
    wc.hInstance = hInstance;
    wc.hIcon = LoadIcon(GetModuleHandle(NULL), MAKEINTRESOURCE(IDI_ICON1));
    wc.hCursor = LoadCursor(NULL, IDC_ARROW);
    wc.hbrBackground = (HBRUSH) (COLOR_WINDOW + 1);
    wc.lpszMenuName = MAKEINTRESOURCE(IDC_VZ);
    wc.lpszClassName = NAME;
    RegisterClass(&wc);

    // Create a window
    *phWnd = CreateWindowEx(0,
                            NAME,
                            TITLE,
                            WS_OVERLAPPEDWINDOW,
                            CW_USEDEFAULT,
                            CW_USEDEFAULT,
                            SIZEX,
                            SIZEY,
                            NULL,
                            NULL,
                            hInstance,
                            NULL);
    if (!*phWnd)
        return DDERR_GENERIC;

	
	//PostMessage(*phWnd, WM_COMMAND, ID_OPTIONS_SIZE_X2, 0);

	hStatus = CreateWindowEx(0, STATUSCLASSNAME, NULL,WS_CHILD | WS_VISIBLE | SBARS_SIZEGRIP, 0, 0, 0, 0,*phWnd, (HMENU)IDC_MAIN_STATUS, GetModuleHandle(NULL), NULL);
	SendMessage(hStatus, SB_SETPARTS, 3, (LPARAM)statwidths); //Set the split between the two sections of the status bar
	SendDlgItemMessage(*phWnd, IDC_MAIN_STATUS, SB_SETTEXT, 0, (LPARAM)"D1: None");
	SendDlgItemMessage(*phWnd, IDC_MAIN_STATUS, SB_SETTEXT, 1, (LPARAM)"D2: None");
	SendDlgItemMessage(*phWnd, IDC_MAIN_STATUS, SB_SETTEXT, 2, (LPARAM)"Cass: None");

	GetWindowRect (hStatus, &statusr);
	

    // Save the window size/pos for switching modes
	GetWindowRect(*phWnd, &g_rcWindow);

	ShowWindow(*phWnd, nCmdShow);
	UpdateWindow(*phWnd);
	PostMessage(*phWnd, WM_SYSCOMMAND, SC_MAXIMIZE, 0);			// maximise screen

	hMenu = GetMenu(*phWnd);
	lStyle = GetWindowLong(g_hWnd, GWL_STYLE);


    ///////////////////////////////////////////////////////////////////////////
    // Create the main DirectDraw object
    ///////////////////////////////////////////////////////////////////////////
    hRet = DirectDrawCreateEx(NULL, (VOID**)&g_pDD, IID_IDirectDraw7, NULL);
    if (hRet != DD_OK)
        return InitFail(*phWnd, hRet, "DirectDrawCreateEx FAILED");


    // Initialize all the surfaces we need
    hRet = InitSurfaces(*phWnd);
    if (FAILED(hRet))
    	return FALSE;

    // Init DirectSound
    if( FAILED( InitDirectSound( *phWnd ) ) )
    {
		g_soundEnabled = false;
    }

    return DD_OK;
}


static void osint_maskinfo (DWORD mask, int *shift, int *precision)
{
	*shift = 0;

	while ((mask & 1L) == 0) {
		mask >>= 1;
		(*shift)++;
	}

	*precision = 0;

	while ((mask & 1L) != 0) {
		mask >>= 1;
		(*precision)++;
	}
}

void osd_GenColors(long *palette)
{
	int c;
	int rcomp, gcomp, bcomp;
	int rsh, rpr;
	int gsh, gpr;
	int bsh, bpr;

	DDPIXELFORMAT ddpf;

	ddpf.dwSize = sizeof (ddpf);
    g_pDDSBuffer->GetPixelFormat(&ddpf);

	
	osint_maskinfo (ddpf.dwRBitMask, &rsh, &rpr);
	osint_maskinfo (ddpf.dwGBitMask, &gsh, &gpr);
	osint_maskinfo (ddpf.dwBBitMask, &bsh, &bpr);

	for (c = 0; c < 13; c++) {
		rcomp = (palette[c] & 0xFF0000) >> 16;
		gcomp = (palette[c] & 0x00FF00) >> 8;
		bcomp = (palette[c] & 0x0000FF) >> 0;

		RGB32BIT[c][3] = 0;
		RGB32BIT[c][2] = (byte)rcomp;
		RGB32BIT[c][1] = (byte)gcomp;
		RGB32BIT[c][0] = (byte)bcomp;

		color_set[c] =	(((DWORD) rcomp >> (8 - rpr)) << rsh) |
						(((DWORD) gcomp >> (8 - gpr)) << gsh) |
						(((DWORD) bcomp >> (8 - bpr)) << bsh);

	}
}




//-----------------------------------------------------------------------------
// Name: osd_BltBuffer()
// Desc: Draw the OS Independent display to the back buffer then blit to the 
//		 primary display
//-----------------------------------------------------------------------------
void osd_BlitBuffer(byte *bufr)
{
    RECT                        rcRect;
    RECT                        destRect;
    HRESULT                     hRet;
    POINT                       pt;
		
    byte*						LockedSurfaceMemory;
    int							SurfacePoint;


	int		x,y;
	int		lpitch;
	int		pitch;
	UINT	color;

		g_pDDSBuffer->Lock(NULL, &g_bdds, DDLOCK_WAIT | DDLOCK_NOSYSLOCK  , NULL);
		LockedSurfaceMemory = (byte*)g_bdds.lpSurface;	
		lpitch = (int)g_bdds.lPitch;

		//g_pDDSBuffer
		for	(y=0;y<240;y++)
		{
			pitch = y * lpitch;
			for (x=0;x<320;x++)
			{
				color = bufr[320*y+x];
				SurfacePoint = (x * BytesPerPixel) + pitch;

				LockedSurfaceMemory[SurfacePoint+0] = RGB32BIT[color][0];
				LockedSurfaceMemory[SurfacePoint+1] = RGB32BIT[color][1];
				LockedSurfaceMemory[SurfacePoint+2] = RGB32BIT[color][2];
				LockedSurfaceMemory[SurfacePoint+3] = RGB32BIT[color][3];
			}
		}
		g_pDDSBuffer->Unlock(NULL);


		rcRect.left = 0;
		rcRect.top = 0;
		rcRect.right = 320;
		rcRect.bottom = 240;

		int statusBarHeight = statusr.bottom - statusr.top;
		
		GetClientRect(g_hWnd, &destRect);
		if (destRect.right < 320)
			destRect.right = 320;
		if (destRect.bottom < 240)
			destRect.bottom = 240+statusBarHeight;

		destRect.bottom -= statusBarHeight;

		pt.x = pt.y = 0;

		ClientToScreen(g_hWnd, &pt);
		OffsetRect(&destRect, pt.x, pt.y);

		int menuButtonsHeight = menubuttonsr.bottom - menubuttonsr.top;

		destRect.top += menuButtonsHeight;

		// correct the aspect ration so the display isn't stretched in full screen

		float f_right = (float) destRect.right;
		float f_left = (float) destRect.left; 
		float f_top = (float) destRect.top; 
		float f_bottom = (float) destRect.bottom; 

		float f_yaxis = f_bottom - f_top; 
		float f_xaxis = f_right - f_left;

		float f_ratio = f_xaxis / f_yaxis;

		if (f_ratio > 1.4)	// 320x240 screen has a 1.3 ratio. Any greater than that need to scale
		{
			float fScale = f_yaxis * 1.3333;				
			int xOffset = (f_xaxis - fScale) / 2; 

			destRect.left += xOffset;
			destRect.right -= xOffset;
		}


		// if screen is 1920*1058, the real resolution should be 1280x960
		// so the xoffset should be (1920-1280) / 2 = 320

		hRet = g_pDDSPrimary->Blt(&destRect, g_pDDSBuffer, &rcRect, DDBLT_ASYNC , NULL);

	if (hRet == DDERR_SURFACELOST)
	{
		if (!RestoreAll())
			return;
	}
}


//-----------------------------------------------------------------------------
// Name: RestoreBuffers()
// Desc: Restore lost buffers and fill them up with sound if possible
//-----------------------------------------------------------------------------
HRESULT RestoreBuffers( BOOL bLooped )
{
    HRESULT hr;
    DWORD dwStatus;
	int	n;

	for (n=0; n<NUMSTREAMINGBUFFERS; n++)
	{

		if( FAILED( hr = g_pDSBuffer[n]->GetStatus( &dwStatus ) ) )
			return hr;

		if( dwStatus & DSBSTATUS_BUFFERLOST )
		{
			// Since the app could have just been activated, then
			// DirectSound may not be giving us control yet, so 
			// the restoring the buffer may fail.  
			// If it does, sleep until DirectSound gives us control.
			do 
			{
				hr = g_pDSBuffer[n]->Restore();
				if( hr == DSERR_BUFFERLOST )
					Sleep( 10 );
			}
			while( hr = g_pDSBuffer[n]->Restore() );
			FillBufferWithSilence(g_pDSBuffer[n]);
		}
	}
    return S_OK;
}

void osd_clearsound()
{
	for (int n=0; n<NUMSTREAMINGBUFFERS; n++)
		FillBufferWithSilence(g_pDSBuffer[n]);
}

void osd_DiskStatus(int drive, char *TS)
{
	if (drive == 0)
	{
		strcpy(D1, "D1: ");
		strcat(D1, D1_fname);
		strcat(D1, TS);
		SendDlgItemMessage(g_hWnd, IDC_MAIN_STATUS, SB_SETTEXT, 0, (LPARAM)D1);
	}
	if (drive == 1)
	{
		strcpy(D2, "D2: ");
		strcat(D2, D2_fname);
		strcat(D2, TS);
		SendDlgItemMessage(g_hWnd, IDC_MAIN_STATUS, SB_SETTEXT, 1, (LPARAM)D2);
	}

}

void osd_PlayTrack()
{
	if (g_soundEnabled) {
	g_pDSSector->Stop();
	g_pDSSector->SetCurrentPosition(0); 
	g_pDSSector->Play(0,0,0);
	}
}

void osd_PlayStepper()
{
	if (g_soundEnabled) {
	g_pDSStepper->Stop();
	g_pDSStepper->SetCurrentPosition(0); 
	g_pDSStepper->Play(0,0,0);
	}
}

void osd_flushSoundBuffer(int b)
{

	HRESULT hr;
	DWORD	dwWritePos;
	DWORD	dwPlayPos;
	DWORD	writeLen;
	DWORD	dwBytesLocked1;
	DWORD	dwBytesLocked2;
	DWORD	dwByteNum; 

	VOID	*pvData1;
	VOID	*pvData2;
	VOID    *pv;

	if( FAILED( hr = g_pDSBuffer[b]->GetCurrentPosition( &dwPlayPos, &dwWritePos ) ) )
		return;

	if (audioBuffers[b].dwLastEndWritePos < dwPlayPos)
		writeLen = dwPlayPos - audioBuffers[b].dwLastEndWritePos;
	else
		writeLen = DXAUDIO_BUFFER_LEN - (audioBuffers[b].dwLastEndWritePos - dwPlayPos);

	int samplesAvailable = audioBuffers[b].writepos - audioBuffers[b].playpos;
	if (samplesAvailable < 0) samplesAvailable += SAMPLELENGTH;

	// Lock the buffer so we can write to it.
    hr = g_pDSBuffer[b]->Lock(audioBuffers[b].dwLastEndWritePos, writeLen, &pvData1, &dwBytesLocked1, &pvData2, &dwBytesLocked2, 0);

	if (!SUCCEEDED(hr))
	{
	   do 
	   {
	        hr = g_pDSBuffer[b]->Restore();
		    if( hr == DSERR_BUFFERLOST )
			    Sleep( 10 );
		}
		while( hr = g_pDSBuffer[b]->Restore() );
		hr = g_pDSBuffer[b]->Lock(audioBuffers[b].dwLastEndWritePos, writeLen, &pvData1, &dwBytesLocked1, &pvData2, &dwBytesLocked2, 0);
	}
	if (SUCCEEDED(hr))
	{
		if (samplesAvailable >= writeLen)
		{
			signed short *pSample;
			// First portion of the buffer.
			pSample = (signed short*)pvData1;
			int pp;
			for (dwByteNum = 0; dwByteNum < (dwBytesLocked1 >> 1); dwByteNum++) 
			{
				pp = audioBuffers[b].playpos; 
				*pSample++ = audioBuffers[b].samples[pp];
				pp++;
				if (pp >= SAMPLELENGTH) pp -= SAMPLELENGTH;
				audioBuffers[b].playpos = pp;
			}
			// If the locked portion of the buffer wrapped around to the 
			// beginning of the buffer then we need to write to it.
			if (dwBytesLocked2 > 0)
			{
				// Second portion of the buffer.
				pv = pvData2;
				pSample = (signed short*)pvData2;
				for (dwByteNum = 0; dwByteNum < (dwBytesLocked2 >> 1); dwByteNum++) 
				{
					pp = audioBuffers[b].playpos; 
					*pSample++ = audioBuffers[b].samples[pp];
					pp++;
					if (pp >= SAMPLELENGTH) pp -= SAMPLELENGTH;
					audioBuffers[b].playpos = pp;
				}
			}
		}
		// Unlock the buffer now that were done with it.
		g_pDSBuffer[b]->Unlock(pvData1, dwBytesLocked1, pvData2, dwBytesLocked2);

		// Save the position of the last place we wrote to so 
		// we can continue the next time this function is called.
		writeLen = dwBytesLocked1+dwBytesLocked2;
			
		audioBuffers[b].dwLastEndWritePos = (DWORD)(audioBuffers[b].dwLastEndWritePos+writeLen);

		if (audioBuffers[b].dwLastEndWritePos >= DXAUDIO_BUFFER_LEN)
		{
			audioBuffers[b].dwLastEndWritePos -= DXAUDIO_BUFFER_LEN;
		}
	}

}


void osd_writeSoundStream(short sample)
{
	// Update the VZ speaker buffer with samples
	int wp = audioBuffers[0].writepos; 
	audioBuffers[0].samples[wp] = sample;
	if (++wp == SAMPLELENGTH) wp = 0;
	audioBuffers[0].writepos = wp;
}

void osd_synchSound()
{
	// check the frequency/volume of the sound registers, generate samples
	// and send to directsound. 

	#define	TWO_PI			(3.1415926f * 2.f)

	// TI sound chip
	int	freq =0; int vol = 0;
	for (int c=0;c<4;c++)		
	{
		float sin_step;	
		vol = vol >> 4;
		sin_step = (TWO_PI * freq) / SOUND_SAMPLE_RATE;

		int wp;
		for (int i=0;i<AUDIO_FRAME_LEN;i++)
		{
			wp = audioBuffers[c+1].writepos; 
			audioBuffers[c+1].samples[wp] = (signed short)(1024*vol * sin(audioBuffers[c+1].sinpos)); 
			wp++;
			if (wp >= SAMPLELENGTH) wp -= SAMPLELENGTH;
			audioBuffers[c+1].writepos = wp;
			audioBuffers[c+1].sinpos += sin_step;
		}
		if (audioBuffers[c+1].sinpos >= TWO_PI) audioBuffers[c+1].sinpos -= TWO_PI;
	}

	for (int n=0; n<NUMSTREAMINGBUFFERS; n++)
	{
		if (g_soundEnabled) osd_flushSoundBuffer(n);
	}
}


int APIENTRY WinMain(HINSTANCE hInstance,
                     HINSTANCE hPrevInstance,
                     LPSTR     lpCmdLine,
                     int       nCmdShow)
{
 	// TODO: Place code here.
	MSG			msg;
	HACCEL		hAccelTable;


	// Initialize global strings
	LoadString(hInstance, IDS_APP_TITLE, szTitle, MAX_LOADSTRING);
	LoadString(hInstance, IDC_VZ, szWindowClass, MAX_LOADSTRING);
	MyRegisterClass(hInstance);

	//
	// Perform application initialization. Window handle is a global but 
	// still pass as a parameter so existing functions don't have to change
	//
	hInst = hInstance;
	if (InitInstance (hInstance, nCmdShow, &g_hWnd) != DD_OK) 
	{
		return FALSE;
	}

	hBrush = CreateSolidBrush(RGB(0,0,0));
	hAccelTable = LoadAccelerators(hInstance, (LPCTSTR)IDC_VZ);
	GetCurrentDirectory(100, vzDir);
	LoadPrefs();

	if (prefs.vzmodel == 0) PostMessage(g_hWnd, WM_COMMAND, ID_OPTIONS_MODEL_VZ200, 0);
	if (prefs.vzmodel == 1) PostMessage(g_hWnd, WM_COMMAND, ID_OPTIONS_MODEL_VZ300, 0);
	if (prefs.display_type == 0) PostMessage(g_hWnd, WM_COMMAND, ID_OPTIONS_MODEL_PAL, 0);
	if (prefs.display_type == 1) PostMessage(g_hWnd, WM_COMMAND, ID_OPTIONS_MODEL_NTSC, 0);
	if (prefs.snow == 0) PostMessage(g_hWnd, WM_COMMAND, ID_OPTIONS_SNOW_OFF, 0);
	if (prefs.snow == 1) PostMessage(g_hWnd, WM_COMMAND, ID_OPTIONS_SNOW_ON, 0);
	if (prefs.monitor == 0) PostMessage(g_hWnd, WM_COMMAND, ID_MONITOR_MONOCHROME, 0);
	if (prefs.monitor == 1) PostMessage(g_hWnd, WM_COMMAND, ID_MONITOR_COLOUR, 0);
	if (prefs.inverse == 0) PostMessage(g_hWnd, WM_COMMAND, ID_HIRES_NORMAL, 0);
	if (prefs.inverse == 1) PostMessage(g_hWnd, WM_COMMAND, ID_HIRES_INVERSE, 0);
	if (prefs.snow == 1) PostMessage(g_hWnd, WM_COMMAND, ID_OPTIONS_SNOW_ON, 0);
	if (prefs.top_of_memory == 36863) PostMessage(g_hWnd, WM_COMMAND, ID_OPTIONS_MEMORY_8K, 0);
	if (prefs.top_of_memory == 47103) PostMessage(g_hWnd, WM_COMMAND, ID_OPTIONS_MEMORY_18K, 0);
	if (prefs.top_of_memory == 53247) PostMessage(g_hWnd, WM_COMMAND, ID_OPTIONS_MEMORY_24K, 0);
	if (prefs.top_of_memory == 63487) PostMessage(g_hWnd, WM_COMMAND, ID_OPTIONS_MEMORY_34K, 0);
	if (prefs.top_of_memory == 65535) PostMessage(g_hWnd, WM_COMMAND, ID_OPTIONS_MEMORY_4MB, 0);
	if (prefs.disk_drives == 0) PostMessage(g_hWnd, WM_COMMAND, ID_OPTIONS_DISKDRIVE_DISABLED, 0);
	if (prefs.disk_drives == 1) PostMessage(g_hWnd, WM_COMMAND, ID_OPTIONS_DISKDRIVE_ENABLED, 0);
	if (prefs.gfx == 0) PostMessage(g_hWnd, WM_COMMAND, ID_OPTIONS_EXTENDEDGFX_DISABLED, 0);
	if (prefs.gfx == 1) PostMessage(g_hWnd, WM_COMMAND, ID_OPTIONS_EXTENDEDGFX_AUS, 0);
	if (prefs.gfx == 2) PostMessage(g_hWnd, WM_COMMAND, ID_OPTIONS_EXTENDEDGFX_GERMAN, 0);
	if (prefs.joystick == 0) PostMessage(g_hWnd, WM_COMMAND, ID_OPTIONS_JOYSTICK_DISABLED, 0);
	if (prefs.joystick == 1) PostMessage(g_hWnd, WM_COMMAND, ID_OPTIONS_JOYSTICK_ENABLED, 0);
	if (prefs.sound_card == 0) PostMessage(g_hWnd, WM_COMMAND, ID_SOUNDCHIP_NONE, 0);
	if (prefs.sound_card == 1) PostMessage(g_hWnd, WM_COMMAND, ID_SOUNDCHIP_SN76489, 0);
	if (prefs.sound_card == 2) PostMessage(g_hWnd, WM_COMMAND, ID_SOUNDCHIP_AY8910, 0);
	if (prefs.synchVZ == 1) PostMessage(g_hWnd, WM_COMMAND, ID_FRAMERATE_NORMAL, 0);
	if (prefs.synchVZ == 0) PostMessage(g_hWnd, WM_COMMAND, ID_FRAMERATE_MAXIMUM, 0);
	if (prefs.rom_writes == 0) PostMessage(g_hWnd, WM_COMMAND, ID_WRITETOROM_DISABLED, 0);
	if (prefs.rom_writes == 1) PostMessage(g_hWnd, WM_COMMAND, ID_WRITETOROM_ENABLED, 0);
	if (prefs.cartridge_writes == 0) PostMessage(g_hWnd, WM_COMMAND, ID_WRITETOCARTRIDGE_DISABLED, 0);
	if (prefs.cartridge_writes == 1) PostMessage(g_hWnd, WM_COMMAND, ID_WRITETOCARTRIDGE_ENABLED, 0);
	if (prefs.cassetteAudio == 0) PostMessage(g_hWnd, WM_COMMAND, ID_CASSETTEAUDIO_DISABLED, 0);
	if (prefs.cassetteAudio == 1) PostMessage(g_hWnd, WM_COMMAND, ID_CASSETTEAUDIO_ENABLED, 0);
	if (prefs.diskAudio == 0) PostMessage(g_hWnd, WM_COMMAND, ID_DISKAUDIO_DISABLED, 0);
	if (prefs.diskAudio == 1) PostMessage(g_hWnd, WM_COMMAND, ID_DISKAUDIO_ENABLED, 0);
	if (prefs.fast_load == 0) PostMessage(g_hWnd, WM_COMMAND, ID_SNAPSHOTS_FASTLOAD, 0);
	if (prefs.fast_load == 1) PostMessage(g_hWnd, WM_COMMAND, ID_SNAPSHOTS_DISPLAYINFO, 0);

	EnableMenuItem(GetMenu(g_hWnd),ID_TAPE_STOP,MF_GRAYED);


	InitVZ();								// reset Z80, load VZ roms & font

	// Need to init all the audio buffers

	for (int n=0; n< NUMSTREAMINGBUFFERS; n++)
	{
		audioBuffers[n].dwLastEndWritePos = 0;
		audioBuffers[n].playpos = 0;
		audioBuffers[n].writepos = 0;
		audioBuffers[n].sinpos = 0.0;
	}

	int argc;
    LPWSTR *szArglist = CommandLineToArgvW(GetCommandLineW(), &argc);
    char **argv = new char*[argc];
    for (int i=0; i<argc; i++) {
        int lgth = wcslen(szArglist[i]);
        argv[i] = new char[lgth+1];
        for (int j=0; j<=lgth; j++)
            argv[i][j] = char(szArglist[i][j]);
    }
    LocalFree(szArglist);

	if (argc > 1)
	{
		// check switch: -f = file, -d = disk
		for (int i = 1; i < argc; i++)
		{
			if (!strcmp(argv[i], "-f"))
				LoadVZFilename(argv[i + 1]);
			if (!strcmp(argv[i], "-d"))
				vtech1_floppy_init(0, argv[i + 1]);
		}
	}


    while (TRUE)
    {
        if (PeekMessage(&msg, NULL, 0, 0, PM_NOREMOVE))
        {
            if (!GetMessage(&msg, NULL, 0, 0))
                return msg.wParam;
	
            TranslateMessage(&msg);
            DispatchMessage(&msg);
        }
        else if (g_bActive)
        {
			if (DoFrame() == 1)
			{
				PostMessage(g_hWnd,WM_COMMAND,ID_UTILS_DEBUGGER,NULL);
			}
        }
        else
        {
              WaitMessage();
        }

    }

	FreeDirectSound();
    if (g_pDD)
        g_pDD->Release();

	return msg.wParam;
}


