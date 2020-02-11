#pragma once

#include <Windows.h>
#include <iostream>
#include "CMusicPlayer.h"
#include "CFileSystem.h"


// Singleton.
class CMainWindow
{
	friend class CFileSystem;

public:
	CMainWindow( const CMainWindow& ) = delete;
	CMainWindow& operator=( const CMainWindow& ) = delete;

	static CMainWindow& Instance( )
	{
		static CMainWindow w;
		return w;
	}

	void Start( HINSTANCE hInstance );

	inline HWND MainHWND( ) { return hwndMainWindow; }

private:
	CMainWindow( );

	void OnCreate( HWND hwnd );
	void OnPaint( );
	void OnCommand( HWND hwnd, WPARAM wParam, LPARAM lParam );
	void OnSystrayMsg( HWND hwnd, WPARAM wParam, LPARAM lParam );

	void UpdatePauseResume( bool bExecute );
	void OnInfo( );
	void OnDirselect( );
	void DirScan( );
	void CleanUp( );

	void CreateButton( HWND &hwndCtrl, const wchar_t *szName, int x, int y, int w, int h, HWND hwnd, HMENU ID, HINSTANCE hInstance );
	void CreateBitmapButton( HWND &hwndCtrl, int x, int y, int w, int h, HWND hwnd, HMENU ID, HINSTANCE hInstance, int iResourceID );
	void CreateCheckbox( HWND &hwndCtrl, const wchar_t *szName, int x, int y, int w, int h, HWND hwnd, HMENU ID, HINSTANCE hInstance );
	void CreateStaticCtrl( const wchar_t *szName, int x, int y, int w, int h, HWND hwnd, HMENU ID, HINSTANCE hInstance );
	void CreateHotkey( HWND &hwndCtrl, int x, int y, int w, int h, HWND hwnd, HMENU ID, HINSTANCE hInstance );

	inline bool IsInRect( int x, int y, const RECT &rect )
	{
		if( x > rect.left && x < rect.right )
			if( y > rect.top && y < rect.bottom )
				return true;
		return false;
	}

	HWND hwndMainWindow; // Main window handle.

	static LRESULT CALLBACK WndProc( HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam );
	static CMainWindow *pInstance;

	LRESULT CALLBACK ThisWndProc( HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam );


	CMusicPlayer musicPlayer;
	CFileSystem fileSystem;

	HWND btnDirSelect, btnSave, btnPlay, btnPauseResume, btnStop, btnNext, btnPrev, btnStepUp, btnStepDown, btnRestart, btnInfo, btnMin, btnExit,
		 cboxRandom, cboxAutoPlay, cboxExitOnFinish, cboxHotkeys,
		 hkNext, hkPrev, hkStepUp, hkStepDown, hkPauseResume, hkStop,
		 editSearch,
		 listMusics;

	HBRUSH hbBackground;
	HPEN hpBorder;
	HPEN hpSlider;

	RECT rectStatus, rectSlider, rectSliderDrawArea, rectSearch;
	int iSliderLength;

	COLORREF dwBackgroundColor, dwBorderColor, dwSliderColor, dwTextColor;

	int iSPos; // Music caret pos
	wchar_t szSearchText[32]; // Typed in search string
	bool bPaused;
	NOTIFYICONDATA niData;
	HMENU hPopupMenu;
	HANDLE hMutex;
};
