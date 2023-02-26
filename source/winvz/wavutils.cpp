BYTE ReadVZByte(FILE *file);

void Wav2VZ()
{

	#define CYCLESHORT	0
	#define	CYCLELONG	1
	#define CYCLENULL	-1
	#define NULLBIT		0xFF

	long		lDataLen = 1000;					// Maximum size for the signature.
	DWORD		dwFileLen, dwFileLen1;

	HANDLE		hFile;
	CHAR		szFile[MAX_PATH] = TEXT(".wav\0");
	OPENFILENAME ofn;

	typedef struct WAVEHDR
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

	VZFILE			vzf;
	HEADER			wavHeader;
	BYTE			wavByte, vzByte, max;
	BYTE*			buffer;						// holds original wav file
	unsigned short	checksum;		
	unsigned short	startAddress, endAddress, fileSize, chk;
	char			m_txtStatus[1024];
	FILE			*wavFile, *vzFile;


	ZeroMemory(&ofn, sizeof(ofn));
	ofn.lStructSize = sizeof(ofn);
	ofn.hwndOwner = g_hWnd;
	ofn.lpstrFile = szFile;

	ofn.lpstrFile[0] = '\0';
	ofn.nMaxFile = sizeof(szFile);
	ofn.lpstrFilter =   "WAV files\0*.wav"; //sets the file filter
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
		if (hFile = NULL) 
		{
			return;
		}
		CloseHandle(hFile);
	}

	strcpy(m_txtStatus,"");
	wavFile = fopen(ofn.lpstrFile, "rb");
	if (wavFile == NULL)
	{
		return;
	}

	strcat(m_txtStatus, "Checking WAV file...");
	fread(&wavHeader, sizeof(HEADER), 1, wavFile); 

	/*
	// checking for "RIFF" and "data" in header
	CString FORMAT = CString(wavHeader.format).Left(4);
	CString WAVE = CString(wavHeader.wave_fmt).Left(7);

	if ((FORMAT.Compare("RIFF") == 0) && (WAVE.Compare("WAVEfmt") == 0))
	{
		// valid header
		strcat(m_txtStatus, "OK!\r\n");
	}
	else
	{
		strcat(m_txtStatus, "ERROR - not a valid WAV file");
		return;
	}
	*/
	strcat(m_txtStatus, "OK!\r\n");


	// Check WAV file is suitable for VZ conversion
	if ((wavHeader.channels != 1) ||
		(wavHeader.fmt_tag != 1) ||
		(wavHeader.sampleRate != 22050)) 
	{
		strcat(m_txtStatus, "ERROR - must be 22050hz 8 bit mono PCM");
		return;
	}

	//
	// search the WAV file for start of signal. 
	//

	fread(&wavByte,1,1,wavFile);
	while (wavByte < 0x90)
	{
		fread(&wavByte,1,1,wavFile);
	}

	// Search for synch Byte
	strcat(m_txtStatus, "Synching to leader...");
	vzByte = ReadVZByte(wavFile);

	while (vzByte != 0x80)
	{
		vzByte = ReadVZByte(wavFile);
	}
	strcat(m_txtStatus, "OK!\r\n");
	

	// Search for preamble Bytes
	long leaderbytes = 0;
	strcat(m_txtStatus, "Finding preamble...");
	while (vzByte == 0x80) 
	{
		leaderbytes++;
		if (leaderbytes == 18)
		{
			leaderbytes = leaderbytes;
		}
		vzByte = ReadVZByte(wavFile);
	}
	if (vzByte != 0xFE)
	{
		strcat(m_txtStatus, "error finding preamble\r\n");
		return;
	}

	// 4 more preamble bytes, 5 in total
	int n;
	for (n=0; n<4;n++) 
	{
		vzByte = ReadVZByte(wavFile);
		if (vzByte != 0xFE)
		{
			strcat(m_txtStatus, "error reading preamble\r\n");
			return;
		}
	}
	strcat(m_txtStatus, "OK!\r\n");


	// Get File type, eg F0 (Basic), F1 (M/C)
	strcat(m_txtStatus, "Reading tape header...\r\n");
	vzByte = ReadVZByte(wavFile);
	vzf.ftype = vzByte;
	strcat(m_txtStatus, "   File type  : ");
	if (vzByte == 0xF0) 
	{
		strcat(m_txtStatus, "BASIC\r\n");
	}
	else
	{
		strcat(m_txtStatus, "M/C\r\n");
	}


	strcat(m_txtStatus, "   Filename  : ");
	for (n=0; n<17; n++)
	{
		vzByte = ReadVZByte(wavFile);
		vzf.filename[n] = vzByte;
		if (vzByte == 0) break;
		//strcat(m_txtStatus, (vzByte));
	}
	strcat(m_txtStatus, "\r\n");

	checksum = 0;	// start checksum

	// Read start address
	vzByte = ReadVZByte(wavFile);
	checksum += vzByte;
	vzf.start_addrl = vzByte;
	vzByte = ReadVZByte(wavFile);
	checksum += vzByte;
	vzf.start_addrh = vzByte;
	startAddress = vzf.start_addrl + 256 * vzf.start_addrh;


	// Read end address
	vzByte = ReadVZByte(wavFile);
	checksum += vzByte;
	endAddress = vzByte;
	vzByte = ReadVZByte(wavFile);
	checksum += vzByte;
	endAddress += 256 * vzByte;

	fileSize = endAddress - startAddress;

	strcat(m_txtStatus, "Creating VZ File...");
	//vzf.vzmagic = 0x2020;

	vzFile = fopen("c:\\temp\\out.vz", "wb");
	if (vzFile == NULL)
	{
		return;
	}
	fwrite(&vzf, sizeof(vzf),1,vzFile);
	for (n=0; n< fileSize; n++)
	{
		vzByte = ReadVZByte(wavFile);
		checksum += vzByte;
		fwrite(&vzByte,1,1,vzFile);
	}
	fclose(vzFile);
	strcat(m_txtStatus, "OK!\r\n");


	// Compare checksums to verify file converted properly
	strcat(m_txtStatus, "Comparing checksum...");
	vzByte = ReadVZByte(wavFile);
	chk = vzByte;
	vzByte = ReadVZByte(wavFile);
	chk += 256 * vzByte;

	if (checksum != chk)
		strcat(m_txtStatus, "error in checksum. Try resampling...\r\n");
	else
		strcat(m_txtStatus, "OK!\r\n");

	strcat(m_txtStatus, "Operation completed.");

}


