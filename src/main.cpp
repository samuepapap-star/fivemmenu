#pragma once
#include <Bridge/Client.hpp>
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
	if ( !Mem.GetMaxPrivileges( GetCurrentProcess( ) ) )
	{
		MessageBoxA(nullptr,
			"Falha ao obter privilegios maximos.\nExecute o .exe como Administrador.",
			"[DIAG] Privilegios", MB_OK | MB_ICONERROR);
		return 0;
	}

	//Utils::OpenConsole( );

	// Aguarda janela do FiveM (grcWindow) com timeout de 30s
	{
		int attempts = 0;
		while ( !g_Variables.g_hGameWindow && attempts < 60 )
		{
			g_Variables.g_hGameWindow = FindWindowA( "grcWindow", nullptr );
			if ( g_Variables.g_hGameWindow )
			{
				auto WindowInfo = Utils::GetWindowPosAndSize( g_Variables.g_hGameWindow );
				g_Variables.g_vGameWindowSize = WindowInfo.second;
				g_Variables.g_vGameWindowPos = WindowInfo.first;
				g_Variables.g_vGameWindowCenter = { g_Variables.g_vGameWindowSize.x / 2, g_Variables.g_vGameWindowSize.y / 2 };
				break;
			}
			std::this_thread::sleep_for( std::chrono::milliseconds(500) );
			attempts++;
		}

		if ( !g_Variables.g_hGameWindow )
		{
			MessageBoxA(nullptr,
				"Janela do FiveM (grcWindow) nao encontrada apos 30 segundos.\n\n"
				"Certifique-se de:\n"
				"1. FiveM esta aberto e carregado no servidor\n"
				"2. O personagem ja apareceu na tela\n"
				"3. O jogo nao esta minimizado",
				"[DIAG] FiveM nao encontrado", MB_OK | MB_ICONWARNING);
			return 0;
		}
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
	if ( ProcIdFiveM == 0 )
	{
		MessageBoxA(nullptr,
			"Encontrou a janela do FiveM mas nao conseguiu obter o PID do processo.\nTente executar como Administrador.",
			"[DIAG] PID invalido", MB_OK | MB_ICONERROR);
		return 0;
	}

	if ( ProcIdFiveM != 0 )
	{
		std::string ProcessName = Mem.GetNameByPid( ProcIdFiveM );
		if ( ProcessName.empty( ) )
		{
			std::string msg = "Nao foi possivel obter o nome do processo. PID: " + std::to_string(ProcIdFiveM);
			MessageBoxA(nullptr, msg.c_str(), "[DIAG] Nome do processo", MB_OK | MB_ICONERROR);
			return 0;
		}

		const std::string arguments=cmdline?cmdline:"";
        if (arguments.find("--diagnostico-recursos") != std::string::npos ||
            (ClientBuild::Detect(ProcessName)==3889 &&
             arguments.find("--diagnostico-3889") != std::string::npos)) {
			Build3889Probe::Run(ProcIdFiveM,ClientBuild::Detect(ProcessName));
			return 0;
		}

		if ( !Mem.OpenProcByPid(ProcIdFiveM) ) {
			std::string msg = "OpenProcess falhou para o processo: " + ProcessName +
				"\nPID: " + std::to_string(ProcIdFiveM) +
				"\nErro Windows: " + std::to_string(GetLastError()) +
				"\n\nCertifique-se de executar o .exe como Administrador.";
			MessageBoxA(nullptr, msg.c_str(), "[DIAG] OpenProcess falhou", MB_OK | MB_ICONERROR);
			return 0;
		}

		//std::cout << xorstr( "[ProcessName]: " ) << ProcessName << std::endl;
		//std::cout << xorstr( "[PID]: " ) << ProcIdFiveM << std::endl;
		//std::cout << xorstr( "[ModuleBase]: " ) << std::hex << std::uppercase << Mem.ModBase << std::endl;

        g_Offsets.CurrentBuild = ClientBuild::Detect(ProcessName);
        AltosDiagnostics::Log("startup process="+ProcessName+" window_pid="+std::to_string(ProcIdFiveM)+
            " attached_pid="+std::to_string(Mem.ProcId)+" build="+std::to_string(g_Offsets.CurrentBuild));

		// Diagnostico: mostra info de attach bem-sucedido
		{
			char diagBuf[512];
			sprintf_s(diagBuf, sizeof(diagBuf),
				"[DIAG] Attach bem-sucedido!\n\n"
				"Processo: %s\n"
				"PID: %lu\n"
				"Build detectada: %u\n"
				"ModBase: 0x%llX\n"
				"ModBaseSize: 0x%llX",
				ProcessName.c_str(), ProcIdFiveM,
				g_Offsets.CurrentBuild,
				(unsigned long long)Mem.ModBase,
				(unsigned long long)Mem.ModBaseSize);
			MessageBoxA(nullptr, diagBuf, "[DIAG] Status do Attach", MB_OK | MB_ICONINFORMATION);
		}

        if(!g_Offsets.CurrentBuild) {
            const std::string error="Esta build do FiveM nao tem enderecos compativeis. Processo: "+ProcessName+
                ". Envie o nome deste processo no suporte.";
            MessageBoxA(nullptr,error.c_str(),"Build do FiveM nao suportada",MB_ICONINFORMATION);
            return 0;
        }
        if(!Mem.ModBase || !Mem.ModBaseSize) {
            AltosDiagnostics::Log("startup_failed module_unavailable");
            MessageBoxA(nullptr,"Nao foi possivel localizar o modulo do processo do FiveM.","Inicializacao",MB_ICONERROR);
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
#ifdef DEBUG_BRIDGE_STATUS
		{
			std::this_thread::sleep_for(std::chrono::milliseconds(500));
			const std::string bridgeStatus = MenuBridge::Connection().Status();
			MessageBoxA(nullptr, bridgeStatus.c_str(), "Bridge Status (DEBUG)", MB_OK | MB_ICONINFORMATION);
		}
#endif
		Gui::cOverlay.Render( );
		AcBypass::Shutdown( );
		return 0;
	}

	return 0;
}


