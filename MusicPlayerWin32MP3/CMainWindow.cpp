#include "CMainWindow.h"
#include <array>
#include <CommCtrl.h>
#include "Util.h"
#include "resource.h"


CMainWindow *CMainWindow::pInstance = nullptr;

constexpr const wchar_t *szClassName = L"myWindowClass";
constexpr const wchar_t *szTitle = L">> Music Player <<";

CMainWindow::CMainWindow( ) :
	iSPos( 0 ), bPaused( false ),
	dwTextColor( RGB( 0, 200, 200 ) ), dwBackgroundColor( RGB( 85, 85, 85 ) ),
	dwBorderColor( RGB( 50, 50, 50 ) ), dwSliderColor( RGB( 50, 50, 50 ) ),
	rectStatus{ 435, 140, 520, 160 }, rectSlider{ 10, 140, 425, 160 },
	rectSliderDrawArea{ 5, 135, 430, 165 }, rectSearch{ 160, 65, 310, 95 },
	iSliderLength( rectSlider.right - rectSlider.left )
{
	pInstance = this;
}

void CMainWindow::Start( HINSTANCE hInstance )
{
	hMutex = CreateMutex( nullptr, TRUE, L"MusicPlayerMp3" );
	if( GetLastError( ) == ERROR_ALREADY_EXISTS )
	{
		FATALERRORMSG( L"Already running." );
	}

	HRESULT hrCoInit = CoInitialize( nullptr );
	if( FAILED( hrCoInit ) )
	{
		FATALERRORMSG( L"if( FAILED( hrCoInit ) )" );
	}

	WNDCLASSEX wc;
    wc.cbSize        = sizeof( WNDCLASSEX );
    wc.style         = 0;
    wc.lpfnWndProc   = &CMainWindow::WndProc;
    wc.cbClsExtra    = 0;
    wc.cbWndExtra    = 0;
    wc.hInstance     = hInstance;
    wc.hIcon         = LoadIcon( nullptr, IDI_APPLICATION );
    wc.hCursor       = LoadCursor( nullptr, IDC_ARROW );
    wc.hbrBackground = (HBRUSH)( COLOR_WINDOW + 1 );
    wc.lpszMenuName  = nullptr;
    wc.lpszClassName = szClassName;
    wc.hIconSm       = LoadIcon( nullptr, IDI_APPLICATION );

    if( !RegisterClassEx( &wc ) )
    {
		FATALERRORMSG( L"Window Registration Failed!" );
    }

	hwndMainWindow = CreateWindow( szClassName, L"Music Player",
								   WS_POPUP,
								   CW_USEDEFAULT, CW_USEDEFAULT, WINDOW_WIDTH, WINDOW_HEIGHT,
								   nullptr, nullptr, hInstance, nullptr );
	HWNDCHECK( hwndMainWindow );
	musicPlayer.hwndMain = hwndMainWindow;


	//============================================================================================
	// LOAD CONFIG
	//============================================================================================
	boost::property_tree::wptree pt;
	if( fileSystem.LoadConfig( pt ) )
	{
		std::wstring strHeader = fileSystem.GetINISection( CFileSystem::INISEC_MP3CONFIG );
		CheckDlgButton( hwndMainWindow, ID_RND,		pt.get<bool>( strHeader + fileSystem.GetININame( CFileSystem::INISEC_MP3CONFIG, 1 ) ) ? BST_CHECKED : BST_UNCHECKED );
		CheckDlgButton( hwndMainWindow, ID_AUTO,	pt.get<bool>( strHeader + fileSystem.GetININame( CFileSystem::INISEC_MP3CONFIG, 2 ) ) ? BST_CHECKED : BST_UNCHECKED );
		CheckDlgButton( hwndMainWindow, ID_HOTKEYS, pt.get<bool>( strHeader + fileSystem.GetININame( CFileSystem::INISEC_MP3CONFIG, 3 ) ) ? BST_CHECKED : BST_UNCHECKED );
	
		strHeader = fileSystem.GetINISection( CFileSystem::INISEC_HOTKEYS );
		std::array<WORD, 6> wKeys;
		for( size_t i = 0; i < wKeys.size( ); i++ )
		{
			wKeys[i] = pt.get<WORD>( strHeader + fileSystem.GetININame( CFileSystem::INISEC_HOTKEYS, i ) );
		}
		SendMessage( hkNext,		HKM_SETHOTKEY, MAKEWORD( wKeys[0], 0 ), 0 );
		SendMessage( hkPrev,		HKM_SETHOTKEY, MAKEWORD( wKeys[1], 0 ), 0 );
		SendMessage( hkPauseResume,	HKM_SETHOTKEY, MAKEWORD( wKeys[2], 0 ), 0 );
		SendMessage( hkStop,		HKM_SETHOTKEY, MAKEWORD( wKeys[3], 0 ), 0 );
		SendMessage( hkStepUp,		HKM_SETHOTKEY, MAKEWORD( wKeys[4], 0 ), 0 );
		SendMessage( hkStepDown,	HKM_SETHOTKEY, MAKEWORD( wKeys[5], 0 ), 0 );

		for( size_t i = 0; i < wKeys.size( ); i++ )
		{
			if( wKeys[i] )
				RegisterHotKey( hwndMainWindow, ID_HK_NEXT + i, 0, wKeys[i] );
		}

		DirScan( );
	}
	//============================================================================================


	//============================================================================================
	// SYSTRAY ICON & MENU
	//============================================================================================
	ZeroMemory( &niData, sizeof( niData ) );
	niData.cbSize = sizeof( niData );
	niData.hWnd = hwndMainWindow;
	niData.uID = SYSTRAY_ID;
	niData.uCallbackMessage = SYSTRAY_MSG;
	niData.hIcon = LoadIcon( hInstance, MAKEINTRESOURCE( IDI_ICON1 ) );
	wcscpy_s( niData.szInfo, L"Music Player has started!" );
	wcscpy_s( niData.szInfoTitle, L"Music Player" );
	niData.dwInfoFlags = NIIF_INFO;
	niData.uFlags = NIF_MESSAGE | NIF_TIP | NIF_ICON | NIF_INFO;
	wcscpy_s( niData.szTip, L"Music Player" );
	Shell_NotifyIcon( NIM_ADD, &niData );

	hPopupMenu = CreatePopupMenu( );
	AppendMenu( hPopupMenu, MF_STRING, SYSTRAY_MENU_RESTORE, L"Restore" );
	AppendMenu( hPopupMenu, MF_STRING, SYSTRAY_MENU_MINIMIZE, L"Minimize" );
	AppendMenu( hPopupMenu, MF_SEPARATOR, -1, nullptr );
	AppendMenu( hPopupMenu, MF_STRING, SYSTRAY_MENU_PLAY, L"Play" );
	AppendMenu( hPopupMenu, MF_STRING, SYSTRAY_MENU_PAUSERESUME, L"Pause/Resume" );
	AppendMenu( hPopupMenu, MF_STRING, SYSTRAY_MENU_STOP, L"Stop" );
	AppendMenu( hPopupMenu, MF_STRING, SYSTRAY_MENU_NEXT, L"Next" );
	AppendMenu( hPopupMenu, MF_STRING, SYSTRAY_MENU_PREVIOUS, L"Previous" );
	AppendMenu( hPopupMenu, MF_STRING, SYSTRAY_MENU_STEPDOWN, L"Step Down" );
	AppendMenu( hPopupMenu, MF_STRING, SYSTRAY_MENU_STEPUP, L"Step Up" );
	AppendMenu( hPopupMenu, MF_STRING, SYSTRAY_MENU_RESTART, L"Restart" );
	AppendMenu( hPopupMenu, MF_SEPARATOR, -1, nullptr );
	AppendMenu( hPopupMenu, MF_STRING, SYSTRAY_MENU_ABOUT, L"About" );
	AppendMenu( hPopupMenu, MF_STRING, SYSTRAY_MENU_EXIT, L"Exit" );
	//============================================================================================
}

