#include "CMusicPlayer.h"
#include <WinUser.h>
#include <algorithm>
#include "Util.h"

constexpr long long llMediaTime = 10000000;


SMusic::SMusic( const std::wstring &strName, const wchar_t *szPath ) : strName( strName )
{
	wcscpy_s( this->szPath, szPath );
	strNameLower = strName;
	std::transform( strNameLower.begin( ), strNameLower.end( ), strNameLower.begin( ), towlower );
}

bool SMusic::operator<( const SMusic &m ) const
{
    return strNameLower.compare( m.strNameLower ) < 0;
}


CMusicPlayer::CMusicPlayer( ) :
	pCurMusic( nullptr ), uIndex( 0 ), iPrevious( -1 ),
	mersenneTwister( std::mt19937( randomDevice( ) ) ),
	pGraph( nullptr ), pControl( nullptr ), pSeek( nullptr ), iLength( 0 )
{

}

void CMusicPlayer::Play( const bool bRnd, const int iForceIndex )
{
	if( pCurMusic != nullptr )
	{
		return;
	}
	if( vMusics.empty( ) )
	{
		MessageBox( nullptr, L"Empty music list!", L"Error", MB_OK | MB_ICONEXCLAMATION );
		return;
	}

	iPrevious = uIndex;

	SMusic *pMusic = nullptr;
	if( iForceIndex != -1 )
	{
		uIndex = iForceIndex;
		pMusic = &vMusics[uIndex];
	}
	else if( bRnd )
	{
		std::uniform_int_distribution<int> distribution( 0, vMusics.size( ) - 1 );
		uIndex = distribution( mersenneTwister );
		pMusic = &vMusics[uIndex];
	}
	else
	{
		if( uIndex < vMusics.size( ) )
		{
			pMusic = &vMusics[uIndex];
			uIndex++;
		}
		else
		{
			uIndex = 0;
			pMusic = &vMusics[uIndex];
		}
	}

	HRESULT hr;
	hr = CoCreateInstance( CLSID_FilterGraph, nullptr, CLSCTX_INPROC_SERVER, IID_IGraphBuilder, (void**)&pGraph );
	if( FAILED( hr ) )
	{
		MessageBox( nullptr, L"Failed creating pGraph", L"Error", MB_OK | MB_ICONEXCLAMATION );
		return;
	}

	pGraph->QueryInterface( IID_IMediaControl, (void**)&pControl );
	pGraph->QueryInterface( IID_IMediaSeeking, (void**)&pSeek );

	hr = pGraph->RenderFile( pMusic->szPath, nullptr );
	if( FAILED( hr ) )
	{
		wchar_t szError[128];
		StringCbPrintf( szError, sizeof( szError ), L"Failed pGraph->RenderFile( )\n%s", pMusic->strName.c_str( ) );
		MessageBox( nullptr, szError, L"Error", MB_OK | MB_ICONEXCLAMATION );
		return;
	}

	long long llDuration;
	pSeek->SetTimeFormat( &TIME_FORMAT_MEDIA_TIME );
	pSeek->GetDuration( &llDuration );
	iLength = (int)( llDuration / llMediaTime );

	pControl->Run( );

	SetTimer( hwndMain, ID_TIMER, 1000, nullptr );
	pCurMusic = pMusic;
}

void CMusicPlayer::Pause( )
{
	if( pCurMusic == nullptr )
		return;

	pControl->Pause( );
}

void CMusicPlayer::Resume( )
{
	if( pCurMusic == nullptr )
		return;

	pControl->Run( );
}

void CMusicPlayer::Close( )
{
	if( pCurMusic == nullptr )
		return;

	pSeek->Release( );
	pControl->Release( );
	pGraph->Release( );

	KillTimer( hwndMain, ID_TIMER );
	pCurMusic = nullptr;
}

int CMusicPlayer::Length( ) const
{
	if( pCurMusic == nullptr )
		return 0;

	return iLength;
}

int CMusicPlayer::Pos( ) const
{
	if( pCurMusic == nullptr )
		return 0;

	long long llPos;
	pSeek->GetCurrentPosition( &llPos );
	return (int)( llPos / llMediaTime );
}

void CMusicPlayer::Seek( int iSeekTo )
{
	if( pCurMusic == nullptr )
		return;

	long long llVar = (long long)iSeekTo * llMediaTime;
	pSeek->SetPositions( &llVar, AM_SEEKING_AbsolutePositioning, nullptr, AM_SEEKING_NoPositioning );
}