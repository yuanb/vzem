#include <stdio.h>
#include <stdlib.h>

extern "C"
{
#include "Z80\z80.h"
}
#include "vzem.h"
#include "Sound.h"
#include "string.h"

// Globals

extern "C"
{
byte					*RAM;					// defined in Z80 lib
}



#define	CYCLES_PER_SCANLINE 228					// video - draw a scanline every 228 cpu cycles
#define	CYCLES_PER_TAPEBYTE 145					// cassette - read a wav sample every 140 cpu cycles

#define MAX_LEN 1024

short	SOUND_LEVELS[] = {32767, 32767, -32768, -32768 };		// square wave sound data

// samples for writing to cassette. Tested to work on a real vz300
static byte	short_high[] = {0x9D, 0xB1, 0xBC, 0xBD, 0xC1, 0xAC};		
static byte	short_low[] =  {0x70, 0x54, 0x49, 0x43, 0x43, 0x41, 0x6D};
static byte	long_high[] = {0x8B, 0xAA, 0xB7, 0xBC, 0xBE, 0xBE, 0xBD, 0xBC, 0xBB, 0xBB, 0xB8, 0xBA, 0x8D};
static byte	long_low[] = {0x59, 0x48, 0x3E, 0x3C, 0x3B, 0x3B, 0x3C, 0x3D, 0x3E, 0x3F, 0x41, 0x42, 0x76};


Z80		Regs;									// Z80 Registers
byte	VZMEM[65536];							// VZ system memory
byte    BKMEM[4194304];							// Bank memory (4 meg!)
byte	GXMEM[8192];							// Graphics memory - used for hardware mods
byte	M0GRAPHICS[24576];						// 256 characters of 8*12 font 
bool	g_redrawScanline[312];					// only draw scanline if necessary
byte	vzkbrd[8];								// VZ keyboard array
byte	vz_latch;								// copy of hardware latch
byte	gfx_mod_latch;							// latch for gfx hardware mod if enabled
int		g_scanline;								// current scanline being drawn
int		g_scanlineOffset;						// used if gfx mode has changed during screen draw
int		g_updateborder;							// check for screen mode change during retrace
int		g_render;
int		skipped_frames;
short	g_soundSample;							// value written to speaker	
char	brkPointArray[80];						// array of 4 digit hex address break points
byte	g_joystickByte;							// value read from joystick
int		g_spkrWrites = 0;
int     active_page = 0;						// bank memory page (0 - 255)

FILE			*tape;							// handle to file
char			wavTape[max_path];				// name of wavfile
unsigned long	g_tapeBytesWritten;				// size of wavfile in bytes 
byte			g_tapeByte;						// value read/written to tape
bool			g_tapeplaying = false;			// is tape playing
bool			g_taperecording = false;		// is tape recording

FILE			*prt;							// handle to printer file
char			prtFile[80];					// name of printer file

bool	g_internalMenu = false;					// internal menu display 
char	romFile[max_path];
char	cartridgeFile[max_path];
char	fontFile[max_path];
char	*properties = NULL;						// properties file 
char	pline[80];


PREFS		prefs;
DISPLAY		display;

//-----------------------------------------------------------------------------
//	Screen specific definitions
//-----------------------------------------------------------------------------


// A "pixel" is drawn to the display buffer by assigning a color index from the list below. 
// Eg Display_Buffer[320*y+x] = 3 will set a RED pixel

byte	Display_Buffer[77760];			// 320*243 device independent display buffer

#define M6847_RGB(r,g,b)	((r << 16) | (g << 8) | (b << 0))

#define		VZ_GREEN	0
#define		VZ_YELLOW	1
#define		VZ_BLUE		2
#define		VZ_RED		3
#define		VZ_BUFF		4
#define		VZ_CYAN		5
#define		VZ_MAGENTA	6
#define		VZ_ORANGE	7
#define		VZ_BLACK	8
#define		VZ_DKGREEN	9
#define		VZ_BRGREEN	10
#define		VZ_DKORANGE 11
#define		VZ_BRORANGE 12

// default colour values
long palette[] =
{
	M6847_RGB(0x00, 0xf0, 0x00),	/* GREEN */
	M6847_RGB(0xff, 0xff, 0x00),	/* YELLOW */
	M6847_RGB(0x00, 0x00, 0xf0),	/* BLUE */
	M6847_RGB(0xf0, 0x00, 0x00),	/* RED */
	M6847_RGB(0xff, 0xff, 0xff),	/* BUFF */
	M6847_RGB(0x00, 0xff, 0xff),	/* CYAN */
	M6847_RGB(0xff, 0x00, 0xff),	/* MAGENTA */
	M6847_RGB(0xff, 0x80, 0x00),	/* ORANGE */
	M6847_RGB(0x00, 0x00, 0x00),	/* BLACK */
	M6847_RGB(0x00, 0x40, 0x00),	/* ALPHANUMERIC DARK GREEN */
	M6847_RGB(0xA0, 0xA0, 0xA0),	/* ALPHANUMERIC BRIGHT GREEN */
	M6847_RGB(0x40, 0x10, 0x00),	/* ALPHANUMERIC DARK ORANGE */
	M6847_RGB(0xff, 0xc4, 0x18)		/* ALPHANUMERIC BRIGHT ORANGE */
};

//	config colours
	long RGB_Green;
	long RGB_Yellow;
	long RGB_Blue;
	long RGB_Red;
	long RGB_Buff;
	long RGB_Cyan;
	long RGB_Magenta;
	long RGB_Orange;
	long RGB_Black;
	long RGB_DarkGreen;
	long RGB_BrightGreen;
	long RGB_DarkOrange;
	long RGB_BrightOrange;


byte external_fontdata8x12[12*256];