LRESULT CALLBACK CMainWindow::WndProc( HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam )
{
	return pInstance->ThisWndProc( hwnd, msg, wParam, lParam );
}

LRESULT CALLBACK CMainWindow::ThisWndProc( HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam )
{
    switch( msg )
    {
		case WM_CREATE:
		{
			OnCreate( hwnd );
			break;
		}
		case WM_PAINT:
		{
			OnPaint( );
			break;
		}
		case WM_COMMAND:
		{
			OnCommand( hwnd, wParam, lParam );
			break;
		}
		case WM_ERASEBKGND:
		{
			RECT rect;
			GetClientRect( hwnd, &rect );
			FillRect( (HDC)wParam, &rect, hbBackground );
			break;
		}
		case WM_CTLCOLORSTATIC:
		case WM_CTLCOLORLISTBOX:
		case WM_CTLCOLORBTN:
		{
			const HDC hdc = (HDC)wParam;
			SetBkMode( hdc, TRANSPARENT );
			SetTextColor( hdc, dwTextColor );
			return (LRESULT)hbBackground;
		}
		case WM_NCHITTEST:
		{
			POINT point;
			point.x = GET_X_LPARAM( lParam );
			point.y = GET_Y_LPARAM( lParam );
			ScreenToClient( hwnd, &point );

			if( point.y < 30 )
			{
				return (LRESULT)HTCAPTION;
			}
			return (LRESULT)HTCLIENT;
		}
		case WM_HOTKEY:
		{
			switch( wParam )
			{
				case ID_HK_NEXT:
					SendMessage( hwnd, WM_COMMAND, MAKEWPARAM( ID_NEXT, 0 ), 0 );
					break;
				case ID_HK_PREV:
					SendMessage( hwnd, WM_COMMAND, MAKEWPARAM( ID_PREVIOUS, 0 ), 0 );
					break;
				case ID_HK_PAUSERESUME:
					SendMessage( hwnd, WM_COMMAND, MAKEWPARAM( ID_PAUSE_RESUME, 0 ), 0 );
					break;
				case ID_HK_STOP:
					SendMessage( hwnd, WM_COMMAND, MAKEWPARAM( ID_STOP, 0 ), 0 );
					break;
				case ID_HK_STEPUP:
					SendMessage( hwnd, WM_COMMAND, MAKEWPARAM( ID_STEPUP, 0 ), 0 );
					break;
				case ID_HK_STEPDOWN:
					SendMessage( hwnd, WM_COMMAND, MAKEWPARAM( ID_STEPDOWN, 0 ), 0 );
					break;
			}
			break;
		}
		case WM_MOUSEMOVE:
		{
			if( musicPlayer.pCurMusic )
			{
				const int x = GET_X_LPARAM( lParam );
				const int y = GET_Y_LPARAM( lParam );
				if( IsInRect( x, y, rectSlider ) )
				{
					if( wParam == MK_LBUTTON )
					{
						iSPos = GET_X_LPARAM( lParam ) - rectSlider.left;
						InvalidateRect( hwnd, &rectSliderDrawArea, true );
					}
				}
			}
			break;
		}
		case WM_LBUTTONUP:
		{
			if( musicPlayer.pCurMusic )
			{
				const int x = GET_X_LPARAM( lParam );
				const int y = GET_Y_LPARAM( lParam );
				if( IsInRect( x, y, rectSlider ) )
				{
					iSPos = x - rectSlider.left;
					InvalidateRect( hwnd, &rectSliderDrawArea, true );
					musicPlayer.Seek( (int)( (float)iSPos / iSliderLength * (float)musicPlayer.Length( ) ) );
				}
			}
			break;
		}
		case WM_TIMER:
		{
			iSPos = (int)( (float)musicPlayer.Pos( ) / (float)musicPlayer.Length( ) * iSliderLength );
			InvalidateRect( hwnd, &rectSliderDrawArea, true );
			InvalidateRect( hwnd, &rectStatus, true );

			if( iSPos == iSliderLength )
			{
				if( IsDlgButtonChecked( hwnd, ID_EXITFINISH ) == BST_CHECKED )
				{
					CleanUp( );
				}
				else if( IsDlgButtonChecked( hwnd, ID_AUTO ) == BST_CHECKED )
				{
					musicPlayer.Close( );
					musicPlayer.Play( IsDlgButtonChecked( hwnd, ID_RND ) == BST_CHECKED );
					InvalidateRect( hwnd, nullptr, true );
				}
			}
			break;
		}
		case SYSTRAY_MSG:
		{
			OnSystrayMsg( hwnd, wParam, lParam );
			break;
		}
		case WM_CLOSE:
		{
			CleanUp( );
			break;
		}
		case WM_DESTROY:
		{
			DeleteObject( hbBackground );
			DeleteObject( hpBorder );
			DeleteObject( hpSlider );
			PostQuitMessage( 0 );
			break;
		}
		default:
		{
			return DefWindowProc( hwnd, msg, wParam, lParam );
		}
	}
    return 0;
}

