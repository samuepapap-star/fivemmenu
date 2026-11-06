#pragma once
#include <Includes/includes.hpp>
#include <windows.h>
#include <iostream>
#include <thread>
#include <Includes/CustomWidgets/Preview.hpp>

#include <Core/Core.hpp>

using namespace std;

namespace Visuals {

	static int iSubTabCount = 0;
	static float SubTabAlpha = 0.f;
	static int iSubTab = 0;

	void Render( ) {


		//ImGui::PushStyleVar( ImGuiStyleVar_::ImGuiStyleVar_ItemSpacing, ImVec2( 6, 0 ) );

		if ( Custom::SubTab( xorstr( "Jogadores" ), 0 == iSubTabCount ) ) {
			iSubTabCount = 0;
		}
		ImGui::SameLine( );
		if ( Custom::SubTab( xorstr( "Veículos" ), 1 == iSubTabCount ) ) {
			iSubTabCount = 1;
		}


		SubTabAlpha = ImClamp( SubTabAlpha + ( 5.f * ImGui::GetIO( ).DeltaTime * ( iSubTabCount == iSubTab ? 1.f : -1.f ) ), 0.f, 1.f );

		if ( SubTabAlpha == 0.f )
			iSubTab = iSubTabCount;


		ImGui::PushStyleVar( ImGuiStyleVar_Alpha, SubTabAlpha * ImGui::GetStyle( ).Alpha );

		ImGui::SetCursorPos( ImVec2( 184, 76 ) );
		ImGui::BeginGroup( );
		{
			switch ( iSubTab )
			{
			case 0: //Players
				ImGui::BeginGroup( );
				{
					ImGui::BeginChild( xorstr( "Globals#ESP" ), ImVec2( 230, 400 ), false, 0 );
					{
						Custom::CheckBox( xorstr( "Ativar" ), &g_Config.ESP->Enabled );
						if ( Custom::CheckBox( xorstr( "Box" ), &g_Config.ESP->Box ) ) {
							if ( !g_Config.ESP->Box ) {
								g_Config.ESP->FilledBox = false;
							}
						}
						if ( Custom::CheckBox( xorstr( "Caixa preenchida" ), &g_Config.ESP->FilledBox ) ) {
							if ( !g_Config.ESP->Box ) {
								g_Config.ESP->Box = true;
							}
						}

						ImGui::SliderFloat("Box Size", &g_Config.ESP->BoxSize, 0.5f, 1.8f, "%.2fx");
                        ImGui::SliderFloat("FillBox Alpha", &g_Config.ESP->FillBoxAlpha, 0.f, 1.f, "%.2f");
                        Custom::CheckBox( xorstr( "Nomes dos jogadores" ), &g_Config.ESP->UserNames );
						Custom::CheckBox( xorstr( "Nomes das armas" ), &g_Config.ESP->WeaponName );
						Custom::CheckBox( xorstr( "Distância" ), &g_Config.ESP->DistanceFromMe );


						if ( Custom::CheckBox( xorstr( "Esqueleto" ), &g_Config.ESP->Skeleton ) )
						{
							Core::SDK::Pointers::pLocalPlayer->RemoveKinematics( );
						}

						Custom::CheckBox( xorstr( "Círculo da cabeça" ), &g_Config.ESP->HeadCircle );
						Custom::CheckBox( xorstr( "Barra de vida" ), &g_Config.ESP->HealthBar );
						Custom::CheckBox( xorstr( "Barra de colete" ), &g_Config.ESP->ArmorBar );
						Custom::CheckBox( xorstr( "Linhas" ), &g_Config.ESP->SnapLines );
						//Custom::CheckBox( xorstr( "Destacar visíveis" ), &g_Config.ESP->HighlightVisible );
						Custom::CheckBoxCfg( xorstr( "Marcar amigos" ), &g_Config.ESP->FriendsMarker, [ ]
							{
								static int KeyMode = 1;
								ImGui::Keybind( xorstr( "Tecla" ), &g_Config.ESP->FriendsMarkerBind, &KeyMode );
							}
						);
						Custom::CheckBox( xorstr( "Mostrar jogador local" ), &g_Config.ESP->ShowLocalPlayer );
						Custom::CheckBox( xorstr( "Ignorar mortos" ), &g_Config.ESP->IgnoreDead );
                        Custom::CheckBox("Possible Noclip", &g_Config.ESP->PossibleNoclip);
                        if (ImGui::IsItemHovered())
                            ImGui::SetTooltip("Air/no-collision flags for 3 seconds. May flag falls or parachutes. Does not identify staff.");
						Custom::CheckBox( xorstr( "Ignorar NPCs" ), &g_Config.ESP->IgnoreNPCs );

						ImGui::SliderInt( xorstr( "Distância máxima" ), &g_Config.ESP->MaxDistance, 0, 1000, xorstr( "%dm" ), 0 );
					}
					ImGui::EndChild( );
				}
				ImGui::EndGroup( );
				ImGui::SameLine( );
				ImGui::BeginGroup( );
				{
					ImGui::BeginChild( xorstr( "Prévia" ), ImVec2( 230, 400 ), false, ImGuiWindowFlags_NoScrollbar );
					{
						Custom::g_EspPreview.DragDropHandler( );
						Custom::g_EspPreview.Draw( );
					}
					ImGui::EndChild( );
				}
				ImGui::EndGroup( );

				break;
			case 1: //Vehicles
				ImGui::BeginGroup( );
				{
					ImGui::BeginChild( xorstr( "Geral" ), ImVec2( 230, 400 ), false, 0 );
					{
						Custom::CheckBox( xorstr( "Ativar" ), &g_Config.VehicleESP->Enabled );
						Custom::CheckBox( xorstr( "Nomes dos veículos" ), &g_Config.VehicleESP->VehName );
						Custom::CheckBox( xorstr( "Trancado/Destrancado" ), &g_Config.VehicleESP->ShowLockUnlock );
						Custom::CheckBox( xorstr( "Linhas" ), &g_Config.VehicleESP->SnapLines );
						Custom::CheckBox( xorstr( "Distância" ), &g_Config.VehicleESP->DistanceFromMe );
					}
					ImGui::EndChild( );
				}
				ImGui::EndGroup( );
				ImGui::SameLine( );
				ImGui::BeginGroup( );
				{
					ImGui::BeginChild( xorstr( "Configurações" ), ImVec2( 230, 400 ), false, 0 );
					{
						ImGui::SliderInt( xorstr( "Distância máxima" ), &g_Config.VehicleESP->MaxDistance, 0, 1000, xorstr( "%dm" ), 0 );
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