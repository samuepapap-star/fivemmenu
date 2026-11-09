#include <Core/Features/SourceOptionLogic.hpp>
#pragma once
#include <Bridge/Features.hpp>
#include <Core/ExtensionSettings.hpp>
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
						Custom::CheckBox("Antecipar movimento", &g_Config.Aimbot->Prediction);
                        Custom::CheckBox("Mira dentro de veículo", &g_Config.Aimbot->InVehicleAim);
                        Custom::CheckBox("Somente visíveis", &g_Config.Aimbot->OnlyVisible);
                        Custom::CheckBox("Ignorar alvos em veículos", &g_Config.Aimbot->SkipVehicles);
                        Custom::CheckBox("Filtrar por vida", &g_Config.Aimbot->HealthFilter);
                        Custom::CheckBox("Ignorar NPCs", &g_Config.Aimbot->IgnoreNPCs);
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
						ImGui::SliderInt("Suavização", &g_Config.Aimbot->AimbotSpeed, 0, 100);
                        ImGui::SliderInt("Distância máxima", &g_Config.Aimbot->MaxDistance, 0, 1000, "%d m");
                        if (g_Config.Aimbot->Prediction)
                            ImGui::SliderFloat("Tempo de previsão", &g_Config.Aimbot->PredictionTime, 0.01f, 1.f, "%.2f s");
                        if (g_Config.Aimbot->HealthFilter)
                            ImGui::SliderFloat("Vida mínima", &g_Config.Aimbot->HealthThreshold, 0.f, 100.f);
                        ImGui::SliderFloat("Variação aleatória", &g_Config.Aimbot->RandomAngle, 0.f, 5.f);
                        ImGui::Combo("Seleção de osso", &g_Config.Aimbot->BoneMode, "Fixo\0Mais próximo da mira\0Aleatório\0");
                        if (!g_Config.Aimbot->BoneMode)
                            ImGui::Combo("Osso", &g_Config.Aimbot->V7Bone, "Cabeça\0Pelve\0Pescoço\0Clavícula D\0Clavícula E\0Braço D\0Braço E\0Antebraço E\0Antebraço D\0Mão E\0Mão D\0Coxa D\0Panturrilha D\0Coxa E\0Panturrilha E\0Pé D\0Pé E\0");
                        static int keyMode = 1;
                        ImGui::Keybind("Tecla", &g_Config.Aimbot->KeyBind, &keyMode);
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
                        ImGui::Combo("Regiao do tiro automatico",&ExtraSettings::TriggerHitbox,"Cabeca\0Pescoco\0Tronco\0");
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
                        ImGui::BeginDisabled(BridgeFeatures::silentRpg);
                        Custom::CheckBox(xorstr("Ativar"),&g_Config.SilentAim->Enabled);
                        ImGui::EndDisabled();
                        if(Custom::CheckBox("Silent RPG (sem equipar)",&BridgeFeatures::silentRpg)){
                            if(BridgeFeatures::silentRpg)BridgeFeatures::EnableRpg();
                        }
                        ImGui::TextWrapped("Silent RPG usa o alvo dentro do FOV e substitui a mira silenciosa padrao enquanto estiver ativo.");
                        BridgeFeatures::Status();
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
						ImGui::Combo("Regiao da mira silenciosa",&ExtraSettings::SilentHitbox,"Cabeca\0Pescoco\0Tronco\0");
                        Custom::CheckBox("Predicao da mira silenciosa",&ExtraSettings::SilentPrediction);
                        if(ExtraSettings::SilentPrediction)ImGui::SliderFloat("Antecipacao",&ExtraSettings::PredictionSeconds,0.f,0.3f,"%.2f s");
                        Custom::CheckBox("Priorizar somente motoristas",&ExtraSettings::SilentForceDriver);
                        Custom::CheckBox("Distancia automatica pela arma",&ExtraSettings::SilentAutoDistance);
                        if(ExtraSettings::SilentAutoDistance&&!SourceOptionLogic::KnownWeaponRangeLayout(g_Offsets.CurrentBuild))
                            ImGui::TextWrapped("Alcance da arma nao validado nesta build; usando distancia manual.");
                        Custom::CheckBox("FOV dinamico",&ExtraSettings::SilentDynamicFov);
                        if(ExtraSettings::SilentDynamicFov)ImGui::SliderInt("FOV minimo",&ExtraSettings::SilentMinFov,1,g_Config.SilentAim->FOV);
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


