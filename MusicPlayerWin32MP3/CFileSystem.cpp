#include <Windows.h>
#include "CMusicPlayer.h"
#include "CFileSystem.h"
#include <boost/property_tree/ini_parser.hpp>
#include <dirent.h>
#include <ShlObj_core.h>
#include <algorithm>
#include <fstream>

using namespace boost::property_tree;

constexpr const char *szMP3ConfigFile = "mp3p_config.ini";


CFileSystem::CFileSystem( )
{
	vINIEntries.push_back( SINIEntry( L"MP3Config." ) );
	vINIEntries[INISEC_MP3CONFIG].vINIValues.push_back( L"path" );
	vINIEntries[INISEC_MP3CONFIG].vINIValues.push_back( L"random" );
	vINIEntries[INISEC_MP3CONFIG].vINIValues.push_back( L"auto" );
	vINIEntries[INISEC_MP3CONFIG].vINIValues.push_back( L"enablehotkeys" );

	vINIEntries.push_back( SINIEntry( L"Hotkeys." ) );
	vINIEntries[INISEC_HOTKEYS].vINIValues.push_back( L"next" );
	vINIEntries[INISEC_HOTKEYS].vINIValues.push_back( L"prev" );
	vINIEntries[INISEC_HOTKEYS].vINIValues.push_back( L"pauseresume" );
	vINIEntries[INISEC_HOTKEYS].vINIValues.push_back( L"stop" );
	vINIEntries[INISEC_HOTKEYS].vINIValues.push_back( L"stepup" );
	vINIEntries[INISEC_HOTKEYS].vINIValues.push_back( L"stepdown" );
}

void CFileSystem::BrowseDir( ) 
{
	BROWSEINFO bi = { 0 };
	bi.lpszTitle = L"Browse for .mp3 file folder.";
	LPITEMIDLIST iidl = SHBrowseForFolder( &bi );
	if( iidl != nullptr )
	{
        if( !SHGetPathFromIDList( iidl, szPath ) )
        {
            MessageBox( nullptr, L"Couldn't get path from id list.", L"Error", MB_OK );
        }
        CoTaskMemFree( iidl );
	}
}

bool CFileSystem::LoadConfig( wptree &pt )
{
	try
	{
		ini_parser::read_ini( szMP3ConfigFile, pt );
		const SINIEntry &entry = vINIEntries[INISEC_MP3CONFIG];
		wcscpy_s( szPath, pt.get<std::wstring>( entry.strHeader + entry.vINIValues[0] ).c_str( ) );
	}
	catch( ini_parser_error& )
	{
		MessageBox( nullptr, L"Could not load config! (missing .ini)", L"Error", MB_OK );
		return false;
	}
	return true;
}

bool CFileSystem::SaveConfig( const std::vector<bool> &vConfigBools, const std::vector<BYTE> &vHotkeys )
{
	const int ret = MessageBoxA( nullptr, "Save config?", "Save", MB_YESNO | MB_ICONQUESTION );
	if( ret == IDYES )
	{
		wptree pt;
		
		SINIEntry &mp3entry = vINIEntries[INISEC_MP3CONFIG];
		std::wstring strHeader = mp3entry.strHeader;
		pt.put( strHeader + mp3entry.vINIValues[0], szPath );
		for( size_t i = 0; i < vConfigBools.size( ); i++ )
		{
			pt.put( strHeader + mp3entry.vINIValues[i + 1], vConfigBools[i] );
		}

		SINIEntry &hkentry = vINIEntries[INISEC_HOTKEYS];
		strHeader = hkentry.strHeader;
		for( size_t i = 0; i < vHotkeys.size( ); i++ )
		{
			pt.put( strHeader + hkentry.vINIValues[i], vHotkeys[i] );
		}

		ini_parser::write_ini( szMP3ConfigFile, pt );
	}
	return true;
}

std::wstring CFileSystem::GetExtension( const std::wstring &str )
{
	for( int i = str.length( ) - 1; i >= 0; i-- )
	{
		if( str[i] == L'.' )
		{
			return str.substr( i );
		}
	}
	return L"unk";
}

bool CFileSystem::DirectoryScan( std::vector<SMusic> &vMusics )
{
	vMusics.clear( );

	if( !RGetFiles( szPath, vMusics ) )
		return false;

	// Sort SMusic vector alphabetically.
	sort( vMusics.begin( ), vMusics.end( ) );

	return true;
}

bool CFileSystem::RGetFiles( const std::wstring &strPath, std::vector<SMusic> &vMusics )
{
	WDIR *pDir = nullptr;
	wdirent *pEnt = nullptr;

	if( ( pDir = wopendir( strPath.c_str( ) ) ) != nullptr )
	{
		while( ( pEnt = wreaddir( pDir ) ) != nullptr )
		{
			if( !wcscmp( pEnt->d_name, L"." ) || !wcscmp( pEnt->d_name, L".." ) )
				continue;

			if( pEnt->d_type == DT_DIR )
			{
				WDIR *pSubdir = nullptr;
				wdirent *pSubdirEnt = nullptr;

				const std::wstring subdirpath = strPath + std::wstring( L"\\" ) + pEnt->d_name;

				if( !RGetFiles( subdirpath, vMusics ) )
					return false;

				wclosedir( pSubdir );
			}
			else
			{
				std::wstring ext = GetExtension( pEnt->d_name );
				if( ext != L".mp3" )
					continue;

				const std::wstring strFullPath = strPath + std::wstring( L"\\" ) + pEnt->d_name;
				vMusics.push_back( SMusic( pEnt->d_name, strFullPath.c_str( ) ) );
			}
		}
	}
	else
	{
		return false;
	}

	wclosedir( pDir );
	return true;
}