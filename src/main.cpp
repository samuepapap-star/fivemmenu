#pragma once
#include <Gui/Overlay/Overlay.hpp>
#include <Includes/Includes.hpp>
#include <Core/SDK/Memory.hpp>
#include <Includes/Utils.hpp>
#include <Core/SDK/SDK.hpp>
#include <Core/Core.hpp>
#include <Gui/gui.hpp>

#include <Security/Api/api.hpp>

#include <winternl.h>
#include <windows.h>
#include <dwmapi.h>
#include <tchar.h>
#include <vector>
#include <regex>

#include <Security/AntiCrack.hpp>

using namespace Core;

int APIENTRY WinMain( HINSTANCE hInst, HINSTANCE hInstPrev, PSTR cmdline, int cmdshow )
{
	if ( !Mem.GetMaxPrivileges( GetCurrentProcess( ) ) )
	{
		//std::cout << xorstr( "[Error]: Failed GetMaxPrivileges\n" );
		//std::cin.get( );
		return 0;
	}


	//Utils::OpenConsole( );



	while ( !g_Variables.g_hGameWindow )
	{

		g_Variables.g_hGameWindow = FindWindowA( xorstr( "grcWindow" ), nullptr );
		if ( g_Variables.g_hGameWindow )
		{
			auto WindowInfo = Utils::GetWindowPosAndSize( g_Variables.g_hGameWindow );
			g_Variables.g_vGameWindowSize = WindowInfo.second;
			g_Variables.g_vGameWindowPos = WindowInfo.first;
			g_Variables.g_vGameWindowCenter = { g_Variables.g_vGameWindowSize.x / 2, g_Variables.g_vGameWindowSize.y / 2 };
			break;
		}

		//std::this_thread::sleep_for( std::chrono::seconds( 3 ) );
	}
	//
 //	char cDcId[ 255 ];
 //	DWORD strlen = 8192;
	//
 //	if ( RegGetValueA( HKEY_CURRENT_USER, NULL, xorstr( "dc_id" ), RRF_RT_REG_SZ, NULL, ( PVOID ) &cDcId, &strlen ) != ERROR_SUCCESS ) {
 //		return 0;
 //	}
	//
	//
	//auto Setup = g_Api.setup( );
	//if ( Setup != xorstr( "success" ) )
	//{
	//	Utils::OpenConsole( );
		
	//	std::cout << xorstr( "Outdated Version." ) << std::endl;
	//	SetConsoleTitle( xorstr( "Wxz" ) );
	//	FreeConsole( );
	//	return 0;
	//}

	 //std::cout << "Setup: " << Setup << "\n";
	 //std::cout << "Login: " << Login << "\n";

	DWORD ProcIdFiveM = 0;
	GetWindowThreadProcessId( g_Variables.g_hGameWindow, &ProcIdFiveM );
	if ( ProcIdFiveM != 0 )
	{
		std::string ProcessName = Mem.GetNameByPid( ProcIdFiveM );
		if ( ProcessName.empty( ) ) { return 0; };

		if ( !Mem.OpenProc( ProcessName.c_str( ) ) ) {
			//std::cout << xorstr( "[Error]: Failed to OpenProc\n" );
			//std::cin.get( );
			return 0;
		}

		//std::cout << xorstr( "[ProcessName]: " ) << ProcessName << std::endl;
		//std::cout << xorstr( "[PID]: " ) << ProcIdFiveM << std::endl;
		//std::cout << xorstr( "[ModuleBase]: " ) << std::hex << std::uppercase << Mem.ModBase << std::endl;

		g_Offsets.CurrentBuild = 0;

		if ( ProcessName.find( xorstr( "FiveM_b2060" ) ) != std::string::npos )
		{
			g_Offsets.CurrentBuild = 2060;
		}
		else if ( ProcessName.find( xorstr( "FiveM_b2189" ) ) != std::string::npos )
		{
			g_Offsets.CurrentBuild = 2189;
		}
		else if ( ProcessName.find( xorstr( "FiveM_b2372" ) ) != std::string::npos )
		{
			g_Offsets.CurrentBuild = 2372;
		}
		else if ( ProcessName.find( xorstr( "FiveM_b2545" ) ) != std::string::npos )
		{

			g_Offsets.CurrentBuild = 2545;
		}
		else if ( ProcessName.find( xorstr( "FiveM_b2612" ) ) != std::string::npos )
		{

			g_Offsets.CurrentBuild = 2612;
		}
		else if ( ProcessName.find( xorstr( "FiveM_b2699" ) ) != std::string::npos )
		{

			g_Offsets.CurrentBuild = 2699;
		}
		else if ( ProcessName.find( xorstr( "FiveM_b2802" ) ) != std::string::npos )
		{

			g_Offsets.CurrentBuild = 2802;
		}
		else if ( ProcessName.find( xorstr( "FiveM_b2944" ) ) != std::string::npos )
		{

			g_Offsets.CurrentBuild = 2944;

		}
		else if ( ProcessName.find( xorstr( "FiveM_b3095" ) ) != std::string::npos )
		{

			g_Offsets.CurrentBuild = 3095;
		}
		else if ( ProcessName.find( xorstr( "FiveM_b3258" ) ) != std::string::npos )
		{
			g_Offsets.CurrentBuild = 3258;
		}
		else {
			g_Offsets.CurrentBuild = 1604;
		}
		
		std::thread( &AntiCrack::DoProtect ).detach( );
		GetOffsets();
		Gui::cOverlay.Render( );
		return 0;
	}

	return 0;
}
