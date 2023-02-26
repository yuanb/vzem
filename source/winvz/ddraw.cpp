
//-----------------------------------------------------------------------------
// Name: ReleaseAllObjects()
// Desc: Finished with all objects we use; release them
//-----------------------------------------------------------------------------
static void
ReleaseAllObjects(void)
{
    if (g_pDD != NULL)
    {
        g_pDD->SetCooperativeLevel(g_hWnd, DDSCL_NORMAL);
		if (g_pDDSBuffer != NULL)
        {
            g_pDDSBuffer->Release();
            g_pDDSBuffer = NULL;
        }
        if (g_pDDSPrimary != NULL)
        {
            g_pDDSPrimary->Release();
            g_pDDSPrimary = NULL;
        }

    }
}

//-----------------------------------------------------------------------------
// Name: InitFail()
// Desc: This function is called if an initialization function fails
//-----------------------------------------------------------------------------
HRESULT
InitFail(HWND hWnd, HRESULT hRet, LPCTSTR szError,...)
{
    char                        szBuff[128];
    va_list                     vl;

    va_start(vl, szError);
    vsprintf(szBuff, szError, vl);
    ReleaseAllObjects();
    MessageBox(hWnd, szBuff, TITLE, MB_OK);
    DestroyWindow(hWnd);
    va_end(vl);
    return hRet;
}