BYTE FindCycle (FILE* file)
{
	// count the number of WAV file Bytes between the +ve and -ve range
	// to determine if the next bit is a binary 1 or 0


	int		pct, nct, count;
	BYTE	cycle, sample;

	// count how long bit stays +ve
	pct = 1;
	fread(&sample, 1,1, file);
	while (sample > 0x7F)
	{
		pct++;
		fread(&sample, 1,1, file);
	}

	// now count how long bit stays -ve
	nct = 1;
	fread(&sample, 1,1, file);
	while (sample < 0x80)
	{
		nct++;
		fread(&sample, 1,1, file);
	}
	
	count = pct + nct;

	cycle = CYCLENULL;
	if ((count >8) && (count <15)) cycle = CYCLESHORT;
	if ((count > 16) && (count < 30)) cycle = CYCLELONG;

	return (cycle);
  	  
}
  


BYTE ReadVZBit(FILE *file)
{
	// Decode the next VZ bit from the WAV file data

	BYTE	bit, c1, c2, c3;

	// A binary 1 is 3 short cycles
	// A binary 0 is 1 short and 1 long cycle

	bit = NULLBIT;				// assume error

	c1 = FindCycle(file);
	if (c1 == CYCLESHORT)		// all bits start on a short cycle
	{
		c2 = FindCycle(file);
		if (c2 == CYCLELONG)	// found binary 0 
		{
			bit = 0;
		}
		else
		{
			if (c2 == CYCLESHORT)
			{
				c3 = FindCycle(file);
				if (c3 == CYCLESHORT)	// found binary 1
				{
					bit = 1;
				}
			}
		}
	}
	return bit;
}

BYTE ReadVZByte(FILE *file)
{
	// Decode the next VZ Byte from the WAV file data

	int		n;
	BYTE	bit, vzb;

	vzb = 0;
	for (n=0;n<8;n++)
	{
		bit = ReadVZBit(file);
		if (bit != NULLBIT)
		{
			vzb = (vzb << 1) | bit;
		}
	}
	return vzb;
}

void VZ2Wav()
{
}