static byte pal_square_fontdata8x12[] =
{

// standard font, but could be defined as lower case

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

// standard font, but could be defined as uppercase 

0xFF, 0xFF, 0xFF, 0xE3, 0xDD, 0xFD, 0xE5, 0xD5, 0xD5, 0xE3, 0xFF, 0xFF, 
0xFF, 0xFF, 0xFF, 0xF7, 0xEB, 0xDD, 0xDD, 0xC1, 0xDD, 0xDD, 0xFF, 0xFF, 
0xFF, 0xFF, 0xFF, 0xC3, 0xED, 0xED, 0xE3, 0xED, 0xED, 0xC3, 0xFF, 0xFF, 
0xFF, 0xFF, 0xFF, 0xE3, 0xDD, 0xDF, 0xDF, 0xDF, 0xDD, 0xE3, 0xFF, 0xFF, 
0xFF, 0xFF, 0xFF, 0xC3, 0xED, 0xED, 0xED, 0xED, 0xED, 0xC3, 0xFF, 0xFF, 
0xFF, 0xFF, 0xFF, 0xC1, 0xDF, 0xDF, 0xC3, 0xDF, 0xDF, 0xC1, 0xFF, 0xFF, 
0xFF, 0xFF, 0xFF, 0xC1, 0xDF, 0xDF, 0xC3, 0xDF, 0xDF, 0xDF, 0xFF, 0xFF, 
0xFF, 0xFF, 0xFF, 0xE1, 0xDF, 0xDF, 0xD9, 0xDD, 0xDD, 0xE1, 0xFF, 0xFF, 
0xFF, 0xFF, 0xFF, 0xDD, 0xDD, 0xDD, 0xC1, 0xDD, 0xDD, 0xDD, 0xFF, 0xFF, 
0xFF, 0xFF, 0xFF, 0xE3, 0xF7, 0xF7, 0xF7, 0xF7, 0xF7, 0xE3, 0xFF, 0xFF, 
0xFF, 0xFF, 0xFF, 0xFD, 0xFD, 0xFD, 0xFD, 0xDD, 0xDD, 0xE3, 0xFF, 0xFF, 
0xFF, 0xFF, 0xFF, 0xDD, 0xDB, 0xD7, 0xCF, 0xD7, 0xDB, 0xDD, 0xFF, 0xFF, 
0xFF, 0xFF, 0xFF, 0xDF, 0xDF, 0xDF, 0xDF, 0xDF, 0xDF, 0xC1, 0xFF, 0xFF, 
0xFF, 0xFF, 0xFF, 0xDD, 0xC9, 0xD5, 0xD5, 0xDD, 0xDD, 0xDD, 0xFF, 0xFF, 
0xFF, 0xFF, 0xFF, 0xDD, 0xCD, 0xD5, 0xD9, 0xDD, 0xDD, 0xDD, 0xFF, 0xFF, 
0xFF, 0xFF, 0xFF, 0xC1, 0xDD, 0xDD, 0xDD, 0xDD, 0xDD, 0xC1, 0xFF, 0xFF, 
0xFF, 0xFF, 0xFF, 0xC3, 0xDD, 0xDD, 0xC3, 0xDF, 0xDF, 0xDF, 0xFF, 0xFF, 
0xFF, 0xFF, 0xFF, 0xE3, 0xDD, 0xDD, 0xDD, 0xD5, 0xDB, 0xE5, 0xFF, 0xFF, 
0xFF, 0xFF, 0xFF, 0xC3, 0xDD, 0xDD, 0xC3, 0xD7, 0xDB, 0xDD, 0xFF, 0xFF, 
0xFF, 0xFF, 0xFF, 0xE3, 0xDD, 0xEF, 0xF7, 0xFB, 0xDD, 0xE3, 0xFF, 0xFF, 
0xFF, 0xFF, 0xFF, 0xC1, 0xF7, 0xF7, 0xF7, 0xF7, 0xF7, 0xF7, 0xFF, 0xFF, 
0xFF, 0xFF, 0xFF, 0xDD, 0xDD, 0xDD, 0xDD, 0xDD, 0xDD, 0xE3, 0xFF, 0xFF, 
0xFF, 0xFF, 0xFF, 0xDD, 0xDD, 0xDD, 0xEB, 0xEB, 0xF7, 0xF7, 0xFF, 0xFF, 
0xFF, 0xFF, 0xFF, 0xDD, 0xDD, 0xDD, 0xD5, 0xD5, 0xC9, 0xDD, 0xFF, 0xFF, 
0xFF, 0xFF, 0xFF, 0xDD, 0xDD, 0xEB, 0xF7, 0xEB, 0xDD, 0xDD, 0xFF, 0xFF, 
0xFF, 0xFF, 0xFF, 0xDD, 0xDD, 0xEB, 0xF7, 0xF7, 0xF7, 0xF7, 0xFF, 0xFF, 
0xFF, 0xFF, 0xFF, 0xC1, 0xFD, 0xFB, 0xF7, 0xEF, 0xDF, 0xC1, 0xFF, 0xFF, 
0xFF, 0xFF, 0xFF, 0xC7, 0xDF, 0xDF, 0xDF, 0xDF, 0xDF, 0xC7, 0xFF, 0xFF, 
0xFF, 0xFF, 0xFF, 0xDF, 0xDF, 0xEF, 0xF7, 0xFB, 0xFD, 0xFD, 0xFF, 0xFF, 
0xFF, 0xFF, 0xFF, 0xF1, 0xFD, 0xFD, 0xFD, 0xFD, 0xFD, 0xF1, 0xFF, 0xFF, 
0xFF, 0xFF, 0xFF, 0xF7, 0xE3, 0xD5, 0xF7, 0xF7, 0xF7, 0xF7, 0xFF, 0xFF, 
0xFF, 0xFF, 0xFF, 0xFF, 0xF7, 0xEF, 0xC1, 0xEF, 0xF7, 0xFF, 0xFF, 0xFF, 
0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 
0xFF, 0xFF, 0xFF, 0xF7, 0xF7, 0xF7, 0xF7, 0xF7, 0xFF, 0xF7, 0xFF, 0xFF, 
0xFF, 0xFF, 0xFF, 0xEB, 0xEB, 0xEB, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 
0xFF, 0xFF, 0xFF, 0xEB, 0xEB, 0xC9, 0xFF, 0xC9, 0xEB, 0xEB, 0xFF, 0xFF, 
0xFF, 0xFF, 0xFF, 0xF7, 0xE1, 0xDF, 0xE3, 0xFD, 0xC3, 0xF7, 0xFF, 0xFF, 
0xFF, 0xFF, 0xFF, 0xCD, 0xCD, 0xFB, 0xF7, 0xEF, 0xD9, 0xD9, 0xFF, 0xFF, 
0xFF, 0xFF, 0xFF, 0xEF, 0xD7, 0xD7, 0xEF, 0xD5, 0xDB, 0xE5, 0xFF, 0xFF, 
0xFF, 0xFF, 0xFF, 0xE7, 0xE7, 0xE7, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 
0xFF, 0xFF, 0xFF, 0xF7, 0xEF, 0xDF, 0xDF, 0xDF, 0xEF, 0xF7, 0xFF, 0xFF, 
0xFF, 0xFF, 0xFF, 0xF7, 0xFB, 0xFD, 0xFD, 0xFD, 0xFB, 0xF7, 0xFF, 0xFF, 
0xFF, 0xFF, 0xFF, 0xFF, 0xF7, 0xE3, 0xC1, 0xE3, 0xF7, 0xFF, 0xFF, 0xFF, 
0xFF, 0xFF, 0xFF, 0xFF, 0xF7, 0xF7, 0xC1, 0xF7, 0xF7, 0xFF, 0xFF, 0xFF, 
0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xCF, 0xCF, 0xEF, 0xDF, 0xFF, 0xFF, 
0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xC1, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 
0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xCF, 0xCF, 0xFF, 0xFF, 
0xFF, 0xFF, 0xFF, 0xFD, 0xFD, 0xFB, 0xF7, 0xEF, 0xDF, 0xDF, 0xFF, 0xFF, 
0xFF, 0xFF, 0xFF, 0xE7, 0xDB, 0xDB, 0xDB, 0xDB, 0xDB, 0xE7, 0xFF, 0xFF, 
0xFF, 0xFF, 0xFF, 0xF7, 0xE7, 0xF7, 0xF7, 0xF7, 0xF7, 0xE3, 0xFF, 0xFF, 
0xFF, 0xFF, 0xFF, 0xE3, 0xDD, 0xFD, 0xE3, 0xDF, 0xDF, 0xC1, 0xFF, 0xFF, 
0xFF, 0xFF, 0xFF, 0xE3, 0xDD, 0xFD, 0xF3, 0xFD, 0xDD, 0xE3, 0xFF, 0xFF, 
0xFF, 0xFF, 0xFF, 0xFB, 0xF3, 0xEB, 0xC1, 0xFB, 0xFB, 0xFB, 0xFF, 0xFF, 
0xFF, 0xFF, 0xFF, 0xC1, 0xDF, 0xC3, 0xFD, 0xFD, 0xDD, 0xE3, 0xFF, 0xFF, 
0xFF, 0xFF, 0xFF, 0xE3, 0xDF, 0xDF, 0xC3, 0xDD, 0xDD, 0xE3, 0xFF, 0xFF, 
0xFF, 0xFF, 0xFF, 0xC1, 0xFD, 0xFB, 0xF7, 0xEF, 0xDF, 0xDF, 0xFF, 0xFF, 
0xFF, 0xFF, 0xFF, 0xE3, 0xDD, 0xDD, 0xE3, 0xDD, 0xDD, 0xE3, 0xFF, 0xFF, 
0xFF, 0xFF, 0xFF, 0xE3, 0xDD, 0xDD, 0xE1, 0xFD, 0xFD, 0xE3, 0xFF, 0xFF, 
0xFF, 0xFF, 0xFF, 0xFF, 0xE7, 0xE7, 0xFF, 0xE7, 0xE7, 0xFF, 0xFF, 0xFF, 
0xFF, 0xFF, 0xFF, 0xE7, 0xE7, 0xFF, 0xE7, 0xE7, 0xF7, 0xEF, 0xFF, 0xFF, 
0xFF, 0xFF, 0xFF, 0xFB, 0xF7, 0xEF, 0xDF, 0xEF, 0xF7, 0xFB, 0xFF, 0xFF, 
0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xC1, 0xFF, 0xC1, 0xFF, 0xFF, 0xFF, 0xFF, 
0xFF, 0xFF, 0xFF, 0xEF, 0xF7, 0xFB, 0xFD, 0xFB, 0xF7, 0xEF, 0xFF, 0xFF, 
0xFF, 0xFF, 0xFF, 0xE7, 0xDB, 0xFB, 0xF7, 0xF7, 0xFF, 0xF7, 0xFF, 0xFF, 

// semigraphics - repeating block 1

0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 
0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x0F, 0x0F, 0x0F, 0x0F, 0x0F, 0x0F, 
0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0xF0, 0xF0, 0xF0, 0xF0, 0xF0, 0xF0, 
0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 
0x0F, 0x0F, 0x0F, 0x0F, 0x0F, 0x0F, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 
0x0F, 0x0F, 0x0F, 0x0F, 0x0F, 0x0F, 0x0F, 0x0F, 0x0F, 0x0F, 0x0F, 0x0F, 
0x0F, 0x0F, 0x0F, 0x0F, 0x0F, 0x0F, 0xF0, 0xF0, 0xF0, 0xF0, 0xF0, 0xF0, 
0x0F, 0x0F, 0x0F, 0x0F, 0x0F, 0x0F, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 
0xF0, 0xF0, 0xF0, 0xF0, 0xF0, 0xF0, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 
0xF0, 0xF0, 0xF0, 0xF0, 0xF0, 0xF0, 0x0F, 0x0F, 0x0F, 0x0F, 0x0F, 0x0F, 
0xF0, 0xF0, 0xF0, 0xF0, 0xF0, 0xF0, 0xF0, 0xF0, 0xF0, 0xF0, 0xF0, 0xF0, 
0xF0, 0xF0, 0xF0, 0xF0, 0xF0, 0xF0, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 
0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 
0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0x0F, 0x0F, 0x0F, 0x0F, 0x0F, 0x0F, 
0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xF0, 0xF0, 0xF0, 0xF0, 0xF0, 0xF0, 
0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 

// semigraphics - repeating block 2

0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 
0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x0F, 0x0F, 0x0F, 0x0F, 0x0F, 0x0F, 
0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0xF0, 0xF0, 0xF0, 0xF0, 0xF0, 0xF0, 
0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 
0x0F, 0x0F, 0x0F, 0x0F, 0x0F, 0x0F, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 
0x0F, 0x0F, 0x0F, 0x0F, 0x0F, 0x0F, 0x0F, 0x0F, 0x0F, 0x0F, 0x0F, 0x0F, 
0x0F, 0x0F, 0x0F, 0x0F, 0x0F, 0x0F, 0xF0, 0xF0, 0xF0, 0xF0, 0xF0, 0xF0, 
0x0F, 0x0F, 0x0F, 0x0F, 0x0F, 0x0F, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 
0xF0, 0xF0, 0xF0, 0xF0, 0xF0, 0xF0, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 
0xF0, 0xF0, 0xF0, 0xF0, 0xF0, 0xF0, 0x0F, 0x0F, 0x0F, 0x0F, 0x0F, 0x0F, 
0xF0, 0xF0, 0xF0, 0xF0, 0xF0, 0xF0, 0xF0, 0xF0, 0xF0, 0xF0, 0xF0, 0xF0, 
0xF0, 0xF0, 0xF0, 0xF0, 0xF0, 0xF0, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 
0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 
0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0x0F, 0x0F, 0x0F, 0x0F, 0x0F, 0x0F, 
0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xF0, 0xF0, 0xF0, 0xF0, 0xF0, 0xF0, 
0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 

// semigraphics - repeating block 3

0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 
0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x0F, 0x0F, 0x0F, 0x0F, 0x0F, 0x0F, 
0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0xF0, 0xF0, 0xF0, 0xF0, 0xF0, 0xF0, 
0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 
0x0F, 0x0F, 0x0F, 0x0F, 0x0F, 0x0F, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 
0x0F, 0x0F, 0x0F, 0x0F, 0x0F, 0x0F, 0x0F, 0x0F, 0x0F, 0x0F, 0x0F, 0x0F, 
0x0F, 0x0F, 0x0F, 0x0F, 0x0F, 0x0F, 0xF0, 0xF0, 0xF0, 0xF0, 0xF0, 0xF0, 
0x0F, 0x0F, 0x0F, 0x0F, 0x0F, 0x0F, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 
0xF0, 0xF0, 0xF0, 0xF0, 0xF0, 0xF0, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 
0xF0, 0xF0, 0xF0, 0xF0, 0xF0, 0xF0, 0x0F, 0x0F, 0x0F, 0x0F, 0x0F, 0x0F, 
0xF0, 0xF0, 0xF0, 0xF0, 0xF0, 0xF0, 0xF0, 0xF0, 0xF0, 0xF0, 0xF0, 0xF0, 
0xF0, 0xF0, 0xF0, 0xF0, 0xF0, 0xF0, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 
0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 
0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0x0F, 0x0F, 0x0F, 0x0F, 0x0F, 0x0F, 
0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xF0, 0xF0, 0xF0, 0xF0, 0xF0, 0xF0, 
0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 

// semigraphics - repeating block 4

0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 
0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x0F, 0x0F, 0x0F, 0x0F, 0x0F, 0x0F, 
0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0xF0, 0xF0, 0xF0, 0xF0, 0xF0, 0xF0, 
0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 
0x0F, 0x0F, 0x0F, 0x0F, 0x0F, 0x0F, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 
0x0F, 0x0F, 0x0F, 0x0F, 0x0F, 0x0F, 0x0F, 0x0F, 0x0F, 0x0F, 0x0F, 0x0F, 
0x0F, 0x0F, 0x0F, 0x0F, 0x0F, 0x0F, 0xF0, 0xF0, 0xF0, 0xF0, 0xF0, 0xF0, 
0x0F, 0x0F, 0x0F, 0x0F, 0x0F, 0x0F, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 
0xF0, 0xF0, 0xF0, 0xF0, 0xF0, 0xF0, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 
0xF0, 0xF0, 0xF0, 0xF0, 0xF0, 0xF0, 0x0F, 0x0F, 0x0F, 0x0F, 0x0F, 0x0F, 
0xF0, 0xF0, 0xF0, 0xF0, 0xF0, 0xF0, 0xF0, 0xF0, 0xF0, 0xF0, 0xF0, 0xF0, 
0xF0, 0xF0, 0xF0, 0xF0, 0xF0, 0xF0, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 
0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 
0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0x0F, 0x0F, 0x0F, 0x0F, 0x0F, 0x0F, 
0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xF0, 0xF0, 0xF0, 0xF0, 0xF0, 0xF0, 
0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 

// semigraphics - repeating block 5

0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 
0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x0F, 0x0F, 0x0F, 0x0F, 0x0F, 0x0F, 
0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0xF0, 0xF0, 0xF0, 0xF0, 0xF0, 0xF0, 
0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 
0x0F, 0x0F, 0x0F, 0x0F, 0x0F, 0x0F, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 
0x0F, 0x0F, 0x0F, 0x0F, 0x0F, 0x0F, 0x0F, 0x0F, 0x0F, 0x0F, 0x0F, 0x0F, 
0x0F, 0x0F, 0x0F, 0x0F, 0x0F, 0x0F, 0xF0, 0xF0, 0xF0, 0xF0, 0xF0, 0xF0, 
0x0F, 0x0F, 0x0F, 0x0F, 0x0F, 0x0F, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 
0xF0, 0xF0, 0xF0, 0xF0, 0xF0, 0xF0, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 
0xF0, 0xF0, 0xF0, 0xF0, 0xF0, 0xF0, 0x0F, 0x0F, 0x0F, 0x0F, 0x0F, 0x0F, 
0xF0, 0xF0, 0xF0, 0xF0, 0xF0, 0xF0, 0xF0, 0xF0, 0xF0, 0xF0, 0xF0, 0xF0, 
0xF0, 0xF0, 0xF0, 0xF0, 0xF0, 0xF0, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 
0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 
0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0x0F, 0x0F, 0x0F, 0x0F, 0x0F, 0x0F, 
0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xF0, 0xF0, 0xF0, 0xF0, 0xF0, 0xF0, 
0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 

// semigraphics - repeating block 6

0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 
0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x0F, 0x0F, 0x0F, 0x0F, 0x0F, 0x0F, 
0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0xF0, 0xF0, 0xF0, 0xF0, 0xF0, 0xF0, 
0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 
0x0F, 0x0F, 0x0F, 0x0F, 0x0F, 0x0F, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 
0x0F, 0x0F, 0x0F, 0x0F, 0x0F, 0x0F, 0x0F, 0x0F, 0x0F, 0x0F, 0x0F, 0x0F, 
0x0F, 0x0F, 0x0F, 0x0F, 0x0F, 0x0F, 0xF0, 0xF0, 0xF0, 0xF0, 0xF0, 0xF0, 
0x0F, 0x0F, 0x0F, 0x0F, 0x0F, 0x0F, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 
0xF0, 0xF0, 0xF0, 0xF0, 0xF0, 0xF0, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 
0xF0, 0xF0, 0xF0, 0xF0, 0xF0, 0xF0, 0x0F, 0x0F, 0x0F, 0x0F, 0x0F, 0x0F, 
0xF0, 0xF0, 0xF0, 0xF0, 0xF0, 0xF0, 0xF0, 0xF0, 0xF0, 0xF0, 0xF0, 0xF0, 
0xF0, 0xF0, 0xF0, 0xF0, 0xF0, 0xF0, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 
0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 
0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0x0F, 0x0F, 0x0F, 0x0F, 0x0F, 0x0F, 
0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xF0, 0xF0, 0xF0, 0xF0, 0xF0, 0xF0, 
0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 

// semigraphics - repeating block 7

0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 
0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x0F, 0x0F, 0x0F, 0x0F, 0x0F, 0x0F, 
0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0xF0, 0xF0, 0xF0, 0xF0, 0xF0, 0xF0, 
0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 
0x0F, 0x0F, 0x0F, 0x0F, 0x0F, 0x0F, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 
0x0F, 0x0F, 0x0F, 0x0F, 0x0F, 0x0F, 0x0F, 0x0F, 0x0F, 0x0F, 0x0F, 0x0F, 
0x0F, 0x0F, 0x0F, 0x0F, 0x0F, 0x0F, 0xF0, 0xF0, 0xF0, 0xF0, 0xF0, 0xF0, 
0x0F, 0x0F, 0x0F, 0x0F, 0x0F, 0x0F, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 
0xF0, 0xF0, 0xF0, 0xF0, 0xF0, 0xF0, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 
0xF0, 0xF0, 0xF0, 0xF0, 0xF0, 0xF0, 0x0F, 0x0F, 0x0F, 0x0F, 0x0F, 0x0F, 
0xF0, 0xF0, 0xF0, 0xF0, 0xF0, 0xF0, 0xF0, 0xF0, 0xF0, 0xF0, 0xF0, 0xF0, 
0xF0, 0xF0, 0xF0, 0xF0, 0xF0, 0xF0, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 
0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 
0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0x0F, 0x0F, 0x0F, 0x0F, 0x0F, 0x0F, 
0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xF0, 0xF0, 0xF0, 0xF0, 0xF0, 0xF0, 
0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 

// semigraphics - repeating block 8

0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 
0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x0F, 0x0F, 0x0F, 0x0F, 0x0F, 0x0F, 
0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0xF0, 0xF0, 0xF0, 0xF0, 0xF0, 0xF0, 
0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 
0x0F, 0x0F, 0x0F, 0x0F, 0x0F, 0x0F, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 
0x0F, 0x0F, 0x0F, 0x0F, 0x0F, 0x0F, 0x0F, 0x0F, 0x0F, 0x0F, 0x0F, 0x0F, 
0x0F, 0x0F, 0x0F, 0x0F, 0x0F, 0x0F, 0xF0, 0xF0, 0xF0, 0xF0, 0xF0, 0xF0, 
0x0F, 0x0F, 0x0F, 0x0F, 0x0F, 0x0F, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 
0xF0, 0xF0, 0xF0, 0xF0, 0xF0, 0xF0, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 
0xF0, 0xF0, 0xF0, 0xF0, 0xF0, 0xF0, 0x0F, 0x0F, 0x0F, 0x0F, 0x0F, 0x0F, 
0xF0, 0xF0, 0xF0, 0xF0, 0xF0, 0xF0, 0xF0, 0xF0, 0xF0, 0xF0, 0xF0, 0xF0, 
0xF0, 0xF0, 0xF0, 0xF0, 0xF0, 0xF0, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 
0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 
0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0x0F, 0x0F, 0x0F, 0x0F, 0x0F, 0x0F, 
0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xF0, 0xF0, 0xF0, 0xF0, 0xF0, 0xF0, 
0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 

};

