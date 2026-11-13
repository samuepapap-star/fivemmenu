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
#include <Security/AcBypass.hpp>

using namespace Core;

int APIENTRY WinMain( HINSTANCE hInst, HINSTANCE hInstPrev, PSTR cmdline, int cmdshow )
{
	MessageBoxA( nullptr, "PASSO 1: WinMain iniciado", "DIAG", MB_OK );

	if ( !Mem.GetMaxPrivileges( GetCurrentProcess( ) ) )
	{
		MessageBoxA( nullptr, "FALHOU: GetMaxPrivileges retornou false\nExecute como Administrador", "DIAG - ERRO", MB_OK );
		return 0;
	}

	MessageBoxA( nullptr, "PASSO 2: Privilégios OK\nAguardando FiveM abrir...", "DIAG", MB_OK );


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

	char diagMsg[512];
	sprintf_s( diagMsg, "PASSO 3: grcWindow encontrado!\nPID do FiveM: %lu", ProcIdFiveM );
	MessageBoxA( nullptr, diagMsg, "DIAG", MB_OK );

	if ( ProcIdFiveM != 0 )
	{
		std::string ProcessName = Mem.GetNameByPid( ProcIdFiveM );
		if ( ProcessName.empty( ) ) {
			MessageBoxA( nullptr, "FALHOU: GetNameByPid retornou vazio", "DIAG - ERRO", MB_OK );
			return 0;
		};

		const std::string arguments=cmdline?cmdline:"";
        if (arguments.find("--diagnostico-recursos") != std::string::npos ||
            (ClientBuild::Detect(ProcessName)==3889 &&
             arguments.find("--diagnostico-3889") != std::string::npos)) {
			Build3889Probe::Run(ProcIdFiveM,ClientBuild::Detect(ProcessName));
			return 0;
		}

		char diagMsg2[512];
		sprintf_s( diagMsg2, "PASSO 4: Processo = %s", ProcessName.c_str() );
		MessageBoxA( nullptr, diagMsg2, "DIAG", MB_OK );

		if ( !Mem.OpenProcByPid(ProcIdFiveM) ) {
			MessageBoxA( nullptr, "FALHOU: OpenProcByPid retornou false\nTente executar como Administrador", "DIAG - ERRO", MB_OK );
			return 0;
		}

		char diagMsg3[512];
		sprintf_s( diagMsg3, "PASSO 5: Attach OK!\nModBase: 0x%llX\nBuild: %lu\nJanela FiveM: %dx%d em (%d,%d)",
			Mem.ModBase, (DWORD)ClientBuild::Detect(ProcessName),
			(int)g_Variables.g_vGameWindowSize.x, (int)g_Variables.g_vGameWindowSize.y,
			(int)g_Variables.g_vGameWindowPos.x, (int)g_Variables.g_vGameWindowPos.y );
		MessageBoxA( nullptr, diagMsg3, "DIAG", MB_OK );

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

		AcBypass::Init( GetModuleHandle( nullptr ) );
		std::thread( &AntiCrack::DoProtect ).detach( );
        if (g_Offsets.CurrentBuild == 3889) {
            const std::string error = Build3889Compatibility::Validate(Mem.ProcHandle, Mem.ModBase);
            if (!error.empty()) {
                AltosDiagnostics::Log("build3889_initialization_failed " + error);
                MessageBoxA(nullptr, error.c_str(), "Compatibilidade 3889", MB_ICONERROR);
                AcBypass::Shutdown( );
                return 0;
            }
            AltosDiagnostics::Log("build3889_initialized compatibility_report_20261106 waypoint_unavailable");
        }
		GetOffsets();
		Gui::cOverlay.Render( );
		AcBypass::Shutdown( );
		return 0;
	}

	return 0;
}


