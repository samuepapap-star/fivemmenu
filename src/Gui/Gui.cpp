#include "Gui.hpp"
#include <Core/Features/ExtraFeatures.hpp>
#include <Core/Features/ExtraCamera.hpp>

#include <Includes/CustomWidgets/Custom.hpp>
#include <Includes/CustomWidgets/WaterMarks.hpp>
#include <Includes/CustomWidgets/Notify.hpp>

#include <Gui/Pages/Combat.hpp>
#include <Gui/Pages/Visuals.hpp>
#include <Gui/Pages/Local.hpp>
#include <Gui/Pages/Exploits.hpp>
#include <Gui/Pages/World.hpp>
#include <Gui/Pages/Settings.hpp>
#include <Gui/Pages/Login.hpp>
#include <Gui/Pages/Categories.hpp>

#include <Core/Features/Exploits/Exploits.hpp>
#include <Includes/CustomWidgets/Notify.hpp>
#include <Core/Features/Esp.hpp>
#include <Core/SDK/ClientState.hpp>
#include <Gui/AdminList.hpp>
#include <Core/Features/NoclipVisibility.hpp>
#include <Core/Features/SelfGodMode.hpp>
#include <Core/Features/VehicleManipulation.hpp>
#include <Core/Features/ExternalCameraOptions.hpp>

#include <Security/Api/api.hpp>
#include <Gui/AltosInterface.hpp>

inline std::mutex DrawMtx;

inline void CheatLogin( )
{
	std::string ClipBoard = ( std::string ) Utils::GetClipboard( );
	auto Login = g_Api.login( ClipBoard, Utils::GetHWID( ) );
	if ( Login == xorstr( "success" ) )
	{
		g_MenuInfo.MenuSize = { 920, 600 };
		g_MenuInfo.IsLogged = true;
		Core::GetOffsets( );
	}
	else {
		NotifyManager::Send(Login, 4000 );
	}

}