char* 	getProperty(char *attribute)
/**************************************************************************
*
* Name:			getProperty
* Purpose:		return an attribute value  
* Arguments: 	pointer to a properties file stored in memory, attribute to
*				search			
*
***************************************************************************/
{
	char *attr = NULL;
	char *value = NULL;
	char *retv = NULL;
	char *spos = NULL;

	spos = strstr(properties,attribute);

	if (spos != NULL)
	{
		for (int n=0;n<80;n++)
		{
			if ((*spos == '\r') ||
			    (*spos == '\n') ||
			    (*spos == '#'))
			{
				pline[n] = '\0';
				break;
			}
			else 
			{
				pline[n] = *spos;
			}
			spos++;
		}

		attr = strtok(pline,"=");
		value = strtok(NULL,"=");
		//retv = (char*) malloc(strlen(value) * sizeof(char));
		strcpy(pline,value);
		retv = pline;
	}
	return retv;
}


//---------------------------------------------------------------
//
//               VZDIO Disk Emulation Routines
//
// These routines are taken from the video technology MESS driver
// by Juergen Buchmueller
//
//---------------------------------------------------------------


#define TRKSIZE_VZ      0x9b0   
#define TRKSIZE_FM      3172    /* size of a standard FM mode track */

FILE *vtech1_fdc_file[3] = {NULL, NULL};
byte vtech1_track_x2[2] = {80, 80};
byte vtech1_fdc_wrprot[2] = {0x80, 0x80};
byte vtech1_fdc_status = 0;
byte vtech1_fdc_data[TRKSIZE_FM];

static int vtech1_data;

static int vtech1_fdc_edge = 0;
static int vtech1_fdc_bits = 8;
static int vtech1_drive = -1;
static int vtech1_fdc_start = 0;
static int vtech1_fdc_write = 0;
static int vtech1_fdc_offs = 0;
static int vtech1_fdc_latch = 0;


void vtech1_floppy_remove(int id)
{
   if( vtech1_fdc_file[id] )
	   fclose(vtech1_fdc_file[id]);
   vtech1_fdc_file[id] = NULL;
}

int vtech1_floppy_init(int id, char *s)
{
    /* first try to open existing image RW */
    vtech1_fdc_wrprot[id] = 0x00;
    vtech1_fdc_file[id] = fopen(s,"rb+");
    /* failed? */
    if( !vtech1_fdc_file[id] )
    {
	 /* try to open existing image RO */
	 vtech1_fdc_wrprot[id] = 0x80;
         vtech1_fdc_file[id] = fopen(s,"rb");
    }
	
    /* failed? */
    if( !vtech1_fdc_file[id] )
    {
	 /* create new image RW */
	 vtech1_fdc_wrprot[id] = 0x00;
         vtech1_fdc_file[id] = fopen(s,"wb+");
    }
    if( vtech1_fdc_file[id] )
	{
		vtech1_track_x2[id] = 0;
		return 0;
	}
	/* failed permanently */
    return 1;
}

