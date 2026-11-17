#pragma once
#include <Includes/includes.hpp>
#include <windows.h>
#include <iostream>
#include <thread>
#include <Includes/CustomWidgets/Preview.hpp>

#include <Core/Core.hpp>
#include <Gui/AdminList.hpp>
#include <Core/ExtensionSettings.hpp>
#include <Bridge/Features.hpp>

using namespace std;

namespace Visuals {

	static int iSubTabCount = 0;
	static float SubTabAlpha = 0.f;
	static int iSubTab = 0;

	void Render(int section = -1) {


		//ImGui::PushStyleVar( ImGuiStyleVar_::ImGuiStyleVar_ItemSpacing, ImVec2( 6, 0 ) );

        if (section >= 0) {
            iSubTabCount = iSubTab = section;
            SubTabAlpha = 1.f;
        } else {
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


        }
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
                        if (Custom::CheckBox("ESP Name", &g_Config.ESP->UserNames) && g_Config.ESP->UserNames)
                            g_Config.ESP->Enabled = true;
                        if (ImGui::IsItemHovered())
                            ImGui::SetTooltip("Consulta o nickname pelo cliente FiveM, sem depender da lista publica. Requer a ponte atualizada. Se o cliente nao receber o nick, mostra o ID.");
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

						Custom::CheckBox("Caixa 3D", &g_Config.ESP->Box3D);
                        Custom::CheckBox("Caixa de cantos", &g_Config.ESP->CornerBox);
                        Custom::CheckBox("ESP estático (limites do modelo)", &g_Config.ESP->StaticESP);
                        ImGui::SliderInt("Largura da caixa", &g_Config.ESP->BoxSize, 0, 30);
                        ImGui::SliderFloat("Opacidade do preenchimento", &g_Config.ESP->FilledBoxCol.Value.w, 0.f, 1.f);
                        Custom::CheckBox("Mostrar prévia", &g_Config.ESP->PreviewEnabled);
                        ImGui::ColorEdit4("Cor da caixa", &g_Config.ESP->BoxCol.Value.x);
                        ImGui::ColorEdit4("Cor dos cantos", &g_Config.ESP->CornerColor.Value.x);
                        if (ImGui::IsItemHovered())
                            ImGui::SetTooltip("Esta cor aparece quando Caixa de cantos esta ativada.");
                        ImGui::ColorEdit4("Cor do preenchimento", &g_Config.ESP->FilledBoxCol.Value.x);
                        ImGui::ColorEdit4("Cor das linhas", &g_Config.ESP->SnapLinesCol.Value.x);
                        ImGui::ColorEdit4("Cor dos nomes", &g_Config.ESP->UserNamesCol.Value.x);
                        ImGui::ColorEdit4("Cor das armas", &g_Config.ESP->WeaponNameCol.Value.x);
                        ImGui::ColorEdit4("Cor da distância", &g_Config.ESP->DistanceCol.Value.x);
                        ImGui::ColorEdit4("Cor do esqueleto", &g_Config.ESP->SkeletonCol.Value.x);
						Custom::CheckBox( xorstr( "Nomes das armas" ), &g_Config.ESP->WeaponName );
						Custom::CheckBox( xorstr( "Distância" ), &g_Config.ESP->DistanceFromMe );
                        Custom::CheckBox("VIP / Rank", &g_Config.ESP->ShowVipStatus);
                        Custom::CheckBox("Horas jogadas", &g_Config.ESP->ShowPlayTime);
                        Custom::CheckBox("Job / Gang", &g_Config.ESP->ShowJobName);
                        Custom::CheckBox("Tag AFK", &g_Config.ESP->ShowAfkTag);
                        // Debug: mostra chaves brutas do /players.json do servidor
                        {
                            static bool showDebugKeys = false;
                            ImGui::SameLine(); ImGui::SetCursorPosX(ImGui::GetCursorPosX() + 4);
                            if (ImGui::SmallButton(showDebugKeys ? "X##dbg" : "?##dbg"))
                                showDebugKeys = !showDebugKeys;
                            if (ImGui::IsItemHovered()) ImGui::SetTooltip("Mostrar chaves JSON do servidor (debug)");
                            if (showDebugKeys) {
                                const auto keys = Core::Threads::g_UpdateNames.GetDebugKeys();
                                ImGui::PushStyleColor(ImGuiCol_ChildBg, ImVec4(0.1f,0.1f,0.1f,0.9f));
                                ImGui::BeginChild("##dbgkeys", ImVec2(310, 68), true, 0);
                                ImGui::PushTextWrapPos(300);
                                ImGui::TextUnformatted(keys.empty() ? "Aguardando dados..." : keys.c_str());
                                ImGui::PopTextWrapPos();
                                ImGui::EndChild();
                                ImGui::PopStyleColor();
                            }
                        }

						Custom::CheckBox( xorstr( "Esqueleto" ), &g_Config.ESP->Skeleton );

						Custom::CheckBox( xorstr( "Círculo na cabeça" ), &g_Config.ESP->HeadCircle );
						Custom::CheckBox( xorstr( "Barra de vida" ), &g_Config.ESP->HealthBar );
                        if (g_Config.ESP->HealthBar) {
                            ImGui::SliderInt("Espessura vida", &g_Config.ESP->HealthBarWidth, 1, 6, "%d px");
                            static const char* barPositions[] = { "Esquerda", "Direita", "Cima", "Baixo" };
                            ImGui::Combo("Posição vida##hp", &g_Config.ESP->HealthBarState, barPositions, IM_ARRAYSIZE(barPositions));
                        }
						Custom::CheckBox( xorstr( "Barra de colete" ), &g_Config.ESP->ArmorBar );
                        if (g_Config.ESP->ArmorBar) {
                            ImGui::SliderInt("Espessura colete", &g_Config.ESP->ArmorBarWidth, 1, 6, "%d px");
                            static const char* barPositions2[] = { "Esquerda", "Direita", "Cima", "Baixo" };
                            ImGui::Combo("Posição colete##arm", &g_Config.ESP->ArmorBarState, barPositions2, IM_ARRAYSIZE(barPositions2));
                        }
						Custom::CheckBox( xorstr( "Linhas" ), &g_Config.ESP->SnapLines );
                        if (g_Config.ESP->SnapLines) {
                            static const char* snapFromOpts[] = { "De baixo", "De cima" };
                            ImGui::Combo("Âncora das linhas", &g_Config.ESP->SnapLinesFrom, snapFromOpts, IM_ARRAYSIZE(snapFromOpts));
                        }
						Custom::CheckBox( xorstr( "Somente visíveis" ), &g_Config.ESP->HighlightVisible );
						Custom::CheckBoxCfg( xorstr( "Marcar amigo" ), &g_Config.ESP->FriendsMarker, [ ]
							{
								static int KeyMode = 1;
								ImGui::Keybind( xorstr( "Tecla" ), &g_Config.ESP->FriendsMarkerBind, &KeyMode );
							}
						);
						Custom::CheckBox( xorstr( "Mostrar meu personagem" ), &g_Config.ESP->ShowLocalPlayer );
						Custom::CheckBox( xorstr( "Ignorar mortos" ), &g_Config.ESP->IgnoreDead );
                        Custom::CheckBox("ESP Admin (noclip + invisíveis)", &g_Config.ESP->AdminEsp);
                        if (ImGui::IsItemHovered())
                            ImGui::SetTooltip("Destaca com nome em RGB jogadores com indícios de noclip (3s) ou invisibilidade. Não confirma cargo de administrador.");
                        Custom::CheckBox("ADM list (possíveis invisíveis)", &g_Config.ESP->AdminList);
                        if(g_Config.ESP->AdminList && ImGui::TreeNode("Ver ADM list no menu")) {
                            AdminList::RenderEntries();
                            ImGui::TreePop();
                        }
                        if (g_Config.ESP->HeadCircle) ImGui::SliderFloat("Raio da cabeça", &g_Config.ESP->HeadSize, 1.f, 20.f);
						Custom::CheckBox( xorstr( "Ignorar NPCs" ), &g_Config.ESP->IgnoreNPCs );

                        ImGui::Separator();
                        ImGui::TextDisabled("Aparencia do ESP");
                        Custom::CheckBox("Brilho ao redor do jogador", &ExtraSettings::PlayerGlow);
                        Custom::CheckBox("Icone da arma", &ExtraSettings::WeaponIcon);
                        Custom::CheckBox("Chapeu conico (China Hat)", &ExtraSettings::ChinaHat);
                        if (ExtraSettings::ChinaHat) ImGui::SliderFloat("Tamanho do chapeu", &ExtraSettings::HatSize, 0.1f, 1.f, "%.2f");
                        Custom::CheckBox("Aviso de possiveis invisiveis", &ExtraSettings::InvisibleWarning);
                        ImGui::SliderFloat("Tamanho dos textos ESP", &ExtraSettings::TextSize, 8.f, 24.f, "%.0f px");
                        ImGui::SliderFloat("Espessura do esqueleto", &ExtraSettings::SkeletonWidth, 0.5f, 5.f, "%.1f");
                        ImGui::SliderFloat("Espessura das caixas", &ExtraSettings::BoxWidth, 0.5f, 4.f, "%.1f");
                        ImGui::Combo("Fonte do ESP", &ExtraSettings::Font, "Atual\0Fonte do menu\0Compacta\0");

						ImGui::SliderInt( xorstr( "Distância máxima" ), &g_Config.ESP->MaxDistance, 0, g_Config.ESP->DistanceLimit, xorstr( "%dm" ), 0 );
					}
					ImGui::EndChild( );
				}
				ImGui::EndGroup( );
				ImGui::SameLine( );
				ImGui::BeginGroup( );
				{
					ImGui::BeginChild( xorstr( "Prévia" ), ImVec2( 340, 440 ), false, 0 );
					{
						if (g_Config.ESP->PreviewEnabled) {
                            Custom::g_EspPreview.DragDropHandler();
                            Custom::g_EspPreview.Draw();
                        }
                        // ---- Spoof de dados de servidor ----
                        ImGui::Separator();
                        ImGui::TextColored(ImVec4(0.6f,0.9f,1.f,1.f), "Spoof de dados de servidor");
                        ImGui::Spacing();
                        Custom::CheckBox("Ativar spoof##esp", &g_Config.ServerSpoof->Enabled);
                        if (g_Config.ServerSpoof->Enabled) {
                            ImGui::Spacing();
                            // ── VIP selector ──────────────────────────────────
                            ImGui::Text("VIP Name:");
                            // Monta lista: VIPs comuns + VIPs descobertos no servidor
                            static const char* commonVips[] = {
                                "SEM VIP","VIP BRONZE","VIP PRATA","VIP OURO",
                                "VIP DIAMANTE","VIP PLATINA","VIP MASTER",
                                "VIP ELITE","VIP PREMIUM","VIP PLUS","VIP MAX",
                                "VIP VITALICIO","VIP BASICO","VIP INICIANTE",
                                "VIP LENDA","VIP STAFF","ADM","ADMIN","MODERADOR",
                                "FUNDADOR","DONO","CO-DONO"
                            };
                            constexpr int nCommon = (int)(sizeof(commonVips)/sizeof(commonVips[0]));
                            const auto discovVips = Core::Threads::g_UpdateNames.GetDiscoveredVips();
                            // Combo usando o campo VipName como preview
                            ImGui::SetNextItemWidth(280.f);
                            if (ImGui::BeginCombo("##vipcombo", g_Config.ServerSpoof->VipName[0] ? g_Config.ServerSpoof->VipName : "Selecionar...", nCommon)) {
                                // VIPs descobertos no servidor (topo da lista)
                                if (!discovVips.empty()) {
                                    ImGui::TextDisabled("-- Servidor atual --");
                                    for (const auto& vn : discovVips) {
                                        const bool sel = (vn == g_Config.ServerSpoof->VipName);
                                        if (ImGui::Selectable(vn.c_str(), sel))
                                            strncpy_s(g_Config.ServerSpoof->VipName, vn.c_str(), 63);
                                        if (sel) ImGui::SetItemDefaultFocus();
                                    }
                                    ImGui::Separator();
                                }
                                ImGui::TextDisabled("-- Comuns --");
                                for (int i = 0; i < nCommon; i++) {
                                    const bool sel = (strcmp(g_Config.ServerSpoof->VipName, commonVips[i]) == 0);
                                    if (ImGui::Selectable(commonVips[i], sel))
                                        strncpy_s(g_Config.ServerSpoof->VipName, commonVips[i], 63);
                                    if (sel) ImGui::SetItemDefaultFocus();
                                }
                                ImGui::EndCombo();
                            }
                            // Input customizado logo abaixo
                            ImGui::SetNextItemWidth(280.f);
                            ImGui::InputText("Personalizado##vip2", g_Config.ServerSpoof->VipName, sizeof(g_Config.ServerSpoof->VipName));
                            ImGui::Spacing();
                            // ── Horas jogadas ─────────────────────────────────
                            ImGui::Text("Horas jogadas:");
                            ImGui::SetNextItemWidth(200.f);
                            ImGui::InputInt("##spoofhoras2", &g_Config.ServerSpoof->PlayTime, 1, 100);
                            if (g_Config.ServerSpoof->PlayTime < 0) g_Config.ServerSpoof->PlayTime = 0;
                            ImGui::Spacing();
                            // ── Job / Gang ────────────────────────────────────
                            ImGui::Text("Job / Gang:");
                            ImGui::SetNextItemWidth(280.f);
                            ImGui::InputText("##spoofjob2", g_Config.ServerSpoof->JobName, sizeof(g_Config.ServerSpoof->JobName));
                            ImGui::Spacing();
                            Custom::CheckBox("Remover modo novato##esp", &g_Config.ServerSpoof->RemoveNovato);
                            ImGui::Spacing();
                            ImGui::Separator();
                            ImGui::Spacing();
                            // ── Injeção de state bag na rede ───────────────
                            static std::string injectStatus;
                            static bool injectBusy=false;
                            if(injectBusy&&!MenuBridge::Connection().Busy()){
                                injectStatus=BridgeFeatures::localStatus.empty()?"Injetado":BridgeFeatures::localStatus;
                                injectBusy=false;
                            }
                            ImGui::PushStyleColor(ImGuiCol_Button,ImVec4(0.15f,0.55f,0.25f,1.0f));
                            ImGui::PushStyleColor(ImGuiCol_ButtonHovered,ImVec4(0.20f,0.70f,0.35f,1.0f));
                            ImGui::PushStyleColor(ImGuiCol_ButtonActive,ImVec4(0.10f,0.40f,0.18f,1.0f));
                            if(ImGui::Button("Injetar na Rede##sbinject",ImVec2(-1,32))){
                                injectStatus.clear();
                                BridgeFeatures::InjectStateBag(
                                    std::string(g_Config.ServerSpoof->VipName),
                                    g_Config.ServerSpoof->PlayTime,
                                    std::string(g_Config.ServerSpoof->JobName)
                                );
                                injectBusy=true;
                                injectStatus="Enviando...";
                            }
                            ImGui::PopStyleColor(3);
                            if(!injectStatus.empty()){
                                ImGui::PushStyleColor(ImGuiCol_Text,ImVec4(0.6f,1.0f,0.6f,1.0f));
                                ImGui::TextWrapped("%s",injectStatus.c_str());
                                ImGui::PopStyleColor();
                            }
                        }
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
                        Custom::CheckBox("Marcador nos veiculos",&ExtraSettings::VehicleMarker);
                        Custom::CheckBox("Ignorar veiculos ocupados",&ExtraSettings::IgnoreOccupied);
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



