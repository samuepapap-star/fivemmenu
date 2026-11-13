#pragma once
#include <Bridge/Features.hpp>
#include <Core/Features/ExtraCamera.hpp>
#include <Core/Features/CarreraAdditions.hpp>
#include <Core/Features/VehicleManipulation.hpp>
#include <Includes/includes.hpp>
#include <windows.h>
#include <iostream>
#include <thread>

using namespace std;

namespace World {

	static int iSubTabCount = 0;
	static float SubTabAlpha = 0.f;
	static int iSubTab = 0;

	void Render(int section = -1, int actionMode = 0)
	{

		//ImGui::PushStyleVar( ImGuiStyleVar_::ImGuiStyleVar_ItemSpacing, ImVec2( 6, 0 ) );

        if (section >= 0) {
            iSubTabCount = iSubTab = section;
            SubTabAlpha = 1.f;
        } else {
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


        }
		ImGui::PushStyleVar( ImGuiStyleVar_Alpha, SubTabAlpha * ImGui::GetStyle( ).Alpha );

		ImGui::SetCursorPos( ImVec2( 184, 136 ) );
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
                static CPed* selectedPlayer=nullptr;
                static int selectedPlayerId=0;
                SeletedIndex=-1;
                for(size_t index=0;index<Core::SDK::Game::EntityList.size();++index) {
                    const auto &entity=Core::SDK::Game::EntityList[index];
                    if(entity.Ped==selectedPlayer && entity.Id==selectedPlayerId && entity.PedType==2 &&
                        entity.Ped!=Core::SDK::Pointers::pLocalPlayer) {
                        SeletedIndex=static_cast<int>(index);break;
                    }
                }
                bool IsSelected;


				ImGui::BeginGroup( );
				{

					ImGui::BeginChild( xorstr( "Lista" ), ImVec2( 340, 440 ), false, 0 );
					{
						ImGui::PushStyleVar( ImGuiStyleVar_ItemSpacing, ImVec2( 14, 8 ) );
						for ( int i = 0; i < Core::SDK::Game::EntityList.size( ); i++ )
						{
							if ( Core::SDK::Game::EntityList[ i ].Ped == Core::SDK::Pointers::pLocalPlayer )
								continue;

							if ( Core::SDK::Game::EntityList[ i ].PedType != 2 )
								continue;

							IsSelected = SeletedIndex == i;
							ImGui::PushID(i);
                            if(ImGui::ListSelectable(Core::SDK::Game::EntityList[i].NetworkInfo.UserName.c_str(),&IsSelected)) {
                                SeletedIndex=i;
                                selectedPlayer=Core::SDK::Game::EntityList[i].Ped;
                                selectedPlayerId=Core::SDK::Game::EntityList[i].Id;
                            }
                            ImGui::PopID();
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
						if (SeletedIndex < 0 || SeletedIndex >= static_cast<int>(Core::SDK::Game::EntityList.size())) {
							ImGui::TextColored( ImColor( g_Col.SecundaryText ), xorstr( "Selecione um jogador" ) );
						}
						else
						{
							auto SelectedPed = Core::SDK::Game::EntityList[ SeletedIndex ];
							ImGui::PushStyleVar( ImGuiStyleVar_ItemSpacing, ImVec2( 14, 4 ) );

							ImGui::TextColored( ImColor( g_Col.FeaturesText ), xorstr( "Nome:" ) ); ImGui::SameLine( );
							ImGui::TextColored( ImColor( g_Col.SecundaryText ), SelectedPed.NetworkInfo.UserName.c_str());
							ImGui::TextColored( ImColor( g_Col.FeaturesText ), xorstr( "ID:" ) ); ImGui::SameLine( );
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
					ImGui::BeginChild( xorstr( "Ações" ), ImVec2(340,260), false, 0 );
					{
						if (SeletedIndex < 0 || SeletedIndex >= static_cast<int>(Core::SDK::Game::EntityList.size())) {
							ImGui::TextColored( ImColor( g_Col.SecundaryText ), xorstr( "Selecione um jogador" ) );
						}
						else
						{
							auto& SelectedPed = Core::SDK::Game::EntityList[ SeletedIndex ];

							ImGui::PushStyleVar( ImGuiStyleVar_ItemSpacing, ImVec2( 14, 8 ) );
							bool Friend = SelectedPed.IsFriend;
							if ((actionMode == 0 || actionMode == 2) && Custom::CheckBox( xorstr( "Amigo" ), &Friend ) )
							{
								Core::SDK::Game::FriendMap[ SelectedPed.Ped ] = Friend;
							}

							ImGui::Spacing( );

							if (actionMode == 2 && Custom::Button( xorstr( "Ir ate o jogador" ), ImVec2( -1, 30 ), 0 ) )
							{
								Core::SDK::Pointers::pLocalPlayer->SetPos( SelectedPed.Pos );
							}
							//if ( Custom::Button( xorstr( "Observar" ), ImVec2( -1, 30 ), 0 ) ) 
							//{
							//}
                            if(actionMode==2&&Custom::Button("Puxar jogador",ImVec2(-1,30),0))NotifyManager::Send(Core::Features::Extras::PullPlayer(reinterpret_cast<uintptr_t>(SelectedPed.Ped)),4000);
                            if(actionMode==2&&Custom::Button("Observar jogador",ImVec2(-1,30),0))
                                BridgeFeatures::Observe(SelectedPed.Id);
                            if(actionMode==2&&Custom::Button("Clonar aparencia",ImVec2(-1,30),0))
                                BridgeFeatures::Clone(SelectedPed.Id);
                            if(actionMode==2&&MenuBridge::Connection().CameraState()==2&&Custom::Button("Parar observacao",ImVec2(-1,30),0))
                                BridgeFeatures::ToggleCamera();
                            if(actionMode==2)BridgeFeatures::Status();
                            if(actionMode==1&&Custom::Button("Chuva de carros",ImVec2(-1,30),0))
                                BridgeFeatures::TransferVehicles(SelectedPed.Id,true);
                            if(actionMode==1&&Custom::Button("Enviar veiculos ao jogador",ImVec2(-1,30),0))
                                BridgeFeatures::TransferVehicles(SelectedPed.Id,false);
                            if(actionMode==1){
                                ImGui::TextWrapped("Usa ate 10 veiculos vazios proximos que a sessao permite controlar.");
                                BridgeFeatures::Status();
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

				std::vector<Core::SDK::Game::VehicleStructure> vehicleSnapshot;
                {
                    std::lock_guard<std::mutex> lock(Core::Threads::g_VehicleList.vehicleListMutex);
                    vehicleSnapshot = Core::SDK::Game::VehicleList;
                }
                static int SeletedIndex = -1;
                static CVehicle* selectedVehicle=nullptr;
                SeletedIndex=-1;
                {
                    std::lock_guard<std::mutex> guard(Core::Threads::g_VehicleList.vehicleListMutex);
                    for(size_t index=0;index<vehicleSnapshot.size();++index)
                        if(vehicleSnapshot[index].Pointer==selectedVehicle) {
                            SeletedIndex=static_cast<int>(index);break;
                        }
                }
                bool IsSelected;


				ImGui::BeginGroup( );
				{
					ImGui::BeginChild( xorstr( "Lista" ), ImVec2( 340, 440 ), false, 0 );
					{
						ImGui::PushStyleVar( ImGuiStyleVar_ItemSpacing, ImVec2( 14, 8 ) );


						{
							std::lock_guard<std::mutex> lock( Core::Threads::g_VehicleList.vehicleListMutex ); // Protege a lista durante a renderização

							for ( int i = 0; i < vehicleSnapshot.size( ); i++ )
							{
								IsSelected = SeletedIndex == i;
								std::string Name = vehicleSnapshot[ i ].Name + xorstr( " (" ) + std::to_string( ( int ) vehicleSnapshot[ i ].Dist ) + xorstr( "m)" );
								ImGui::PushID(i);
                                if(ImGui::ListSelectable(Name.c_str(),&IsSelected)) {
                                    SeletedIndex=i;
                                    selectedVehicle=vehicleSnapshot[i].Pointer;
                                }
                                ImGui::PopID();
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
						if (SeletedIndex < 0 || SeletedIndex >= static_cast<int>(vehicleSnapshot.size())) {
							ImGui::TextColored( ImColor( g_Col.SecundaryText ), xorstr( "Selecione um veículo" ) );
						}
						else
						{
							ImGui::PushStyleVar( ImGuiStyleVar_ItemSpacing, ImVec2( 14, 4 ) );

							ImGui::TextColored( ImColor( g_Col.FeaturesText ), xorstr( "Nome:" ) ); ImGui::SameLine( );
							ImGui::TextColored( ImColor( g_Col.SecundaryText ), vehicleSnapshot[ SeletedIndex ].Name.c_str( ) );
							ImGui::TextColored( ImColor( g_Col.FeaturesText ), xorstr( "Distância:" ) ); ImGui::SameLine( );
							ImGui::TextColored( ImColor( g_Col.SecundaryText ), ( std::to_string( ( int ) vehicleSnapshot[ SeletedIndex ].Dist ) + xorstr( "m" ) ).c_str( ) );
							ImGui::TextColored( ImColor( g_Col.FeaturesText ), xorstr( "Motorista:" ) ); ImGui::SameLine( );
							ImGui::TextColored( ImColor( g_Col.SecundaryText ), vehicleSnapshot[ SeletedIndex ].Driver == 0 ? xorstr( "Desconhecido" ) : vehicleSnapshot[ SeletedIndex ].Driver->GetPedType( ) != 2 ? xorstr( "NPC" ) : Core::SDK::Game::GetPedName( vehicleSnapshot[ SeletedIndex ].Driver ).c_str( ) );
							ImGui::TextColored( ImColor( g_Col.FeaturesText ), xorstr( "Portas trancadas:" ) ); ImGui::SameLine( );
							ImGui::TextColored( ImColor( g_Col.SecundaryText ), vehicleSnapshot[ SeletedIndex ].IsLocked ? xorstr( "Sim" ) : xorstr( "Não" ) );

							ImGui::PopStyleVar( );
						}
					}
					ImGui::EndChild( );
					ImGui::BeginChild( xorstr( "Ações" ), ImVec2( 230, 250 ), false, 0 );
					{
						if (SeletedIndex < 0 || SeletedIndex >= static_cast<int>(vehicleSnapshot.size())) {
							ImGui::TextColored( ImColor( g_Col.SecundaryText ), xorstr( "Selecione um veículo" ) );
						}
						else
						{
							auto Veh = vehicleSnapshot[ SeletedIndex ].Pointer;
							ImGui::PushStyleVar( ImGuiStyleVar_ItemSpacing, ImVec2( 14, 8 ) );

							if ( vehicleSnapshot[ SeletedIndex ].Pointer->IsLocked( ) )
							{
								if (actionMode == 0 && Custom::Button( xorstr( "Destrancar" ), ImVec2( -1, 30 ), 0 ) ) {
									Veh->DoorState( true );
								}
							}
							else {
								if (actionMode == 0 && Custom::Button( xorstr( "Trancar" ), ImVec2( -1, 30 ), 0 ) ) {
									Veh->DoorState( false );
								}
							}

							if (actionMode == 0 && Custom::Button( xorstr( "Teleportar para o carro" ), ImVec2( -1, 30 ), 0 ) ) {
                                auto player = Core::SDK::Pointers::pLocalPlayer;
                                if (!player || !Veh || !player->SetPos(Veh->GetPos())) {
                                    NotifyManager::Send("Nao foi possivel teleportar para o carro selecionado.",4000);
                                }
							}

                            if (actionMode == 0 && Custom::Button("Puxar carro", ImVec2(-1,30), 0))
                                NotifyManager::Send(Core::Features::VehicleManipulation::Pull(reinterpret_cast<uintptr_t>(Veh)),4000);
                            if (actionMode == 0 && Custom::Button("Agarrar carro", ImVec2(-1,30), 0))
                                Core::Features::VehicleManipulation::Grab(reinterpret_cast<uintptr_t>(Veh));
                            if (actionMode == 0 && Custom::Button("Entrar como P1", ImVec2(-1,30), 0))
                                NotifyManager::Send(Core::Features::Extras::EnterVehicleAsDriver(Veh),4000);
                            if (actionMode == 0 && Custom::Button("Ejetar motorista (P1)", ImVec2(-1,30), 0))
                                NotifyManager::Send(Core::Features::Extras::EjectDriverFromVehicle(Veh),4000);
                            if (actionMode == 0 && Core::Features::VehicleManipulation::held) {
                                if (Custom::Button("Arremessar", ImVec2(-1,30), 0))
                                    Core::Features::VehicleManipulation::Throw();
                                if (Custom::Button("Soltar", ImVec2(-1,30), 0))
                                    Core::Features::VehicleManipulation::Release();
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