void vtech1_floppy_exit(int id)
{
   if( vtech1_fdc_file[id] )
	   fclose(vtech1_fdc_file[id]);
   vtech1_fdc_file[id] = NULL;
}

static void vtech1_get_track(void)
{
    /* drive selected or and image file ok? */
	int size, offs, r;
	FILE *f;
	if( vtech1_drive >= 0 && vtech1_fdc_file[vtech1_drive] != NULL )
    {
		size = TRKSIZE_VZ;
		offs = TRKSIZE_VZ * vtech1_track_x2[vtech1_drive]/2;
		r = fseek(vtech1_fdc_file[vtech1_drive], offs, SEEK_SET);
		f = vtech1_fdc_file[vtech1_drive];
		size = fread(&vtech1_fdc_data, size, 1, f);
		if (prefs.diskAudio) osd_PlayTrack();

		char TS[16];
		sprintf(TS,"  T:%02d S:%02d",
			VZMEM[Regs.IY.W+18],
			VZMEM[Regs.IY.W+17]);
		osd_DiskStatus(vtech1_drive, TS);

    }
	else
	{
		for (int n=0;n < TRKSIZE_VZ; n++) vtech1_fdc_data[n] = 0x00;
	}
    vtech1_fdc_offs = 0;
    vtech1_fdc_write = 0;
}

static void vtech1_put_track(void)
{
    /* drive selected and image file ok? */
    if( vtech1_drive >= 0 && vtech1_fdc_file[vtech1_drive] != NULL )
    {
		int size, offs;
		offs = TRKSIZE_VZ * vtech1_track_x2[vtech1_drive]/2;
		fseek(vtech1_fdc_file[vtech1_drive], offs + vtech1_fdc_start, SEEK_SET);
		size = fwrite(&vtech1_fdc_data[vtech1_fdc_start],1, vtech1_fdc_write,vtech1_fdc_file[vtech1_drive]);
		if (prefs.diskAudio) osd_PlayTrack();

		char TS[16];
		sprintf(TS,"  T:%02d S:%02d",
			VZMEM[Regs.IY.W+18],
			VZMEM[Regs.IY.W+17]);
		osd_DiskStatus(vtech1_drive, TS);

	}
	else
	{
		for (int n=0;n < TRKSIZE_VZ; n++) vtech1_fdc_data[n] = 0x00;
	}
}

#define PHI0(n) (((n)>>0)&1)
#define PHI1(n) (((n)>>1)&1)
#define PHI2(n) (((n)>>2)&1)
#define PHI3(n) (((n)>>3)&1)

int vtech1_fdc_r(int offset)
{
    int data = 0xff;
    switch( offset )
    {
    case 1: /* data (read-only) */
	   if( vtech1_fdc_bits > 0 )
	   {
	       if( vtech1_fdc_status & 0x80 )
		   vtech1_fdc_bits--;
	       data = (vtech1_data >> vtech1_fdc_bits) & 0xff;
	   }
	   if( vtech1_fdc_bits == 0 )
	   {
	       vtech1_data = vtech1_fdc_data[vtech1_fdc_offs];
	       if( vtech1_fdc_status & 0x80 )
	       {
		   vtech1_fdc_bits = 8;
		   vtech1_fdc_offs = (vtech1_fdc_offs + 1)  % TRKSIZE_FM;
	       }
	       vtech1_fdc_status &= ~0x80;
	   }
	   break;
    case 2: /* polling (read-only) */
	   /* fake */
	   if( vtech1_drive >= 0 )
	       vtech1_fdc_status |= 0x80;
	   data = vtech1_fdc_status;
	   break;
    case 3: /* write protect status (read-only) */
	   if( vtech1_drive >= 0 )
	       data = vtech1_fdc_wrprot[vtech1_drive];
	   break;
    }
    return data;
}


void vtech1_fdc_w(int offset, int data)

{
	int drive;

    switch (offset)
	{
	case 0: /* latch (write-only) */
		drive = (data & 0x10) ? 0 : (data & 0x80) ? 1 : -1;
		if (drive != vtech1_drive)
		{
			vtech1_drive = drive;
			if (vtech1_drive >= 0)
				vtech1_get_track();
        }
		if (vtech1_drive >= 0)
        {
			if ((PHI0(data) && !(PHI1(data) || PHI2(data) || PHI3(data)) && PHI1(vtech1_fdc_latch)) ||
				(PHI1(data) && !(PHI0(data) || PHI2(data) || PHI3(data)) && PHI2(vtech1_fdc_latch)) ||
				(PHI2(data) && !(PHI0(data) || PHI1(data) || PHI3(data)) && PHI3(vtech1_fdc_latch)) ||
				(PHI3(data) && !(PHI0(data) || PHI1(data) || PHI2(data)) && PHI0(vtech1_fdc_latch)))
            {
				if (vtech1_track_x2[vtech1_drive] > 0)
				{
					vtech1_track_x2[vtech1_drive]--;
				}
				if ((vtech1_track_x2[vtech1_drive] & 1) == 0)
				{
					vtech1_get_track();
					if (prefs.diskAudio) osd_PlayStepper();
				}
            }
            else
			if ((PHI0(data) && !(PHI1(data) || PHI2(data) || PHI3(data)) && PHI3(vtech1_fdc_latch)) ||
				(PHI1(data) && !(PHI0(data) || PHI2(data) || PHI3(data)) && PHI0(vtech1_fdc_latch)) ||
				(PHI2(data) && !(PHI0(data) || PHI1(data) || PHI3(data)) && PHI1(vtech1_fdc_latch)) ||
				(PHI3(data) && !(PHI0(data) || PHI1(data) || PHI2(data)) && PHI2(vtech1_fdc_latch)))
            {
				if (vtech1_track_x2[vtech1_drive] < 2*40)
				{
					vtech1_track_x2[vtech1_drive]++;
				}
				if ((vtech1_track_x2[vtech1_drive] & 1) == 0)
				{
					vtech1_get_track();
					if (prefs.diskAudio) osd_PlayStepper();
				}
            }
            if ((data & 0x40) == 0)
			{
				vtech1_data <<= 1;
				if ((vtech1_fdc_latch ^ data) & 0x20)
					vtech1_data |= 1;
				if ((vtech1_fdc_edge ^= 1) == 0)
                {
					if (--vtech1_fdc_bits == 0)
					{
						byte value = 0;
						vtech1_data &= 0xffff;
						if (vtech1_data & 0x4000 ) value |= 0x80;
						if (vtech1_data & 0x1000 ) value |= 0x40;
						if (vtech1_data & 0x0400 ) value |= 0x20;
						if (vtech1_data & 0x0100 ) value |= 0x10;
						if (vtech1_data & 0x0040 ) value |= 0x08;
						if (vtech1_data & 0x0010 ) value |= 0x04;
						if (vtech1_data & 0x0004 ) value |= 0x02;
						if (vtech1_data & 0x0001 ) value |= 0x01;
						vtech1_fdc_data[vtech1_fdc_offs] = value;
						vtech1_fdc_offs = (vtech1_fdc_offs + 1) % TRKSIZE_FM;
						vtech1_fdc_write++;
						vtech1_fdc_bits = 8;
					}
                }
            }
			/* change of write signal? */
			if ((vtech1_fdc_latch ^ data) & 0x40)
            {
                /* falling edge? */
				if (vtech1_fdc_latch & 0x40)
                {
					vtech1_fdc_start = vtech1_fdc_offs;
					vtech1_fdc_edge = 0;
                }
                else
                {
                    /* data written to track before? */
					if (vtech1_fdc_write)
						vtech1_put_track();
                }
				vtech1_fdc_bits = 8;
				vtech1_fdc_write = 0;
            }
        }
		vtech1_fdc_latch = data;
		break;
    }
}

void Disassemble(char *disText, int startAddr, int endAddr)
{
	char	line[20];
	char	addr[8];
	char	bytes[8];
	int		n,c;
	int		offset = 0;
	byte	v;
	
	strcpy(disText,"");
	while (startAddr+offset <= endAddr)
	{
		strcpy(line, "");
		sprintf(addr, "%04X",startAddr+offset);
		n = DAsm(line, &RAM[startAddr+offset]);
		strcat(disText, addr);
		strcat(disText, "  ");
		for (c=0; c<3 ;c++)
		{
			if (c<n)
			{
				v = RAM[startAddr+offset+c];
				sprintf(bytes, "%02X", v);
			} 
			else
			{
				sprintf(bytes, "  ");
			}
			strcat(disText, bytes);
			strcat(disText, " ");
		}
		strcat(disText, "   ");
		strcat(disText, line);
		strcat(disText,"\r\n");
		offset += n;
	}
}