void CMainWindow::OnCreate( HWND hwnd )
{
	const HINSTANCE hInstance = GetModuleHandle( nullptr );
	if( hInstance == NULL )
	{
		FATALERRORMSG( L"Failed getting hInstance" );
	}


	//============================================================================================
	// TEXTS
	//============================================================================================
	const HDC hdc = GetDC( hwnd );
	if( hdc == NULL )
	{
		FATALERRORMSG( L"Failed getting DC" );
	}

	RECT rect;
	DrawText( hdc, szTitle, wcslen( szTitle ), &rect, DT_CALCRECT );
	const LONG lWidth = rect.right - rect.left;
	ReleaseDC( hwnd, hdc );

	CreateStaticCtrl( szTitle, WINDOW_WIDTH / 2 - lWidth / 2, 5, 125, 20, hwnd, (HMENU)-1, hInstance );
	CreateStaticCtrl( L"[Next]",    430, 270, 50, 20, hwnd, (HMENU)-1, hInstance );
	CreateStaticCtrl( L"[Prev]",    430, 320, 50, 20, hwnd, (HMENU)-1, hInstance );
	CreateStaticCtrl( L"[Pause]",   430, 370, 50, 20, hwnd, (HMENU)-1, hInstance );
	CreateStaticCtrl( L"[Stop]",    430, 420, 50, 20, hwnd, (HMENU)-1, hInstance );
	CreateStaticCtrl( L"[Step XX]", 430, 470, 60, 20, hwnd, (HMENU)-1, hInstance );
	CreateStaticCtrl( L"[Step XX]", 430, 520, 60, 20, hwnd, (HMENU)-1, hInstance );
	//============================================================================================

	//============================================================================================
	// MAIN CONTROLS
	//============================================================================================
	CreateButton( btnDirSelect,		L"Select Path", 10, 10, 100, 25,	hwnd, (HMENU)ID_DIRSELECT, hInstance );
	CreateButton( btnPlay,			L"Play", 10, 110, 60, 25,			hwnd, (HMENU)ID_PLAY, hInstance );
	CreateButton( btnPauseResume,	L"Pause", 70, 110, 60, 25,			hwnd, (HMENU)ID_PAUSE_RESUME, hInstance );
	CreateButton( btnStop,			L"Stop", 130, 110, 60, 25,			hwnd, (HMENU)ID_STOP, hInstance );
	CreateButton( btnNext,			L"Next", 190, 110, 60, 25,			hwnd, (HMENU)ID_NEXT, hInstance );
	CreateButton( btnPrev,			L"Prev.", 250, 110, 60, 25,			hwnd, (HMENU)ID_PREVIOUS, hInstance );
	CreateButton( btnStepDown,		L"\u25BC", 310, 110, 30, 25,		hwnd, (HMENU)ID_STEPDOWN, hInstance );
	CreateButton( btnStepUp,		L"\u25B2", 340, 110, 30, 25,		hwnd, (HMENU)ID_STEPUP, hInstance );
	CreateButton( btnRestart,		L"Restart", 370, 110, 60, 25,		hwnd, (HMENU)ID_RESTART, hInstance );
	CreateButton( btnSave,			L"Save Config", 430, 165, 100, 25,	hwnd, (HMENU)ID_SAVE, hInstance );


	CreateBitmapButton( btnInfo, 475, 0, 25, 25, hwnd, (HMENU)ID_INFO,    hInstance, IDB_INFOBTN );
	CreateBitmapButton( btnMin,  500, 0, 25, 25, hwnd, (HMENU)ID_MINBTN,  hInstance, IDB_MINBTN );
	CreateBitmapButton( btnExit, 525, 0, 25, 25, hwnd, (HMENU)ID_EXITBTN, hInstance, IDB_EXITBTN );


	editSearch = CreateWindow( L"EDIT", L"", WS_VISIBLE | WS_CHILD | WS_BORDER,
							   10, 70, 150, 20, hwnd, (HMENU)ID_SEARCH, hInstance, nullptr );
	HWNDCHECK( editSearch );

	listMusics = CreateWindowEx( WS_EX_CLIENTEDGE, L"LISTBOX", L"", 
								 WS_CHILD | WS_VISIBLE | WS_VSCROLL | LBS_NOTIFY,
								 10, 170, 415, 400, hwnd, (HMENU)ID_LIST, hInstance, nullptr );
	HWNDCHECK( listMusics );
	//============================================================================================


	//============================================================================================
	// SIDE CONTROLS
	//============================================================================================
	CreateCheckbox( cboxExitOnFinish,	L"Exit@finish", 430, 190, 100, 20, hwnd, (HMENU)ID_EXITFINISH, hInstance );
	CreateCheckbox( cboxRandom,			L"Random",		430, 210, 80, 20,  hwnd, (HMENU)ID_RND, hInstance );
	CreateCheckbox( cboxAutoPlay,		L"Autoplay",	430, 230, 80, 20,  hwnd, (HMENU)ID_AUTO, hInstance );
	CreateCheckbox( cboxHotkeys,		L"Hotkeys",		430, 250, 80, 20,  hwnd, (HMENU)ID_HOTKEYS, hInstance );

	// Win7 fix
	/* #pragma comment( lib, "UxTheme.lib" )
	#include <Uxtheme.h>
	SetWindowTheme( cboxExitOnFinish,	L" ", L" " );
	SetWindowTheme( cboxRandom,			L" ", L" " );
	SetWindowTheme( cboxAutoPlay,		L" ", L" " );
	SetWindowTheme( cboxHotkeys,		L" ", L" " );*/

	CreateHotkey( hkNext,			430, 290, 100, 20, hwnd, (HMENU)ID_HK_NEXT, hInstance );
	CreateHotkey( hkPrev,			430, 340, 100, 20, hwnd, (HMENU)ID_HK_PREV, hInstance );
	CreateHotkey( hkPauseResume,	430, 390, 100, 20, hwnd, (HMENU)ID_HK_PAUSERESUME, hInstance );
	CreateHotkey( hkStop,			430, 440, 100, 20, hwnd, (HMENU)ID_HK_STOP, hInstance );
	CreateHotkey( hkStepUp,			430, 490, 100, 20, hwnd, (HMENU)ID_HK_STEPUP, hInstance );
	CreateHotkey( hkStepDown,		430, 540, 100, 20, hwnd, (HMENU)ID_HK_STEPDOWN, hInstance );
	//============================================================================================


	//============================================================================================
	hbBackground = CreateSolidBrush( dwBackgroundColor );
	hpBorder = CreatePen( PS_SOLID, 10, dwBorderColor );
	hpSlider = CreatePen( PS_SOLID, 3, dwSliderColor );
	//============================================================================================
}

