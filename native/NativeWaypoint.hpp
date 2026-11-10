#pragma once
#include "NativeActions.hpp"
#include <limits>

namespace MenuBridge {
namespace WaypointN {
constexpr std::uint64_t FirstBlip=0x1BEDE233E6CD2A1FULL, BlipExists=0xA6DB27D19ECBB7DAULL,
 BlipCoords=0xFA7C7F0AADF25D09ULL, Ground=0xC906A7DAB05C8D2BULL,
 SceneActive=0xA41A05B6CB741B85ULL, SceneStart=0xACCFB4ACF53551B0ULL,
 SceneLoaded=0x01B8247A7A8B9AD1ULL, SceneStop=0xC197616D221FF4A4ULL,
 SetCoords=0x239A3351AC1DA385ULL, Driver=0xBB40DD2270B65366ULL;
// Colisao/fisica
constexpr std::uint64_t FreezeEntity=0x428CA6DBD1094446ULL; // FREEZE_ENTITY_POSITION
}
// Fase do teleporte
enum class TpPhase:std::uint8_t{Idle,DropPhase,GroundPhase};
class NativeWaypoint {
 NativeApi& api;
 Command command{};
 TpPhase phase=TpPhase::Idle;
 std::uint64_t started=0,nextProbe=0;
 int ped=0,blip=0,probes=0;
 Vec destination{};
 std::uint64_t Call(std::uint64_t hash,std::initializer_list<std::uint64_t> args={}) {
  return api.Call(hash,std::vector<std::uint64_t>(args));
 }
 void Finish(bool success,const char* message){
  phase=TpPhase::Idle;
  reply={};reply.id=command.id;reply.success=success?1u:0u;
  Copy(reply.message,sizeof(reply.message),message);hasReply=true;
 }
 // Teleporta para Z=800 e entra em GroundPhase para buscar o chao real
 void StartDrop(){
  const auto entity=static_cast<std::uint64_t>(ped);
  // Desativa fisica temporariamente para o queda nao ser jogado
  Call(WaypointN::FreezeEntity,{entity,1});
  Call(WaypointN::SetCoords,{entity,Float(destination.x),Float(destination.y),Float(800.f),0,0,0});
  Call(WaypointN::FreezeEntity,{entity,0});
  phase=TpPhase::GroundPhase;
  probes=0;
 }
 void DoFinalTeleport(float z){
  const auto entity=static_cast<std::uint64_t>(ped);
  Call(WaypointN::SetCoords,{entity,Float(destination.x),Float(destination.y),Float(z+1.f),0,0,0});
  Finish(true,"Teleporte confirmado");
 }
public:
 Reply reply{};bool hasReply=false;
 explicit NativeWaypoint(NativeApi& native):api(native){}
 bool Pending()const{return phase!=TpPhase::Idle;}
 void Reset(){phase=TpPhase::Idle;hasReply=false;ped=blip=probes=0;}
 void Abort(const char* reason){if(phase!=TpPhase::Idle)Finish(false,reason);}
 void Submit(const Command& value,const Frame& frame,std::uint64_t now){
  if(phase!=TpPhase::Idle)throw std::runtime_error("Teleporte ja em andamento");
  command=value;hasReply=false;probes=0;
  if(value.action!=Action::TeleportWaypoint||!Valid(value)||!Fresh(frame,now)){Finish(false,"Pedido de teleporte invalido");return;}
  blip=static_cast<int>(Call(WaypointN::FirstBlip,{8}));
  if(blip<=0||!Call(WaypointN::BlipExists,{static_cast<std::uint64_t>(blip)})){Finish(false,"Marque um ponto no mapa primeiro");return;}
  destination=api.Vector(WaypointN::BlipCoords,{static_cast<std::uint64_t>(blip)});
  if(!Sane(destination)){Finish(false,"Coordenadas do marcador invalidas");return;}
  ped=static_cast<int>(Call(N::PlayerPed));
  started=now;nextProbe=now;
  // Tenta probe rapido se terreno ja estiver carregado
  float ground=std::numeric_limits<float>::quiet_NaN();
  const bool gotGround=Call(WaypointN::Ground,{Float(destination.x),Float(destination.y),Float(800.f),
      reinterpret_cast<std::uint64_t>(&ground),0,0})!=0;
  if(gotGround&&std::isfinite(ground)&&ground>-500.f&&ground<799.f){
   phase=TpPhase::DropPhase; // precisa de um tick para entrar em GroundPhase
   DoFinalTeleport(ground);
   phase=TpPhase::Idle;
   return;
  }
  // Terreno nao carregado: inicia fase de queda no Z=800 para forcar o streaming
  phase=TpPhase::DropPhase;
 }
 void Tick(const Frame& frame,std::uint64_t now,std::uint64_t cancelled){
  if(phase==TpPhase::Idle)return;
  if(cancelled==command.id||!Fresh(frame,now)){Finish(false,"Teleporte cancelado");return;}
  if(now-started>=12000){
   // Timeout: teleporta para o Z que tiver (ou 50 como minimo seguro)
   float ground=std::numeric_limits<float>::quiet_NaN();
   Call(WaypointN::Ground,{Float(destination.x),Float(destination.y),Float(800.f),
       reinterpret_cast<std::uint64_t>(&ground),0,0});
   DoFinalTeleport((std::isfinite(ground)&&ground>-500.f)?ground:50.f);
   return;
  }
  if(phase==TpPhase::DropPhase){
   // Coloca o ped em Z=800 para forcar o jogo a carregar o terreno embaixo
   StartDrop();
   nextProbe=now+200; // aguarda 200ms para o streaming reagir
   return;
  }
  // GroundPhase: sonda o chao a cada 150ms enquanto o jogo carrega o chunk
  if(now<nextProbe)return;
  nextProbe=now+150;
  if(++probes>60){
   // Muitas tentativas: usa o que tiver
   float ground=std::numeric_limits<float>::quiet_NaN();
   Call(WaypointN::Ground,{Float(destination.x),Float(destination.y),Float(800.f),
       reinterpret_cast<std::uint64_t>(&ground),0,0});
   DoFinalTeleport((std::isfinite(ground)&&ground>-500.f)?ground:50.f);
   return;
  }
  float ground=std::numeric_limits<float>::quiet_NaN();
  const bool gotGround=Call(WaypointN::Ground,{Float(destination.x),Float(destination.y),Float(800.f),
      reinterpret_cast<std::uint64_t>(&ground),0,0})!=0;
  if(!gotGround||!std::isfinite(ground)||ground<=-500.f||ground>=799.f)return;
  // Terreno encontrado! Teleporta para o Z correto
  DoFinalTeleport(ground);
 }
};
}
