//DirectXAVI.cpp : AVI code by Tobias Johansson

#include "DirectXAVI.h"

AVISTREAMINFO aviStreamInfo;
PAVIFILE      pAviFile = NULL;
PAVISTREAM    psStream = NULL;
PAVISTREAM    psCompressed = NULL;

AVICOMPRESSOPTIONS FAR * lpOptions;

//Width of video
int gAVIWidth  = 0;
//Height of video
int gAVIHeight = 0;
//Fps of the video
int gAVIfps    = 0;
//pixel depth of the video
int gAVIBpp    = 0;
//current video frame
long gVideoFrame = 0;

BOOL bAVIFileOpened = FALSE;

#define ERROR_LOG(x) LogFile->Print(x)
//#define ERROR_LOG(x) MessageBox(NULL, x,"Fatal Error",MB_OK)

int WriteAVIFile( char *fname, LPDIRECTDRAWSURFACE7 lpSurf, int FramesPerSecond )
{
  HRESULT        hr;
  DDSURFACEDESC2 ddsd;

  //See if the vfw is new enough
   WORD wVer = HIWORD( VideoForWindowsVersion() );
   if (wVer < 0x010a)
   {
     ERROR_LOG("VideoForWindows too old.\n");
     return AVI_ERROR_OTHERERROR;   
   }

   _DDInit( ddsd );
   lpSurf->GetSurfaceDesc( &ddsd );

   gAVIWidth  = ddsd.dwWidth;
   gAVIHeight = ddsd.dwHeight;
   gAVIBpp    = ddsd.ddpfPixelFormat.dwRGBBitCount;
   gAVIfps    = MAX( FramesPerSecond, 0 ); //If some wiseguy tries a negative fps :)

   if( gAVIBpp <= 8 )
   {
	   ERROR_LOG("Unsupported Pixel Format.\n");
	   return AVI_ERROR_UNSUPPORTEDPIXELFORMAT;
   }

   AVIFileInit();

   hr = AVIFileOpen( &pAviFile, fname, OF_WRITE | OF_CREATE, NULL );
   if( hr != AVIERR_OK )
   {
	   ERROR_LOG("AVIFileOpen failed.\n");
	   return AVI_ERROR_OTHERERROR;
   }

   BITMAPINFO bmpVideoFormat = 
   {
	  { 
		sizeof( BITMAPINFO ), 
		gAVIWidth, 
		gAVIHeight, 
		1, 
		24, 
		BI_RGB, 
		gAVIWidth * gAVIHeight * 3, 
		0,
		0,
		0,
		0 
	  }
   };  

   
    memset(&aviStreamInfo, 0, sizeof( aviStreamInfo ));

	aviStreamInfo.fccType                = streamtypeVIDEO;
	aviStreamInfo.fccHandler             = 0;
	aviStreamInfo.dwScale                = 1;
	aviStreamInfo.dwRate                 = gAVIfps;		    
	aviStreamInfo.dwSuggestedBufferSize  = 0;
	aviStreamInfo.dwQuality              = -1;
	SetRect(&aviStreamInfo.rcFrame, 0, 0, (int) bmpVideoFormat.bmiHeader.biWidth, (int) bmpVideoFormat.bmiHeader.biHeight);

    hr = AVIFileCreateStream( pAviFile, &psStream, &aviStreamInfo );
    if( hr != AVIERR_OK )
    { 
      ERROR_LOG("Couldn't create AVI stream.\n");
	  CloseAVIFile();
      return AVI_ERROR_OTHERERROR;
    }

    lpOptions = new AVICOMPRESSOPTIONS;	
	memset( lpOptions, 0, sizeof( AVICOMPRESSOPTIONS ));
	
	if( !AVISaveOptions( gMainWnd , ICMF_CHOOSE_KEYFRAME|ICMF_CHOOSE_DATARATE , 1, &psStream, &lpOptions))
	{
      ERROR_LOG("AVISaveOptions failed.\n");
	  CloseAVIFile();
      return AVI_ERROR_OTHERERROR;
    }

    hr = AVIMakeCompressedStream( &psCompressed, psStream, lpOptions, NULL);
    if( hr != AVIERR_OK )
    { 
      ERROR_LOG("Couldn't create compressed stream.\n");
	  CloseAVIFile();
      return AVI_ERROR_OTHERERROR;
    }
	  
    hr = AVIStreamSetFormat( psCompressed, 0, &bmpVideoFormat, sizeof( bmpVideoFormat ));
    if( hr != AVIERR_OK )
    { 
      ERROR_LOG("AVIStreamSetFormat returned an ERROR_LOG.\n");
	  CloseAVIFile();
      return AVI_ERROR_OTHERERROR;
    }

   gVideoFrame = 0;
   bAVIFileOpened = TRUE;

   delete lpOptions;
   return 1;
}

void CloseAVIFile()
{
	if ( psStream )
		AVIStreamClose( psStream );

	if ( psCompressed )
		AVIStreamClose( psCompressed );

	if ( pAviFile )
		AVIFileClose( pAviFile );

	psStream = psCompressed = NULL;
	pAviFile = NULL;
	AVIFileExit();
	bAVIFileOpened = FALSE;
}