void CMainWindow::OnPaint( )
{
	PAINTSTRUCT ps;
	HDC hdc;
	RECT rect;

	hdc = BeginPaint( hwndMainWindow, &ps );
	GetClientRect( hwndMainWindow, &rect );


	const int iSavedDC = SaveDC( hdc );
	SelectObject( hdc, hpBorder );
	SelectObject( hdc, GetStockObject( NULL_BRUSH ) );
	Rectangle( hdc, 0, 0, (rect.right - rect.left), (rect.bottom - rect.top) );
	RestoreDC( hdc, iSavedDC );

	SetTextColor( hdc, dwTextColor );	
	LOGBRUSH logbrush;
	GetObject( hbBackground, sizeof( LOGBRUSH ), &logbrush );
	SetBkColor( hdc, logbrush.lbColor );

	rect.left += 10;
	rect.top += 35;

	// Draw path
	std::wstring strPath = L"Path=" + std::wstring( fileSystem.szPath );
	DrawText( hdc, strPath.c_str( ), strPath.length( ), &rect, DT_LEFT );

	// Loaded files
	std::wstring strFilesLoaded = L"--> " + std::to_wstring( musicPlayer.vMusics.size( ) ) + L" .mp3 file(s) loaded!";
	rect.top += 15;
	DrawText( hdc, strFilesLoaded.c_str( ), strFilesLoaded.length( ), &rect, DT_LEFT );

	// Currently played music
	std::wstring music = musicPlayer.pCurMusic == nullptr ? L">> None" : std::to_wstring( musicPlayer.uIndex ) + L") " + musicPlayer.pCurMusic->strName;
	rect.top += 45;
	DrawText( hdc, music.c_str( ), music.length( ), &rect, DT_LEFT );


	// Search
	if( szSearchText[0] )
	{
		std::vector<size_t> vIndexList;
		for( size_t i = 0; i < musicPlayer.vMusics.size( ); i++ )
		{
			std::wstring strMusicName = musicPlayer.vMusics[i].strName;
			std::wstring strSearch( szSearchText );

			std::transform( strMusicName.begin( ), strMusicName.end( ), strMusicName.begin( ), towlower );
			std::transform( strSearch.begin( ), strSearch.end( ), strSearch.begin( ), towlower );
			
			if( strMusicName.find( strSearch ) != std::wstring::npos )
			{
				vIndexList.push_back( i );
			}
		}

		if( !vIndexList.empty( ) )
		{
			wchar_t buff[32];
			wcscpy_s( buff, L"--> " );
			
			for( size_t i = 0; i < 5 && i < vIndexList.size( ); i++ )
			{
				wchar_t tmp[8];
				StringCbPrintf( tmp, sizeof( tmp ), L"%d, ", vIndexList[i] );
				wcscat_s( buff, tmp );
			}
			TextOut( hdc, 160, 70, buff, wcslen( buff ) );
		}
	}

	// Time
	wchar_t szTime[32];
	USHORT uTime[4];
	if( musicPlayer.pCurMusic == nullptr )
	{
		ZeroMemory( uTime, sizeof( uTime ) );
	}
	else
	{
		USHORT tmp = musicPlayer.Pos( ); // (s)
		uTime[0] = tmp / 60;
		uTime[1] = tmp % 60;
		tmp = musicPlayer.Length( );
		uTime[2] = tmp / 60;
		uTime[3] = tmp % 60;
	}
	StringCbPrintf( szTime, sizeof( szTime ), L"%d:%d / %d:%d", uTime[0], uTime[1], uTime[2], uTime[3] );
	DrawText( hdc, szTime, wcslen( szTime ), &rectStatus, DT_LEFT );

	// Slider
	SelectObject( hdc, hpSlider );
	MoveToEx( hdc, 10, 150, nullptr );
	LineTo( hdc, 425, 150 );
	MoveToEx( hdc, rectSlider.left + iSPos, 140, nullptr );
	LineTo( hdc, rectSlider.left + iSPos, 160 );

	EndPaint( hwndMainWindow, &ps );
}