void GetDisassembly(char *disText, char *regText, char *ptrText, char *hwText, char *brkText)
{
	char	line[20];
	char	addr[8];
	char	bytes[8];
	int		i,n,c;
	int		offset = 0;
	byte	b,v;
	int		value;
	
	for (i=0;i<80;i++) brkText[i] = brkPointArray[i];
	strcpy(disText,"");
	for (i=0;i<18;i++)
	{
		strcpy(line, "");
		sprintf(addr, "%04X",Regs.PC.W+offset);
		n = DAsm(line, &RAM[Regs.PC.W+offset]);
		strcat(disText, addr);
		strcat(disText, "  ");
		for (c=0; c<3 ;c++)
		{
			if (c<n)
			{
				v = RAM[Regs.PC.W+offset+c];
				sprintf(bytes, "%02X", v);
			} 
			else
			{
				sprintf(bytes, "  ");
			}
			strcat(disText, bytes);
			strcat(disText, " ");
		}
		strcat(disText, "   ");
		strcat(disText, line);
		strcat(disText,"\r\n");
		offset += n;
	}
	// Registers

	strcpy(regText,"");
	strcat(regText,"AF  ");	sprintf(addr, "%04X  ",Regs.AF.W); strcat(regText, addr);
	strcat(regText,"AF' ");	sprintf(addr, "%04X\r\n",Regs.AF1.W); strcat(regText, addr);

	strcat(regText,"BC  ");	sprintf(addr, "%04X  ",Regs.BC.W); strcat(regText, addr);
	strcat(regText,"BC' ");	sprintf(addr, "%04X\r\n",Regs.BC1.W); strcat(regText, addr);

	strcat(regText,"DE  ");	sprintf(addr, "%04X  ",Regs.DE.W); strcat(regText, addr);
	strcat(regText,"DE' ");	sprintf(addr, "%04X\r\n",Regs.DE1.W); strcat(regText, addr);

	strcat(regText,"HL  ");	sprintf(addr, "%04X  ",Regs.HL.W); strcat(regText, addr);
	strcat(regText,"HL' ");	sprintf(addr, "%04X\r\n",Regs.HL1.W); strcat(regText, addr);

	strcat(regText,"IX  ");	sprintf(addr, "%04X\r\n",Regs.IX.W); strcat(regText, addr);
	strcat(regText,"IY  ");	sprintf(addr, "%04X\r\n",Regs.IY.W); strcat(regText, addr);
	strcat(regText,"PC  ");	sprintf(addr, "%04X\r\n",Regs.PC.W); strcat(regText, addr);
	strcat(regText,"SP  ");	sprintf(addr, "%04X\r\n",Regs.SP.W); strcat(regText, addr);
	strcat(regText,"\r\n");
	strcat(regText,"Flags:  SZ.A.PNC\r\n");
	v = Regs.AF.B.l;
	b = 0x80;
	strcpy(ptrText,"");
	for (int m = 0; m<8; m++)
	{
		if ((v & b) == b)
			strcat(ptrText, "1");
		else
			strcat(ptrText,"0");
		b = (b >> 1);
	}
	strcat(regText,"        ");
	strcat(regText,ptrText);
	strcpy(ptrText,"");
	
	// Pointers

	strcpy(ptrText,"");
	strcpy(ptrText, "Start of Basic: ");
	value = RAM[30884] + 256 * RAM[30885];
	sprintf(line, "%04X\r\n", value);
	strcat(ptrText, line);

	strcat(ptrText, "End of Basic  : ");
	value = RAM[30969] + 256 * RAM[30970];
	sprintf(line, "%04X\r\n", value);
	strcat(ptrText, line);

	strcat(ptrText, "Top of Memory : ");
	value = RAM[30897] + 256 * RAM[30898];
	sprintf(line, "%04X\r\n\r\n", value);
	strcat(ptrText, line);

	// Hardware

	strcpy(hwText, "Scanline   :  ");
	sprintf(line, "%2d\r\n", g_scanline);
	strcat(hwText, line);

	strcat(hwText, "Intr Cycles:  ");
	sprintf(line, "%2d\r\n", Regs.ICount);
	strcat(hwText, line);
	
	strcat(hwText, "\r\n");
	strcat(hwText, "Latch:  ..SVVCCS \r\n");
	v = RAM[30779];
	b = 0x80;
	strcpy(line,"");
	for (int m = 0; m<8; m++)
	{
		if ((v & b) == b)
			strcat(line, "1");
		else
			strcat(line,"0");
		b = (b >> 1);
	}
	strcat(hwText,"        ");
	strcat(hwText,line);
}

void LoadFont(byte *fileBuffer)
/**************************************************************************
*
* Name:			LoadFont
* Purpose:		Load an external font file. Only valid for Australian mod
* Arguments: 	pointer to font data 
*
***************************************************************************/

{
	// 64 alpha characters + 16 semigraphic characters
	for (int i=0;i<3072;i++)
	{
		external_fontdata8x12[i] = fileBuffer[i];
	}
}

void LoadVZFilename(char *s)
/**************************************************************************
*
* Name:			LoadVZFile
* Purpose:		Load a .vz snapshot into memory
* Arguments: 	pointer to filename
*
***************************************************************************/
{
	FILE *vzfile;
	unsigned short n, start_addr;
	byte b;


	VZFILE vzf;

	vzfile = fopen(s, "rb");
	if (!vzfile)
	{
		return;
	}


	// read the vzheader

	fread(&vzf, sizeof(vzf), 1, vzfile);
	start_addr = vzf.start_addrh;
	start_addr = start_addr * 256 + vzf.start_addrl;


	n = 0;
	fread(&b, 1, 1, vzfile);
	while (!feof(vzfile))
	{
		VZMEM[start_addr + n] = b;
		n++;
		fread(&b, 1, 1, vzfile);
	}
	fclose(vzfile);

	//
	// Basic programs cannot be loaded at startup because the basic pointers
	// are overwritten when the rom is loaded
	//
	if (vzf.ftype == 0xF0)
	{
		VZMEM[0x78A4] = vzf.start_addrl;
		VZMEM[0x78A5] = vzf.start_addrh;
		VZMEM[0x78F9] = ((start_addr + n) & 0x00FF);
		VZMEM[0x78FA] = (((start_addr + n) & 0xFF00) >> 8);
	}


	if (vzf.ftype == 0xF1)
	{
		Regs.PC.B.l = vzf.start_addrl;
		Regs.PC.B.h = vzf.start_addrh;
	}
}



void LoadVZFile(byte *fileBuffer, int fileLength, VZFILE *vzf)
{
		unsigned short	n,start_addr;

		start_addr = vzf->start_addrh; 
		start_addr = start_addr * 256 + vzf->start_addrl;
		for (n=0;n < fileLength; n++) RAM[start_addr + n] = fileBuffer[n+24];

	if (vzf->ftype == 0xF1)
	{
		Regs.PC.B.l = vzf->start_addrl;
		Regs.PC.B.h = vzf->start_addrh;
	}
	if (vzf->ftype == 0xF0)
	{
		RAM[0x78A4] = vzf->start_addrl;
		RAM[0x78A5] = vzf->start_addrh;
		RAM[0x78F9] = ((start_addr+n) & 0x00FF);
		RAM[0x78FA] = (((start_addr+n) & 0xFF00) >> 8);
	}

}


byte readVZKbrd(int A)
{
	byte	k = 0xFF;

	if (!(A & 0x0001)) k &= vzkbrd[0];
	if (!(A & 0x0002)) k &= vzkbrd[1];
	if (!(A & 0x0004)) k &= vzkbrd[2];
	if (!(A & 0x0008)) k &= vzkbrd[3];
	if (!(A & 0x0010)) k &= vzkbrd[4];
	if (!(A & 0x0020)) k &= vzkbrd[5];
	if (!(A & 0x0040)) k &= vzkbrd[6];
	if (!(A & 0x0080)) k &= vzkbrd[7];

	return k;
}


byte RdZ80(word A)
{
	byte	b;

	b = RAM[A];

	if ((A > 0x67FF) && (A < 0x7000))		// reading keyboard
	{
		b = readVZKbrd(A);
		
		if (g_scanline > display.OFF) 
		{
			b = (b & 0x7f);					// retrace 
		}
		if (g_tapeByte >  0xB5)				// best result for 8 bit unsigned 22050 wav file
		{
			b = (b & 0xbf);
		}
	}

    if ((A > 0x6FFF) && (A < 0x7800))		// screen memory
    {
		if (prefs.gfx > 0)
		{
			int bank = gfx_mod_latch & 0x03;
			b = GXMEM[A-0x7000+bank*0x800];
		}
	}

	if (A > prefs.top_of_memory)
	{
		b = 0x7f;
	}

    return b;
}


void WrZ80(word A, byte v)
{
	// German GFX Mod writes to 30779
	if (prefs.gfx == 2)
	{
		if (A == 0x783B)
		{
			if ((v & 0x08) == 0x08)
			{
				gfx_mod_latch |= 28;
				g_render = 4; 
			}
		}
	}

    //
    // writing to screen memory
    //
    if ((A > 0x6FFF) && (A < 0x7800))
    {
		g_render = 4; 
		//
		// produce snow if display active.
		// Note. could be in the middle of an LDIR, so g_scanline
		// may not exactly match calculated scanline
		//
	
		if (prefs.snow == 1)
		{
			if (v != 0x00) 
			{
				int display_start = display.lines_blanking + 
									display.lines_dummy_top +
									display.lines_top_border;

				int elapsedCycles = Regs.IPeriod - Regs.ICount;
				int scanline = (elapsedCycles / CYCLES_PER_SCANLINE);
				int cyclesInScanline = elapsedCycles - (scanline * CYCLES_PER_SCANLINE);

				if ((scanline >= display_start) && (scanline <= display.OFF))
				{
					float pos = ((float)cyclesInScanline) / CYCLES_PER_SCANLINE;
					int xpos = (int)(pos*32);
					int offs = 320*(scanline-(display.lines_blanking + display.lines_dummy_top))+32+xpos*8;

					for (int x=0;x<8;x++)
					{
						Display_Buffer[offs+x] = VZ_BRGREEN;
					}
				}
			}
		}
		if (prefs.gfx > 0)
		{
			int bank = gfx_mod_latch & 0x03;
			GXMEM[A-0x7000+bank*0x800] = v;
		}
    }               

	// trap writes to ROM or cartridge memory

	if ((A < 0x4000) && (prefs.rom_writes == 1))
	{
		RAM[A] = v;
	}

	if ((A > 0x3FFF) && (A < 0x6800) && (prefs.cartridge_writes == 1))
	{
		RAM[A] = v;
	}


	if ((A > 0x67FF) && (A <= prefs.top_of_memory))  
	{
		RAM[A] = v;
	}

	// writing to latch
	if ((A > 0x67FF) && (A < 0x7000))
	{
		// speaker bits toggle? 
		if( (vz_latch ^ v ) & 0x21 )
		{
	        g_soundSample = SOUND_LEVELS[(v & 0x01) | ((v >> 4) & 0x02)];
		}

		if( (vz_latch ^ v ) & 0x06 )  
		{
			
			short tapeSample = SOUND_LEVELS[((v >> 1) & 0x01) | ((v >> 1) & 0x02)];
			switch (tapeSample)
			{
				case 32767:		g_tapeByte = 0xFF;
								break;
				case -32768:	g_tapeByte = 0x00;
								break;
			}
			WriteCassette(g_tapeByte);
		}

		// If you change the screen mode during the retrace period
		// it won't come into effect until the display is turned back on.
		// So the top border will not be drawn in the new mode

		// calculate the scanline accurately, as g_scanline will not be accurate
		// during an LDIR

		if ((vz_latch & 0x08) != (v & 0x08))
		{
			g_render = 4;

			const int scanline = (Regs.IPeriod - Regs.ICount) / CYCLES_PER_SCANLINE;
			if ((scanline >= display.FS) ||
				(scanline < (display.lines_blanking + display.lines_top_border)))
			{
				g_updateborder = vz_latch;
			}
            // are we changing graphics modes while the display is being drawn 
            // switching from text to graphics 
            if ((g_scanline > display.lines_blanking + display.lines_dummy_top + display.lines_top_border) && 
                    (g_scanline < display.FS))         
            { 
                    g_scanlineOffset = g_scanline - (display.lines_blanking + display.lines_dummy_top + display.lines_top_border); 
                    g_scanlineOffset -= 12;                // adjust for 1 line of text 
            }    
		}
		vz_latch = v;
	}
}


