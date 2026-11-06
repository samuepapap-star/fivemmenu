#pragma once
#include <Includes/includes.hpp>
#include <windows.h>
#include <iostream>
#include <thread>

using namespace std;

namespace World {

	static int iSubTabCount = 0;
	static float SubTabAlpha = 0.f;
	static int iSubTab = 0;

	void Render( )
	{

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
			{
				/*struct PlayerList_t {
					std::string Name;
				};

				std::vector<PlayerList_t> Player = {
					PlayerList_t( xorstr( "MorningGay" ) ),
					PlayerList_t( xorstr( "PolloGay" ) ),
				};*/

				static int SeletedIndex = -1;
				bool IsSelected;


				ImGui::BeginGroup( );
				{

					ImGui::BeginChild( xorstr( "Lista" ), ImVec2( 230, 400 ), false, 0 );
					{
						ImGui::PushStyleVar( ImGuiStyleVar_ItemSpacing, ImVec2( 14, 8 ) );
						for ( int i = 0; i < Core::SDK::Game::EntityList.size( ); i++ )
						{
							if ( Core::SDK::Game::EntityList[ i ].Ped == Core::SDK::Pointers::pLocalPlayer )
								continue;

							if ( Core::SDK::Game::EntityList[ i ].PedType != 2 )
								continue;

							IsSelected = SeletedIndex == i;
							if ( ImGui::ListSelectable( Core::SDK::Game::EntityList[ i ].NetworkInfo.UserName.c_str( ), &IsSelected ) ) SeletedIndex = i;
						}
						ImGui::PopStyleVar( );
					}
					ImGui::EndChild( );
				}
				ImGui::EndGroup( );
				ImGui::SameLine( );
				ImGui::BeginGroup( );
				{
					ImGui::BeginChild( xorstr( "Informações" ), ImVec2( 230, 180 ), false, 0 );
					{
						if ( SeletedIndex == -1 ) {
							ImGui::TextColored( ImColor( g_Col.SecundaryText ), xorstr( "Selecionar jogador" ) );
						}
						else
						{
							auto SelectedPed = Core::SDK::Game::EntityList[ SeletedIndex ];
							ImGui::PushStyleVar( ImGuiStyleVar_ItemSpacing, ImVec2( 14, 4 ) );

							ImGui::TextColored( ImColor( g_Col.FeaturesText ), xorstr( "Nome:" ) ); ImGui::SameLine( );
							ImGui::TextColored( ImColor( g_Col.SecundaryText ), SelectedPed.NetworkInfo.UserName.c_str());
							ImGui::TextColored( ImColor( g_Col.FeaturesText ), xorstr( "Id:" ) ); ImGui::SameLine( );
							ImGui::TextColored( ImColor( g_Col.SecundaryText ), std::to_string( SelectedPed.Id ).c_str( ) );
							ImGui::TextColored( ImColor( g_Col.FeaturesText ), xorstr( "Distância:" ) ); ImGui::SameLine( );
							ImGui::TextColored( ImColor( g_Col.SecundaryText ), std::to_string( SelectedPed.Distance ).c_str( ) );
							ImGui::TextColored( ImColor( g_Col.FeaturesText ), xorstr( "Arma:" ) ); ImGui::SameLine( );
							ImGui::TextColored( ImColor( g_Col.SecundaryText ), SelectedPed.WeaponName.c_str( ) );
							ImGui::TextColored( ImColor( g_Col.FeaturesText ), xorstr( "Amigo:" ) ); ImGui::SameLine( );
							ImGui::TextColored( ImColor( g_Col.SecundaryText ), SelectedPed.IsFriend ? xorstr( "Sim" ) : xorstr( "Não" ) );

							ImGui::PopStyleVar( );
						}
					}
					ImGui::EndChild( );
					ImGui::BeginChild( xorstr( "Ações" ), ImVec2( 230, 210 ), false, 0 );
					{
						if ( SeletedIndex == -1 ) {
							ImGui::TextColored( ImColor( g_Col.SecundaryText ), xorstr( "Selecionar jogador" ) );
						}
						else
						{
							auto& SelectedPed = Core::SDK::Game::EntityList[ SeletedIndex ];

							ImGui::PushStyleVar( ImGuiStyleVar_ItemSpacing, ImVec2( 14, 8 ) );
							bool Friend = SelectedPed.IsFriend;
							if ( Custom::CheckBox( xorstr( "Amigo" ), &Friend ) )
							{
								Core::SDK::Game::FriendMap[ SelectedPed.Ped ] = Friend;
							}

							ImGui::Spacing( );

							if ( Custom::Button( xorstr( "Teleportar" ), ImVec2( -1, 30 ), 0 ) )
							{
								Core::SDK::Pointers::pLocalPlayer->SetPos( SelectedPed.Pos );
							}
							//if ( Custom::Button( xorstr( "Observar" ), ImVec2( -1, 30 ), 0 ) ) 
							//{
							//}
							//if ( Custom::Button( xorstr( "Copiar roupa" ), ImVec2( -1, 30 ), 0 ) ) 
							//{
							//}
							if ( Custom::Button( xorstr( "Copiar Discord" ), ImVec2( -1, 30 ), 0 ) )
							{
								std::string PlayerDiscord = SelectedPed.NetworkInfo.DiscordId;
								if ( PlayerDiscord.empty( ) )
								{
									std::thread( [ ] ( ) { NotifyManager::Send( xorstr( "DiscordId not found." ), 4000 ); } ).detach( );
								}
								else
								{
									std::thread( [ ] ( ) { NotifyManager::Send( xorstr( "DiscordId copied to clipboard!" ), 4000 ); } ).detach( );
									Utils::PasteClipboard( PlayerDiscord.c_str( ) );
								}
							}
							if ( Custom::Button( xorstr( "Copiar Steam" ), ImVec2( -1, 30 ), 0 ) )
							{
								std::string PlayerSteam = SelectedPed.NetworkInfo.SteamId;
								if ( PlayerSteam.empty( ) )
								{
									std::thread( [ ] ( ) { NotifyManager::Send( xorstr( "SteamId not found." ), 4000 ); } ).detach( );
								}
								else
								{
									std::thread( [ ] ( ) { NotifyManager::Send( xorstr( "SteamId copied to clipboard!" ), 4000 ); } ).detach( );
									Utils::PasteClipboard( PlayerSteam.c_str( ) );
								}
							}

							ImGui::PopStyleVar( );
						}
					}
					ImGui::EndChild( );
				}
				ImGui::EndGroup( );
			}
			break;
			case 1: //Vehicles
			{

				static int SeletedIndex = -1;
				bool IsSelected;


				ImGui::BeginGroup( );
				{
					ImGui::BeginChild( xorstr( "Lista" ), ImVec2( 230, 400 ), false, 0 );
					{
						ImGui::PushStyleVar( ImGuiStyleVar_ItemSpacing, ImVec2( 14, 8 ) );


						{
							std::lock_guard<std::mutex> lock( Core::Threads::g_VehicleList.vehicleListMutex ); // Protege a lista durante a renderização

							for ( int i = 0; i < Core::SDK::Game::VehicleList.size( ); i++ )
							{
								IsSelected = SeletedIndex == i;
								std::string Name = Core::SDK::Game::VehicleList[ i ].Name + xorstr( " (" ) + std::to_string( ( int ) Core::SDK::Game::VehicleList[ i ].Dist ) + xorstr( "m)" );
								if ( ImGui::ListSelectable( Name.c_str( ), &IsSelected ) ) SeletedIndex = i;
							}
						}

						ImGui::PopStyleVar( );
					}
					ImGui::EndChild( );
				}
				ImGui::EndGroup( );
				ImGui::SameLine( );
				ImGui::BeginGroup( );
				{
					ImGui::BeginChild( xorstr( "Informações" ), ImVec2( 230, 140 ), false, 0 );
					{
						if ( SeletedIndex == -1 ) {
							ImGui::TextColored( ImColor( g_Col.SecundaryText ), xorstr( "Selecionar veículo" ) );
						}
						else
						{
							ImGui::PushStyleVar( ImGuiStyleVar_ItemSpacing, ImVec2( 14, 4 ) );

							ImGui::TextColored( ImColor( g_Col.FeaturesText ), xorstr( "Nome:" ) ); ImGui::SameLine( );
							ImGui::TextColored( ImColor( g_Col.SecundaryText ), Core::SDK::Game::VehicleList[ SeletedIndex ].Name.c_str( ) );
							ImGui::TextColored( ImColor( g_Col.FeaturesText ), xorstr( "Distância:" ) ); ImGui::SameLine( );
							ImGui::TextColored( ImColor( g_Col.SecundaryText ), ( std::to_string( ( int ) Core::SDK::Game::VehicleList[ SeletedIndex ].Dist ) + xorstr( "m" ) ).c_str( ) );
							ImGui::TextColored( ImColor( g_Col.FeaturesText ), xorstr( "Motorista:" ) ); ImGui::SameLine( );
							ImGui::TextColored( ImColor( g_Col.SecundaryText ), Core::SDK::Game::VehicleList[ SeletedIndex ].Driver == 0 ? xorstr( "Desconhecido" ) : Core::SDK::Game::VehicleList[ SeletedIndex ].Driver->GetPedType( ) != 2 ? xorstr( "NPC" ) : Core::SDK::Game::GetPedName( Core::SDK::Game::VehicleList[ SeletedIndex ].Driver ).c_str( ) );
							ImGui::TextColored( ImColor( g_Col.FeaturesText ), xorstr( "Portas trancadas:" ) ); ImGui::SameLine( );
							ImGui::TextColored( ImColor( g_Col.SecundaryText ), Core::SDK::Game::VehicleList[ SeletedIndex ].IsLocked ? xorstr( "Sim" ) : xorstr( "Não" ) );

							ImGui::PopStyleVar( );
						}
					}
					ImGui::EndChild( );
					ImGui::BeginChild( xorstr( "Ações" ), ImVec2( 230, 250 ), false, 0 );
					{
						if ( SeletedIndex == -1 ) {
							ImGui::TextColored( ImColor( g_Col.SecundaryText ), xorstr( "Selecionar veículo" ) );
						}
						else
						{
							auto Veh = Core::SDK::Game::VehicleList[ SeletedIndex ].Pointer;
							ImGui::PushStyleVar( ImGuiStyleVar_ItemSpacing, ImVec2( 14, 8 ) );

							if ( Core::SDK::Game::VehicleList[ SeletedIndex ].Pointer->IsLocked( ) )
							{
								if ( Custom::Button( xorstr( "Destrancar" ), ImVec2( -1, 30 ), 0 ) ) {
									Veh->DoorState( true );
								}
							}
							else {
								if ( Custom::Button( xorstr( "Trancar" ), ImVec2( -1, 30 ), 0 ) ) {
									Veh->DoorState( false );
								}
							}

							if ( Custom::Button( xorstr( "Teleportar" ), ImVec2( -1, 30 ), 0 ) ) {
								Core::SDK::Pointers::pLocalPlayer->SetPos( Veh->GetPos( ) );
							}


							ImGui::PopStyleVar( );
						}
					}
					ImGui::EndChild( );

				}
				ImGui::EndGroup( );
			}
			break;
			default:
				break;
			}

		}
		ImGui::EndGroup( );
		ImGui::PopStyleVar( );

	}
}