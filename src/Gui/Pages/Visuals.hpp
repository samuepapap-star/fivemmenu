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

		if ( Custom::SubTab( xorstr( "ESP" ), 0 == iSubTabCount ) ) {
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

		ImGui::SetCursorPos( ImVec2( 184, 136 ) );
		ImGui::BeginGroup( );
		{
			switch ( iSubTab )
			{
			case 0: //Players
				ImGui::BeginGroup( );
				{
					ImGui::BeginChild( xorstr( "Geral#ESP" ), ImVec2( 340, 440 ), false, 0 );
					{
						Custom::CheckBox( xorstr( "Ativar" ), &g_Config.ESP->Enabled );
						if ( Custom::CheckBox( xorstr( "Caixa" ), &g_Config.ESP->Box ) ) {
							if ( !g_Config.ESP->Box ) {
								g_Config.ESP->FilledBox = false;
							}
						}
						if ( Custom::CheckBox( xorstr( "Caixa preenchida" ), &g_Config.ESP->FilledBox ) ) {
							if ( !g_Config.ESP->Box ) {
								g_Config.ESP->Box = true;
							}
						}

						Custom::CheckBox( xorstr( "Nomes dos jogadores" ), &g_Config.ESP->UserNames );
						Custom::CheckBox( xorstr( "Nomes das armas" ), &g_Config.ESP->WeaponName );
						Custom::CheckBox( xorstr( "Distância" ), &g_Config.ESP->DistanceFromMe );


						if ( Custom::CheckBox( xorstr( "Esqueleto" ), &g_Config.ESP->Skeleton ) )
						{
							Core::SDK::Pointers::pLocalPlayer->RemoveKinematics( );
						}

						Custom::CheckBox( xorstr( "Círculo na cabeça" ), &g_Config.ESP->HeadCircle );
						Custom::CheckBox( xorstr( "Barra de vida" ), &g_Config.ESP->HealthBar );
						Custom::CheckBox( xorstr( "Barra de colete" ), &g_Config.ESP->ArmorBar );
						Custom::CheckBox( xorstr( "Linhas" ), &g_Config.ESP->SnapLines );
						Custom::CheckBox( xorstr( "Destacar visíveis" ), &g_Config.ESP->HighlightVisible );
						Custom::CheckBoxCfg( xorstr( "Marcar amigo" ), &g_Config.ESP->FriendsMarker, [ ]
							{
								static int KeyMode = 1;
								ImGui::Keybind( xorstr( "Tecla" ), &g_Config.ESP->FriendsMarkerBind, &KeyMode );
							}
						);
						Custom::CheckBox( xorstr( "Mostrar meu personagem" ), &g_Config.ESP->ShowLocalPlayer );
						Custom::CheckBox( xorstr( "Ignorar mortos" ), &g_Config.ESP->IgnoreDead );
                        Custom::CheckBox("Possível noclip", &g_Config.ESP->PossibleNoclip);
                        if (ImGui::IsItemHovered())
                            ImGui::SetTooltip("Indícios de voo ou ausência de colisão por 3 segundos. Pode marcar quedas ou paraquedas. Não identifica administradores.");
						Custom::CheckBox( xorstr( "Ignorar NPCs" ), &g_Config.ESP->IgnoreNPCs );

						ImGui::SliderInt( xorstr( "Distância máxima" ), &g_Config.ESP->MaxDistance, 0, 5000, xorstr( "%dm" ), 0 );
					}
					ImGui::EndChild( );
				}
				ImGui::EndGroup( );
				ImGui::SameLine( );
				ImGui::BeginGroup( );
				{
					ImGui::BeginChild( xorstr( "Prévia" ), ImVec2( 340, 440 ), false, ImGuiWindowFlags_NoScrollbar );
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
					ImGui::BeginChild( xorstr( "Geral" ), ImVec2( 340, 440 ), false, 0 );
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
					ImGui::BeginChild( xorstr( "Ajustes" ), ImVec2( 340, 440 ), false, 0 );
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