byte InZ80(word P)
{
	
    byte b = 0xFF;

    //
    // Disk reads
    //

    if ((P >= 0x10) && (P <= 0x1F)) 
    {
		b = vtech1_fdc_r(P - 0x10);
    }

	//
	// Printer port
	//

    if ((P >= 0x00) && (P <= 0x0F)) 
    {
		if 	(prt != NULL)		// have we mapped to an output file
		{
			b &= ~0x01;
		}
    }

	//
	// Joystick port
	//
	bool bFIRE = false;
	if (prefs.joystick == 1)
	{
		if ((g_joystickByte & 0x80) == 0x00)		// hacked byte for FIRE signal
		{
			bFIRE = true;
			g_joystickByte = (g_joystickByte | 0x80);
		}
		if ((P >= 0x20) && (P <= 0x2F)) 
		{
			word offs = P - 0x20;
			b = g_joystickByte;
		}
	}
      
    return b;
}

void OutZ80(word P, byte B)
{

	// 
	// Is graphics mod enabled?
	//
	if (prefs.gfx == 1)
	{
		if (P == 0x20)			// output port for gfx mod
		{
			gfx_mod_latch = B;
			g_render = 4;		// could be changing some graphics handling, so force redraw 
		}
	}

	if (prefs.gfx == 2)
	{
		if (P == 0xDE)			// output port for gfx mod
		{
			gfx_mod_latch &= 0xFC;			// mask first 2 bits
			gfx_mod_latch |= (B & 0x03);	// and select bank
		}
	}

	//
    // Disk writes
    //

    if ((P >= 0x10) && (P <= 0x1F))
    {
       vtech1_fdc_w(P - 0x10, B);
    }

	//
	// Printer
	//

    if ((P >= 0x00) && (P <= 0x0F)) 
    {
		if 	(prt != NULL)		// have we mapped to an output file
		{
			if ((P == 0x0E) || (P == 0x00))
			{
				fwrite(&B,1,1,prt);
			}
		}
    }

    //
    // Memory Bank switch
    //

    static int current_page = 1;
    int i;
    long cpoffs, apoffs;

    if ((P >= 0x70) && ( P <= 0x7F) && (prefs.top_of_memory == 65535))
    {
		if (B == 0) B = 1;
        active_page = B;
        cpoffs = current_page * 16384;
        apoffs = active_page * 16384;
        for (i = 0; i < 16384; i++)
        {
           // copy current page to bank memory
           BKMEM[cpoffs+i] = VZMEM[0xC000 + i];

           // copy bank memory to selected page
           VZMEM[0xC000 + i] = BKMEM[apoffs + i];
        }

        current_page = active_page;
    }

}

void PatchZ80(Z80 *R)
{
}

void GenInterrupt(Z80 *R)
{
	  register pair J;

	  J.W = NULL;

  /* If we have come after EI, get address from IRequest */
  /* Otherwise, get it from the loop handler             */
  if(R->IFF&0x20)
  {
        J.W=R->IRequest;         /* Get pending interrupt    */
        R->ICount+=R->IBackup-1; /* Restore the ICount       */
        R->IFF&=0xDF;            /* Done with AfterEI state  */
  }

  if(J.W==INT_QUIT) return;		 /* Exit if INT_QUIT */
  if(J.W!=INT_NONE)
  {
	  IntZ80(R,J.W);   /* Int-pt if needed */
  }
}


void DrawBorder()
{
	const int		mode = g_updateborder & 0x08;
	const int		bgcolor = g_updateborder & 0x10;
	int		bdcolor;
	int		x,y;

	// adjust y coordinate for display buffer (eg no dummy lines in buffer)
	y = g_scanline - (display.lines_blanking + display.lines_dummy_top);	

	if (mode == 0)
	{
		bdcolor = VZ_BLACK;
	}
	else
	{
		// gfx mode
		if (bgcolor == 0)
		{
			bdcolor = VZ_GREEN;
		}
		else
		{
			bdcolor = VZ_BUFF;
		}
	}
	for (x=0;x<320;x++)
	{
		Display_Buffer[320*y+x] = bdcolor;
	}
}


void DrawGMScanLine(int gmMode, int y, int bdcolor)
/*************************************************************************

	Inputs:	gmMode	-	0 = 64x64 color graphics one
						1 = 128x64 resolution graphics one
						2 - 128x64 color graphics two
						3 - 128x96 resolution graphics two
						4 - 128x96 color graphics three
						5 - 128x192 resolution graphics three
						6 - 128x192 color graphics six
						7 - 256x192 resolution graphics six
			
			y		-	current scanline
			bdcolor	-	border color. Green or buff

*************************************************************************/
{
	 int  n,x;
	 int  pixelrow;
	 byte mask,ch,adj, r, pcol, c0, c1, c2, c3;
	 int  colorset;
	 int  xm, ym, bpp, lp, offset;
	 int  fcol,bcol;


	 colorset = vz_latch & 0x10;

	 fcol = VZ_DKGREEN;
	 bcol = bdcolor;

	 if (prefs.inverse == 1)
	 {
		 fcol = bdcolor;
		 bcol = VZ_DKGREEN;
	 }

	 //
	 //		resolution for GM0 to GM7 is from 64x64 to 256x192, so need to calc
	 //		x,y scale factor for 256x192 display
	 //
	 
	 static int gm_mode_scaling3x8[]  = 
		{3,3,2,  2,3,1,  2,3,2,  2,2,1,  2,2,2,  2,1,1,  2,1,2,  1,1,1} ;
	 
	 offset = gmMode*3;
	 xm = gm_mode_scaling3x8[offset];
	 ym = gm_mode_scaling3x8[offset+1];
	 bpp = gm_mode_scaling3x8[offset+2];
	 lp = 8 / bpp;     // 8 or 4 pixels per byte, depending on graphics mode

	 //
	 // determine color set based on latch bit
	 //

	 if (colorset == 0)
	 {
		  c0 = VZ_GREEN;
		  c1 = VZ_YELLOW;
		  c2 = VZ_BLUE;
		  c3 = VZ_RED;
	 }
	 else
	 {
		  c0 = VZ_BUFF;
		  c1 = VZ_CYAN;
		  c2 = VZ_MAGENTA;
		  c3 = VZ_ORANGE;
	 }

	 pixelrow = (y - display.lines_top_border)/ym;  // adjust for top border
	 if (lp == 4)           // colour mode - 4 pixels per byte
	 {
		for (x=0;x<32;x++)         // 32 bytes in row
		{
			ch = GXMEM[32*pixelrow+x];      // Get the screen byte
			mask = 0xC0;
			r = 6;
			for (n=0;n<lp;n++)
			{
				adj = (ch & mask) >> r;

				if (adj == 0x00) pcol = c0;
				if (adj == 0x01) pcol = c1;
				if (adj == 0x02) pcol = c2;
				if (adj == 0x03) pcol = c3;

				offset = 320*y+32+x*8;
    
				for (int pm=0;pm<xm;pm++)   // scaling factor will be 1, 2 or 3
				{
					Display_Buffer[offset+n*xm+pm] = pcol;
				} 
				mask = mask >> 2;
				r -= 2;
			}
		}
	}
	else   // bw mode - 8 pixels per byte
	{
		for (x=0;x<32;x++)         // 32 bytes in row
		{
			ch = GXMEM[32*pixelrow+x];      // Get the screen byte
			mask = 0x80;
			for (n=0;n<8;n++)
			{
				if (ch & mask)
				{
					Display_Buffer[320*y+32+x*8+n] = bcol;   // VZ_DKGREEN; 
				}
				else
				{
					Display_Buffer[320*y+32+x*8+n] = fcol;	// bdcolor;
				}
				mask = mask >> 1;
			}
		}
	}
}


void DrawGFXScanLine(int y, int bdcolor)
{

	int	gm;

	if (prefs.gfx > 0)
	{
		gm = (gfx_mod_latch & 0x1C) >> 2;
		DrawGMScanLine(gm, y, bdcolor);
	}
	else
	{
		DrawGMScanLine(2, y, bdcolor);
	}
}


void DrawTextScanLine(int y, int txtfcolor, int txtbcolor)
{

	int		textrow;
	int		n,x;
	int		offset;
	int		forecolor;
	int		backcolor;
	byte	ch;
	byte	font;
	byte	mask;


	// y will be between 0-191. There are 16 rows of text. Each text row 
	// is 12 lines. So the text row will be y/12. 

	textrow = (y-display.lines_top_border)/12;						// 16 text lines: 0-15
	offset = (y-display.lines_top_border) - textrow*12;				// 12 scanlines per character

	// check for external font mod
	bool external_font = false;
	bool semigraphics = true;
	if (prefs.external_font == 1)
	{
		if (gfx_mod_latch & 0x20) external_font = true;              
		if (gfx_mod_latch & 0x80) semigraphics = false;
	}

	for (x=0;x<32;x++)												// 32 text columns
	{
		if (prefs.gfx > 0)
		{
			ch = GXMEM[32*textrow+x];			// Get the screen character
		}
		else
		{
			ch = VZMEM[28672+32*textrow+x];		// Get the screen character
		}

		forecolor = txtfcolor;
		backcolor = txtbcolor;

		// assume internal font
		font = pal_square_fontdata8x12[12*ch+offset];
		if (external_font)
		{
			if ((semigraphics) && (ch > 127))		
			{
				// semigraphics flag set - keep using internal font
			}
			else
			{
				font = external_fontdata8x12[12*ch+offset];
			}
		}

		if (ch > 127)																						
		{
			if ((external_font) && (semigraphics == false))
			{
				// semigraphics flag not set - keep foreground & background colours
			}
			else
			{
				// adjust colours for internal semigraphics 
				forecolor = (ch-128) >> 4; 
				backcolor = VZ_BLACK;
			}
		}
		

		mask = 0x80;
		int offs = 320*y+32+x*8;
		for (n=0;n<8;n++)
		{
			if (font & mask)
			{
				Display_Buffer[offs+n] = forecolor;	// 8000 = offset for top border
			}
			else
			{
				Display_Buffer[offs+n] = backcolor;
			}
			mask = mask >> 1;
		}
	}
}


