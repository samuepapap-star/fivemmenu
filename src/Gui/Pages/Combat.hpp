#pragma once
#include <Core/Features/SilentAim.hpp>
#include <Includes/Includes.hpp>
#include <windows.h>
#include <Core/Features/ExternalCameraOptions.hpp>
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
                        ImGui::ColorEdit4("Cor do FOV##Aimbot", &g_Config.Aimbot->FovColor.Value.x, ImGuiColorEditFlags_AlphaBar);
                        ImGui::ColorEdit3("Cor do menu", &g_Config.General->MenuAccent.x);
						Custom::CheckBox( xorstr( "Somente visíveis" ), &g_Config.Aimbot->OnlyVisible );
						ImGui::Combo("Alvos", &g_Config.Aimbot->TargetType, "Todos\0Somente jogadores\0Somente NPCs\0");
                        ImGui::Combo("Modo de mira", &g_Config.Aimbot->AimMode, "Mouse\0Câmera\0");
                        ImGui::Combo("Prioridade", &g_Config.Aimbot->SelectionMode, "Distância\0Proximidade da mira\0");
                        Custom::CheckBox("Retículo central", &g_Config.Aimbot->Crosshair);
                        Custom::CheckBox("Alterar FOV da câmera", &g_Config.Aimbot->CameraFovEnabled);
                        ImGui::SliderFloat("FOV da câmera", &g_Config.Aimbot->CameraFov, 50.f, 160.f, "%.0f");
                        ImGui::BeginDisabled(!Core::Features::ExternalCameraOptions::RangeBuildSupported());
                        Custom::CheckBox("Alterar alcance da arma", &g_Config.Aimbot->WeaponRangeEnabled);
                        ImGui::SliderFloat("Alcance da arma", &g_Config.Aimbot->WeaponRange, 1.f, 1000.f, "%.0f m");
                        ImGui::EndDisabled();
                        if (!Core::Features::ExternalCameraOptions::RangeBuildSupported())
                            ImGui::TextWrapped("Alcance indisponível nesta build: campo não identificado pela source.");
                        Custom::CheckBox("Modo Legit", &g_Config.Aimbot->LegitMode, true, "Aumenta a suavização da mira.");
                        Custom::CheckBox("Suavização X/Y separada", &g_Config.Aimbot->SeparateSmooth);
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
						ImGui::Combo("Região de mira", &g_Config.Aimbot->Hitbox, "Cabeça\0Tronco\0Perna esquerda\0Perna direita\0Braço direito\0Braço esquerdo\0");
                        if (g_Config.Aimbot->SeparateSmooth) {
                            ImGui::SliderInt("Suavização X", &g_Config.Aimbot->SmoothHorizontal, 1, 100);
                            ImGui::SliderInt("Suavização Y", &g_Config.Aimbot->SmoothVertical, 1, 100);
                        } else {
                            ImGui::SliderInt( xorstr( "Suavização" ), &g_Config.Aimbot->AimbotSpeed, 0, 100, xorstr( "%d" ), 0 );
                        }
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
                        ImGui::ColorEdit4("Cor do FOV##TriggerBot", &g_Config.TriggerBot->FovColor.Value.x, ImGuiColorEditFlags_AlphaBar);
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
                        ImGui::ColorEdit4("Cor do FOV##SilentAim", &g_Config.SilentAim->FovColor.Value.x, ImGuiColorEditFlags_AlphaBar);
                        ImGui::TextWrapped("%s",Core::Features::g_SilentAim.StatusText());
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

