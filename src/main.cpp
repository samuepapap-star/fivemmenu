#pragma once
#include <Gui/Overlay/Overlay.hpp>
#include <Includes/Includes.hpp>
#include <Core/SDK/Memory.hpp>
#include <Includes/Utils.hpp>
#include <Core/SDK/SDK.hpp>
#include <Core/SDK/SupportedBuild.hpp>
#include <Core/SDK/Build3889Probe.hpp>
#include <Core/SDK/Build3889Compatibility.hpp>
#include <Core/SDK/TeleportTrace.hpp>
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

		if (ClientBuild::Detect(ProcessName) == 3889 &&
            std::string(cmdline ? cmdline : "").find("--diagnostico-3889") != std::string::npos) {
			Build3889Probe::Run(ProcIdFiveM);
			return 0;
		}

		if ( !Mem.OpenProcByPid(ProcIdFiveM) ) {
			//std::cout << xorstr( "[Error]: Failed to OpenProc\n" );
			//std::cin.get( );
			return 0;
		}

		//std::cout << xorstr( "[ProcessName]: " ) << ProcessName << std::endl;
		//std::cout << xorstr( "[PID]: " ) << ProcIdFiveM << std::endl;
		//std::cout << xorstr( "[ModuleBase]: " ) << std::hex << std::uppercase << Mem.ModBase << std::endl;

        g_Offsets.CurrentBuild = ClientBuild::Detect(ProcessName);
        AltosDiagnostics::Log("startup process="+ProcessName+" window_pid="+std::to_string(ProcIdFiveM)+
            " attached_pid="+std::to_string(Mem.ProcId)+" build="+std::to_string(g_Offsets.CurrentBuild));
        if(!g_Offsets.CurrentBuild) {
            const std::string error="Esta build do FiveM não tem endereços compatíveis neste projeto. Processo: "+ProcessName+
                ". Envie o nome deste processo e o registro ALTOS_PODERES_teleporte.log.";
            MessageBoxA(nullptr,error.c_str(),"Build do FiveM não suportada",MB_ICONINFORMATION);
            return 0;
        }
        if(!Mem.ModBase || !Mem.ModBaseSize) {
            AltosDiagnostics::Log("startup_failed module_unavailable");
            MessageBoxA(nullptr,"Não foi possível localizar o módulo do processo do FiveM.","Inicialização",MB_ICONERROR);
            return 0;
        }

		std::thread( &AntiCrack::DoProtect ).detach( );
        if (g_Offsets.CurrentBuild == 3889) {
            const std::string error = Build3889Compatibility::Validate(Mem.ProcHandle, Mem.ModBase);
            if (!error.empty()) {
                AltosDiagnostics::Log("build3889_initialization_failed " + error);
                MessageBoxA(nullptr, error.c_str(), "Compatibilidade 3889", MB_ICONERROR);
                return 0;
            }
            AltosDiagnostics::Log("build3889_initialized compatibility_report_20261106 waypoint_unavailable");
        }
		GetOffsets();
		Gui::cOverlay.Render( );
		return 0;
	}

	return 0;
}


