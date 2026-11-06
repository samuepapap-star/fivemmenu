#pragma once
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

		if ( Custom::SubTab( xorstr( "Principal" ), 0 == iSubTabCount ) ) {
			iSubTabCount = 0;
		}
		ImGui::SameLine( );
		if ( Custom::SubTab( xorstr( "Cores" ), 1 == iSubTabCount ) ) {
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
			case 0: //Settings
				ImGui::BeginGroup( );
				{
					ImGui::BeginChild( xorstr( "Geral" ), ImVec2( 340, 440 ), false, 0 );
					{
						Custom::CheckBox( xorstr( "VSync" ), &g_Config.General->VSync );
                        Custom::CheckBox("Painel de teclas", &g_Config.General->BindManager);
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


						static int KeyMode = 1;
						ImGui::Keybind( xorstr( "Tecla do menu" ), &g_Config.General->MenuKey, &KeyMode );

						if ( Custom::Button( xorstr( "Encerrar" ), ImVec2( -1, 30 ), 0 ) ) 
						{
                            g_Config.Player->NoCollisionEnabled=false;
                            Core::Features::CarreraAdditions::TickCollision(true);
							exit( 0 );
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
			case 1: //Colors
				ImGui::BeginGroup( );
				{
					ImGui::BeginChild( xorstr( "Cores" ), ImVec2( g_MenuInfo.MenuSize.x - ( 184 + 24 ), 300 ), false, 0 );
					{
                        ImGui::TextUnformatted("Aparência do menu");
                        if (ImGui::ColorEdit3("Cor de destaque", &g_Col.Base.x, ImGuiColorEditFlags_NoInputs))
                            g_Config.SetMenuAccent(g_Col.Base.x,g_Col.Base.y,g_Col.Base.z);
                        if (ImGui::Button("Restaurar padrão"))
                            g_Config.SetMenuAccent(138.f/255.f,43.f/255.f,226.f/255.f);
                        ImGui::TextWrapped("Salve a configuração para manter a cor escolhida.");
                        ImGui::Separator();
						static float AimbotFovCol[ 4 ] = { g_Config.Aimbot->FovColor.Value.x, g_Config.Aimbot->FovColor.Value.y, g_Config.Aimbot->FovColor.Value.z,g_Config.Aimbot->FovColor.Value.w };
						if ( ImGui::ColorEdit4( xorstr( "FOV da mira assistida" ), AimbotFovCol, ImGuiColorEditFlags_AlphaBar ) )
						{
							g_Config.Aimbot->FovColor.Value.x = AimbotFovCol[ 0 ];
							g_Config.Aimbot->FovColor.Value.y = AimbotFovCol[ 1 ];
							g_Config.Aimbot->FovColor.Value.z = AimbotFovCol[ 2 ];
							g_Config.Aimbot->FovColor.Value.w = AimbotFovCol[ 3 ];
						}


						static float TriggerFovCol[ 4 ] = { g_Config.TriggerBot->FovColor.Value.x, g_Config.TriggerBot->FovColor.Value.y, g_Config.TriggerBot->FovColor.Value.z,g_Config.TriggerBot->FovColor.Value.w };
						if ( ImGui::ColorEdit4( xorstr( "FOV do tiro automático" ), TriggerFovCol, ImGuiColorEditFlags_AlphaBar ) )
						{
							g_Config.TriggerBot->FovColor.Value.x = TriggerFovCol[ 0 ];
							g_Config.TriggerBot->FovColor.Value.y = TriggerFovCol[ 1 ];
							g_Config.TriggerBot->FovColor.Value.z = TriggerFovCol[ 2 ];
							g_Config.TriggerBot->FovColor.Value.w = TriggerFovCol[ 3 ];
						}

						static float SilentAimFovCol[ 4 ] = { g_Config.SilentAim->FovColor.Value.x, g_Config.SilentAim->FovColor.Value.y, g_Config.SilentAim->FovColor.Value.z,g_Config.SilentAim->FovColor.Value.w };
						if ( ImGui::ColorEdit4( xorstr( "FOV da mira silenciosa" ), SilentAimFovCol, ImGuiColorEditFlags_AlphaBar ) )
						{
							g_Config.SilentAim->FovColor.Value.x = SilentAimFovCol[ 0 ];
							g_Config.SilentAim->FovColor.Value.y = SilentAimFovCol[ 1 ];
							g_Config.SilentAim->FovColor.Value.z = SilentAimFovCol[ 2 ];
							g_Config.SilentAim->FovColor.Value.w = SilentAimFovCol[ 3 ];
						}

						static float box_col[ 4 ] = { g_Config.ESP->BoxCol.Value.x, g_Config.ESP->BoxCol.Value.y, g_Config.ESP->BoxCol.Value.z,g_Config.ESP->BoxCol.Value.w };
						if ( ImGui::ColorEdit4( xorstr( "Caixa" ), box_col, ImGuiColorEditFlags_AlphaBar ) )
						{
							g_Config.ESP->BoxCol.Value.x = box_col[ 0 ];
							g_Config.ESP->BoxCol.Value.y = box_col[ 1 ];
							g_Config.ESP->BoxCol.Value.z = box_col[ 2 ];
							g_Config.ESP->BoxCol.Value.w = box_col[ 3 ];
						}

						static float filled_box_col[ 4 ] = { g_Config.ESP->FilledBoxCol.Value.x, g_Config.ESP->FilledBoxCol.Value.y, g_Config.ESP->FilledBoxCol.Value.z,g_Config.ESP->FilledBoxCol.Value.w };
						if ( ImGui::ColorEdit4( xorstr( "Caixa preenchida" ), filled_box_col, ImGuiColorEditFlags_AlphaBar ) )
						{
							g_Config.ESP->FilledBoxCol.Value.x = filled_box_col[ 0 ];
							g_Config.ESP->FilledBoxCol.Value.y = filled_box_col[ 1 ];
							g_Config.ESP->FilledBoxCol.Value.z = filled_box_col[ 2 ];
							g_Config.ESP->FilledBoxCol.Value.w = filled_box_col[ 3 ];
						}


						static float skeleton_col[ 4 ] = { g_Config.ESP->SkeletonCol.Value.x, g_Config.ESP->SkeletonCol.Value.y, g_Config.ESP->SkeletonCol.Value.z,g_Config.ESP->SkeletonCol.Value.w };
						if ( ImGui::ColorEdit4( xorstr( "Esqueleto" ), skeleton_col, ImGuiColorEditFlags_AlphaBar ) )
						{
							g_Config.ESP->SkeletonCol.Value.x = skeleton_col[ 0 ];
							g_Config.ESP->SkeletonCol.Value.y = skeleton_col[ 1 ];
							g_Config.ESP->SkeletonCol.Value.z = skeleton_col[ 2 ];
							g_Config.ESP->SkeletonCol.Value.w = skeleton_col[ 3 ];
						}

						static float lines_col[ 4 ] = { g_Config.ESP->SnapLinesCol.Value.x, g_Config.ESP->SnapLinesCol.Value.y, g_Config.ESP->SnapLinesCol.Value.z,g_Config.ESP->SnapLinesCol.Value.w };
						if ( ImGui::ColorEdit4( xorstr( "Linhas" ), lines_col, ImGuiColorEditFlags_AlphaBar ) )
						{
							g_Config.ESP->SnapLinesCol.Value.x = lines_col[ 0 ];
							g_Config.ESP->SnapLinesCol.Value.y = lines_col[ 1 ];
							g_Config.ESP->SnapLinesCol.Value.z = lines_col[ 2 ];
							g_Config.ESP->SnapLinesCol.Value.w = lines_col[ 3 ];
						}

						static float names_col[ 4 ] = { g_Config.ESP->UserNamesCol.Value.x, g_Config.ESP->UserNamesCol.Value.y, g_Config.ESP->UserNamesCol.Value.z,g_Config.ESP->UserNamesCol.Value.w };
						if ( ImGui::ColorEdit4( xorstr( "Nomes" ), names_col, ImGuiColorEditFlags_AlphaBar ) )
						{
							g_Config.ESP->UserNamesCol.Value.x = names_col[ 0 ];
							g_Config.ESP->UserNamesCol.Value.y = names_col[ 1 ];
							g_Config.ESP->UserNamesCol.Value.z = names_col[ 2 ];
							g_Config.ESP->UserNamesCol.Value.w = names_col[ 3 ];
						}

						static float weapon_names_col[ 4 ] = { g_Config.ESP->WeaponNameCol.Value.x, g_Config.ESP->WeaponNameCol.Value.y, g_Config.ESP->WeaponNameCol.Value.z,g_Config.ESP->WeaponNameCol.Value.w };
						if ( ImGui::ColorEdit4( xorstr( "Nomes das armas" ), weapon_names_col, ImGuiColorEditFlags_AlphaBar ) )
						{
							g_Config.ESP->WeaponNameCol.Value.x = weapon_names_col[ 0 ];
							g_Config.ESP->WeaponNameCol.Value.y = weapon_names_col[ 1 ];
							g_Config.ESP->WeaponNameCol.Value.z = weapon_names_col[ 2 ];
							g_Config.ESP->WeaponNameCol.Value.w = weapon_names_col[ 3 ];
						}

						static float dist_col[ 4 ] = { g_Config.ESP->DistanceCol.Value.x, g_Config.ESP->DistanceCol.Value.y, g_Config.ESP->DistanceCol.Value.z,g_Config.ESP->DistanceCol.Value.w };
						if ( ImGui::ColorEdit4( xorstr( "Distância" ), dist_col, ImGuiColorEditFlags_AlphaBar ) )
						{
							g_Config.ESP->DistanceCol.Value.x = dist_col[ 0 ];
							g_Config.ESP->DistanceCol.Value.y = dist_col[ 1 ];
							g_Config.ESP->DistanceCol.Value.z = dist_col[ 2 ];
							g_Config.ESP->DistanceCol.Value.w = dist_col[ 3 ];
						}

					}
					ImGui::EndChild( );
				}
				ImGui::EndGroup( );
				ImGui::SameLine( );
				ImGui::BeginGroup( );
				{
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