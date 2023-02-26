//------------------------------------------------------------------------------
// Name: FillBufferWithSilence()
// Desc: Does exactly what is says. For 8-bit waves 0x80 is silent, for 
//       16-bit wave files 0 is silence.
//------------------------------------------------------------------------------
BOOL FillBufferWithSilence(LPDIRECTSOUNDBUFFER& lpdsbSound)
{
	WAVEFORMATEX    wfx;
	DWORD           dwSizeWritten;

	PBYTE   pb1;
	DWORD   cb1;

	if (FAILED(lpdsbSound->GetFormat(&wfx, sizeof(WAVEFORMATEX), &dwSizeWritten)))
		return FALSE;

	if (SUCCEEDED(lpdsbSound->Lock(0, 0, (LPVOID*)&pb1, &cb1, NULL, NULL, DSBLOCK_ENTIREBUFFER)))
	{
		FillMemory(pb1, cb1, (wfx.wBitsPerSample == 8) ? 128 : 0);

		lpdsbSound->Unlock(pb1, cb1, NULL, 0);
		return TRUE;
	}

	return FALSE;
} // end FillBufferWithSilence()



//-----------------------------------------------------------------------------
// Name: InitDirectSound()
// Desc: Initilizes DirectSound
//-----------------------------------------------------------------------------
HRESULT InitDirectSound( HWND hDlg )
{
    HRESULT             hr;
    LPDIRECTSOUNDBUFFER pDSBPrimary = NULL;

    // Initialize COM
    if( hr = CoInitialize(NULL) )
        return hr;

    // Create IDirectSound using the primary sound device
    if( FAILED( hr = DirectSoundCreate( NULL, &g_pDS, NULL ) ) )
        return hr;

    // Set coop level to DSSCL_PRIORITY
    if( FAILED( hr = g_pDS->SetCooperativeLevel( hDlg, DSSCL_PRIORITY ) ) )
        return hr;

    // Get the primary buffer 
    DSBUFFERDESC        dsbd;
    ZeroMemory( &dsbd, sizeof(DSBUFFERDESC) );
    dsbd.dwSize        = sizeof(DSBUFFERDESC);
    dsbd.dwFlags       = DSBCAPS_PRIMARYBUFFER;
    dsbd.dwBufferBytes = 0;
    dsbd.lpwfxFormat   = NULL;
       
    if( FAILED( hr = g_pDS->CreateSoundBuffer( &dsbd, &pDSBPrimary, NULL ) ) )
        return hr;

    // Set primary buffer format 
    WAVEFORMATEX wfx;
    ZeroMemory( &wfx, sizeof(WAVEFORMATEX) ); 
    wfx.wFormatTag      = WAVE_FORMAT_PCM; 
    wfx.nChannels       = 1; 
    wfx.nSamplesPerSec  = SOUND_SAMPLE_RATE; 
    wfx.wBitsPerSample  = 16; 
    wfx.nBlockAlign     = wfx.wBitsPerSample / 8 * wfx.nChannels;
    wfx.nAvgBytesPerSec = wfx.nSamplesPerSec * wfx.nBlockAlign;

    if( FAILED( hr = pDSBPrimary->SetFormat(&wfx) ) )
        return hr;

	DSBUFFERDESC bufferDesc;
	memset(&bufferDesc,0,sizeof(bufferDesc));
	bufferDesc.dwSize = sizeof(bufferDesc);
	bufferDesc.dwFlags = DSBCAPS_GETCURRENTPOSITION2 ;
	bufferDesc.dwBufferBytes = DXAUDIO_BUFFER_LEN;
	bufferDesc.lpwfxFormat = &wfx;

	for (int n=0;n<NUMSTREAMINGBUFFERS; n++)
	{
		if( FAILED( hr = g_pDS->CreateSoundBuffer( &bufferDesc, &g_pDSBuffer[n], NULL ) ) )
			return hr;

		FillBufferWithSilence(g_pDSBuffer[n]);

		if( FAILED( hr = g_pDSBuffer[n]->Play(0, 0, DSBPLAY_LOOPING)))
		return hr;
	}

	// Initialise the sampled sounds - static buffer

	HRSRC hResource;
	HGLOBAL hFileResource;
	LPVOID lpFile;
	DWORD	dwSize;
    VOID*   pbData  = NULL;
    VOID*   pbData2 = NULL;

	hResource = FindResource(hInst, MAKEINTRESOURCE(IDR_WAVE_READSECTOR), _T("WAVE")); 
	hFileResource = LoadResource(hInst, hResource);
	lpFile = LockResource(hFileResource); 
	dwSize = SizeofResource(hInst, hResource);
 
	bufferDesc.dwFlags = DSBCAPS_STATIC ;
	bufferDesc.dwBufferBytes = dwSize-44;

	if( FAILED( hr = g_pDS->CreateSoundBuffer( &bufferDesc, &g_pDSSector, NULL ) ) )
		return hr;

	PBYTE   pb1;
	DWORD   cb1;
	if (SUCCEEDED(g_pDSSector->Lock(0, 0, (LPVOID*)&pb1, &cb1, NULL, NULL, DSBLOCK_ENTIREBUFFER)))
	{
		byte *bptr = (byte*)lpFile;
		memcpy( pb1, bptr+44, dwSize-44);
		g_pDSSector->Unlock(pb1, cb1, NULL, 0);
	}

	hResource = FindResource(hInst, MAKEINTRESOURCE(IDR_WAVE_CHANGETRK), _T("WAVE")); 
	hFileResource = LoadResource(hInst, hResource);
	lpFile = LockResource(hFileResource); 
	dwSize = SizeofResource(hInst, hResource);
 
	bufferDesc.dwFlags = DSBCAPS_STATIC ;
	bufferDesc.dwBufferBytes = dwSize-44;

	if( FAILED( hr = g_pDS->CreateSoundBuffer( &bufferDesc, &g_pDSStepper, NULL ) ) )
		return hr;

	if (SUCCEEDED(g_pDSStepper->Lock(0, 0, (LPVOID*)&pb1, &cb1, NULL, NULL, DSBLOCK_ENTIREBUFFER)))
	{
		byte *bptr = (byte*)lpFile;
		memcpy( pb1, bptr+44, dwSize-44);
		g_pDSStepper->Unlock(pb1, cb1, NULL, 0);
	}


	SAFE_RELEASE( pDSBPrimary );
    return S_OK;
}


//-----------------------------------------------------------------------------
// Name: FreeDirectSound()
// Desc: Releases DirectSound 
//-----------------------------------------------------------------------------
HRESULT FreeDirectSound()
{
	for (int n=0;n<NUMSTREAMINGBUFFERS;n++) SAFE_RELEASE(g_pDSBuffer[n]);

    SAFE_RELEASE( g_pDS ); 

    // Release COM
    CoUninitialize();

    return S_OK;
}






//-----------------------------------------------------------------------------
// Name: StopBuffer()
// Desc: Stop the DirectSound buffer
//-----------------------------------------------------------------------------
HRESULT StopBuffer() 
{
	for (int n=0;n<NUMSTREAMINGBUFFERS;n++)
	{
		if( NULL != g_pDSBuffer[n] )
		{
			g_pDSBuffer[n]->Stop();
			//SAFE_RELEASE(g_pDSBuffer[n]);
		}
	}
    return S_OK;
}


