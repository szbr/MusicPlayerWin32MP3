#pragma once

#include <vector>
#include <boost/property_tree/ptree.hpp>


class CFileSystem
{
public:
	CFileSystem( );
	CFileSystem( const CFileSystem& ) = delete;
	CFileSystem& operator=( const CFileSystem& ) = delete;

	enum EINISection
	{
		INISEC_MP3CONFIG,
		INISEC_HOTKEYS
	};

	void BrowseDir( );
	bool LoadConfig( boost::property_tree::wptree &pt );
	bool SaveConfig( const std::vector<bool> &vConfigBools, const std::vector<BYTE> &vHotkeys );
	std::wstring GetExtension( const std::wstring &str );
	bool DirectoryScan( std::vector<SMusic> &vMusics );

	inline const std::wstring& GetINISection( EINISection section ) const { return vINIEntries[section].strHeader; }
	inline const std::wstring& GetININame( EINISection section, size_t uIdx ) const { return vINIEntries[section].vINIValues[uIdx]; }

	wchar_t szPath[MAX_PATH];

private:
	struct SINIEntry
	{
		SINIEntry( const std::wstring &strHeader ) : strHeader( strHeader ) { }

		std::wstring strHeader;
		std::vector<std::wstring> vINIValues;
	};
	std::vector<SINIEntry> vINIEntries;

	bool RGetFiles( const std::wstring &strPath, std::vector<SMusic> &vMusics );
};
