#pragma once

#include <random>
#include <dshow.h>


struct SMusic
{
	std::wstring strName;
	wchar_t szPath[MAX_PATH];

	SMusic( const std::wstring &strName, const wchar_t *szPath );
	bool operator<( const SMusic &m ) const;

private:
	std::wstring strNameLower;
};

class CMusicPlayer
{
public:
	CMusicPlayer( );
	CMusicPlayer( const CMusicPlayer& ) = delete;
	CMusicPlayer& operator =( const CMusicPlayer& ) = delete;


	void Play( const bool bRnd, const int iForceIndex = -1 );
	void Pause( );
	void Resume( );
	void Close( );
	int Length( ) const;
	int Pos( ) const;
	void Seek( int iSeekTo );

	SMusic *pCurMusic;
	size_t uIndex;
	int iPrevious;
	std::random_device randomDevice;
	std::mt19937 mersenneTwister;

	std::vector<SMusic> vMusics;

	HWND hwndMain;
private:
	IGraphBuilder *pGraph;
	IMediaControl *pControl;
	IMediaSeeking *pSeek;
	int iLength;
};