void DrawDisplay()
{
	int		mode = vz_latch & 0x08;
	int		bgcolor = vz_latch & 0x10;
	int		bdcolor;
	int		txtfcolor;
	int		txtbcolor;
	int		x,y;

	// adjust y coordinate for display buffer (eg no dummy lines in buffer)
	y = g_scanline - (display.lines_blanking + display.lines_dummy_top);

	// For text mode, the border color is always black. The text background color is 
	// always dark green. Set the text foreground color to green or orange, according
	// to the latch

	if (mode == 0)
	{
		bdcolor = VZ_BLACK;
		txtbcolor = VZ_DKGREEN;
		if (bgcolor == 0)
		{
			txtfcolor = VZ_GREEN;
		}
		else
		{
			txtfcolor = VZ_BRORANGE;
		}
		// first draw side borders
		for (x=0;x<32;x++)
		{
			Display_Buffer[320*y+x] = bdcolor;
			Display_Buffer[320*y+288+x] = bdcolor;
		}
		// now draw main display scanline
		DrawTextScanLine(y,txtfcolor,txtbcolor);
	}
	else
	{
		// gfx mode
		if (bgcolor == 0)
		{
			bdcolor = VZ_GREEN;
		}
		else
		{
			bdcolor = VZ_BUFF;
		}
		// first draw side borders
		for (x=0;x<32;x++)
		{
			Display_Buffer[320*y+x] = bdcolor;
			Display_Buffer[320*y+288+x] = bdcolor;
		}
		// now draw main display scanline
		DrawGFXScanLine(y,bdcolor);
	}
}

void DrawScanLine()
{
	static int topborder_startline = display.lines_blanking + display.lines_dummy_top;
	static int maindisplay_startline = topborder_startline + display.lines_top_border;
	static int bottomborder_startline = maindisplay_startline + display.lines_display;

	//
	//	Scanlines will be 0-311 for a VZ200
	//
	//	  0-12		Blanking		 13 lines
	//   13-37		Dummy lines		 25 lines
	//	 38-62		Top border		 25 lines
	//	 63-254		Main display	192 lines
	//  255-280		Bottom border	 26 lines
	//	281-305		Dummy lines		 25 lines
	//	306-311		Retrace			  6 lines

	if ((g_scanline >= topborder_startline) && 
		(g_scanline < maindisplay_startline))
	{
		// Top border
		DrawBorder();
	}

	if ((g_scanline >= maindisplay_startline) && (g_scanline < bottomborder_startline)) 
	{
		// Main Display
		g_updateborder = vz_latch;
		DrawDisplay();
	}

	if ((g_scanline >= bottomborder_startline) && 
		(g_scanline < bottomborder_startline + display.lines_bottom_border)) 
	{
		// Bottom border
		DrawBorder();
	}


}
void MapTape(char *s)
{
	char	ext[8];

	strcpy(wavTape,s);
	strcpy(ext,".wav");
	if (strchr(s,46) == NULL)	// if no extension specified, add .wav
	{
		strcat(wavTape,ext);
	}
}

void MapPrinter(char *s)
{
	char	ext[8];

	strcpy(prtFile,s);
	strcpy(ext,".txt");
	if (strchr(s,46) == NULL)	// if no extension specified, add .wav
	{
		strcat(prtFile,ext);
	}
	prt = fopen(prtFile,"wb+");
}


int PlayTape()
{
	g_tapeplaying = false;

	if (wavTape[0] == 0)
	{
		return -1;		// no valid tape image selected
	}
	tape = fopen(wavTape,"rb");
	if (!tape)
	{
		return -2;		// cannot open selected file
	}
	HEADER wavHeader;
	fread(&wavHeader, sizeof(HEADER),1,tape);
	if ((wavHeader.sampleRate != 22050)  ||
		(wavHeader.bits_per_sample != 8) ||
		(wavHeader.channels != 1))
	{
		return -3;		// wav file is not correct type
	}
	g_tapeplaying = true;
	return 0;
}

void RecordTape()
{
	tape = fopen(wavTape,"wb+");
	if (!tape)
	{
		g_taperecording = false;
	}
	HEADER			wavHeader = {0x52,0x49,0x46,0x46,
					0, 0x57,0x41,0x56,0x45,0x66,0x6D,0x74,0x20,
					16,1,1,22050,22050,1,8,
					0x64,0x61,0x74,0x61,0};

	fwrite(&wavHeader,sizeof(HEADER),1,tape);
	g_tapeBytesWritten = 0;
	g_taperecording = true;
}

void StopTape()
{
	HEADER wavHeader;

	if (g_taperecording)
	{
		g_taperecording = false;
		fseek(tape,0,0);
		fread(&wavHeader,sizeof(HEADER),1,tape);
		wavHeader.data_len = g_tapeBytesWritten;
		wavHeader.fileLength = wavHeader.data_len + 36;
		fseek(tape,0,0);
		fwrite(&wavHeader,sizeof(HEADER),1,tape);
		fclose(tape);
	}
	
	if (g_tapeplaying)
	{
		g_tapeplaying = false;
		g_tapeByte = 0x80;
		fclose(tape);
	}
}

void WriteCassette(byte g_tapeByte)
{
	static int	lastCount = Regs.IPeriod;
	int			n,wavbytes;
	
	int elapsedCycles;

	if (g_taperecording)
	{
		if (Regs.ICount > lastCount) // interrupt period has expired
		{
			lastCount += Regs.IPeriod; 
		}
		elapsedCycles = lastCount - Regs.ICount; 

		// manually tweak wavbytes to write to get correct result for 22050 file
		wavbytes = 39;	// gap for 1 bit silence - 22050 hz file
		if (elapsedCycles > 1700)
		{
			byte silence = 0x80;
			for (n=0;n<wavbytes;n++)
			{
				fwrite(&silence,1,1,tape);
				g_tapeBytesWritten++;
			}
		}

		if ((elapsedCycles > 700) && (elapsedCycles < 1700))	// long pulse
		{
			if (g_tapeByte == 0x00)
			{
				wavbytes = 13;
				for (n=0;n<wavbytes;n++)
					fwrite(&long_low[n], 1, 1, tape);
				g_tapeBytesWritten += wavbytes;
			}
			if (g_tapeByte == 0xFF)
			{
				wavbytes = 13;
				for (n=0;n<wavbytes;n++)
					fwrite(&long_high[n], 1, 1, tape);
				g_tapeBytesWritten += wavbytes;
			}
		}
		if (elapsedCycles < 700) // short pulse
		{
			if (g_tapeByte == 0x00)
			{
				wavbytes = 7;
				for (n=0;n<wavbytes;n++)
					fwrite(&short_low[n], 1, 1, tape);
				g_tapeBytesWritten += wavbytes;
			}
			if (g_tapeByte == 0xFF)
			{
				wavbytes = 6;
				for (n=0;n<wavbytes;n++)
					fwrite(&short_high[n], 1, 1, tape);
				g_tapeBytesWritten += wavbytes;
			}
		}
		lastCount = Regs.ICount;
	}
}

word LoopZ80(Z80 *R)
{

	static int	lastCount = Regs.IPeriod;
	static int  cycles = 0;
	static int  audioCycles = 0;
	static int	tapeCycles = 0;
	static int  cpuCyclesPerSoundByte = (Regs.IPeriod/(SOUND_SAMPLE_RATE/50));
	
	if (R->ICount > lastCount) // interrupt period has expired
	{
		lastCount += Regs.IPeriod; 
	}
	cycles += lastCount - R->ICount;
	audioCycles += lastCount - R->ICount;

	while (cycles > CYCLES_PER_SCANLINE)
	{
		DrawScanLine();
		g_scanline++;
		
		if (g_scanline == display.total_lines)
		{
			g_scanline = 0;
			g_scanlineOffset = -1;
		}
		if (g_scanline == display.FS)
		{
			GenInterrupt(R);
		}

		cycles -= CYCLES_PER_SCANLINE;
	}

	if ((g_tapeplaying || g_taperecording))
	{
		if (prefs.cassetteAudio)
		{
			if (g_tapeByte > 128)
				g_soundSample = 32767;
			else
				g_soundSample = -32768;
		}
	}

	while (audioCycles > cpuCyclesPerSoundByte)
	{
		osd_writeSoundStream(g_soundSample);
		audioCycles -= cpuCyclesPerSoundByte;
	}
	static long bytesread = 0;
	if (g_tapeplaying)
	{
		tapeCycles += lastCount - R->ICount;
		while (tapeCycles > CYCLES_PER_TAPEBYTE)
		{
			if (!feof(tape))
			{
				fread(&g_tapeByte,1,1,tape);
				bytesread++;
			}
			else
			{
				g_tapeplaying = false;
				tapeCycles = 0;
				g_tapeByte = 0;
				bytesread = 0;
				fclose(tape);
			}
			tapeCycles -= CYCLES_PER_TAPEBYTE;
		}
	}

	lastCount = Regs.ICount;

	// check if we need to break on next instruction 

	if (brkPointArray[0] != 0x20)		// have any breakpoints been entered? (non space)
	{
		char	addr[8];

		// Get current program counter, convert to a hex string
		sprintf(addr, "%04X", R->PC.W);
		// See if it is in breakpoint list
		R->Trace = 0;
		for (int n=0;n<76;n++)
		{
			if	((addr[0] == brkPointArray[n]) &&
				(addr[1] == brkPointArray[n+1]) &&
				(addr[2] == brkPointArray[n+2]) &&
				(addr[3] == brkPointArray[n+3]))
			{
				R->Trace = 1;
				R->Trap = R->PC.W;
			}
		}
	}
	return R->PC.W;
}


void ScanKbrd()
{
   int  n;
 
	// clear the matrix to assume no keys pressed
   for (n=0;n<8;n++) vzkbrd[n] = 0xFF;
   
   // Call the OS dependent function to check what keys are pressed
   // and populate the vz keyboard matrix 
 
   osd_ScanKbrd(vzkbrd);
}


