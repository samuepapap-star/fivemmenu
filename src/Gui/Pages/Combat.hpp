#pragma once
#include <Includes/Includes.hpp>
#include <windows.h>
#include <iostream>
#include <thread>

using namespace std;

namespace Combat {

	static int iSubTabCount = 0;
	static float SubTabAlpha = 0.f;
	static int iSubTab = 0;

	void Render( ) {


		//ImGui::PushStyleVar( ImGuiStyleVar_::ImGuiStyleVar_ItemSpacing, ImVec2( 6, 0 ) );

		if ( Custom::SubTab( xorstr( "Mira assistida" ), 0 == iSubTabCount ) ) {
			iSubTabCount = 0;
		}
		ImGui::SameLine( );
		if ( Custom::SubTab( xorstr( "Tiro automático" ), 1 == iSubTabCount ) ) {
			iSubTabCount = 1;
		}
		ImGui::SameLine( );
		if ( Custom::SubTab( xorstr( "Mira silenciosa" ), 2 == iSubTabCount ) ) {
			iSubTabCount = 2;
		}

		SubTabAlpha = ImClamp( SubTabAlpha + ( 5.f * ImGui::GetIO( ).DeltaTime * ( iSubTabCount == iSubTab ? 1.f : -1.f ) ), 0.f, 1.f );

		if ( SubTabAlpha == 0.f )
			iSubTab = iSubTabCount;

		ImGui::PushStyleVar( ImGuiStyleVar_Alpha, SubTabAlpha * ImGui::GetStyle( ).Alpha );

		ImGui::SetCursorPos( ImVec2( 184, 136 ) );
		ImGui::BeginGroup( );
		{
			switch ( iSubTab )
			{
			case 0: //Aimbot
				ImGui::BeginGroup( );
				{
					ImGui::BeginChild( xorstr( "Geral" ), ImVec2( 340, 440 ), false, 0 );
					{
						Custom::CheckBox( xorstr( "Ativar" ), &g_Config.Aimbot->Enabled );
						Custom::CheckBox( xorstr( "Mostrar FOV" ), &g_Config.Aimbot->ShowFov );
						Custom::CheckBox( xorstr( "Somente visíveis" ), &g_Config.Aimbot->OnlyVisible );
						Custom::CheckBox( xorstr( "Ignorar NPCs" ), &g_Config.Aimbot->IgnoreNPCs );
					}
					ImGui::EndChild( );
				}
				ImGui::EndGroup( );
				ImGui::SameLine( );
				ImGui::BeginGroup( );
				{
					ImGui::BeginChild( xorstr( "Ajustes" ), ImVec2( 340, 440 ), false, 0 );
					{
						ImGui::SliderInt( xorstr( "Campo de visão" ), &g_Config.Aimbot->FOV, 0, 400, xorstr( "%d" ), 0 );
						ImGui::Combo("Região de mira", &g_Config.Aimbot->Hitbox, "Cabeça\0Tronco\0");
                        ImGui::SliderInt( xorstr( "Suavização" ), &g_Config.Aimbot->AimbotSpeed, 0, 100, xorstr( "%d" ), 0 );
						//ImGui::SliderInt( xorstr( "Suavização vertical" ), &g_Config.Aimbot->SmoothVertical, 0, 100, xorstr( "%d" ), 0 );
						ImGui::SliderInt( xorstr( "Distância máxima" ), &g_Config.Aimbot->MaxDistance, 0, 1000, xorstr( "%dm" ), 0 );
						static int KeyMode = 1;
						ImGui::Keybind( xorstr( "Tecla" ), &g_Config.Aimbot->KeyBind, &KeyMode );
                        ImGui::TextDisabled("Sem tecla = automático");
                        if (ImGui::Button("Remover tecla##Aimbot"))
                            g_Config.Aimbot->KeyBind = 0;
					}
					ImGui::EndChild( );
				}
				ImGui::EndGroup( );

				break;
			case 1: //Trigger
				ImGui::BeginGroup( );
				{
					ImGui::BeginChild( xorstr( "Geral" ), ImVec2( 340, 440 ), false, 0 );
					{
						Custom::CheckBox( xorstr( "Ativar" ), &g_Config.TriggerBot->Enabled );

						if (!g_Config.TriggerBot->SmartTrigger) {
						Custom::CheckBox( xorstr( "Mostrar FOV" ), &g_Config.TriggerBot->ShowFov );
						}
						else
						{
							g_Config.TriggerBot->ShowFov = false;
						}

						Custom::CheckBox( xorstr( "Tiro inteligente" ), &g_Config.TriggerBot->SmartTrigger );
						Custom::CheckBox( xorstr( "Somente visíveis" ), &g_Config.TriggerBot->OnlyVisible );
						Custom::CheckBox( xorstr( "Ignorar NPCs" ), &g_Config.TriggerBot->IgnoreNPCs );
					}
					ImGui::EndChild( );
				}
				ImGui::EndGroup( );
				ImGui::SameLine( );
				ImGui::BeginGroup( );
				{
					ImGui::BeginChild( xorstr( "Ajustes" ), ImVec2( 340, 440 ), false, 0 );
					{
						if (!g_Config.TriggerBot->SmartTrigger) {
							ImGui::SliderInt(xorstr("Campo de visão"), &g_Config.TriggerBot->FOV, 0, 400, xorstr("%d"), 0);
							ImGui::SliderInt(xorstr("Distância máxima"), &g_Config.TriggerBot->MaxDistance, 0, 1000, xorstr("%dm"), 0);
						}
						
						ImGui::SliderInt( xorstr( "Atraso" ), &g_Config.TriggerBot->Delay, 0, 10, xorstr( "%d" ), 0 );
						static int KeyMode = 1;
						ImGui::Keybind( xorstr( "Tecla" ), &g_Config.TriggerBot->KeyBind, &KeyMode );
					}
					ImGui::EndChild( );
				}
				ImGui::EndGroup( );
				break;
			case 2: //Silent
				ImGui::BeginGroup( );
				{
					ImGui::BeginChild( xorstr( "Geral" ), ImVec2( 340, 440 ), false, 0 );
					{
						Custom::CheckBox( xorstr( "Ativar" ), &g_Config.SilentAim->Enabled );
						Custom::CheckBox( xorstr( "Mostrar FOV" ), &g_Config.SilentAim->ShowFov );
						Custom::CheckBox( xorstr( "Balas mágicas" ), &g_Config.SilentAim->MagicBullets );
						Custom::CheckBox( xorstr( "Somente visíveis" ), &g_Config.SilentAim->OnlyVisible );
						Custom::CheckBox( xorstr( "Ignorar NPCs" ), &g_Config.SilentAim->IgnoreNPCs );
					}
					ImGui::EndChild( );
				}
				ImGui::EndGroup( );
				ImGui::SameLine( );
				ImGui::BeginGroup( );
				{
					ImGui::BeginChild( xorstr( "Ajustes" ), ImVec2( 340, 440 ), false, 0 );
					{
						ImGui::SliderInt( xorstr( "Campo de visão" ), &g_Config.SilentAim->FOV, 0, 400, xorstr( "%d" ), 0 );
						ImGui::SliderInt( xorstr( "Chance de errar" ), &g_Config.SilentAim->MissChance, 0, 100, xorstr( "%dx" ), 0 );
						ImGui::SliderInt( xorstr( "Distância máxima" ), &g_Config.SilentAim->MaxDistance, 0, 1000, xorstr( "%dm" ), 0 );
						static int KeyMode = 1;
						ImGui::Keybind( xorstr( "Tecla" ), &g_Config.SilentAim->KeyBind, &KeyMode );
                        ImGui::TextDisabled("Sem tecla = automático");
                        if (ImGui::Button("Remover tecla##SilentAim"))
                            g_Config.SilentAim->KeyBind = 0;
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