//-----------------------------------------------------------------------------
// Name: InitSurfaces()
// Desc: Create all the needed DDraw surfaces and set the coop level
//-----------------------------------------------------------------------------
static HRESULT
InitSurfaces(HWND hWnd)
{
    HRESULT		        hRet;
    DDSURFACEDESC2      ddsd;
    DDSCAPS2            ddscaps;
    LPDIRECTDRAWCLIPPER pClipper;

    if (g_bWindowed)
    {
        // Get normal windowed mode
        hRet = g_pDD->SetCooperativeLevel(hWnd, DDSCL_NORMAL);
        if (hRet != DD_OK)
            return InitFail(hWnd, hRet, "SetCooperativeLevel FAILED");

    	// Get the dimensions of the viewport and screen bounds
    	GetClientRect(hWnd, &g_rcViewport);
    	GetClientRect(hWnd, &g_rcScreen);
    	ClientToScreen(hWnd, (POINT*)&g_rcScreen.left);
    	ClientToScreen(hWnd, (POINT*)&g_rcScreen.right);

        // Create the primary surface
        ZeroMemory(&ddsd,sizeof(ddsd));
        ddsd.dwSize = sizeof(ddsd);
        ddsd.dwFlags = DDSD_CAPS;
        ddsd.ddsCaps.dwCaps = DDSCAPS_PRIMARYSURFACE;
        hRet = g_pDD->CreateSurface(&ddsd, &g_pDDSPrimary, NULL);
        if (hRet != DD_OK)
            return InitFail(hWnd, hRet, "CreateSurface FAILED");

        // Create a clipper object since this is for a Windowed render
        hRet = g_pDD->CreateClipper(0, &pClipper, NULL);
        if (hRet != DD_OK)
            return InitFail(hWnd, hRet, "CreateClipper FAILED");

        // Associate the clipper with the window
        pClipper->SetHWnd(0, hWnd);
        g_pDDSPrimary->SetClipper(pClipper);
        pClipper->Release();
        pClipper = NULL;


        // Get the backbuffer. For fullscreen mode, the backbuffer was created
        // along with the primary, but windowed mode still needs to create one.
	    ddsd.dwFlags        = DDSD_WIDTH | DDSD_HEIGHT | DDSD_CAPS;
		ddsd.dwWidth        = SIZEX;
		ddsd.dwHeight       = SIZEY;
		ddsd.ddsCaps.dwCaps = DDSCAPS_OFFSCREENPLAIN;
        hRet = g_pDD->CreateSurface(&ddsd, &g_pDDSBuffer, NULL);
        if (hRet != DD_OK)
            return InitFail(hWnd, hRet, "CreateSurface2 FAILED");
		
		g_bdds = ddsd;			// Save surface description for faster blitting
	    DDPIXELFORMAT DDpf;

		ZeroMemory (&DDpf, sizeof(DDpf));
		DDpf.dwSize = sizeof(DDpf);
		g_pDDSBuffer->GetPixelFormat(&DDpf);
		BytesPerPixel = DDpf.dwRGBBitCount/8;
    }
    else
    {
        // Get exclusive mode
        hRet = g_pDD->SetCooperativeLevel(hWnd, DDSCL_EXCLUSIVE |
                                                DDSCL_FULLSCREEN);
        if (hRet != DD_OK)
            return InitFail(hWnd, hRet, "SetCooperativeLevel FAILED");

        // Set the video mode to 640x480x32
        hRet = g_pDD->SetDisplayMode( 640, 480, 32, 0, 0);
        if (hRet != DD_OK)
            return InitFail(hWnd, hRet, "SetDisplayMode FAILED");

    	// Get the dimensions of the viewport and screen bounds
    	// Store the rectangle which contains the renderer
    	SetRect(&g_rcViewport, 0, 0, 640, 480 );
    	memcpy(&g_rcScreen, &g_rcViewport, sizeof(RECT) );

        // Create the primary surface with 1 back buffer
        ZeroMemory(&ddsd,sizeof(ddsd));
        ddsd.dwSize = sizeof(ddsd);
        ddsd.dwFlags = DDSD_CAPS |
                       DDSD_BACKBUFFERCOUNT;
        ddsd.ddsCaps.dwCaps = DDSCAPS_PRIMARYSURFACE |
                			  DDSCAPS_FLIP |
                			  DDSCAPS_COMPLEX;
        ddsd.dwBackBufferCount = 1;
        hRet = g_pDD->CreateSurface( &ddsd, &g_pDDSPrimary, NULL);
        if (hRet != DD_OK)
            return InitFail(hWnd, hRet, "CreateSurface FAILED");

        ZeroMemory(&ddscaps, sizeof(ddscaps));
        ddscaps.dwCaps = DDSCAPS_BACKBUFFER;
        hRet = g_pDDSPrimary->GetAttachedSurface(&ddscaps, &g_pDDSBuffer);
        if (hRet != DD_OK)
            return InitFail(hWnd, hRet, "GetAttachedSurface FAILED");

		g_bdds = ddsd;			// Save surface description for faster blitting
	    DDPIXELFORMAT DDpf;

		ZeroMemory (&DDpf, sizeof(DDpf));
		DDpf.dwSize = sizeof(DDpf);
		g_pDDSBuffer->GetPixelFormat(&DDpf);
		BytesPerPixel = DDpf.dwRGBBitCount/8;
    }
    return DD_OK;
}



//-----------------------------------------------------------------------------
// Name: RestoreAll()
// Desc: Restore all lost objects
//-----------------------------------------------------------------------------
BOOL
RestoreAll(void)
{
    return g_pDDSPrimary->Restore() == DD_OK &&
        g_pDDSBuffer->Restore();

}

//-----------------------------------------------------------------------------
// Name: ChangeCoopLevel()
// Desc: Called when the user wants to toggle between Full-Screen & Windowed
//-----------------------------------------------------------------------------
HRESULT
ChangeCoopLevel(HWND hWnd )
{
    HRESULT hRet;

    // Release all objects that need to be re-created for the new device
    ReleaseAllObjects();

    // In case we're coming from a fullscreen mode, restore the window size
    if (g_bWindowed)
    {
        SetWindowPos(hWnd, HWND_NOTOPMOST, g_rcWindow.left, g_rcWindow.top,
                     (g_rcWindow.right - g_rcWindow.left), 
                     (g_rcWindow.bottom - g_rcWindow.top), SWP_SHOWWINDOW );
    }

    // Re-create the surfaces
    hRet = InitSurfaces(hWnd);
    return hRet;
}
