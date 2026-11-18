#pragma once
#include <Bridge/Features.hpp>
#include <Core/Features/ExtraFeatures.hpp>
#include <Core/Features/SelfGodMode.hpp>
#include <Includes/includes.hpp>
#include <windows.h>
#include <iostream>
#include <thread>
#include <cmath>

#include <Core/Core.hpp>
#include <Core/Features/VehicleManipulation.hpp>
#include <Core/Features/CarreraAdditions.hpp>
#include <Core/Features/NoclipVisibility.hpp>
#include <Core/Features/Exploits/Exploits.hpp>
#include <Core/Features/Exploits/HandlingEditor.hpp>
#include <Core/Features/Exploits/GiveWeapon.hpp>

using namespace std;

namespace Local {

	static int iSubTabCount = 0;
	static float SubTabAlpha = 0.f;
	static int iSubTab = 0;

	void Render(int section = -1)
	{
        if (section >= 0) {
            iSubTabCount = iSubTab = section;
            SubTabAlpha = 1.f;
        } else {
		if ( Custom::SubTab( xorstr( "Principal" ), 0 == iSubTabCount ) ) {
			iSubTabCount = 0;
		}
		ImGui::SameLine( );
		if ( Custom::SubTab( xorstr( "Veículos" ), 1 == iSubTabCount ) ) {
			iSubTabCount = 1;
		}
		ImGui::SameLine( );
		if ( Custom::SubTab( xorstr( "Teleportes" ), 2 == iSubTabCount ) ) {
			iSubTabCount = 2;
		}
		ImGui::SameLine( );
		if ( Custom::SubTab( xorstr( "Armas" ), 3 == iSubTabCount ) ) {
			iSubTabCount = 3;
		}

		SubTabAlpha = ImClamp( SubTabAlpha + ( 5.f * ImGui::GetIO( ).DeltaTime * ( iSubTabCount == iSubTab ? 1.f : -1.f ) ), 0.f, 1.f );

		if ( SubTabAlpha == 0.f ) iSubTab = iSubTabCount;

        }
		ImGui::PushStyleVar( ImGuiStyleVar_Alpha, SubTabAlpha * ImGui::GetStyle( ).Alpha );

		ImGui::SetCursorPos( ImVec2( 184, 136 ) );
		ImGui::BeginGroup( );
		{
			switch ( iSubTab )
			{
			case 0: //Main
				ImGui::BeginGroup( );
				{
					ImGui::BeginChild( xorstr( "Geral" ), ImVec2( 340, 440 ), false, 0 );
					{

                        if (Custom::Button("Restaurar vida e colete", ImVec2(-1, 33), 0)) {
                            auto ped = Core::SDK::Pointers::pLocalPlayer;
                            if (ped) {
                                // Se m_MaxHealth retornar valor invalido (offset errado para build atual),
                                // usa 200.f como fallback (valor padrao do jogador no GTA5).
                                const float rawMax = ped->GetMaxHealth();
                                const float hp = (std::isfinite(rawMax) && rawMax > 50.f && rawMax <= 10000.f) ? rawMax : 400.f;
                                ped->SetHealth(hp);
                                ped->SetArmor(100.f);
                            } else {
                                NotifyManager::Send("Personagem indisponivel para restaurar vida e colete.", 4000);
                            }
                        }
                        if (ImGui::IsItemHovered())
                            ImGui::SetTooltip("Restaura a vida maxima e 100 de colete. Nao e uma funcao de reviver.");

                        Custom::CheckBox("Velocidade de natação", &Core::Features::Exploits::swimEnabled);
                        if (Core::Features::Exploits::swimEnabled)
                            ImGui::SliderInt("Velocidade ao nadar", &Core::Features::Exploits::swimSpeed, 1, 5);
                        Custom::CheckBox("Regenerar vida enquanto vivo", &ExtraSettings::RegenHealth);
                        Custom::CheckBox("Regenerar colete", &ExtraSettings::RegenArmor);
                        if (Custom::Button("Zerar vida do personagem", ImVec2(-1,30),0)) {
                            auto ped=Core::Features::Extras::Player();if(ped)ped->SetHealth(0.f);
                        }
                        static float extraMaximumHealth=200.f;
                        ImGui::SliderFloat("Vida maxima",&extraMaximumHealth,100.f,1000.f,"%.0f");
                        if(Custom::Button("Aplicar vida maxima",ImVec2(-1,30),0))
                            Core::Features::Extras::SetMaximumHealth(extraMaximumHealth);


						if ( Custom::CheckBoxCfg( xorstr( "God em si mesmo" ), &g_Config.Player->EnableGodMode, [ & ] ( ) { static int KeyMode = 1; ImGui::Keybind( xorstr( "Tecla" ), &g_Config.Player->GodModeKey, &KeyMode ); }, true, xorstr( "Esta função pode ser detectada pelo anticheat do servidor" ), ICON_FA_TRIANGLE_EXCLAMATION ) )
						{
							Core::Features::SelfGodMode::Tick();
						}

                        ImGui::TextWrapped("%s", Core::Features::SelfGodMode::status.c_str());
                        if (g_Config.Player->EnableGodMode)
                            ImGui::TextDisabled("God nao revive o personagem.");

                        if ( Custom::CheckBox( xorstr( "Semi God Mode" ), &g_Config.Player->SemiGodEnabled ) )
                        {
                            if ( !g_Config.Player->SemiGodEnabled ) {
                                // desativar: restaurar estado normal sem forcar nada
                            }
                        }
                        if (ImGui::IsItemHovered())
                            ImGui::SetTooltip("Regenera vida e colete automaticamente ao cair abaixo de 50%%. Menos detectavel que o God Mode completo.");
                        if (g_Config.Player->SemiGodEnabled)
                            ImGui::TextDisabled("Regen ativo: vida < 50%% restaura para maxima.");

						if ( Custom::CheckBoxCfg( xorstr( "Voo livre" ), &g_Config.Player->NoClipEnabled, [ & ] ( ) { static int KeyMode = 1; ImGui::Keybind( xorstr( "Tecla" ), &g_Config.Player->NoClipKey, &KeyMode ); ImGui::SliderFloat( xorstr( "Velocidade" ), &g_Config.Player->NoClipSpeed, 0.1f, 20.f, xorstr( "%1.2fm/s" ) ); }, true, xorstr( "Esta função pode ser detectada pelo anticheat do servidor" ), ICON_FA_TRIANGLE_EXCLAMATION ) )
						{
							Core::SDK::Pointers::pLocalPlayer->FreezePed( g_Config.Player->NoClipEnabled );
						}

						if ( Custom::CheckBoxCfg( xorstr( "Corrida rápida" ), &g_Config.Player->FastRun, [ & ] ( ) {
							if ( ImGui::SliderFloat( xorstr( "Velocidade" ), &g_Config.Player->RunSpeed, 1.f, 10.f, xorstr( "%1.2fm/s" ) ) )
							{
								if ( g_Config.Player->FastRun )
									Core::SDK::Pointers::pLocalPlayer->SetSpeed( g_Config.Player->RunSpeed );
							}
							} ) )
						{
							if ( !g_Config.Player->FastRun ) {
								Core::SDK::Pointers::pLocalPlayer->SetSpeed( 1.f );
							}
						}

						if ( Custom::CheckBox( xorstr( "Proteção contra tiro na cabeça" ), &g_Config.Player->AntiHSEnabled, true, xorstr( "Esta função pode ser detectada pelo anticheat do servidor" ), ICON_FA_TRIANGLE_EXCLAMATION ) )
						{
							Core::SDK::Pointers::pLocalPlayer->SetConfigFlag( ePedConfigFlag::NoCriticalHits, g_Config.Player->AntiHSEnabled );
						}

						if ( Custom::CheckBox( xorstr( "Diminuir personagem" ), &g_Config.Player->ShrinkEnabled, true, xorstr( "Esta função pode ser detectada pelo anticheat do servidor" ), ICON_FA_TRIANGLE_EXCLAMATION ) )
						{
							Core::SDK::Pointers::pLocalPlayer->SetConfigFlag( ePedConfigFlag::Shrink, g_Config.Player->ShrinkEnabled );
						}

						if ( Custom::CheckBox( xorstr( "Sem colisão" ), &g_Config.Player->NoCollisionEnabled ) )
						{
							Core::Features::CarreraAdditions::TickCollision( true );
						}
                        if(g_Config.Player->NoCollisionEnabled)
                            ImGui::TextWrapped("%s",Core::Features::CarreraAdditions::collisionStatus.c_str());
                        if ( Custom::CheckBox( xorstr( "Evitar quedas" ), &g_Config.Player->NoRagDollEnabled ) )
						{
							Core::SDK::Pointers::pLocalPlayer->NoRagDoll( g_Config.Player->NoRagDollEnabled );
						}

                        Custom::CheckBox( xorstr( "Sem dano de queda" ), &g_Config.Player->NoFallDamageEnabled );
                        if (ImGui::IsItemHovered())
                            ImGui::SetTooltip("Bloqueia velocidade vertical negativa antes do impacto, eliminando dano de queda fatal.");

						if ( Custom::CheckBox( xorstr( "Entrar em veículos" ), &g_Config.Player->StealCarEnabled ) )
						{
							if ( g_Config.Player->StealCarEnabled )
							{
								Core::SDK::Pointers::pLocalPlayer->SetConfigFlag( ePedConfigFlag::NotAllowedToJackAnyPlayers, false );
								Core::SDK::Pointers::pLocalPlayer->SetConfigFlag( ePedConfigFlag::PlayerCanJackFriendlyPlayers, true );
								Core::SDK::Pointers::pLocalPlayer->SetConfigFlag( ePedConfigFlag::WillJackAnyPlayer, true );
							}
							else
							{
								Core::SDK::Pointers::pLocalPlayer->SetConfigFlag( ePedConfigFlag::NotAllowedToJackAnyPlayers, true );
								Core::SDK::Pointers::pLocalPlayer->SetConfigFlag( ePedConfigFlag::PlayerCanJackFriendlyPlayers, false );
								Core::SDK::Pointers::pLocalPlayer->SetConfigFlag( ePedConfigFlag::WillJackAnyPlayer, false );
							}
						}

                        if ( Custom::CheckBox( xorstr( "Invisível" ), &g_Config.Player->InvisibleEnabled ) )
                        {
                            Core::Features::NoclipVisibility::Tick();
                        }
                        if (ImGui::IsItemHovered())
                            ImGui::SetTooltip("Independente do Voo livre: permite andar normalmente. Você aparece semitransparente na própria tela. Visibilidade para outros jogadores não validada.");

						if ( Custom::CheckBox( xorstr( "Rolamento infinito" ), &g_Config.Player->InfiniteCombatRoll ) )
						{
							std::thread InfiniteCombatRoll( [ ] ( ) { Core::SDK::Pointers::pLocalPlayer->SetInfCombatRoll( g_Config.Player->InfiniteCombatRoll ); } );
							InfiniteCombatRoll.detach( );
						}

						if ( Custom::CheckBox( xorstr( "Fôlego infinito" ), &g_Config.Player->InfiniteStamina, true, xorstr( "Esta função pode ser detectada pelo anticheat do servidor" ), ICON_FA_TRIANGLE_EXCLAMATION ) )
						{
							Core::SDK::Pointers::pLocalPlayer->SetInfStamina( g_Config.Player->InfiniteStamina );
						}

						if ( Custom::CheckBox( xorstr( "Forçar roda de armas" ), &g_Config.Player->ForceWeaponWheel ) )
						{
							std::thread ForceWeaponWheel( [ ] ( ) { Core::SDK::Pointers::pLocalPlayer->ForceWeaponWheel( g_Config.Player->ForceWeaponWheel ); } );
							ForceWeaponWheel.detach( );
						}
					}
					ImGui::EndChild( );
				}
				ImGui::EndGroup( );
				ImGui::SameLine( );
				ImGui::BeginGroup( );
				{
					ImGui::BeginChild( xorstr( "Outros" ), ImVec2( 340, 440 ), false, 0 );
					{
                        auto ped = Core::SDK::Pointers::pLocalPlayer;
                        if (ped) {
                            ImGui::Text("Vida: %.0f / %.0f", ped->GetHealth(), ped->GetMaxHealth());
                            ImGui::Text("Colete: %.0f / 100", ped->GetArmor());
                        } else {
                            ImGui::TextDisabled("Personagem indisponivel.");
                        }

                        // ── Nameplate spoofer (ALT nameplate: VIP / Rank / Classe Social / Horas) ──
                        ImGui::Separator();
                        ImGui::TextColored(ImVec4(0.4f,0.9f,1.f,1.f), "Nameplate (ALT)");
                        ImGui::TextDisabled("Altera os dados do nameplate (pressione ALT no servidor).");
                        ImGui::Spacing();

                        // ── Rank select ──────────────────────────────────────
                        ImGui::Spacing();
                        ImGui::Text("Rank:");
                        {
                            static const char* rankOpts[] = {
                                "INICIANTE","CALOURO","NOVATO","BASICO",
                                "INTERMEDIARIO","REGULAR","EXPERIENTE","AVANCADO",
                                "VETERANO","SENIOR","ELITE","MESTRE","LENDA","FUNDADOR"
                            };
                            constexpr int nRank = (int)(sizeof(rankOpts)/sizeof(rankOpts[0]));
                            ImGui::SetNextItemWidth(280.f);
                            const char* rankPreview = g_Config.ServerSpoof->RankName[0] ? g_Config.ServerSpoof->RankName : "Selecionar...";
                            ImGui::PushID("nprank");
                            if (ImGui::BeginCombo("", rankPreview, nRank)) {
                                for (int i = 0; i < nRank; i++) {
                                    const bool sel = (strcmp(g_Config.ServerSpoof->RankName, rankOpts[i]) == 0);
                                    if (ImGui::Selectable(rankOpts[i], sel))
                                        strncpy_s(g_Config.ServerSpoof->RankName, rankOpts[i], 63);
                                    if (sel) ImGui::SetItemDefaultFocus();
                                }
                                ImGui::EndCombo();
                            }
                            ImGui::PopID();
                            ImGui::SetNextItemWidth(280.f);
                            ImGui::InputText("Personalizado##npranktxt", g_Config.ServerSpoof->RankName, sizeof(g_Config.ServerSpoof->RankName));
                        }

                        // ── Classe Social select ─────────────────────────────
                        ImGui::Spacing();
                        ImGui::Text("Classe Social:");
                        {
                            static const char* socialOpts[] = {
                                "SOCIAL 1","SOCIAL 2","SOCIAL 3","SOCIAL 4","SOCIAL 5",
                                "SOCIAL 6","SOCIAL 7","SOCIAL 8","SOCIAL 9","SOCIAL 10"
                            };
                            constexpr int nSocial = (int)(sizeof(socialOpts)/sizeof(socialOpts[0]));
                            ImGui::SetNextItemWidth(280.f);
                            const char* socPreview = g_Config.ServerSpoof->SocialClass[0] ? g_Config.ServerSpoof->SocialClass : "Selecionar...";
                            ImGui::PushID("npsocial");
                            if (ImGui::BeginCombo("", socPreview, nSocial)) {
                                for (int i = 0; i < nSocial; i++) {
                                    const bool sel = (strcmp(g_Config.ServerSpoof->SocialClass, socialOpts[i]) == 0);
                                    if (ImGui::Selectable(socialOpts[i], sel))
                                        strncpy_s(g_Config.ServerSpoof->SocialClass, socialOpts[i], 63);
                                    if (sel) ImGui::SetItemDefaultFocus();
                                }
                                ImGui::EndCombo();
                            }
                            ImGui::PopID();
                            ImGui::SetNextItemWidth(280.f);
                            ImGui::InputText("Personalizado##npsocialtxt", g_Config.ServerSpoof->SocialClass, sizeof(g_Config.ServerSpoof->SocialClass));
                        }

                        ImGui::Spacing();
                        ImGui::Separator();
                        ImGui::Spacing();

                        // ── Continuous injection state ───────────────────────
                        static bool  npContinuous   = false;
                        static float npLastInject   = -999.f;
                        static std::string npStatus;
                        static bool  npBusy         = false;

                        // Poll bridge reply
                        if (npBusy && !MenuBridge::Connection().Busy()) {
                            npStatus = MenuBridge::Connection().Status();
                            npBusy   = false;
                        }

                        // Continuous injection tick (every 1 s)
                        if (npContinuous && !npBusy) {
                            const float now = static_cast<float>(ImGui::GetTime());
                            if (now - npLastInject >= 1.0f) {
                                BridgeFeatures::InjectStateBag(
                                    std::string(g_Config.ServerSpoof->JobName),
                                    std::string(g_Config.ServerSpoof->RankName),
                                    std::string(g_Config.ServerSpoof->SocialClass)
                                );
                                npLastInject = now;
                                npBusy       = true; // captura resposta da bridge
                            }
                        }

                        // ── Continuous toggle button ─────────────────────────
                        if (npContinuous) {
                            ImGui::PushStyleColor(ImGuiCol_Button,        ImVec4(0.6f,0.1f,0.1f,1.f));
                            ImGui::PushStyleColor(ImGuiCol_ButtonHovered, ImVec4(0.8f,0.15f,0.15f,1.f));
                            ImGui::PushStyleColor(ImGuiCol_ButtonActive,  ImVec4(0.4f,0.05f,0.05f,1.f));
                            if (ImGui::Button("Parar Injecao Continua##npcont", ImVec2(-1, 32))) {
                                npContinuous = false;
                                npStatus = "Injecao continua pausada";
                            }
                            ImGui::PopStyleColor(3);
                        } else {
                            ImGui::PushStyleColor(ImGuiCol_Button,        ImVec4(0.1f,0.45f,0.65f,1.f));
                            ImGui::PushStyleColor(ImGuiCol_ButtonHovered, ImVec4(0.15f,0.60f,0.85f,1.f));
                            ImGui::PushStyleColor(ImGuiCol_ButtonActive,  ImVec4(0.07f,0.30f,0.45f,1.f));
                            if (ImGui::Button("Injecao Continua (1s)##npcont", ImVec2(-1, 32))) {
                                npContinuous  = true;
                                npLastInject  = -999.f; // força injeção imediata
                                npStatus      = "Iniciando...";
                            }
                            ImGui::PopStyleColor(3);
                        }

                        // ── Single-shot inject button ────────────────────────
                        ImGui::PushStyleColor(ImGuiCol_Button,        ImVec4(0.15f,0.55f,0.25f,1.f));
                        ImGui::PushStyleColor(ImGuiCol_ButtonHovered, ImVec4(0.20f,0.70f,0.35f,1.f));
                        ImGui::PushStyleColor(ImGuiCol_ButtonActive,  ImVec4(0.10f,0.40f,0.18f,1.f));
                        if (ImGui::Button("Injetar Agora##nponce", ImVec2(-1, 28))) {
                            npStatus = "Enviando...";
                            npBusy   = true;
                            BridgeFeatures::InjectStateBag(
                                std::string(g_Config.ServerSpoof->JobName),
                                std::string(g_Config.ServerSpoof->RankName),
                                std::string(g_Config.ServerSpoof->SocialClass)
                            );
                        }
                        ImGui::PopStyleColor(3);

                        // ── Status line (resposta real da bridge) ────────────
                        if (!npStatus.empty()) {
                            const bool ok  = npStatus.find("injetado")  != std::string::npos
                                          || npStatus.find("Nameplate")  != std::string::npos
                                          || npStatus.find("chaves")     != std::string::npos
                                          || npStatus.find("disparado")  != std::string::npos
                                          || npStatus.find("recursos)")  != std::string::npos
                                          || npStatus.find("candidatos") != std::string::npos;
                            const bool err = npStatus.find("invalido")   != std::string::npos
                                          || npStatus.find("Nada")       != std::string::npos
                                          || npStatus.find("erro")       != std::string::npos
                                          || npStatus.find("falhou")     != std::string::npos
                                          || npStatus.find("Native")     != std::string::npos
                                          || npStatus.find("Falha")      != std::string::npos
                                          || npStatus.find("GetPlayerInfo") != std::string::npos;
                            ImGui::PushStyleColor(ImGuiCol_Text, err
                                ? ImVec4(1.f,0.3f,0.3f,1.f)
                                : ok ? ImVec4(0.4f,1.f,0.4f,1.f)
                                     : ImVec4(1.f,0.85f,0.3f,1.f));
                            ImGui::TextWrapped("%s", npStatus.c_str());
                            ImGui::PopStyleColor();
                        }
                        ImGui::TextDisabled("Escrita local: servidor nao ve nem sobrescreve.");
                        ImGui::TextDisabled("Use injecao continua caso o sync do servidor reponha.");

                        // ── Trigger Evento Local ────────────────────────────
                        ImGui::Separator();
                        ImGui::TextColored(ImVec4(1.f,0.75f,0.3f,1.f), "Trigger Evento Local (avancado)");
                        ImGui::TextDisabled("Dispara o evento de sync do nameplate no servidor.");
                        ImGui::Spacing();

                        // Preset event name candidates — no typing needed
                        static const char* kEventCandidates[] = {
                            "maresia:syncPlayer",
                            "maresia:updateHUD",
                            "maresia:playerLoaded",
                            "maresia:syncHUD",
                            "maresia:syncPlayerData",
                            "maresia:updateNameplate",
                            "maresia:characterLoaded",
                            "esx:playerLoaded",
                        };
                        static constexpr int kNumEvt = 8;
                        static int npEvtIdx = 0;
                        // Keep EventName in sync with combo selection
                        strncpy_s(g_Config.ServerSpoof->EventName,
                                  kEventCandidates[npEvtIdx],
                                  sizeof(g_Config.ServerSpoof->EventName)-1);

                        ImGui::Text("Evento:");
                        ImGui::SetNextItemWidth(-1.f);
                        if (ImGui::BeginCombo("##npevtcombo", kEventCandidates[npEvtIdx], kNumEvt)) {
                            for (int i = 0; i < kNumEvt; i++) {
                                const bool sel = (npEvtIdx == i);
                                if (ImGui::Selectable(kEventCandidates[i], sel))
                                    npEvtIdx = i;
                                if (sel) ImGui::SetItemDefaultFocus();
                            }
                            ImGui::EndCombo();
                        }
                        ImGui::Spacing();

                        // Char ID: auto-detect from bridge, no typing
                        // Parse "auto:HEXID|PlayerName" from npStatus
                        static std::string g_detectedCharId;
                        static std::string g_detectedPlayerName;
                        {
                            const std::string pfx = "auto:";
                            if (npStatus.rfind(pfx, 0) == 0) {
                                // parse auto:HEXID|PlayerName
                                std::string rest = npStatus.substr(pfx.size());
                                auto pipe = rest.find('|');
                                if (pipe != std::string::npos) {
                                    g_detectedCharId    = rest.substr(0, pipe);
                                    g_detectedPlayerName= rest.substr(pipe+1);
                                } else {
                                    g_detectedCharId    = rest;
                                    g_detectedPlayerName.clear();
                                }
                                // copy to config
                                strncpy_s(g_Config.ServerSpoof->CharId,
                                          g_detectedCharId.c_str(),
                                          sizeof(g_Config.ServerSpoof->CharId)-1);
                                npStatus.clear(); // consume
                            }
                        }

                        // Show detected ID or placeholder
                        if (!g_detectedCharId.empty()) {
                            ImGui::TextColored(ImVec4(0.4f,1.f,0.5f,1.f),
                                "Char ID: %s", g_detectedCharId.c_str());
                            if (!g_detectedPlayerName.empty())
                                ImGui::TextDisabled("Nick: %s", g_detectedPlayerName.c_str());
                        } else if (g_Config.ServerSpoof->CharId[0] != '\0') {
                            ImGui::TextColored(ImVec4(1.f,0.85f,0.3f,1.f),
                                "Char ID: %s", g_Config.ServerSpoof->CharId);
                        } else {
                            ImGui::TextDisabled("Char ID: (clique Auto-Detectar)");
                        }
                        ImGui::Spacing();

                        if (ImGui::Button("Auto-Detectar ID##npdetect", ImVec2(-1, 26))) {
                            npStatus = "Detectando player info...";
                            npBusy   = true;
                            BridgeFeatures::GetPlayerInfo();
                        }
                        if (ImGui::Button("Listar Recursos##nplista", ImVec2(-1, 26))) {
                            npStatus = "Listando recursos...";
                            npBusy   = true;
                            BridgeFeatures::ListResources();
                        }
                        ImGui::Spacing();
                        if (ImGui::Button("Trigger Evento##nptrigger", ImVec2(-1, 28))) {
                            npStatus = "Disparando evento...";
                            npBusy   = true;
                            BridgeFeatures::TriggerLocalEvent(
                                std::string(g_Config.ServerSpoof->EventName),
                                std::string(g_Config.ServerSpoof->CharId),
                                std::string(g_Config.ServerSpoof->RankName),
                                std::string(g_Config.ServerSpoof->SocialClass)
                            );
                        }
                        // Testar todos os candidatos em sequencia
                        if (ImGui::Button("Testar Todos Eventos##nptestall", ImVec2(-1, 26))) {
                            // fire each candidate; whichever syncs the nameplate is the right one
                            for (int i = 0; i < kNumEvt; i++) {
                                BridgeFeatures::TriggerLocalEvent(
                                    std::string(kEventCandidates[i]),
                                    std::string(g_Config.ServerSpoof->CharId),
                                    std::string(g_Config.ServerSpoof->RankName),
                                    std::string(g_Config.ServerSpoof->SocialClass)
                                );
                            }
                            npStatus = "Disparados " + std::to_string(kNumEvt) + " candidatos";
                        }
					}
					ImGui::EndChild( );
				}
				ImGui::EndGroup( );
				break;
			case 1: //Vehicles
			{
				bool InVehicle = Core::SDK::Pointers::pLocalPlayer->InVehicle( );

				auto CurrentVehicle = Core::SDK::Pointers::pLocalPlayer->GetLastVehicle( );
				ImGui::BeginGroup( );
				{


					ImGui::BeginChild( xorstr( "Outros" ), ImVec2( 340, 440 ), false, 0 );
					{
						if (Custom::Button("Cor aleatória do veículo", ImVec2(-1, 30), 0))
                            Core::Features::Exploits::PaintVehicle();
                        Custom::CheckBox("Pintura contínua", &Core::Features::Exploits::cyclePaint);
                        ImGui::TextWrapped("%s",Core::Features::Exploits::paintStatus.c_str());
                        if (Custom::CheckBox("Blindagem do veiculo", &BridgeFeatures::vehicleArmor) && BridgeFeatures::vehicleArmor) {
                            MenuBridge::Command connect{};
                            connect.action = MenuBridge::Action::Connect;
                            if (!BridgeFeatures::Submit(connect)) BridgeFeatures::vehicleArmor = false;
                        }
                        if (ImGui::IsItemHovered()) ImGui::SetTooltip("Protege voce e o carro contra tiros, fogo e explosoes. Repara os vidros.");
                        if ( Custom::CheckBox( xorstr( "Invencibilidade" ), &g_Config.Player->VehicleGodMode ) )
						{
							if ( !InVehicle )
							{
								g_Config.Player->VehicleGodMode = false;
								std::thread VehicleAlert( [ ] ( ) {  NotifyManager::Send( xorstr( "Entre em um veículo para usar esta função" ), 4000 );  } );
								VehicleAlert.detach( );
							}
							else {
								//Atualizar Offsets
								CurrentVehicle->SetGodMode( g_Config.Player->VehicleGodMode );
							}
						}
						static bool Locked = CurrentVehicle->IsLocked( );
						if ( Custom::CheckBox( xorstr( "Portas trancadas" ), &Locked ) )
						{
							CurrentVehicle->DoorState( !Locked );
						}

						if ( Custom::CheckBox( xorstr( "Cinto de segurança" ), &g_Config.Player->SeatBelt ) )
						{
							if ( !InVehicle )
							{
								g_Config.Player->VehicleGodMode = false;
								std::thread VehicleAlert( [ ] ( ) {  NotifyManager::Send( xorstr( "Entre em um veículo para usar esta função" ), 4000 );  } );
								VehicleAlert.detach( );
							}
							else {

								std::thread SeatBelt( [ ] ( ) 
									{
									Core::SDK::Pointers::pLocalPlayer->SeatBealt( g_Config.Player->SeatBelt );
									}
								);
								SeatBelt.detach( );

							}
						}


						if ( Custom::CheckBoxPage( xorstr( "Editor de dirigibilidade" ), &g_Config.Player->HandlingEditor, [ & ] ( ) {
							if ( !InVehicle ) {
								std::thread VehicleAlert( [ ] ( ) {  NotifyManager::Send( xorstr( "Entre em um veículo para usar esta função" ), 4000 );  } );
								VehicleAlert.detach( );
							}
							else {
								iSubTabCount = 4;
							}
							}, true, xorstr( "Abrir o editor de dirigibilidade" ) ) )
						{
							if ( !InVehicle )
							{
								g_Config.Player->HandlingEditor = false;
								std::thread VehicleAlert( [ ] ( ) {  NotifyManager::Send( xorstr( "Entre em um veículo para usar esta função" ), 4000 );  } );
								VehicleAlert.detach( );
							}

							if ( g_Config.Player->HandlingEditor )
							{
								Features::Exploits::g_HandlingEditor.SaveHandlingValues( );
							}
							else
							{
								Features::Exploits::g_HandlingEditor.RestoreHandlingValues( );
							}

						}

					}
					ImGui::EndChild( );
				}
				ImGui::EndGroup( );
				ImGui::SameLine( );
				ImGui::BeginGroup( );
				{
					ImGui::BeginChild( xorstr( "Ajustes" ), ImVec2( 340, 440 ), false, 0 );
					{
						ImGui::PushStyleVar( ImGuiStyleVar_ItemSpacing, ImVec2( 14, 8 ) );

                        Custom::CheckBox("Agarrar e arremessar carros", &Core::Features::VehicleManipulation::enabled);
                        if (Core::Features::VehicleManipulation::enabled) {
                            ImGui::TextWrapped("Y: agarra o carro na mira / arremessa. X: solta. Setas: altura.");
                            ImGui::SliderFloat("Altura do carro", &Core::Features::VehicleManipulation::height, 3.f, 30.f, "%.1f m");
                            ImGui::SliderFloat("Forca do arremesso", &Core::Features::VehicleManipulation::throwSpeed, 10.f, 100.f, "%.0f");
                            ImGui::TextWrapped("%s", Core::Features::VehicleManipulation::status);
                        }
                        if (Custom::Button("Puxar 5 carros proximos", ImVec2(-1,33), 0)) {
                            Core::Features::VehicleManipulation::PullFive();
                            NotifyManager::Send(Core::Features::VehicleManipulation::status,4000);
                        }

						if ( Custom::Button( xorstr( "Reparar veículo" ), ImVec2( -1, 33 ), 0 ) )
						{
							BridgeFeatures::RepairVehicle( );
                                const std::string diagnostic = BridgeFeatures::localStatus.empty()
                                    ? "Reparo solicitado; acompanhe o resultado no status abaixo."
                                    : BridgeFeatures::localStatus;
                                NotifyManager::Send(diagnostic, 5000);
                            }
                            if(Custom::Button("Apagar veiculo proximo (15 m)",ImVec2(-1,33),0))BridgeFeatures::DeleteClosestVehicle();
                            ImGui::TextWrapped("Apaga o veiculo elegivel mais proximo, mesmo ocupado. Ignora seu veiculo atual e veiculos sem controle local.");
                            BridgeFeatures::Status();
                            if(Custom::Button("Deletar todos em 20 m",ImVec2(-1,33),0))BridgeFeatures::DeleteVehiclesRadius(20);
                            ImGui::TextWrapped("Apaga da rede todos os veiculos a ate 20 m via native DeleteVehicle. Ignora seu veiculo atual e veiculos sem controle local.");

                        ImGui::BeginDisabled(!InVehicle || !CurrentVehicle || g_Config.Player->VehicleGodMode);
                        if (Custom::Button("Destruir veículo atual", ImVec2(-1,33), 0)) {
                            const std::string result = Core::Features::CarreraAdditions::DestroyCurrentVehicle();
                            NotifyManager::Send(result.c_str(), 4000);
                        }
                        ImGui::EndDisabled();
                        ImGui::TextWrapped("Danifica o motor do veículo atual. Não apaga o veículo. Desative a invencibilidade para usar.");


						ImGui::PopStyleVar( );
					}
					ImGui::EndChild( );
				}
				ImGui::EndGroup( );
			}
			break;
			case 2: //Teleports
				ImGui::SetCursorPos( ImVec2( 184, 136 ) );
				ImGui::BeginGroup( );
				{
					struct Locations_t {
						std::string Name;
						D3DXVECTOR3 Coords;
					};

					std::vector<Locations_t> Locations = {
						Locations_t( xorstr( "Ponto marcado" ), D3DXVECTOR3( 0, 0, 0 ) ),
						Locations_t( xorstr( "Praça" ), D3DXVECTOR3( 156.184, -1043.17, 29.3236 ) ),
						Locations_t( xorstr( "Píer" ), D3DXVECTOR3( -1847.72, -1223.36, 13.8745 ) ),
						Locations_t( xorstr( "Paleto Bay" ), D3DXVECTOR3( -397.605, 6047.57, 32.1797 ) ),
						Locations_t( xorstr( "Banco central" ), D3DXVECTOR3( 221.781, 217.278, 106.705 ) ),
						Locations_t( xorstr( "Cassino" ), D3DXVECTOR3( 885.322, 16.8489, 80.65 ) ),
						Locations_t( xorstr( "Aeroporto de Los Santos" ), D3DXVECTOR3( -975.532, -2880.89, 16.2665 ) ),
						Locations_t( xorstr( "Sandy Shores" ), D3DXVECTOR3( 1681.48, 3251.91, 40.809 ) ),
					};

					static int SeletedIndex = 0;
					bool IsSelected;

					ImGui::BeginChild( xorstr( "Destinos" ), ImVec2( g_MenuInfo.MenuSize.x - ( 184 + 24 ), 250 ), false, 0 );
					{
						ImGui::PushStyleVar( ImGuiStyleVar_ItemSpacing, ImVec2( 14, 8 ) );
						for ( int i = 0; i < Locations.size( ); i++ )
						{
							IsSelected = SeletedIndex == i;
							if ( ImGui::ListSelectable( Locations[ i ].Name.c_str( ), &IsSelected ) ) SeletedIndex = i;
						}
						ImGui::PopStyleVar( );
					}
					ImGui::EndChild( );
					static ImVec2 ContentSize = ImVec2( g_MenuInfo.MenuSize.x - ( 184 + 24 ), 180 );
					ImGui::BeginContent( xorstr( "LocationsInfo" ), ContentSize, false, 0 );
					{
						ImGui::BeginGroup( );
						{
                            auto Location = Locations[ SeletedIndex ];
							std::string Loc = xorstr( "Teleportar para " ) + Location.Name;
							if ( Custom::Button( Loc.c_str( ), ImVec2( ContentSize.x, 32 ), 0 ) ) {
								if ( SeletedIndex == 0 )
								{
									Core::Features::Exploits::TpToWaypoint( );
								}
								else {
									Core::SDK::Pointers::pLocalPlayer->SetPos( Location.Coords );
								}
							}
						}
						ImGui::EndGroup( );

					}
					ImGui::EndContent( );


				}
				ImGui::EndGroup( );
				break;
			case 3: //Weapons
				ImGui::BeginGroup( );
				{
					ImGui::BeginChild( xorstr( "Outros" ), ImVec2( 340, 440 ), false, 0 );
					{
						g_Config.Player->RecoilValue = Core::SDK::Pointers::pLocalPlayer->GetWeaponManager( )->GetRecoil( );
						static const float RecoilBackup = g_Config.Player->RecoilValue;

						g_Config.Player->SpreadValue = Core::SDK::Pointers::pLocalPlayer->GetWeaponManager( )->GetSpread( );
						static const float SpreadBackup = g_Config.Player->RecoilValue;

						if ( Custom::CheckBoxCfg( xorstr( "Controle de recuo" ), &g_Config.Player->NoRecoilEnabled, [ & ] ( ) {

							if ( ImGui::SliderFloat( xorstr( "Valor" ), &g_Config.Player->RecoilValue, 0.0f, 10.f, xorstr( "%1.2fx" ) ) ) {
								if ( g_Config.Player->NoRecoilEnabled ) {
									Core::SDK::Pointers::pLocalPlayer->GetWeaponManager( )->SetRecoil( g_Config.Player->RecoilValue );
								}
							}
							} ) )
						{
							if ( !g_Config.Player->NoRecoilEnabled ) {
								Core::SDK::Pointers::pLocalPlayer->GetWeaponManager( )->SetRecoil( RecoilBackup );
							}
						}

						if ( Custom::CheckBoxCfg( xorstr( "Controle de dispersão" ), &g_Config.Player->NoSpreadEnabled, [ & ] ( ) {
							if ( ImGui::SliderFloat( xorstr( "Valor" ), &g_Config.Player->SpreadValue, 0.0f, 10.f, xorstr( "%1.2fx" ) ) ) {
								if ( g_Config.Player->NoSpreadEnabled ) {
									Core::SDK::Pointers::pLocalPlayer->GetWeaponManager( )->SetSpread( g_Config.Player->SpreadValue );
								}
							}
							} ) )
						{
							if ( !g_Config.Player->NoSpreadEnabled ) {
								Core::SDK::Pointers::pLocalPlayer->GetWeaponManager( )->SetSpread( SpreadBackup );
							}
						}


						if ( Custom::CheckBox( xorstr( "Sem recarga" ), &g_Config.Player->NoReloadEnabled ) )
						{
							if ( g_Config.Player->NoReloadEnabled ) {
								Mem.PatchFunc( g_Offsets.m_InfiniteAmmo0, 3 );
							}
							else {
								Mem.WriteBytes( g_Offsets.m_InfiniteAmmo0, { 0x41, 0x2B, 0xC9, 0x3B, 0xC8, 0x0F, 0x4D, 0xC8 } );
							}

						}

						if ( Custom::CheckBox( xorstr( "Munição infinita" ), &g_Config.Player->InfiniteAmmoEnabled ) )
						{
							if ( !g_Offsets.m_InfiniteAmmo0 || !g_Offsets.m_InfiniteAmmo1 )
							{
								// Signatures not found — reset toggle and warn
								g_Config.Player->InfiniteAmmoEnabled = false;
								std::thread t( NotifyManager::Send,
									std::string( xorstr( "Munição infinita: assinatura não encontrada." ) ), 4000 );
								t.detach( );
							}
							else if ( g_Config.Player->InfiniteAmmoEnabled ) {
								Mem.PatchFunc( g_Offsets.m_InfiniteAmmo0, 3 );
								Mem.PatchFunc( g_Offsets.m_InfiniteAmmo1, 3 );
							}
							else {
								if ( !g_Config.Player->NoReloadEnabled )
								{
									Mem.WriteBytes( g_Offsets.m_InfiniteAmmo0, { 0x41, 0x2B, 0xC9, 0x3B, 0xC8, 0x0F, 0x4D, 0xC8 } );
								}
								Mem.WriteBytes( g_Offsets.m_InfiniteAmmo1, { 0x41, 0x2B, 0xD1, 0xE8 } );
							}

						}

					}
					ImGui::EndChild( );
				}
				ImGui::EndGroup( );
				ImGui::SameLine( );
				ImGui::BeginGroup( );
				{
					ImGui::BeginChild( xorstr( "Ajustes" ), ImVec2( 340, 440 ), false, 0 );
					{
						ImGui::PushStyleVar( ImGuiStyleVar_ItemSpacing, ImVec2( 14, 8 ) );

						static int Ammo = 0;
						ImGui::SliderInt( xorstr( "Quantidade de munição" ), &Ammo, 0, 250 );

						if ( Custom::Button( xorstr( "Adicionar munição" ), ImVec2( -1, 33 ), 0, true, xorstr( "Esta função pode ser detectada pelo anticheat do servidor" ), ICON_FA_TRIANGLE_EXCLAMATION ) )
						{
							// DBG D1: m_LocalPlayer addr + valor lido + pLocalPlayer atual + build
							{
								uintptr_t pedAddr  = (uintptr_t)Core::SDK::Pointers::pLocalPlayer;
								uintptr_t lpAddr   = g_Offsets.m_LocalPlayer; // endereco absoluto do ptr direto
								uintptr_t lpRead   = lpAddr ? Mem.Read<uintptr_t>(lpAddr) : 0; // o q tem la
								char dbg[256];
								snprintf(dbg, sizeof(dbg), xorstr("D1 ped=%llX lp=%llX rd=%llX b=%u"),
									(unsigned long long)pedAddr,
									(unsigned long long)lpAddr,
									(unsigned long long)lpRead,
									(unsigned)g_Offsets.CurrentBuild);
								std::thread t([s=std::string(dbg)](){ NotifyManager::Send(s, 9000); }); t.detach();
							}
							auto pWeapMgr = Core::SDK::Pointers::pLocalPlayer ? Core::SDK::Pointers::pLocalPlayer->GetWeaponManager( ) : nullptr;
							CWeaponInfo * pCWeaponInfo = pWeapMgr ? pWeapMgr->GetWeaponInfo( ) : nullptr;
							if ( pCWeaponInfo )
							{
								// Offset 0x60 = CWeapon::AmmoCount (confirmado pelo dump b3258 e AcBypass.hpp).
								// Sig scan foi removido: pattern 48 8B 43 ?? pode encontrar GetClipAmmo (0x5C)
								// antes de GetAmmoCount (0x60), cacheando offset errado para a sessão inteira.
								static const uintptr_t s_ammoOff = 0x60;

								uintptr_t AmmoInfo = Mem.Read<uintptr_t>( ( uintptr_t ) pCWeaponInfo + s_ammoOff );

								if ( AmmoInfo )
								{
									uintptr_t pAmmoCount = Mem.Read<uintptr_t>( ( uintptr_t ) AmmoInfo + 0x8 );
									if ( pAmmoCount )
									{
										uintptr_t CurrentAmmoAddr = Mem.Read<uintptr_t>( pAmmoCount + 0x0 ) + 0x18;
										int CurrentAmmo = Mem.Read<int>( CurrentAmmoAddr );
										Mem.Write( CurrentAmmoAddr, CurrentAmmo + Ammo );
										// DEBUG: confirma o write
										char dbg[64]; snprintf(dbg, sizeof(dbg), xorstr("Ammo: %d->%d"), CurrentAmmo, CurrentAmmo + Ammo);
										std::thread t([s=std::string(dbg)](){ NotifyManager::Send(s, 3000); }); t.detach();
									}
									else
									{
										// DEBUG: pAmmoCount = 0
										char dbg[64]; snprintf(dbg, sizeof(dbg), xorstr("DBG: pAmmoCount=0 AmmoInfo=0x%llX"), (unsigned long long)AmmoInfo);
										std::thread t([s=std::string(dbg)](){ NotifyManager::Send(s, 5000); }); t.detach();
									}
								}
								else
								{
									// DEBUG: AmmoInfo = 0 (mostra tambem o ptr do CWeapon)
									char dbg[64]; snprintf(dbg, sizeof(dbg), xorstr("DBG: AmmoInfo=0 CWeap=0x%llX"), (unsigned long long)(uintptr_t)pCWeaponInfo);
									std::thread t([s=std::string(dbg)](){ NotifyManager::Send(s, 5000); }); t.detach();
								}
							}
							else
							{
								// DEBUG: pCWeaponInfo = null
								char dbg[64]; snprintf(dbg, sizeof(dbg), xorstr("DBG: CWeap=null pMgr=0x%llX"), (unsigned long long)(uintptr_t)pWeapMgr);
								std::thread t([s=std::string(dbg)](){ NotifyManager::Send(s, 5000); }); t.detach();
							}
						}

						if ( Custom::Button( xorstr( "Definir 200 munições" ), ImVec2( -1, 33 ), 0, true, xorstr( "Esta função pode ser detectada pelo anticheat do servidor" ), ICON_FA_TRIANGLE_EXCLAMATION ) )
						{
							// DBG D1: m_LocalPlayer addr + valor lido + pLocalPlayer atual + build
							{
								uintptr_t pedAddr  = (uintptr_t)Core::SDK::Pointers::pLocalPlayer;
								uintptr_t lpAddr   = g_Offsets.m_LocalPlayer;
								uintptr_t lpRead   = lpAddr ? Mem.Read<uintptr_t>(lpAddr) : 0;
								char dbg[256];
								snprintf(dbg, sizeof(dbg), xorstr("D1 ped=%llX lp=%llX rd=%llX b=%u"),
									(unsigned long long)pedAddr,
									(unsigned long long)lpAddr,
									(unsigned long long)lpRead,
									(unsigned)g_Offsets.CurrentBuild);
								std::thread t([s=std::string(dbg)](){ NotifyManager::Send(s, 9000); }); t.detach();
							}
							auto pWeapMgr200 = Core::SDK::Pointers::pLocalPlayer ? Core::SDK::Pointers::pLocalPlayer->GetWeaponManager( ) : nullptr;
							CWeaponInfo * pCWeaponInfo200 = pWeapMgr200 ? pWeapMgr200->GetWeaponInfo( ) : nullptr;
							if ( pCWeaponInfo200 )
							{
								// Mesmo offset do botao AddAmmo — 0x60 = CWeapon::AmmoCount (b3258 confirmado).
								static const uintptr_t s_ammoOff200 = 0x60;

								uintptr_t AmmoInfo200 = Mem.Read<uintptr_t>( ( uintptr_t ) pCWeaponInfo200 + s_ammoOff200 );
								if ( AmmoInfo200 )
								{
									uintptr_t pAmmoCount200 = Mem.Read<uintptr_t>( ( uintptr_t ) AmmoInfo200 + 0x8 );
									if ( pAmmoCount200 )
									{
										uintptr_t CurrentAmmoAddr200 = Mem.Read<uintptr_t>( pAmmoCount200 + 0x0 ) + 0x18;
										Mem.Write( CurrentAmmoAddr200, 200 );
										{ std::thread t([](){ NotifyManager::Send(xorstr("200 munições definidas!"), 2000); }); t.detach(); }
									}
									else
									{
										char dbg[64]; snprintf(dbg, sizeof(dbg), xorstr("DBG: pAmmoCount200=0 AmmoInfo=0x%llX"), (unsigned long long)AmmoInfo200);
										std::thread t([s=std::string(dbg)](){ NotifyManager::Send(s, 5000); }); t.detach();
									}
								}
								else
								{
									char dbg[64]; snprintf(dbg, sizeof(dbg), xorstr("DBG200: AmmoInfo=0 CWeap=0x%llX"), (unsigned long long)(uintptr_t)pCWeaponInfo200);
									std::thread t([s=std::string(dbg)](){ NotifyManager::Send(s, 5000); }); t.detach();
								}
							}
							else
							{
								char dbg[80]; snprintf(dbg, sizeof(dbg), xorstr("D2 weap=null mgr=%llX build=%u"),
									(unsigned long long)(uintptr_t)pWeapMgr200, (unsigned)g_Offsets.CurrentBuild);
								std::thread t([s=std::string(dbg)](){ NotifyManager::Send(s, 5000); }); t.detach();
							}
						}

						ImGui::PopStyleVar( );
					}
					ImGui::EndChild( );
				}
				ImGui::EndGroup( );
				break;
			case 4: //HandlingEditor
				ImGui::BeginGroup( );
				{
					bool InVehicle = Core::SDK::Pointers::pLocalPlayer->InVehicle( );

					if ( !InVehicle ) 
						iSubTabCount = 1;

					ImGui::SetCursorPos( ImVec2( 184, 120 ) );

					ImGui::PushStyleVar( ImGuiStyleVar_ItemSpacing, ImVec2( 10, 4 ) );
					{
						ImGui::TextColored( g_Col.FeaturesText, xorstr( "Editor de dirigibilidade" ) );
						ImGui::PushFont( g_Variables.FontAwesomeSolidSmall );
						{
							ImGui::SameLine( );
							ImGui::TextColored( g_Col.Base, ICON_FA_REPLY );
							if ( ImGui::IsItemClicked( ) )
							{
								iSubTabCount = 1;
							}
						}
						ImGui::PopFont( );
						ImGui::PushFont( g_Variables.m_FontSecundary );
						{
							ImGui::TextColored( g_Col.SecundaryText, xorstr( "Ajuste a dirigibilidade do seu veículo." ) );
						}
						ImGui::PopFont( );
					}
					ImGui::PopStyleVar( );

					ImGui::Spacing( );

					ImGui::BeginChild( xorstr( "Dirigibilidade" ), ImVec2( g_MenuInfo.MenuSize.x - ( 184 + 24 ), 360 ), false, 0 );
					{

						if ( ImGui::SliderFloat( xorstr( "Aceleração" ), &Features::Exploits::g_HandlingEditor.fAcceleration, 0.0f, 400.f, xorstr( "%1.1f" ) ) )
						{
							Features::Exploits::g_HandlingEditor.ApplyHandlingValues( );
						}

						if (ImGui::SliderFloat(xorstr("Força de frenagem"), &Features::Exploits::g_HandlingEditor.fBreakForce, 0.0f, 100.f, xorstr("%1.1f")))
						{
							Features::Exploits::g_HandlingEditor.ApplyHandlingValues();
						}

						if ( ImGui::SliderFloat( xorstr( "Tração mínima" ), &Features::Exploits::g_HandlingEditor.fTractionCurveMin, 0.0f, 100.f, xorstr( "%1.1f" ) ) )
						{
							Features::Exploits::g_HandlingEditor.ApplyHandlingValues( );
						}

					}
					ImGui::EndChild( );
				}
				ImGui::EndGroup( );
				break;
			case 5: //WeaponSpawn
				ImGui::BeginGroup( );
				{
					ImGui::SetCursorPos( ImVec2( 184, 120 ) );

					ImGui::PushStyleVar( ImGuiStyleVar_ItemSpacing, ImVec2( 10, 4 ) );
					{
						ImGui::TextColored( g_Col.FeaturesText, xorstr( "Gerar arma" ) );
						ImGui::PushFont( g_Variables.FontAwesomeSolidSmall );
						{
							ImGui::SameLine( );
							ImGui::TextColored( g_Col.Base, ICON_FA_REPLY );
							if ( ImGui::IsItemClicked( ) ) {
								iSubTabCount = 3;
							}
						}
						ImGui::PopFont( );
						ImGui::PushFont( g_Variables.m_FontSecundary );
						{
							ImGui::TextColored( g_Col.SecundaryText, xorstr( "Seleção de armas" ) );
						}
						ImGui::PopFont( );
					}
					ImGui::PopStyleVar( );

					ImGui::Spacing( );

					ImGui::BeginContent( xorstr( "Lista de armas" ), ImVec2( g_MenuInfo.MenuSize.x - ( 188 + 24 ), 360 ), 0, 0 );
					{
						struct WeaponSpawn_t {
							std::string Name;
							unsigned int WeaponHash;
							ID3D11ShaderResourceView * Picture;
						};

						std::vector<WeaponSpawn_t> WeaponList = {
							// Melee Weapons
							WeaponSpawn_t( xorstr( "Adaga" ), 0x92A27487, g_Variables.FiveM_WeaponsPic.weapon_dagger ),
							WeaponSpawn_t( xorstr( "Taco de beisebol" ), 0x958A4A8F, g_Variables.FiveM_WeaponsPic.weapon_bat ),
							WeaponSpawn_t( xorstr( "Garrafa" ), 0xF9E6AA4B, g_Variables.FiveM_WeaponsPic.weapon_bottle ),
							WeaponSpawn_t( xorstr( "Pé de cabra" ), 0x84BD7BFD, g_Variables.FiveM_WeaponsPic.weapon_crowbar ),
							//WeaponSpawn_t( xorstr( "Desarmado" ), 0xA2719263, g_Variables.FiveM_WeaponsPic.weapon_unarmed ),
							WeaponSpawn_t( xorstr( "Lanterna" ), 0x8BB05FD7, g_Variables.FiveM_WeaponsPic.weapon_flashlight ),
							WeaponSpawn_t( xorstr( "Taco de golfe" ), 0x440E4788, g_Variables.FiveM_WeaponsPic.weapon_golfclub ),
							WeaponSpawn_t( xorstr( "Martelo" ), 0x4E875F73, g_Variables.FiveM_WeaponsPic.weapon_hammer ),
							WeaponSpawn_t( xorstr( "Machadinha" ), 0xF9DCBF2D, g_Variables.FiveM_WeaponsPic.weapon_hatchet ),
							WeaponSpawn_t( xorstr( "Soco-inglês" ), 0xD8DF3C3C, g_Variables.FiveM_WeaponsPic.weapon_knuckle ),
							WeaponSpawn_t( xorstr( "Faca" ), 0x99B507EA, g_Variables.FiveM_WeaponsPic.weapon_knife ),
							WeaponSpawn_t( xorstr( "Facão" ), 0xDD5DF8D9, g_Variables.FiveM_WeaponsPic.weapon_machete ),
							WeaponSpawn_t( xorstr( "Canivete" ), 0xDFE37640, g_Variables.FiveM_WeaponsPic.weapon_switchblade ),
							WeaponSpawn_t( xorstr( "Cassetete" ), 0x678B81B1, g_Variables.FiveM_WeaponsPic.weapon_nightstick ),
							WeaponSpawn_t( xorstr( "Chave inglesa" ), 0x19044EE0, g_Variables.FiveM_WeaponsPic.weapon_wrench ),
							WeaponSpawn_t( xorstr( "Machado de batalha" ), 0xCD274149, g_Variables.FiveM_WeaponsPic.weapon_battleaxe ),
							WeaponSpawn_t( xorstr( "Taco de sinuca" ), 0x94117305, g_Variables.FiveM_WeaponsPic.weapon_poolcue ),
							WeaponSpawn_t( xorstr( "Machado de pedra" ), 0x3813FC08, g_Variables.FiveM_WeaponsPic.weapon_stone_hatchet ),

							// Handguns
							WeaponSpawn_t( xorstr( "Pistola" ), 0x1B06D571, g_Variables.FiveM_WeaponsPic.weapon_pistol ),
							WeaponSpawn_t( xorstr( "Pistola Mk II" ), 0xBFE256D4, g_Variables.FiveM_WeaponsPic.weapon_pistol_mk2 ),
							WeaponSpawn_t( xorstr( "Pistola de combate" ), 0x5EF9FEC4, g_Variables.FiveM_WeaponsPic.weapon_combatpistol ),
							WeaponSpawn_t( xorstr( "Pistola automática" ), 0x22D8FE39, g_Variables.FiveM_WeaponsPic.weapon_appistol ),
							WeaponSpawn_t( xorstr( "Arma de choque" ), 0x3656C8C1, g_Variables.FiveM_WeaponsPic.weapon_stungun ),
							WeaponSpawn_t( xorstr( "Pistola calibre .50" ), 0x99AEEB3B, g_Variables.FiveM_WeaponsPic.weapon_pistol50 ),
							WeaponSpawn_t( xorstr( "Pistola SNS" ), 0xBFD21232, g_Variables.FiveM_WeaponsPic.weapon_snspistol ),
							WeaponSpawn_t( xorstr( "Pistola SNS Mk II" ), 0x88374054, g_Variables.FiveM_WeaponsPic.weapon_snspistol_mk2 ),
							WeaponSpawn_t( xorstr( "Pistola pesada" ), 0xD205520E, g_Variables.FiveM_WeaponsPic.weapon_heavypistol ),
							WeaponSpawn_t( xorstr( "Pistola antiga" ), 0x83839C4, g_Variables.FiveM_WeaponsPic.weapon_vintagepistol ),
							WeaponSpawn_t( xorstr( "Pistola sinalizadora" ), 0x47757124, g_Variables.FiveM_WeaponsPic.weapon_flaregun ),
							WeaponSpawn_t( xorstr( "Pistola de precisão" ), 0xDC4DB296, g_Variables.FiveM_WeaponsPic.weapon_marksmanpistol ),
							WeaponSpawn_t( xorstr( "Revólver" ), 0xC1B3C3D1, g_Variables.FiveM_WeaponsPic.weapon_revolver ),
							WeaponSpawn_t( xorstr( "Revólver Mk II" ), 0xCB96392F, g_Variables.FiveM_WeaponsPic.weapon_revolver_mk2 ),
							WeaponSpawn_t( xorstr( "Revólver de ação dupla" ), 0x97EA20B8, g_Variables.FiveM_WeaponsPic.weapon_doubleaction ),
							WeaponSpawn_t( xorstr( "Pistola de raios" ), 0xAF3696A1, g_Variables.FiveM_WeaponsPic.weapon_raypistol ),
							WeaponSpawn_t( xorstr( "Pistola de cerâmica" ), 0x2B5EF5EC, g_Variables.FiveM_WeaponsPic.weapon_ceramicpistol ),
							WeaponSpawn_t( xorstr( "Revólver da marinha" ), 0x917F6C8C, g_Variables.FiveM_WeaponsPic.weapon_navyrevolver ),

							// SMGs
							WeaponSpawn_t( xorstr( "Micro SMG" ), 0x13532244, g_Variables.FiveM_WeaponsPic.weapon_microsmg ),
							WeaponSpawn_t( xorstr( "SMG" ), 0x2BE6766B, g_Variables.FiveM_WeaponsPic.weapon_smg ),
							WeaponSpawn_t( xorstr( "SMG Mk II" ), 0x78A97CD0, g_Variables.FiveM_WeaponsPic.weapon_smg_mk2 ),
							WeaponSpawn_t( xorstr( "SMG de assalto" ), 0xEFE7E2DF, g_Variables.FiveM_WeaponsPic.weapon_assaultsmg ),
							WeaponSpawn_t( xorstr( "PDW de combate" ), 0xA3D4D34, g_Variables.FiveM_WeaponsPic.weapon_combatpdw ),
							WeaponSpawn_t( xorstr( "Pistola metralhadora" ), 0xDB1AA450, g_Variables.FiveM_WeaponsPic.weapon_machinepistol ),
							WeaponSpawn_t( xorstr( "Mini SMG" ), 0xBD248B55, g_Variables.FiveM_WeaponsPic.weapon_minismg ),
							WeaponSpawn_t( xorstr( "Carabina de raios" ), 0x476BF155, g_Variables.FiveM_WeaponsPic.weapon_raycarbine ),

							// Shotguns
							WeaponSpawn_t( xorstr( "Escopeta de ação" ), 0x1D073A89, g_Variables.FiveM_WeaponsPic.weapon_pumpshotgun ),
							WeaponSpawn_t( xorstr( "Escopeta de ação Mk II" ), 0x555AF99A, g_Variables.FiveM_WeaponsPic.weapon_pumpshotgun_mk2 ),
							WeaponSpawn_t( xorstr( "Escopeta serrada" ), 0x7846A318, g_Variables.FiveM_WeaponsPic.weapon_sawnoffshotgun ),
							WeaponSpawn_t( xorstr( "Escopeta de assalto" ), 0xE284C527, g_Variables.FiveM_WeaponsPic.weapon_assaultshotgun ),
							WeaponSpawn_t( xorstr( "Escopeta bullpup" ), 0x9D61E50F, g_Variables.FiveM_WeaponsPic.weapon_bullpupshotgun ),
							WeaponSpawn_t( xorstr( "Mosquete" ), 0xA89CB99E, g_Variables.FiveM_WeaponsPic.weapon_musket ),
							WeaponSpawn_t( xorstr( "Escopeta pesada" ), 0x3AABBBAA, g_Variables.FiveM_WeaponsPic.weapon_heavyshotgun ),
							WeaponSpawn_t( xorstr( "Escopeta de cano duplo" ), 0xEF951FBB, g_Variables.FiveM_WeaponsPic.weapon_dbshotgun ),
							WeaponSpawn_t( xorstr( "Escopeta automática" ), 0x12E82D3D, g_Variables.FiveM_WeaponsPic.weapon_autoshotgun ),

							// Assault Rifles
							WeaponSpawn_t( xorstr( "Fuzil de assalto" ), 0xBFEFFF6D, g_Variables.FiveM_WeaponsPic.weapon_assaultrifle ),
							WeaponSpawn_t( xorstr( "Fuzil de assalto Mk II" ), 0x394F415C, g_Variables.FiveM_WeaponsPic.weapon_assaultrifle_mk2 ),
							WeaponSpawn_t( xorstr( "Carabina" ), 0x83BF0278, g_Variables.FiveM_WeaponsPic.weapon_carbinerifle ),
							WeaponSpawn_t( xorstr( "Carabina Mk II" ), 0xFAD1F1C9, g_Variables.FiveM_WeaponsPic.weapon_carbinerifle_mk2 ),
							WeaponSpawn_t( xorstr( "Fuzil avançado" ), 0xAF113F99, g_Variables.FiveM_WeaponsPic.weapon_advancedrifle ),
							WeaponSpawn_t( xorstr( "Carabina especial" ), 0xC0A3098D, g_Variables.FiveM_WeaponsPic.weapon_specialcarbine ),
							WeaponSpawn_t( xorstr( "Carabina especial Mk II" ), 0x969C3D67, g_Variables.FiveM_WeaponsPic.weapon_specialcarbine_mk2 ),
							WeaponSpawn_t( xorstr( "Fuzil bullpup" ), 0x7F229F94, g_Variables.FiveM_WeaponsPic.weapon_bullpuprifle ),
							WeaponSpawn_t( xorstr( "Fuzil bullpup Mk II" ), 0x84D6FAFD, g_Variables.FiveM_WeaponsPic.weapon_bullpuprifle_mk2 ),
							WeaponSpawn_t( xorstr( "Fuzil compacto" ), 0x624FE830, g_Variables.FiveM_WeaponsPic.weapon_compactrifle ),

							// Machine Guns
							WeaponSpawn_t( xorstr( "Metralhadora" ), 0x9D07F764, g_Variables.FiveM_WeaponsPic.weapon_mg ),
							WeaponSpawn_t( xorstr( "Metralhadora de combate" ), 0x7FD62962, g_Variables.FiveM_WeaponsPic.weapon_combatmg ),
							WeaponSpawn_t( xorstr( "Metralhadora de combate Mk II" ), 0xDBBD7280, g_Variables.FiveM_WeaponsPic.weapon_combatmg_mk2 ),
							WeaponSpawn_t( xorstr( "Gusenberg" ), 0x61012683, g_Variables.FiveM_WeaponsPic.weapon_gusenberg ),

							// Sniper Rifles
							WeaponSpawn_t( xorstr( "Rifle de precisão" ), 0x5FC3C11, g_Variables.FiveM_WeaponsPic.weapon_sniperrifle ),
							WeaponSpawn_t( xorstr( "Rifle de precisão pesado" ), 0xC472FE2, g_Variables.FiveM_WeaponsPic.weapon_heavysniper ),
							WeaponSpawn_t( xorstr( "Rifle de precisão pesado Mk II" ), 0xA914799, g_Variables.FiveM_WeaponsPic.weapon_heavysniper_mk2 ),
							WeaponSpawn_t( xorstr( "Rifle de atirador" ), 0xC734385A, g_Variables.FiveM_WeaponsPic.weapon_marksmanrifle ),
							WeaponSpawn_t( xorstr( "Rifle de atirador Mk II" ), 0x6A6C02E0, g_Variables.FiveM_WeaponsPic.weapon_marksmanrifle_mk2 ),

							// Heavy Weapons
							WeaponSpawn_t( xorstr( "RPG" ), 0xB1CA77B1, g_Variables.FiveM_WeaponsPic.weapon_rpg ),
							WeaponSpawn_t( xorstr( "Lança-granadas" ), 0xA284510B, g_Variables.FiveM_WeaponsPic.weapon_grenadelauncher ),
							WeaponSpawn_t( xorstr( "Lança-granadas de fumaça" ), 0x4DD2DC56, g_Variables.FiveM_WeaponsPic.weapon_grenadelauncher_smoke ),
							WeaponSpawn_t( xorstr( "Metralhadora giratória" ), 0x42BF8A85, g_Variables.FiveM_WeaponsPic.weapon_minigun ),
							WeaponSpawn_t( xorstr( "Lança-fogos" ), 0x7F7497E5, g_Variables.FiveM_WeaponsPic.weapon_firework ),
							WeaponSpawn_t( xorstr( "Canhão eletromagnético" ), 0x6D544C99, g_Variables.FiveM_WeaponsPic.weapon_railgun ),
							WeaponSpawn_t( xorstr( "Lança-mísseis guiados" ), 0x63AB0442, g_Variables.FiveM_WeaponsPic.weapon_hominglauncher ),
							WeaponSpawn_t( xorstr( "Lançador compacto" ), 0x781FE4A, g_Variables.FiveM_WeaponsPic.weapon_compactlauncher ),
							WeaponSpawn_t( xorstr( "Metralhadora de raios" ), 0xB62D1F67, g_Variables.FiveM_WeaponsPic.weapon_rayminigun ),

							// Throwables
							WeaponSpawn_t( xorstr( "Granada" ), 0x93E220BD, g_Variables.FiveM_WeaponsPic.weapon_grenade ),
							WeaponSpawn_t( xorstr( "Gás BZ" ), 0xA0973D5E, g_Variables.FiveM_WeaponsPic.weapon_bzgas ),
							WeaponSpawn_t( xorstr( "Granada de fumaça" ), 0xFDBC8A50, g_Variables.FiveM_WeaponsPic.weapon_smokegrenade ),
							WeaponSpawn_t( xorstr( "Sinalizador" ), 0x497FACC3, g_Variables.FiveM_WeaponsPic.weapon_flare ),
							WeaponSpawn_t( xorstr( "Molotov" ), 0x24B17070, g_Variables.FiveM_WeaponsPic.weapon_molotov ),
							WeaponSpawn_t( xorstr( "Bomba adesiva" ), 0x2C3731D9, g_Variables.FiveM_WeaponsPic.weapon_stickybomb ),
							WeaponSpawn_t( xorstr( "Mina de proximidade" ), 0xAB564B93, g_Variables.FiveM_WeaponsPic.weapon_proxmine ),
							WeaponSpawn_t( xorstr( "Bola de neve" ), 0x787F0BB, g_Variables.FiveM_WeaponsPic.weapon_snowball ),
							WeaponSpawn_t( xorstr( "Bomba caseira" ), 0xBA45E8B8, g_Variables.FiveM_WeaponsPic.weapon_pipebomb ),
							WeaponSpawn_t( xorstr( "Bola" ), 0x23C9F95C, g_Variables.FiveM_WeaponsPic.weapon_ball ),

							// Miscellaneous
							WeaponSpawn_t( xorstr( "Galão de combustível" ), 0x34A67B97, g_Variables.FiveM_WeaponsPic.weapon_petrolcan ),
							WeaponSpawn_t( xorstr( "Extintor" ), 0x60EC506, g_Variables.FiveM_WeaponsPic.weapon_fireextinguisher ),
							WeaponSpawn_t( xorstr( "Paraquedas" ), 0xFBAB5776, g_Variables.FiveM_WeaponsPic.gadget_parachute ),
							WeaponSpawn_t( xorstr( "Galão de material perigoso" ), 0xBA536372, g_Variables.FiveM_WeaponsPic.weapon_hazardcan )
						};

						int count = 0;
						//ImGui::SetCursorPosX( 6 );
						for ( auto Weapon : WeaponList )
						{

							if ( Weapon.Name.empty( ) || Weapon.Picture == nullptr ) {
								continue;
							}

							if ( Custom::WeaponButtonHeld( Weapon.Picture, Weapon.Name.c_str( ), 0 ) ) {
								bool ok = Core::Features::Exploits::g_WeaponSpawn.Spawn( Weapon.WeaponHash, 999 );
								std::string Msg = ok
									? Weapon.Name + xorstr( " foi gerada." )
									: xorstr( "Falha ao gerar arma. Sig não encontrada." );
								std::thread NotifyThread( NotifyManager::Send, Msg, 4000 );
								NotifyThread.detach( );
							}

							count++;

							if ( count % 4 != 0 ) {
								ImGui::SameLine( );
							}
							else {
								count = 0;
								//ImGui::SetCursorPosX( 6 );
							}
						}
					}
					ImGui::EndContent( );
				}
				ImGui::EndGroup( );
			default:
				break;
			}

		}
		ImGui::EndGroup( );
		ImGui::PopStyleVar( );

	}
}