void Gui::Rendering( )
{
    static bool diagnosticSession=false;
    if(!diagnosticSession) {
        AltosDiagnostics::Log("session_started version=client_state_diagnostics");
        diagnosticSession=true;
    }
    Core::Features::CarreraAdditions::TickCollision();
    ClientState::Tick();
    Core::Features::Extras::Tick();
    Core::Features::ExtraCamera::Tick();
    Core::Features::Extras::DrawRadar();
    Core::Features::VehicleManipulation::Tick();
    Core::Features::NoclipVisibility::Tick();
    Core::Features::SelfGodMode::Tick();
    AdminList::Draw();
    Core::Features::Extras::DrawWarnings();
    Core::Features::ExternalCameraOptions::Tick();
    g_Col.Base=g_Config.General->MenuAccent;
    Core::Features::Extras::DrawMenuEffects();
    g_Col.ButtonHovered=ImVec4(g_Col.Base.x,g_Col.Base.y,g_Col.Base.z,220.f/255.f);

	if ( !g_MenuInfo.IsLogged )
	{
		g_MenuInfo.MenuSize = { 500, 350 };
	}
	ImGui::SetNextWindowSize( g_MenuInfo.MenuSize );

    const ImVec2 display = ImGui::GetIO().DisplaySize;
    static bool positionInitialized = false;
    if (!positionInitialized && display.x > 0.f && display.y > 0.f) {
        const float x = display.x > g_MenuInfo.MenuSize.x ? (display.x - g_MenuInfo.MenuSize.x) * 0.5f : 0.f;
        const float y = display.y > g_MenuInfo.MenuSize.y ? (display.y - g_MenuInfo.MenuSize.y) * 0.5f : 0.f;
        ImGui::SetNextWindowPos(ImVec2(x, y), ImGuiCond_Always);
        positionInitialized = true;
    }

    ImGui::Begin(" ", nullptr, ImGuiWindowFlags | ImGuiWindowFlags_NoMove |
        (g_MenuInfo.IsOpen ? 0 : ImGuiWindowFlags_NoInputs));
    // Clip the menu and child controls when closed; background overlays still render.
    const bool menuHidden = !g_MenuInfo.IsOpen;
    if (menuHidden)
        ImGui::PushClipRect(ImVec2(0.f, 0.f), ImVec2(0.f, 0.f), true);
    // Keep the header reachable after resolution changes or an off-screen move.
    auto clampPosition = [&](ImVec2 pos) {
        const float maxX = display.x > g_MenuInfo.MenuSize.x ? display.x - g_MenuInfo.MenuSize.x : 0.f;
        const float maxY = display.y > g_MenuInfo.MenuSize.y ? display.y - g_MenuInfo.MenuSize.y : 0.f;
        pos.x = pos.x < 0.f ? 0.f : (pos.x > maxX ? maxX : pos.x);
        pos.y = pos.y < 0.f ? 0.f : (pos.y > maxY ? maxY : pos.y);
        return pos;
    };
    if (display.x > 0.f && display.y > 0.f)
        ImGui::SetWindowPos(clampPosition(ImGui::GetWindowPos()));

	ImGui::PushFont( g_Variables.m_FontNormal );
	{

		AltosInterface::Background();
        // Keep dragging inside the logo/sidebar; combat tabs occupy x >= 176.
        const ImVec2 savedCursor = ImGui::GetCursorPos();
        ImGui::SetCursorPos(ImVec2(285.f, 0.f));
        ImGui::InvisibleButton("##MenuDragHandle", ImVec2(g_MenuInfo.MenuSize.x - 340.f, 60.f));
        if (g_MenuInfo.IsOpen && ImGui::IsItemActive() &&
            ImGui::IsMouseDragging(ImGuiMouseButton_Left) && display.x > 0.f && display.y > 0.f) {
            ImVec2 pos = ImGui::GetWindowPos();
            const ImVec2 delta = ImGui::GetIO().MouseDelta;
            ImGui::SetWindowPos(clampPosition(ImVec2(pos.x + delta.x, pos.y + delta.y)));
        }
        ImGui::SetCursorPos(savedCursor);

        if (g_MenuInfo.IsLogged) {
            ImGui::PushID("SidebarNavigation");
            AltosInterface::Navigation();
            ImGui::PopID();
        }

		g_MenuInfo.TabAlpha = ImClamp( g_MenuInfo.TabAlpha + ( 5.f * ImGui::GetIO( ).DeltaTime * ( g_MenuInfo.iTabCount == g_MenuInfo.iCurrentPage ? 1.f : -1.f ) ), 0.f, 1.f );

		if ( g_MenuInfo.TabAlpha == 0.f )
			g_MenuInfo.iCurrentPage = g_MenuInfo.iTabCount;

		ImGuiStyle * style = &ImGui::GetStyle( );
		ImGui::PushStyleVar( ImGuiStyleVar_Alpha, g_MenuInfo.TabAlpha * style->Alpha );

		if ( g_MenuInfo.IsLogged )
		{


			ImGui::SetCursorPos( ImVec2( 176, 76 ) );
			ImGui::BeginGroup( );



			ImGui::PushID("PageContent");
            ImGui::PushID(g_MenuInfo.iCurrentPage);
            switch ( g_MenuInfo.iCurrentPage )
			{

			case g_MenuInfo.Combat:
				Combat::Render( );
				break;
			case g_MenuInfo.Visuals:
				Visuals::Render( );
				break;
			case g_MenuInfo.Local:
				Categories::Personagem();
				break;
			case g_MenuInfo.World:
				World::Render( );
				break;
			case g_MenuInfo.Exploits:
				Exploits::Render(1);
				break;
            case g_MenuInfo.Cars: Categories::Cars(); break;
            case g_MenuInfo.Players: Categories::Players(); break;
            case g_MenuInfo.Weapons: Categories::Weapons(); break;
            case g_MenuInfo.Teleports: Categories::Teleports(); break;
			case g_MenuInfo.Settings:
				Categories::Settings();
				break;

			}

			ImGui::PopID();
            ImGui::PopID();
            ImGui::EndGroup( );


			HWND ActiveWindow = GetForegroundWindow( );

			{
				std::lock_guard<std::mutex> Lock( DrawMtx );

				NotifyManager::Render( );

				if ( ActiveWindow == g_Variables.g_hGameWindow )
				{
					if ( GetAsyncKeyState( g_Config.Player->GodModeKey ) & 1 )
					{
						g_Config.Player->EnableGodMode = !g_Config.Player->EnableGodMode;

						Core::Features::SelfGodMode::Tick();

						std::thread( [ & ] ( )
							{
								NotifyManager::Send( xorstr( "Invencibilidade " ) + ( std::string ) ( g_Config.Player->EnableGodMode ? xorstr( "ativado!" ) : xorstr( "desativado!" ) ), 2000 );
							}
						).detach( );

					}

					if ( GetAsyncKeyState( g_Config.Player->NoClipKey ) & 1 )
					{
						g_Config.Player->NoClipEnabled = !g_Config.Player->NoClipEnabled;

						Core::SDK::Pointers::pLocalPlayer->FreezePed( g_Config.Player->NoClipEnabled );

						std::thread( [ & ] ( )
							{
								NotifyManager::Send( xorstr( "Voo livre " ) + ( std::string ) ( g_Config.Player->NoClipEnabled ? xorstr( "ativado!" ) : xorstr( "desativado!" ) ), 2000 );
							}
						).detach( );
					}

					if ( g_Config.Player->NoClipEnabled )
						Features::Exploits::NoClip( );

				}

				if ( ActiveWindow == g_Variables.g_hGameWindow || ActiveWindow == g_Variables.g_hCheatWindow )
				{
					struct FovFuncs_t {
						bool * Enabled;
						int * FovSize;
						ImVec4 FovColor;
					};

					std::vector<FovFuncs_t> FovDrawList = {
						FovFuncs_t( &g_Config.Aimbot->ShowFov, &g_Config.Aimbot->FOV, g_Config.Aimbot->FovColor ),
						FovFuncs_t( &g_Config.SilentAim->ShowFov, &g_Config.SilentAim->FOV, g_Config.SilentAim->FovColor ),
						FovFuncs_t( &g_Config.TriggerBot->ShowFov, &g_Config.TriggerBot->FOV, g_Config.TriggerBot->FovColor ),
					};

					static std::vector<float> Alphas( FovDrawList.size( ), 0.0f );
					static std::vector<float> Sizes( FovDrawList.size( ), 0.0f );

					for ( int i = 0; i < FovDrawList.size( ); ++i )
					{
						auto & Fov = FovDrawList[ i ];

						Alphas[ i ] = ImClamp( ImLerp( Alphas[ i ], *Fov.Enabled ? 1.f : 0.f, ImGui::GetIO( ).DeltaTime * 10.f ), 0.f, 1.f );
						Sizes[ i ] = ImLerp( Sizes[ i ], ( float ) *Fov.FovSize, ImGui::GetIO( ).DeltaTime * 12.f );

						ImGui::PushStyleVar( ImGuiStyleVar_Alpha, Alphas[ i ] );

						ImGui::GetBackgroundDrawList( )->AddCircle( ImVec2( g_Variables.g_vGameWindowCenter.x, g_Variables.g_vGameWindowCenter.y ), Sizes[ i ], ImGui::GetColorU32( Fov.FovColor ), 999 );

						ImGui::PopStyleVar( );
					}

					if ( g_Config.General->WaterMark )
						Custom::WaterMark::Render( );


					ImGui::PushFont( g_Variables.m_DrawFont );

					Features::g_Esp.Draw( );
					Features::g_Esp.DrawVehicle( );

					ImGui::PopFont( );
				}
				else {

					if ( ImGui::GetStyle( ).Alpha >= 0.9f )
					{
						g_MenuInfo.IsOpen = false;
						SetWindowLong( g_Variables.g_hCheatWindow, GWL_EXSTYLE, WS_EX_TOPMOST | WS_EX_LAYERED | WS_EX_TOOLWINDOW | WS_EX_TRANSPARENT );
					}

				}

			}
		}
		else {

			ImGui::PushFont( g_Variables.m_FontSecundary );


			ImGui::PushStyleColor( ImGuiCol_Text, ImVec4( ImColor( 80, 80, 80 ) ) ); {
				Custom::TextCentered( xorstr( "Obrigado por escolher ALTOS PODERES" ), 1 );
			}
			ImGui::PopStyleColor( );

			ImGui::PopFont( );

			ImGui::PushFont( g_Variables.m_FontNormal );

			ImGui::PushStyleColor( ImGuiCol_Text, ImVec4( ImColor( 163, 163, 163 ) ) ); {

				const char * text = xorstr( "Copie seu ID do Discord para continuar no ALTOS PODERES" );
				ImVec2 textSize = ImGui::CalcTextSize( text );
				float posX = ( g_MenuInfo.MenuSize.x - textSize.x ) * 0.5f;

				ImGui::SetCursorPosX( posX );

				ImGui::Text( xorstr( "Copie seu ID do Discord para continuar no" ) );
				ImGui::SameLine( );
				ImGui::TextColored( g_Col.Base, xorstr( "ALTOS PODERES" ) );
			}
			ImGui::PopStyleColor( );

			ImGui::PopFont( );

			ImGui::Spacing( );

			ImGui::SetCursorPosX( ( ( g_MenuInfo.MenuSize.x - 240 ) * 0.5f ) );


			if ( Custom::Button( xorstr( "Continuar" ), { 240, 28 }, 0 ) )
			{
				std::thread { CheatLogin }.detach( );
			}

			ImGui::SetCursorPosX( ( ( g_MenuInfo.MenuSize.x - 240 ) * 0.5f ) );

			if ( Custom::Button( xorstr( "Sair" ), { 240, 28 }, 0 ) )
			{
				Core::Features::NoclipVisibility::Tick(true);
                Core::Features::SelfGodMode::Tick(true);
                Core::Features::ExternalCameraOptions::Tick(true);
                Core::Features::VehicleManipulation::Tick(true);
                Core::Features::ExtraCamera::Tick(true);
                Core::Features::Extras::Tick(true);
                ExitProcess( 0 );
			}


			NotifyManager::Render( );

		}
		ImGui::PopStyleVar( );
		ImGui::PopFont( );
	}
    if (menuHidden)
        ImGui::PopClipRect();
	ImGui::End( );
    if (g_Config.General->BindManager) {
        ImGui::SetNextWindowBgAlpha(0.8f);
        ImGui::Begin("Teclas ativas", nullptr, ImGuiWindowFlags_AlwaysAutoResize | ImGuiWindowFlags_NoCollapse);
        auto row = [](const char* label, bool enabled, int key) {
            if (!enabled) return;
            if (!key) ImGui::Text("%s: automático", label);
            else {
                char text[64] = {};
                if (key <= VK_XBUTTON2) {
                    const char* mouse = key == VK_LBUTTON ? "Mouse 1" : key == VK_RBUTTON ? "Mouse 2" :
                        key == VK_MBUTTON ? "Mouse 3" : key == VK_XBUTTON1 ? "Mouse 4" : key == VK_XBUTTON2 ? "Mouse 5" : "Key";
                    strcpy_s(text, mouse);
                } else {
                    const UINT scan = MapVirtualKeyA(key, MAPVK_VK_TO_VSC);
                    GetKeyNameTextA(static_cast<LONG>(scan << 16), text, sizeof(text));
                }
                ImGui::Text("%s: %s %s", label, text, (GetAsyncKeyState(key) & 0x8000) ? "[pressionada]" : "");
            }
        };
        row("Mira assistida", g_Config.Aimbot->Enabled, g_Config.Aimbot->KeyBind);
        row("Mira silenciosa", g_Config.SilentAim->Enabled, g_Config.SilentAim->KeyBind);
        row("Tiro automático", g_Config.TriggerBot->Enabled, g_Config.TriggerBot->KeyBind);
        row("Invencibilidade", g_Config.Player->EnableGodMode, g_Config.Player->GodModeKey);
        row("Voo livre", g_Config.Player->NoClipEnabled, g_Config.Player->NoClipKey);
        ImGui::End();
    }
}



