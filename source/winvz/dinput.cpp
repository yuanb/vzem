

//-----------------------------------------------------------------------------
// Name: EnumJoysticksCallback()
// Desc: Called once for each enumerated joystick. If we find one, 
//       create a device interface on it so we can play with it.
//-----------------------------------------------------------------------------
BOOL CALLBACK EnumJoysticksCallback( LPCDIDEVICEINSTANCE pInst, 
                                     VOID* pvContext )
{
    memcpy( pvContext, &pInst->guidInstance, sizeof(GUID) );

    return DIENUM_STOP;
}




//-----------------------------------------------------------------------------
// Name: CreateDInput()
// Desc: Initialize the DirectInput objects.
//-----------------------------------------------------------------------------
HRESULT CreateDInput( HWND hWnd )
{
    // Create the main DirectInput object

 

    HRESULT hr = DirectInputCreateEx( hInst, DIRECTINPUT_VERSION,IID_IDirectInput7, (LPVOID*)&g_pDI, NULL );
    if( FAILED(hr) ) 
        return hr;

    // Check to see if a joystick is present. If so, the enumeration callback
    // will save the joystick's GUID, so we can create it later.
    ZeroMemory( &g_guidJoystick, sizeof(GUID) );
    
    //g_pDI->EnumDevices( DIDEVTYPE_JOYSTICK, EnumJoysticksCallback,
    //                    &g_guidJoystick, DIEDFL_ATTACHEDONLY );

    return S_OK;
}




//-----------------------------------------------------------------------------
// Name: CreateInputDevice()
// Desc: Create a DirectInput device.
//-----------------------------------------------------------------------------
HRESULT CreateInputDevice( HWND hWnd, GUID guidDevice,
                           const DIDATAFORMAT* pdidDataFormat, DWORD dwFlags )
{
    // Obtain an interface to the input device
    if( FAILED( g_pDI->CreateDeviceEx( guidDevice, IID_IDirectInputDevice2,
		                               (VOID**)&g_pdidDevice2, NULL ) ) )
    {
        return E_FAIL;
    }


    // Set the device data format. Note: a data format specifies which
    // controls on a device we are interested in, and how they should be
    // reported.
    if( FAILED( g_pdidDevice2->SetDataFormat( pdidDataFormat ) ) )
    {
        return E_FAIL;
    }

    // Set the cooperativity level to let DirectInput know how this device
    // should interact with the system and with other DirectInput applications.
    if( FAILED( g_pdidDevice2->SetCooperativeLevel( hWnd, dwFlags ) ) )
    {
        return E_FAIL;
    }

    return S_OK;
}




//-----------------------------------------------------------------------------
// Name: DestroyInputDevice()
// Desc: Release the DirectInput device
//-----------------------------------------------------------------------------
VOID DestroyInputDevice()
{
    // Unacquire and release the device's interfaces
    if( g_pdidDevice2 )
    {
        g_pdidDevice2->Unacquire();
        g_pdidDevice2->Release();
        g_pdidDevice2 = NULL;
    }
        
}




//-----------------------------------------------------------------------------
// Name: DestroyDInput()
// Desc: Terminate our usage of DirectInput
//-----------------------------------------------------------------------------
VOID DestroyDInput()
{
    // Destroy the DInput object
    if( g_pDI )
        g_pDI->Release();
    g_pDI = NULL;
}