void LoadVZRom()
{
    FILE    *vzrom;
	FILE	*vzcart;
	FILE	*extfont;
    int	    n;
	
    //
    // Load the VZ rom
    //

    vzrom = fopen(romFile ,"rb");
    if (vzrom  == NULL) return;
	fseek(vzrom, 0, SEEK_END);
	n=ftell(vzrom);
	fseek(vzrom, 0, SEEK_SET);
	fread(VZMEM,sizeof(byte),n,vzrom);
    fclose(vzrom);

	// clear out the cartridge memory in case disabling disk
	for (n=0;n<8192;n++)
	{
		VZMEM[16384+n] = 0x7f;
	}

	if (strcmp(cartridgeFile,"NONE") != 0)
	{
		vzcart = fopen(cartridgeFile, "rb");
		if (vzcart == NULL) return;
		fseek(vzcart, 0, SEEK_END);
		n=ftell(vzcart);
		fseek(vzcart, 0, SEEK_SET);
		fread(&VZMEM[16384],sizeof(byte),n,vzcart);
		fclose(vzcart);
	}

	if (strcmp(fontFile,"NONE") != 0)
	{
		extfont = fopen(fontFile, "rb");
		if (extfont == NULL) return;
		fseek(extfont, 0, SEEK_END);
		n=ftell(extfont);
		fseek(extfont, 0, SEEK_SET);
		fread(external_fontdata8x12,sizeof(byte),n,extfont);
		fclose(extfont);
	}

}


void setMonitor(int monitor)
{

	palette[0] = RGB_Green;
	palette[1] = RGB_Yellow;
	palette[2] = RGB_Blue;
	palette[3] = RGB_Red;
	palette[4] = RGB_Buff;
	palette[5] = RGB_Cyan;
	palette[6] = RGB_Magenta;
	palette[7] = RGB_Orange;
	palette[8] = RGB_Black;
	palette[9] = RGB_DarkGreen;
	palette[10] = RGB_BrightGreen;
	palette[11] = RGB_DarkOrange;
	palette[12] = RGB_BrightOrange;

	if (monitor == 0)
	{
		// adjust colours for monochrome display
		for (int c=0; c<13; c++)
		{
			long RGB_Color = palette[c];
			const byte r = (byte) ((RGB_Color & 0xFF0000 ) >> 16);
			const byte g = (byte) ((RGB_Color & 0x00FF00 ) >> 8);
			const byte b = (byte) ((RGB_Color & 0x0000FF )) ;

			const byte RGB_grey = (byte)(r * 0.3f  + g * 0.59f  + b * 0.11f);

			const long RGB24 = (long)((RGB_grey << 16) | (RGB_grey << 8) | RGB_grey);
			palette[c] = RGB24;
		}
	}
	osd_GenColors(palette);
}

void LoadPrefs()
{
	FILE *fp = fopen("vzemcfg.txt", "r");
	if (fp != NULL)
	{
		if (fseek(fp, 0L, SEEK_END) == 0)
		{
			long bufsize = ftell(fp);
			properties = (char*) malloc(sizeof(char) * (bufsize + 1));
			fseek(fp, 0L, SEEK_SET);
			size_t newLen = fread(properties, sizeof(char), bufsize, fp);
			properties[newLen++] = '\0'; 
		}
		fclose(fp);
	}
	

	prefs.vzmodel = 0;
	prefs.display_type = 0;
	prefs.gfx = 2;
	prefs.external_font = 0;
	prefs.snow = 0;
	prefs.monitor = 1;
	prefs.inverse = 0; 
	prefs.disk_drives = 1;
	prefs.joystick = 0;
	prefs.rom_writes = 0;
	prefs.cartridge_writes = 0;
	prefs.synchVZ = 1;
	prefs.cassetteAudio = 1;
	prefs.diskAudio = 1;
	prefs.top_of_memory = 65535;
	prefs.fast_load = 1;


	int vzmodel = atoi(getProperty("vzmodel"));
	int display_type = atoi(getProperty("display"));
	int gfx = atoi(getProperty("gfx"));
	int external_font = atoi(getProperty("external_font"));
	int snow = atoi(getProperty("snow"));
	int monitor = atoi(getProperty("monitor"));
	int inverse = atoi(getProperty("inverse"));
	int disk_drives = atoi(getProperty("disk_drives"));
	int joystick = atoi(getProperty("joystick"));
	int rom_writes = atoi(getProperty("rom_writes"));
	int cartridge_writes = atoi(getProperty("cartridge_writes"));
	int synchVZ = atoi(getProperty("synchVZ"));
	int cassetteAudio = atoi(getProperty("cassetteAudio"));
	int diskAudio = atoi(getProperty("diskAudio"));
	int top_of_memory = atoi(getProperty("top_of_memory"));
	int fast_load = atoi(getProperty("fast_load"));


	strcpy(romFile,getProperty("romFile"));	
	strcpy(cartridgeFile,getProperty("cartFile"));
	strcpy(fontFile,getProperty("fontFile"));


	romFile[strcspn(romFile, " ")] = '\0';					// trim trailing space
	cartridgeFile[strcspn(cartridgeFile, " ")] = '\0';		// 
	fontFile[strcspn(fontFile, " ")] = '\0';	


	prefs.vzmodel = vzmodel;
	prefs.display_type = display_type;
	prefs.gfx = gfx;
	prefs.external_font = external_font;
	prefs.snow = snow;
	prefs.monitor = monitor;
	prefs.inverse = inverse; 
	prefs.disk_drives = disk_drives;
	prefs.joystick = joystick;
	prefs.rom_writes = rom_writes;
	prefs.cartridge_writes = cartridge_writes;
	prefs.synchVZ = synchVZ;
	prefs.cassetteAudio = cassetteAudio;
	prefs.diskAudio = diskAudio;
	prefs.top_of_memory = top_of_memory;
	prefs.fast_load = fast_load;

	RGB_Green = (long)strtol(getProperty("RGB_Green"), NULL, 0);
	RGB_Yellow = (long)strtol(getProperty("RGB_Yellow"), NULL, 0);
	RGB_Blue = (long)strtol(getProperty("RGB_Blue"), NULL, 0);
	RGB_Red = (long)strtol(getProperty("RGB_Red"), NULL, 0);
	RGB_Buff = (long)strtol(getProperty("RGB_Buff"), NULL, 0);
	RGB_Cyan = (long)strtol(getProperty("RGB_Cyan"), NULL, 0);
	RGB_Magenta = (long)strtol(getProperty("RGB_Magenta"), NULL, 0);
	RGB_Orange = (long)strtol(getProperty("RGB_Orange"), NULL, 0);
	RGB_Black = (long)strtol(getProperty("RGB_Black"), NULL, 0);
	RGB_DarkGreen = (long)strtol(getProperty("RGB_DarkGreen"), NULL, 0);
	RGB_BrightGreen = (long)strtol(getProperty("RGB_BrightGreen"), NULL, 0);
	RGB_DarkOrange = (long)strtol(getProperty("RGB_DarkOrange"), NULL, 0);
	RGB_BrightOrange = (long)strtol(getProperty("RGB_BrightOrange"), NULL, 0);

	setMonitor(prefs.monitor);

	free(properties);	// call this once all properties assigned 

}


//-------------------------------------------------------------------
//   Function: InitVZ();
//   Purpose:  Load the vz rom, preload the graphics, create a timer 
//-------------------------------------------------------------------

void InitVZ()
{

	if (prefs.display_type == 0)					
	{
		// PAL
		display.lines_dummy_top = 24;
		display.lines_dummy_bottom = 24;
	}
	else
	{
		// NTSC
		display.lines_dummy_top = 0;
		display.lines_dummy_bottom = 0;
	}

	if (prefs.vzmodel == 0)							
	{
		// VZ200
		prefs.cpu_speed = 3.5795f;
		display.lines_dummy_top++;
		display.lines_dummy_bottom++;
	}
	else
	{
		// VZ300
		prefs.cpu_speed = 3.5469f;
	}

	display.lines_blanking = 13;
	display.lines_top_border = 25;
	display.lines_display = 192;
	display.lines_bottom_border = 26;
	display.lines_retrace = 6;

	display.total_lines =	display.lines_blanking +
							display.lines_dummy_top +
							display.lines_top_border +
							display.lines_display +
							display.lines_bottom_border +
							display.lines_dummy_bottom +
							display.lines_retrace;

	display.FS = display.total_lines - display.lines_retrace;
	
	display.OFF =	display.lines_blanking +
					display.lines_dummy_top +
					display.lines_top_border +
					display.lines_display;


    RAM = VZMEM;									// RAM pointer defined in Z80 library		
    Regs.IPeriod = (int) (prefs.cpu_speed * 1000000/50);
	g_scanline = 0;
	g_scanlineOffset = -1;
	for (int i=0;i<312;i++) g_redrawScanline[i] = true;
	gfx_mod_latch = 8;					// set default mode to 128x64
	ResetZ80(&Regs);
	LoadVZRom();
	osd_GenColors(palette);
	osd_InitTimer();					// Create a 20ms timer

	g_render = 4; 

	for (int i=0;i<80;i++) brkPointArray[i] = 0x20;

}


void SaveVZFile(byte *fileBuffer, unsigned long *dwFileLen, VZFILE *vzf)
{
		word	startAddr;
		unsigned long		n;
		
		memcpy(fileBuffer,vzf,24);

		startAddr = vzf -> start_addrh * 256 + vzf -> start_addrl;  
		for (n=0;n<*dwFileLen;n++)
		{
			fileBuffer[24+n] = RAM[startAddr+n];
		}
		*dwFileLen += 24;
}

/*----------------------------------------------------------------*\
   Function: DoFrame();
   Purpose:  Run the Z80, check VZ hardware, draw the screen
\*----------------------------------------------------------------*/

int DoFrame()
{
	word PC;
	PC = RunZ80(&Regs);				// execute Z80 till next interrupt

	if (Regs.Trace == 0)
	{
		ScanKbrd(); 							// Get key presses/releases 
		if (g_internalMenu)
		{
			// TODO
		}

		if (g_render > 0)
		{
			skipped_frames = 0;
			osd_BlitBuffer(Display_Buffer);		// Paint the display
			g_render--; 
		}		
		else 
		{
			skipped_frames++;
		}

		if (prefs.synchVZ)						// normal speed
		{
			osd_synchSound();
			osd_SynchVZ();						// Synchronise to 50 fps
		}
	}
	return Regs.Trace;
}

void DoStep()
{
	word PC;

	PC = ExecZ80(&Regs);	
	if (Regs.ICount < 0)
	{
		 Regs.ICount +=Regs.IPeriod;    
	}
}

void SetBreakPoints(char *brkPoints)
{
	char ch;
	for (int n=0; n<80; n++)
	{
		ch = *(brkPoints+n);
		if (ch > 96) ch -= 32;
		brkPointArray[n] = ch;
	}
}


void GetVideoBuffer(byte **bufr, int *scanline, long **pal)
{
	*bufr = Display_Buffer;
	*scanline = g_scanline;
	*pal = palette;
}