void CMainWindow::OnCommand( HWND hwnd, WPARAM wParam, LPARAM lParam )
{
	switch( HIWORD( wParam ) )
	{
		case LBN_DBLCLK:
		{
			int iIndex = SendMessage( listMusics, LB_GETCURSEL, 0, 0 );
			musicPlayer.Close( );
			musicPlayer.Play( false, iIndex );
			InvalidateRect( hwnd, nullptr, true );
			break;
		}
		case EN_CHANGE:
		{
			const WORD wCtrlID = LOWORD( wParam );
			if( wCtrlID == ID_SEARCH )
			{
				GetWindowText( editSearch, szSearchText, sizeof( szSearchText ) / sizeof( szSearchText[0] ) );
				InvalidateRect( hwnd, &rectSearch, true );
			}
			else
			{
				HWND hwndKey;
				switch( wCtrlID )
				{
					case ID_HK_NEXT:		hwndKey = hkNext; break;
					case ID_HK_PREV:		hwndKey = hkPrev; break;
					case ID_HK_PAUSERESUME: hwndKey = hkPauseResume; break;
					case ID_HK_STOP:		hwndKey = hkStop; break;
					case ID_HK_STEPDOWN:	hwndKey = hkStepDown; break;
					case ID_HK_STEPUP:		hwndKey = hkStepUp; break;
					default: FATALERRORMSG( L"Hotkey ID not found" );
				}

				const BYTE btKey = LOBYTE( SendMessage( hwndKey, HKM_GETHOTKEY, 0, 0 ) );
				UnregisterHotKey( hwnd, wCtrlID );
				RegisterHotKey( hwnd, wCtrlID, 0, btKey );
			}
			break;
		}
	}
	switch( LOWORD( wParam ) )
	{
		case ID_DIRSELECT:
		{
			OnDirselect( );
			break;
		}
		case ID_SAVE:
		{
			std::vector<bool> vConfigBools;
			std::vector<BYTE> vHotkeys;
			
			vConfigBools.push_back( IsDlgButtonChecked( hwnd, ID_RND )     == BST_CHECKED );
			vConfigBools.push_back( IsDlgButtonChecked( hwnd, ID_AUTO )    == BST_CHECKED );
			vConfigBools.push_back( IsDlgButtonChecked( hwnd, ID_HOTKEYS ) == BST_CHECKED );

			vHotkeys.push_back( LOBYTE( SendMessage( hkNext,		HKM_GETHOTKEY, 0, 0 ) ) );
			vHotkeys.push_back( LOBYTE( SendMessage( hkPrev,		HKM_GETHOTKEY, 0, 0 ) ) );
			vHotkeys.push_back( LOBYTE( SendMessage( hkPauseResume,	HKM_GETHOTKEY, 0, 0 ) ) );
			vHotkeys.push_back( LOBYTE( SendMessage( hkStop,		HKM_GETHOTKEY, 0, 0 ) ) );
			vHotkeys.push_back( LOBYTE( SendMessage( hkStepUp,		HKM_GETHOTKEY, 0, 0 ) ) );
			vHotkeys.push_back( LOBYTE( SendMessage( hkStepDown,	HKM_GETHOTKEY, 0, 0 ) ) );

			fileSystem.SaveConfig( vConfigBools, vHotkeys );
			break;
		}
		case ID_PLAY:
		{
			musicPlayer.Play( IsDlgButtonChecked( hwnd, ID_RND ) == BST_CHECKED );
			InvalidateRect( hwnd, nullptr, true );
			break;
		}
		case ID_PAUSE_RESUME:
		{
			if( musicPlayer.pCurMusic == nullptr )
				break;

			bPaused = !bPaused;
			UpdatePauseResume( true );
			break;
		}
		case ID_STOP:
		{
			musicPlayer.Close( );
			iSPos = 0;
			UpdatePauseResume( false );
			InvalidateRect( hwnd, nullptr, true );
			break;
		}
		case ID_NEXT:
		{
			musicPlayer.Close( );
			musicPlayer.Play( IsDlgButtonChecked( hwnd, ID_RND ) == BST_CHECKED );
			UpdatePauseResume( false );
			InvalidateRect( hwnd, nullptr, true );
			break;
		}
		case ID_PREVIOUS:
		{
			if( musicPlayer.iPrevious == -1 )
				break;

			musicPlayer.Close( );
			musicPlayer.Play( false, musicPlayer.iPrevious );
			UpdatePauseResume( false );
			InvalidateRect( hwnd, nullptr, true );
			break;
		}
		case ID_STEPDOWN:
		{		
			size_t uIndex = musicPlayer.uIndex + 1;
			if( uIndex >= musicPlayer.vMusics.size( ) )
			{
				uIndex = 0;
			}
			musicPlayer.Close( );
			musicPlayer.Play( false, uIndex );
			UpdatePauseResume( false );
			InvalidateRect( hwnd, nullptr, true );
			break;
		}
		case ID_STEPUP:
		{
			int iIndex = musicPlayer.uIndex - 1;
			if( iIndex < 0 )
			{
				iIndex = musicPlayer.vMusics.size( ) - 1;
			}
			musicPlayer.Close( );
			musicPlayer.Play( false, iIndex );
			UpdatePauseResume( false );
			InvalidateRect( hwnd, nullptr, true );
			break;
		}
		case ID_RESTART:
		{
			musicPlayer.Close( );
			musicPlayer.Play( false, musicPlayer.uIndex );
			UpdatePauseResume( false );
			InvalidateRect( hwnd, nullptr, true );
			break;
		}
		case ID_INFO:
		{
			OnInfo( );
			break;
		}
		case ID_MINBTN:
		{
			ShowWindow( hwnd, SW_HIDE );
			break;
		}
		case ID_EXITBTN:
		{
			CleanUp( );
			break;
		}
	}
}