int WriteFrame( LPDIRECTDRAWSURFACE7 lpSurf )
{
   DDSURFACEDESC2  ddsd; 
   BYTE           *bSurface;
   BYTE           *bAVIData;
   WORD            wPitch, wPixel;
   HRESULT         hr;
   DWORD           dwIndex, dwPixel;
   int             iX, iY, iByte;
   
   if( !bAVIFileOpened )
	   return AVI_ERROR_OTHERERROR;

   _DDInit( ddsd );
   lpSurf->GetSurfaceDesc( &ddsd );

   if( (gAVIWidth != (int)ddsd.dwWidth) || (gAVIHeight != (int)ddsd.dwHeight) || (gAVIBpp != (int)ddsd.ddpfPixelFormat.dwRGBBitCount ))
   {
	   ERROR_LOG("Wrong Surface.\n");
	   return AVI_ERROR_WRONGSURFACE;
   }
   

   _DDInit( ddsd );    
   hr = lpSurf->Lock( NULL , &ddsd, DDLOCK_SURFACEMEMORYPTR | DDLOCK_WAIT , NULL ); 
   if( FAILED( hr ))
   {
	   ERROR_LOG("Couldn't lock surface.\n");
	   return AVI_ERROR_OTHERERROR;
   }
   bSurface = (BYTE *)ddsd.lpSurface;
   wPitch   = (WORD)ddsd.lPitch;

   if( gAVIBpp == 16 ) wPitch /= 2; 

   dwIndex  = 0;
   bAVIData = new BYTE[ gAVIWidth * gAVIHeight * 3 ];

   switch( gAVIBpp )
   {
      case 32:

          for( iY = gAVIHeight - 1; iY >= 0; iY -- )
		  {
	         for( iX = 0; iX < gAVIWidth; iX ++ )
			 {

		        dwPixel = *((DWORD *)( bSurface + iX * 4 + iY * wPitch ));

  		        iByte                  = ( dwPixel       ) & 0x000000ff; //Blue
		        bAVIData[ dwIndex ++ ] = iByte;
		        iByte                  = ( dwPixel >> 8  ) & 0x000000ff; //Green
		        bAVIData[ dwIndex ++ ] = iByte; 
		        iByte                  = ( dwPixel >> 16 ) & 0x000000ff; //Red
 		        bAVIData[ dwIndex ++ ] = iByte; 

			 }
		  }

	  break;

	  case 24:

          for( iY = gAVIHeight - 1; iY >= 0; iY -- )
		  {
	         for( iX = 0; iX < gAVIWidth; iX ++ )
			 {		        
		        bAVIData[ dwIndex ++ ] = *( bSurface + iX * 3 + iY * wPitch + 2 );
		        bAVIData[ dwIndex ++ ] = *( bSurface + iX * 3 + iY * wPitch + 1 ); 
 		        bAVIData[ dwIndex ++ ] = *( bSurface + iX * 3 + iY * wPitch + 0 ); 
			 }
		  }

	  break;

	  case 16:

 	      if( ddsd.ddpfPixelFormat.dwGBitMask == 0x07E0 ) // 565 mode
		  {
            for( iY = gAVIHeight - 1; iY >= 0; iY -- )
			{
	           for( iX = 0; iX < gAVIWidth; iX ++ )
			   {                    
                   wPixel = *((WORD *)( bSurface + iX * 2 + iY * wPitch * 2));

                   iByte                  = ( 8 * (( wPixel ) & 0x001f ));
		           bAVIData[ dwIndex ++ ] = iByte;

                   iByte                  = ( 4 * (( wPixel >> 5 ) & 0x003f )); 
		           bAVIData[ dwIndex ++ ] = iByte;

                   iByte                  = ( 8 * (( wPixel >> 11 ) & 0x001f ));
		           bAVIData[ dwIndex ++ ] = iByte;
			   }
			}
		  }                           
          else // 5550 (1) mode
		  {
            for( iY = gAVIHeight - 1; iY >= 0; iY -- )
			{
	           for( iX = 0; iX < gAVIWidth; iX ++ )
			   {                    
                   wPixel = *((WORD *)( bSurface + iX * 2 + iY * wPitch * 2));

                   iByte                  = ( 8 * (( wPixel ) & 0x001f ));
		           bAVIData[ dwIndex ++ ] = iByte;

                   iByte                  = ( 8 * (( wPixel >> 5 ) & 0x001f )); 
		           bAVIData[ dwIndex ++ ] = iByte;

                   iByte                  = ( 8 * (( wPixel >> 10 ) & 0x001f ));
 	               bAVIData[ dwIndex ++ ] = iByte;
			   }
			}
		  }

       break;

   } //switch

   hr = AVIStreamWrite( psCompressed, gVideoFrame, 1, bAVIData, gAVIWidth * gAVIHeight * 3, 0, NULL, NULL );
   if (hr != AVIERR_OK)
   { 
      ERROR_LOG("Couldn't write video data.\n");
 	  CloseAVIFile();
      return AVI_ERROR_OTHERERROR;
   }

   gVideoFrame ++;
   delete bAVIData;
   lpSurf->Unlock( NULL );

   return 1;
}