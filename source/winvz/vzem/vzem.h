#define byte unsigned char
#define word unsigned short
typedef __int64 LONGLONG;

#define SOUND_SAMPLE_RATE	44100
#define AUDIO_FRAME_LEN		882

#define CYCLESHORT	0
#define	CYCLELONG	1
#define CYCLENULL	-1
#define NULLBIT		0xFF

#define max_path 512


typedef struct vzFile				
{
	byte	vzmagic[4];
	byte	filename[17];
	byte	ftype;
	byte	start_addrl;
	byte	start_addrh;
} VZFILE;

typedef struct WAVEHEADR
{
	char            format[4];          // RIFF
	unsigned long   fileLength;         // filelength
	char            wave_fmt[8];        // "WAVEfmt "
	unsigned long   chunkSize;          // 16 for PCM
   	unsigned short  fmt_tag;            // PCM = 1
	unsigned short  channels;           // Mono/Stereo
	unsigned long   sampleRate;			// 11025, 22050 etc
	unsigned long   ByteRate;			// SampleRate * Channels * BitsPerSample/8
	unsigned short  blk_align;			// NumChannels * BitsPerSample/8
	unsigned short  bits_per_sample;	// 8, 16 etc
	char			data[4];            // "data"
	unsigned long   data_len;           // NumSamples * NumChannels * BitsPerSample/8
} HEADER;

typedef struct vzPrefs
{
	int		vzmodel;				// 0 = VZ200, 1 = VZ300
	float	cpu_speed;				// 3.58 or 3.54
	int		display_type;			// pal=0 ntsc=1
	int		gfx;					// normal=0 1=Australian 3=German
	int		external_font;			// no=0 yes=1
	int		snow;					// no=0 yes=1
	int		monitor;				// 0=monochrome, 1=colour
	int		inverse;				// 0=normal, 1 = inverse hires
	int		disk_drives;			// no=0 yes=1
	int		joystick;				// no=0 1=arrows 2=directx
	int		rom_writes;				// no=0 yes=1
	int		cartridge_writes;		// no=0 yes=1
	int		top_of_memory;			// 36863=8k 47103=18k 53247=24k 63487=34k 65535=64k 5=4meg
	int		sound_card;				// 0 = none, 1 = SN76489, 2 = 
	int		sound_port;				// output port mapped to sound card
	int		synchVZ;				// synchronise at normal 50fps
	int		cassetteAudio;
	int		diskAudio;
	char	romFile[max_path];
	char	cartridgeFile[max_path];
	int		fast_load;
} PREFS;


typedef struct vzDisplay
{
	int		lines_blanking;
	int		lines_dummy_top;
	int		lines_top_border;
	int		lines_display;
	int		lines_bottom_border;
	int		lines_dummy_bottom;
	int		lines_retrace;
	int		total_lines;
	int		FS;
	int		OFF;
} DISPLAY;


void WrSpeaker();
byte RdZ80(word A);
void WrZ80(word A, byte v);
byte InZ80(word P);
void OutZ80(word P, byte B);
void vtech1_floppy_remove(int id);
void InitVZ();
void setMonitor(int monitor);
void LoadPrefs();
void SavePrefs();
int  PlayTape();
void RecordTape();
void WriteCassette(byte v);
void StopTape();
void LoadVZFilename(char *s);
void LoadVZFile(byte *fileBuffer, int fileLength, VZFILE *vzf);
void LoadFont(byte *fileBuffer);
int DoFrame();
void UpdateSound();					
void ScanKbrd();                         
void DrawScreen(byte latch);
void SaveVZFile(byte *fileBuffer, unsigned long *dwFileLen, VZFILE *vzf);
int vtech1_floppy_init(int id, char *s);
void MapTape(char *s);
void MapPrinter(char *s);
void GetDisassembly(char *disText, char *regText, char *ptrText, char *hwText, char *brkText);
void Disassemble(char *disText, int startAddr, int endAddr);
void DoStep();
void SetBreakPoints(char *brkPoints);
void GetVideoBuffer(byte **bufr, int *scanline, long **palette);

void osd_ScanKbrd(byte *kbrd);
bool osd_SynchVZ();	
void osd_LoadVZRom(byte *mem);			
void osd_InitTimer();
void osd_WriteAVIFile();
void osd_LoadVZFile(byte *fileBuffer, word *fileLen);
void osd_BlitBuffer(byte *bufr);
void osd_GenColors(long *palette);
//void osd_writeSoundStream(short *buffer, int len);
void osd_writeSoundStream(short sample);
void osd_flushSoundBuffer(int buffer);
void osd_synchSound();
void osd_clearsound();
void osd_PlayTrack();
void osd_PlayStepper();
void osd_DiskStatus(int drive, char *TS);

int PlayResource(int sound);