void CMainWindow::OnSystrayMsg( HWND hwnd, WPARAM wParam, LPARAM lParam )
{
	switch( lParam )
	{
		case WM_LBUTTONUP:
		{
			ShowWindow( hwnd, SW_RESTORE );
			break;
		}
		case WM_RBUTTONDOWN:
		{
			POINT pMouse;
			GetCursorPos( &pMouse );

			const int iTrack = TrackPopupMenu( hPopupMenu, TPM_RETURNCMD | TPM_NONOTIFY, pMouse.x, pMouse.y, 0, hwndMainWindow, nullptr );

			switch( iTrack )
			{
				case SYSTRAY_MENU_RESTORE:
					ShowWindow( hwnd, SW_RESTORE );
					break;
				case SYSTRAY_MENU_MINIMIZE:
					ShowWindow( hwnd, SW_HIDE );
					break;
				case SYSTRAY_MENU_PLAY:
					SendMessage( hwndMainWindow, WM_COMMAND, MAKEWPARAM( ID_PLAY, 0 ), (LPARAM)0 );
					break;
				case SYSTRAY_MENU_PAUSERESUME:
					SendMessage( hwndMainWindow, WM_COMMAND, MAKEWPARAM( ID_PAUSE_RESUME, 0 ), (LPARAM)0 );
					break;
				case SYSTRAY_MENU_STOP:
					SendMessage( hwndMainWindow, WM_COMMAND, MAKEWPARAM( ID_STOP, 0 ), (LPARAM)0 );
					break;
				case SYSTRAY_MENU_NEXT:
					SendMessage( hwndMainWindow, WM_COMMAND, MAKEWPARAM( ID_NEXT, 0 ), (LPARAM)0 );
					break;
				case SYSTRAY_MENU_PREVIOUS:
					SendMessage( hwndMainWindow, WM_COMMAND, MAKEWPARAM( ID_PREVIOUS, 0 ), (LPARAM)0 );
					break;
				case SYSTRAY_MENU_STEPDOWN:
					SendMessage( hwndMainWindow, WM_COMMAND, MAKEWPARAM( ID_STEPDOWN, 0 ), (LPARAM)0 );
					break;
				case SYSTRAY_MENU_STEPUP:
					SendMessage( hwndMainWindow, WM_COMMAND, MAKEWPARAM( ID_STEPUP, 0 ), (LPARAM)0 );
					break;
				case SYSTRAY_MENU_RESTART:
					SendMessage( hwndMainWindow, WM_COMMAND, MAKEWPARAM( ID_RESTART, 0 ), (LPARAM)0 );
					break;
				case SYSTRAY_MENU_ABOUT:
					SendMessage( hwndMainWindow, WM_COMMAND, MAKEWPARAM( ID_INFO, 0 ), (LPARAM)0 );
					break;
				case SYSTRAY_MENU_EXIT:
					CleanUp( );
					break;
			}
			break;
		}
	}
}