/*
void VZ2Wav()
{
	typedef struct WAVEHDR
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


	typedef struct vzFile
	{
		long	vzmagic;
		byte	filename[17];
		byte	ftype;
		byte	start_addrl;
		byte	start_addrh;
	} VZFILE;


	VZFILE			vzf;
	HEADER			wavHeader = {0x52,0x49,0x46,0x46,
					0,
					0x57,0x41,0x56,0x45,0x66,0x6D,0x74,0x20,
					16,1,1,21600,21600,1,8,
					0x64,0x61,0x74,0x61,0};

	BYTE			wavByte, vzByte, max;
	CFile			f, f2;
	BYTE*			buffer;						// holds original wav file
	int				checksum, n;		
	unsigned short	startAddress, endAddress, fileSize, chk;



		// validate file is a VZ 
	try 
	{
		f.Read(&vzf, sizeof(VZFILE));
	}
	catch(CFileException *e)
	{
		MessageBox("Fatal error reading source file");
	    e->Delete();
		return;
	}

	// Display filename

	m_txtStatus += "   Filename  : ";
	for (n=0; n<17; n++)
	{
		if (vzf.filename[n] == 0) break;
		m_txtStatus += CString(vzf.filename[n]);
	}
	m_txtStatus += "\r\n" ;	

	
	// Display file type 
	m_txtStatus += "   File type  : ";
	vzByte = vzf.ftype;
	if (vzByte == 0xF0) 
	{
		m_txtStatus += CString("BASIC\r\n");
	}
	else
	{
		m_txtStatus += CString("M/C\r\n");
	}

		if( f2.Open(m_txtTargetFile, CFile::modeCreate|CFile::modeWrite) == FALSE )
	{
		MessageBox("Error - cannot create target file");
		return;
	}

	f2.Write(&wavHeader, sizeof(wavHeader));


	// Write some silent bytes
	wavByte = 0x80;
	wavHeader.data_len = 0;
	for (n=0; n<5000; n++)
	{
		f2.Write(&wavByte,1);
		wavHeader.data_len++;
	}
	
	// Write 255 leader bytes
	for (n=0; n<255; n++)
	{
		wavHeader.data_len += WriteVZByte(&f2, 0x80);
	}
	
	// Write 5 preamble bytes
	for (n=0; n<5; n++)
	{
		wavHeader.data_len += WriteVZByte(&f2, 0xFE);
	}

	// Write file type
	wavHeader.data_len += WriteVZByte(&f2, vzf.ftype);

	// Write file name
	for (n=0; n<17; n++)
	{
		vzByte = vzf.filename[n];
		wavHeader.data_len += WriteVZByte(&f2, vzByte);
		if (vzByte == 0) break;
	}
	

	// there is a 1 bit length gap between header and start address
	wavByte = 0x00;
	for (n=0;n<36;n++)
	{
		f2.Write(&wavByte,1);
		wavHeader.data_len++;
	}


	chk = 0;

	// Write start address
	wavHeader.data_len += WriteVZByte(&f2, vzf.start_addrl); chk += vzf.start_addrl;
	wavHeader.data_len += WriteVZByte(&f2, vzf.start_addrh); chk += vzf.start_addrh;
	startAddress = vzf.start_addrl + 256 * vzf.start_addrh;


	// Write end address
	endAddress = startAddress + f.GetLength() - 24;		// end address = file length - vzheader
	wavHeader.data_len += WriteVZByte(&f2, (endAddress & 0x00FF)); chk += (endAddress & 0x00FF);
	wavHeader.data_len += WriteVZByte(&f2, (endAddress & 0xFF00) >> 8); chk += (endAddress & 0xFF00) >> 8;

	// Write the program

	unsigned long filesize = endAddress - startAddress;
	for (n=0; n<filesize; n++)
	{
		f.Read(&vzByte, 1);
		wavHeader.data_len += WriteVZByte(&f2, vzByte);
		chk += vzByte;
	}

	// Write the checksum

	BYTE	hb, lb;

	hb = (BYTE)(chk/256);
	lb = (BYTE)(chk - 256*hb);
	wavHeader.data_len += WriteVZByte(&f2, lb);
	wavHeader.data_len += WriteVZByte(&f2, hb);

	// Write some silent bytes

	wavByte = 0x80;
	for (n=0; n<5000; n++)
	{
		f2.Write(&wavByte,1);
		wavHeader.data_len++;
	}


	//write final wave header
	wavHeader.fileLength = wavHeader.data_len + 36;
	f2.Seek(0,CFile::begin);
	f2.Write(&wavHeader, sizeof(wavHeader));
	f2.Close();
	f.Close();

}



int WriteVZByte(CFile* pf, BYTE vzByte)
{

	BYTE BIT1[36] =	{	0xC0, 0xC0, 0xC0, 0xC0, 0xC0, 0xC0, 0x40, 0x40, 0x40, 0x40, 0x40,0x40,
						0xC0, 0xC0, 0xC0, 0xC0, 0xC0, 0xC0, 0x40, 0x40, 0x40, 0x40, 0x40,0x40,
						0xC0, 0xC0, 0xC0, 0xC0, 0xC0, 0xC0, 0x40, 0x40, 0x40, 0x40, 0x40,0x40
					};	// 3 short pulses

	BYTE BIT0[36] = {	0xC0, 0xC0, 0xC0, 0xC0, 0xC0, 0xC0, 0x40, 0x40, 0x40, 0x40, 0x40,0x40,
						0xC0, 0xC0, 0xC0, 0xC0, 0xC0, 0xC0, 0xC0, 0xC0, 0xC0, 0xC0, 0xC0,0xC0,
						0x40, 0x40, 0x40, 0x40, 0x40, 0x40, 0x40, 0x40, 0x40, 0x40, 0x40,0x40
					};	// 1 short 1 long pulse



	BYTE	vzb, mask;
	int		total = 0;

	mask = 0x80;
	for (int n=0; n<8; n++)
	{
		if ((vzByte & mask) == mask)	// binary 1
		{
			pf->Write(&BIT1, 36);
		}
		else
		{
			pf->Write(&BIT0, 36);
		}
		mask = (mask >> 1);
		total += 36;
	}
	return total; 
}
*/
