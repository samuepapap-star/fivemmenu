#pragma once
#include <Gui/MenuShortcut.hpp>
#include <Gui/MenuScale.hpp>
#include <Core/Features/ExtraCamera.hpp>
#include <Core/Features/SelfGodMode.hpp>
#include <Core/SDK/ClientState.hpp>
#include <Core/Features/NoclipVisibility.hpp>
#include <Core/Features/ExternalCameraOptions.hpp>
#include <Core/Features/CarreraAdditions.hpp>
#include <Gui/Pages/NamedConfigs.hpp>
#include <Includes/Includes.hpp>
#include <windows.h>
#include <iostream>
#include <thread>

using namespace std;

namespace Settings {

	static int iSubTabCount = 0;
	static float SubTabAlpha = 0.f;
	static int iSubTab = 0;


	void Render( ) {


		//ImGui::PushStyleVar( ImGuiStyleVar_::ImGuiStyleVar_ItemSpacing, ImVec2( 6, 0 ) );

		ImGui::PushStyleVar( ImGuiStyleVar_Alpha, ImGui::GetStyle( ).Alpha );

		ImGui::SetCursorPos( ImVec2( 184, 136 ) );
		ImGui::BeginGroup( );
		{
			switch ( iSubTab )
			{
			case 0: //Settings
				ImGui::BeginGroup( );
				{
					ImGui::BeginChild( xorstr( "Geral" ), ImVec2( 340, 440 ), false, 0 );
					{
						ImGui::TextUnformatted("Abrir e fechar o menu");
                        if (ImGui::Button(MenuShortcut::Label(g_Config.General->MenuKey).c_str()))
                            MenuShortcut::Begin();
                        ImGui::TextDisabled("P e Insert continuam ativos.");
                        Custom::CheckBox( xorstr( "VSync" ), &g_Config.General->VSync );
                        Custom::CheckBox("Painel de teclas", &g_Config.General->BindManager);
                        ImGui::SliderInt("Escala do menu", &g_Config.General->MenuScale, 0, 100);
                        ImGui::TextDisabled("0 = 50%% | 50 = 100%% | 100 = 150%%");
                        ImGui::TextDisabled("Solte o mouse para aplicar. Atual: %.0f%%",MenuScale::factor*100.f);
                        if (ImGui::Button("Restaurar escala")) g_Config.General->MenuScale = 50;
						if ( Custom::CheckBox( xorstr( "Ocultar em capturas" ), &g_Config.General->StreamProof ) )
						{
							if ( g_Config.General->StreamProof )
							{
								SetWindowDisplayAffinity( g_Variables.g_hCheatWindow, WDA_EXCLUDEFROMCAPTURE );
							}
							else
							{
								SetWindowDisplayAffinity( g_Variables.g_hCheatWindow, WDA_NONE );
							}
						}

						//Custom::CheckBox( xorstr( "Marca d'água" ), &g_Config.General->WaterMark );
						//Custom::CheckBox( xorstr( "Lista de funções" ), &g_Config.General->ArrayList );

						//Custom::CheckBoxCfg( xorstr( "Marca d'água" ), &g_Config.General->WaterMark, [ & ] ( ) {
						//	static bool Rainbow = false;
						//	Custom::CheckBox( xorstr( "Arco-íris" ), &Rainbow );
						//	} );

						//Custom::CheckBoxCfg( xorstr( "Lista de funções" ), &g_Config.General->ArrayList, [ & ] ( ) {
						//	static bool Rainbow = false;
						//	Custom::CheckBox( xorstr( "Arco-íris" ), &Rainbow );
						//} );


                        ClientState::Render();

						if ( Custom::Button( xorstr( "Encerrar" ), ImVec2( -1, 30 ), 0 ) ) 
						{
                            Core::Features::ExtraCamera::Stop();
                            if(Core::Features::ExtraCamera::patched)NotifyManager::Send(Core::Features::ExtraCamera::status,4000);
                            Core::Features::Extras::Tick(true);
                            Core::Features::SelfGodMode::Tick(true);
                            g_Config.Player->NoCollisionEnabled=false;
                            Core::Features::NoclipVisibility::Tick(true);
                            Core::Features::ExternalCameraOptions::Tick(true);
                            Core::Features::CarreraAdditions::TickCollision(true);
							if(!Core::Features::ExtraCamera::patched)exit( 0 );
						}
					}
					ImGui::EndChild( );
				}
				ImGui::EndGroup( );
				ImGui::SameLine( );
				ImGui::BeginGroup( );
				{
					ImGui::BeginChild( xorstr( "Configurações" ), ImVec2( 340, 440 ), false, 0 );
					{
						ImGui::PushStyleVar( ImGuiStyleVar_ItemSpacing, ImVec2( 14, 8 ) );
						if ( Custom::Button( xorstr( "Exportar configuração" ), ImVec2( -1, 30 ), 0 ) ) 
						{
							std::thread cfgExport( [ ] {
								std::string CfgMsg = NamedConfigs::PortugueseStatus(g_Config.SaveCurrentConfig( xorstr( "..." ) ));
								NotifyManager::Send( CfgMsg.c_str( ), 3000 );
								} );

							cfgExport.detach( );
						}
						if ( Custom::Button( xorstr( "Importar configuração" ), ImVec2( -1, 30 ), 0 ) ) 
						{
							std::thread cfgImport( [ ] { NotifyManager::Send( NamedConfigs::PortugueseStatus(g_Config.LoadCfg( xorstr( "..." ), Utils::GetClipboard( ) )).c_str( ), 3000 ); } );

							cfgImport.detach( );
						}
                        NamedConfigs::Render();
						ImGui::PopStyleVar( );
					}
					ImGui::EndChild( );
				}
				ImGui::EndGroup( );

				break;
			default:
				break;
			}

		}
		ImGui::EndGroup( );

		ImGui::PopStyleVar( );
	}
}