void CMainWindow::UpdatePauseResume( bool bExecute )
{
	if( bExecute )
	{
		if( bPaused )
		{
			musicPlayer.Pause( );
			SetWindowText( btnPauseResume, L"Resume" );
		}
		else
		{
			musicPlayer.Resume( );
			SetWindowText( btnPauseResume, L"Pause" );
		}
	}
	else
	{
		bPaused = false;
		SetWindowText( btnPauseResume, L"Pause" );
	}
}

void CMainWindow::OnInfo( )
{
	const wchar_t *szText = L"Music Player ( Win32 )\n"
							L"WinAPI & DirectShow\n\n"
							L"- Directory scan: selected/*.mp3 | selected/subfolders/*.mp3\n"
							L"- Richárd Szernyák.";
	MessageBox( nullptr, szText, L"About", MB_ICONINFORMATION | MB_OK );
}

void CMainWindow::OnDirselect( )
{
	wchar_t tmp[MAX_PATH];
	wcscpy_s( tmp, fileSystem.szPath );
	fileSystem.BrowseDir( );
	if( wcscmp( tmp, fileSystem.szPath ) ) // if path has changed.
	{
		DirScan( );
		InvalidateRect( hwndMainWindow, nullptr, true );
	}
}

void CMainWindow::DirScan( )
{
	SendMessage( listMusics, LB_RESETCONTENT, 0, 0 );

	if( !fileSystem.DirectoryScan( musicPlayer.vMusics ) )
	{
		FATALERRORMSG( "if( !fileSystem.DirectoryScan( musicPlayer.vMusics ) )" );
	}

	for( size_t i = 0; i < musicPlayer.vMusics.size( ); i++ )
	{
		SendMessage( listMusics, LB_ADDSTRING, 0,
			(LPARAM)( std::to_wstring( i ) + L") " + musicPlayer.vMusics[i].strName ).c_str( ) );
	}
}

