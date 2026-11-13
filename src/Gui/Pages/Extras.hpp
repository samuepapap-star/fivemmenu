#pragma once
#include <Bridge/Features.hpp>
#include <Core/Features/ExtraFeatures.hpp>
#include <Core/Features/ExtraCamera.hpp>
#include <Core/Features/WorldClock.hpp>
#include <Core/Features/VehicleManipulation.hpp>
#include <Core/Features/GrabLaunch.hpp>
namespace ExtraPages {
inline void Key(const char* label,int& value){static int mode=1;ImGui::Keybind(label,&value,&mode);}
inline void Render(int section){
 using namespace ExtraSettings;using namespace Core::Features;
 ImGui::SetCursorPos(ImVec2(184,136));
 ImGui::BeginChild("Extras##Organized",ImVec2(704,440),false,0);
 ImGui::PushID(section);
 if(section==0){
  ImGui::TextDisabled("Personagem / movimento e camera");
  Custom::CheckBox("Super pulo",&SuperJump);Custom::CheckBox("Pulo da besta",&BeastJump);
  Custom::CheckBox("Bloquear mira assistida local",&BlockAim);
  Custom::CheckBox("Modo passivo local",&PassiveTargeting);
  Custom::CheckBox("Spinbot",&Spin);if(Spin)ImGui::SliderFloat("Velocidade da rotacao",&SpinSpeed,10.f,360.f,"%.0f graus/s");
  if(Spin){
   Custom::CheckBox("Combat Glitch (giro no rolamento)",&CombatGlitch);
   if(CombatGlitch)ImGui::TextWrapped("Gira durante 800 ms apos mirar + Espaco, enquanto estiver no chao.");
  }
  if(Custom::Button(BridgeFeatures::cameraRequested||MenuBridge::Connection().CameraState()?"Parar camera livre":"Camera livre independente",ImVec2(-1,30),0))BridgeFeatures::ToggleCamera();
  ImGui::SliderFloat("Velocidade da camera livre",&ExtraCamera::speed,1.f,50.f,"%.0f m/s");
  Custom::CheckBox("Alterar distancia da camera",&CameraDistanceEnabled);
  if(CameraDistanceEnabled)ImGui::SliderFloat("Afastamento da camera",&CameraDistancePercent,0.f,100.f,"%.0f%%");
  ImGui::TextWrapped("Camera livre: feche o menu; WASD, mouse, Espaco/Ctrl; Shift acelera; Esc encerra.");
  BridgeFeatures::Status();
  ImGui::TextWrapped("%s",Extras::status);
 }
 if(section==1){
  GrabLaunch::Tick();
  ImGui::TextDisabled("Carro / portas e impulso");
  ImGui::TextDisabled("Agarrar & Arremessar veiculo");
  if(GrabLaunch::IsIdle()){
   if(Custom::Button("Agarrar Veiculo",ImVec2(-1,30),0))GrabLaunch::Grab();
  }
  if(GrabLaunch::IsHolding()){
   ImGui::PushStyleColor(ImGuiCol_Button,ImVec4(0.8f,0.2f,0.2f,1.0f));
   if(Custom::Button("Arremessar!",ImVec2(-1,30),0))GrabLaunch::Throw();
   ImGui::PopStyleColor();
  }
  ImGui::Separator();
  if(Custom::Button("Destrancar mais proximo",ImVec2(-1,30),0))Extras::SetDoors(true,true);
  if(Custom::Button("Destrancar carros proximos",ImVec2(-1,30),0))Extras::SetDoors(true,false);
  if(Custom::Button("Trancar carros proximos",ImVec2(-1,30),0))Extras::SetDoors(false,false);
  Custom::CheckBox("Boost por tecla",&HornBoost);if(HornBoost){Key("Tecla do boost",HornKey);ImGui::SliderFloat("Forca do boost",&BoostPower,1.f,100.f,"%.0f");}
  Custom::CheckBox("Tracao maxima personalizada",&MaxTractionOn);if(MaxTractionOn)ImGui::SliderFloat("Tracao maxima",&MaxTraction,0.1f,10.f,"%.2f");
  Custom::CheckBox("Suspensao personalizada",&SuspensionOn);if(SuspensionOn)ImGui::SliderFloat("Forca da suspensao",&Suspension,0.1f,10.f,"%.2f");
  Custom::CheckBox("Altura da suspensao",&LiftOn);if(LiftOn)ImGui::SliderFloat("Elevacao",&Lift,-0.5f,1.f,"%.2f");
  Custom::CheckBox("Gravidade do veiculo",&GravityOn);if(GravityOn)ImGui::SliderFloat("Gravidade",&Gravity,0.1f,20.f,"%.1f");
  Custom::CheckBox("Modo drift",&Drift);
  if(Custom::Button("Preset de aderencia",ImVec2(-1,30),0)){Drift=false;MaxTractionOn=true;MaxTraction=3.f;SuspensionOn=true;Suspension=2.5f;}
  if(Custom::Button("Restaurar ajustes adicionais",ImVec2(-1,30),0)){Drift=MaxTractionOn=SuspensionOn=LiftOn=GravityOn=false;}
  Custom::CheckBox("Rocket Boost",&RocketBoost);Custom::CheckBox("Carro pulando",&JumpCar);Custom::CheckBox("Carro planando",&PlaningCar);
  ImGui::TextWrapped("%s",Extras::status);
 }
 if(section==2){
  ImGui::TextDisabled("Armas / efeitos e velocidade");
  Custom::CheckBox("Congelar quantidade de municao",&FreezeAmmo);
  Custom::CheckBox("Punho explosivo",&ExplosiveFist);Custom::CheckBox("Municao incendiaria",&FireAmmo);Custom::CheckBox("Municao explosiva",&ExplosiveAmmo);
  Custom::CheckBox("Recarga rapida",&FastReload);if(FastReload)ImGui::SliderFloat("Multiplicador da recarga",&ReloadSpeed,1.f,10.f,"%.1fx");
  Custom::CheckBox("Cadencia ajustavel",&RapidFire);if(RapidFire)ImGui::SliderFloat("Multiplicador da cadencia",&FireRate,1.f,10.f,"%.1fx");
  Custom::CheckBox("Alterar tamanho da arma",&WeaponScaleOn);if(WeaponScaleOn)ImGui::SliderFloat("Tamanho da arma",&WeaponScale,0.1f,5.f,"%.1fx");
 }
 if(section==3){
  ImGui::TextDisabled("Jogadores / radar e apresentacao");
  Custom::CheckBox("Radar",&Radar);Custom::CheckBox("Veiculos no radar",&RadarVehicles);Custom::CheckBox("Nomes no radar",&RadarNames);
  Custom::CheckBox("Girar com a camera",&RadarRotate);Custom::CheckBox("NPCs no radar",&RadarNPCs);
  ImGui::SliderFloat("Tamanho do radar",&RadarSize,120.f,400.f,"%.0f px");ImGui::SliderFloat("Alcance do radar",&RadarRange,20.f,1000.f,"%.0f m");
  ImGui::TextDisabled("Arraste o radar enquanto o menu estiver aberto.");
  ImGui::SliderFloat("Tamanho dos textos ESP",&TextSize,8.f,24.f,"%.0f px");ImGui::SliderFloat("Espessura do esqueleto",&SkeletonWidth,0.5f,5.f,"%.1f");
  ImGui::SliderFloat("Espessura das caixas",&BoxWidth,0.5f,4.f,"%.1f");ImGui::Combo("Fonte do ESP",&Font,"Atual\0Fonte do menu\0Compacta\0");
  Custom::CheckBox("Brilho ao redor do jogador",&PlayerGlow);
  Custom::CheckBox("Icone da arma",&WeaponIcon);
  Custom::CheckBox("Chapeu conico (China Hat)",&ChinaHat);if(ChinaHat)ImGui::SliderFloat("Tamanho do chapeu",&HatSize,0.1f,1.f,"%.2f");
  Custom::CheckBox("Aviso de possiveis invisiveis",&InvisibleWarning);
  Custom::CheckBox("Tecla para ativar/desativar ESP",&ESPHotkeyOn);if(ESPHotkeyOn)Key("Tecla ESP",EspKey);
 }
 if(section==4){
  ImGui::TextDisabled("Teleportar / posicao salva");
  if(Custom::Button("Salvar posicao atual",ImVec2(-1,33),0))Extras::SavePeek();
  ImGui::BeginDisabled(!Extras::peekSaved);if(Custom::Button("Voltar a posicao salva",ImVec2(-1,33),0))Extras::ReturnPeek();ImGui::EndDisabled();
  Key("Tecla para salvar",SavePeekKey);Key("Tecla para voltar",ReturnPeekKey);
  if(Extras::peekSaved)ImGui::Text("X %.1f  Y %.1f  Z %.1f",Extras::peek.x,Extras::peek.y,Extras::peek.z);
  ImGui::TextWrapped("%s",Extras::status);
 }
 if(section==5){
  ImGui::TextDisabled("Configuracoes / aparencia");
  Custom::CheckBox("Cores animadas no menu",&Rainbow);if(Rainbow)ImGui::SliderFloat("Velocidade das cores",&RainbowSpeed,0.01f,1.f,"%.2f");
  ImGui::Combo("Fundo animado",&Background,"Atual\0Matrix\0Estrelas\0Ondas\0Coracoes\0Vortice\0Raios\0Galaxia\0Chuva\0");
  ImGui::Combo("Estilo do menu",&Theme,"Atual\0Arredondado\0Quadrado\0");ImGui::SliderFloat("Espessura da borda",&BorderSize,0.f,4.f,"%.1f");
  Custom::CheckBox("Particulas",&Particles);if(Particles){ImGui::SliderInt("Quantidade",&ParticleCount,0,150);ImGui::SliderFloat("Velocidade das particulas",&ParticleSpeed,0.05f,2.f,"%.2f");ImGui::SliderFloat("Tamanho das particulas",&ParticleSize,1.f,5.f,"%.1f");}
  Custom::CheckBox("Ocultar da barra de tarefas",&HideTaskbar);
  Custom::CheckBox("Projetar overlay em outro monitor",&SecondMonitor);
  if(SecondMonitor)ImGui::TextWrapped("Move o overlay para um monitor secundario mantendo a resolucao do jogo. Sem outro monitor, permanece no jogo.");
  Custom::CheckBox("Reticulo central",&g_Config.Aimbot->Crosshair);ImGui::Combo("Tipo do reticulo",&CrossType,"Cruz\0Ponto\0Circulo\0Cruz com ponto\0");
  ImGui::SliderFloat("Tamanho do reticulo",&CrossSize,1.f,30.f,"%.0f");ImGui::SliderFloat("Espessura do reticulo",&CrossWidth,0.5f,5.f,"%.1f");ImGui::ColorEdit4("Cor do reticulo",&CrossColor.x);
 }
 if(section==6){
  ImGui::TextDisabled("Carro / ESP adicional");Custom::CheckBox("Marcador nos veiculos",&VehicleMarker);Custom::CheckBox("Ignorar veiculos ocupados",&IgnoreOccupied);
 }
 if(section==7){
  ImGui::TextDisabled("Ambiente / horario local");
  ImGui::SliderInt("Hora",&ClockHour,0,23,"%02d");
  ImGui::SliderInt("Minuto",&ClockMinute,0,59,"%02d");
  if(Custom::Button("Aplicar horario local",ImVec2(-1,30),0))WorldClock::Request();
  Custom::CheckBox("Manter horario local",&FreezeClock);
  if(Custom::Button("Manha (08:00)",ImVec2(200,30),0)){ClockHour=8;ClockMinute=0;WorldClock::Request();}
  ImGui::SameLine();
  if(Custom::Button("Meio-dia (12:00)",ImVec2(200,30),0)){ClockHour=12;ClockMinute=0;WorldClock::Request();}
  if(Custom::Button("Entardecer (18:30)",ImVec2(200,30),0)){ClockHour=18;ClockMinute=30;WorldClock::Request();}
  ImGui::SameLine();
  if(Custom::Button("Meia-noite (00:00)",ImVec2(200,30),0)){ClockHour=0;ClockMinute=0;WorldClock::Request();}
  ImGui::TextWrapped("Ao desligar, o relogio volta a avancar. O servidor pode ressincronizar o horario.");
  ImGui::TextWrapped("%s",WorldClock::status);
 }
 ImGui::PopID();ImGui::EndChild();
}
}
