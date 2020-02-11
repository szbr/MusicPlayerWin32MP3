#include "CMainWindow.h"
#include "Util.h"

#pragma comment( linker, "\"/manifestdependency:type='win32' \
name='Microsoft.Windows.Common-Controls' version='6.0.0.0' \
processorArchitecture='*' publicKeyToken='6595b64144ccf1df' language='*'\"" )

#pragma comment( lib, "Strmiids.lib" )


int APIENTRY WinMain(
	_In_ HINSTANCE hInstance,
	_In_opt_ HINSTANCE hPrevInstance,
	_In_ LPSTR lpCmdLine,
	_In_ int nCmdShow )
{
	CMainWindow &w = CMainWindow::Instance( );
	w.Start( hInstance );

	HWND hwndMain = w.MainHWND( );
    ShowWindow( hwndMain, nCmdShow );
    UpdateWindow( hwndMain );

	BOOL bRet;
	MSG msg;
    while( (bRet = GetMessage( &msg, nullptr, 0, 0 )) != 0 )
    {
		if( bRet == -1 )
		{
			FATALERRORMSG( "GetMessage( ) -1" );
		}

        TranslateMessage( &msg );
        DispatchMessageA( &msg );
    }
    return (int)msg.wParam;
}