void CMainWindow::CleanUp( )
{
	if( musicPlayer.pCurMusic != nullptr )
	{
	    musicPlayer.Close( );
	}

	for( BYTE i = ID_HK_NEXT; i <= ID_HK_STEPDOWN; i++ )
	{
		UnregisterHotKey( hwndMainWindow, i );
	}

	Shell_NotifyIcon( NIM_DELETE, &niData );
	CoUninitialize( );
	
	CloseHandle( hMutex );
	DestroyWindow( hwndMainWindow );
}

void CMainWindow::CreateButton( HWND &hwndCtrl, const wchar_t *szName, int x, int y, int w, int h, HWND hwnd, HMENU ID, HINSTANCE hInstance )
{
	hwndCtrl = CreateWindow( L"BUTTON", szName, WS_CHILD | WS_VISIBLE | BS_PUSHBUTTON, x, y, w, h, hwnd,
							 ID, hInstance, nullptr );
	HWNDCHECK( hwndCtrl );
}

void CMainWindow::CreateBitmapButton( HWND &hwndCtrl, int x, int y, int w, int h, HWND hwnd, HMENU ID, HINSTANCE hInstance, int iResourceID )
{
	hwndCtrl = CreateWindow( L"BUTTON", L"", WS_CHILD | WS_VISIBLE | BS_BITMAP, x, y, w, h, hwnd,
							 ID, hInstance, nullptr );
	HWNDCHECK( hwndCtrl );
	const HBITMAP hBitmap = (HBITMAP)LoadImage( hInstance, MAKEINTRESOURCE( iResourceID ), IMAGE_BITMAP, 0, 0, NULL );
	SendMessage( hwndCtrl, BM_SETIMAGE, (WPARAM)IMAGE_BITMAP, (LPARAM)hBitmap );
}

void CMainWindow::CreateCheckbox( HWND &hwndCtrl, const wchar_t *szName, int x, int y, int w, int h, HWND hwnd, HMENU ID, HINSTANCE hInstance )
{
	hwndCtrl = CreateWindow( L"BUTTON", szName, WS_CHILD | WS_VISIBLE | BS_AUTOCHECKBOX, x, y, w, h, hwnd,
							 ID, hInstance, nullptr );
	HWNDCHECK( hwndCtrl );
}

void CMainWindow::CreateStaticCtrl( const wchar_t *szName, int x, int y, int w, int h, HWND hwnd, HMENU ID, HINSTANCE hInstance )
{
	const HWND hwndCtrl = CreateWindow( L"STATIC", szName, WS_CHILD | WS_VISIBLE, x, y, w, h, hwnd,
										ID, hInstance, nullptr );
	HWNDCHECK( hwndCtrl );
}

void CMainWindow::CreateHotkey( HWND &hwndCtrl, int x, int y, int w, int h, HWND hwnd, HMENU ID, HINSTANCE hInstance )
{
	hwndCtrl = CreateWindow( HOTKEY_CLASS, L"", WS_CHILD | WS_VISIBLE,
							 x, y, w, h, hwnd, ID, hInstance, nullptr );
	HWNDCHECK( hwndCtrl );
}