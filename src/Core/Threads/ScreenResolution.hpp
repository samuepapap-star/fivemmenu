#pragma once
#include <Core/ExtensionSettings.hpp>
#include <Includes/Includes.hpp>
#include <Includes/Utils.hpp>

#include <iostream>
#include <fstream>
#include <string>
#include <regex>


namespace Core 
{
	namespace Threads 
	{
		class cScreenResolution 
		{
		public:
			void MoveAndSetResolution( )
			{
				g_Variables.g_hGameWindow = FindWindowA( xorstr( "grcWindow" ), nullptr );
				if ( !g_Variables.g_hGameWindow ) { exit( 0 ); }

				//Log in Check
				//if ( g_MenuInfo.IsLogged && !g_Variables.g_bPassedByThisVerify ) { exit( 0 ); }
				//if ( g_MenuInfo.IsLogged && g_Variables.g_VerifyLogin != 348975682703 ) { exit( 0 ); }

				auto WindowInfo = Utils::GetWindowPosAndSize( g_Variables.g_hGameWindow );
				g_Variables.g_vGameWindowSize = WindowInfo.second;
				g_Variables.g_vGameWindowPos = WindowInfo.first;
				g_Variables.g_vGameWindowCenter = { g_Variables.g_vGameWindowSize.x / 2, g_Variables.g_vGameWindowSize.y / 2 };

				int overlayX=static_cast<int>(g_Variables.g_vGameWindowPos.x),overlayY=static_cast<int>(g_Variables.g_vGameWindowPos.y);
                if(ExtraSettings::SecondMonitor){
                    struct Target{HMONITOR game;RECT area;bool found;};
                    Target target{MonitorFromWindow(g_Variables.g_hGameWindow,MONITOR_DEFAULTTONEAREST),{},false};
                    EnumDisplayMonitors(nullptr,nullptr,[](HMONITOR monitor,HDC,LPRECT rect,LPARAM data)->BOOL{
                        auto* t=reinterpret_cast<Target*>(data);if(monitor!=t->game&&!t->found){t->area=*rect;t->found=true;}return TRUE;
                    },reinterpret_cast<LPARAM>(&target));
                    if(target.found){overlayX=target.area.left;overlayY=target.area.top;}
                }
                MoveWindow( g_Variables.g_hCheatWindow, overlayX, overlayY, g_Variables.g_vGameWindowSize.x, g_Variables.g_vGameWindowSize.y, true );
			}

			void Update( )
			{
				while ( true )
				{
					try
					{
						MoveAndSetResolution( );
					}
					catch ( std::exception e )
					{
						MessageBox( NULL, xorstr( "Crash Detected. Code: 1" ), xorstr( "" ), MB_ICONERROR );
						continue;
					}

					std::this_thread::sleep_for( std::chrono::milliseconds( 300 ) );
				}
			}
		};

		inline cScreenResolution g_ScreenResolution;
	}
}
