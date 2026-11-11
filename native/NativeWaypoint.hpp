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
// DO_SCREEN_FADE_OUT(ms) — escurece a tela
constexpr std::uint64_t ScreenFadeOut=0xD82958B4FD20EAB4ULL;
// DO_SCREEN_FADE_IN(ms)  — clareia a tela
constexpr std::uint64_t ScreenFadeIn =0xD4E8E24955024033ULL;
}
class NativeWaypoint {
 NativeApi& api;
 Command command{};
 // phase: 0=inativo, 1=aguardando fade escurecer, 2=teleportado aguardando clarear
 int phase=0;
 std::uint64_t phaseStart=0;
 int ped=0,blip=0;
 Vec destination{};
 std::uint64_t Call(std::uint64_t hash,std::initializer_list<std::uint64_t> args={}) {
  return api.Call(hash,std::vector<std::uint64_t>(args));
 }
 void Finish(bool success,const char* message){
  phase=0;
  reply={};reply.id=command.id;reply.success=success?1u:0u;
  Copy(reply.message,sizeof(reply.message),message);hasReply=true;
 }
 void DoTeleport(){
  const auto entity=static_cast<std::uint64_t>(ped);
  float ground=std::numeric_limits<float>::quiet_NaN();
  const bool ok=Call(WaypointN::Ground,{Float(destination.x),Float(destination.y),Float(1000.f),
      reinterpret_cast<std::uint64_t>(&ground),0,0})!=0;
  const float z=(ok&&std::isfinite(ground)&&ground>-500.f&&ground<999.f)?ground:0.f;
  Call(WaypointN::SetCoords,{entity,Float(destination.x),Float(destination.y),Float(z),0,0,1});
 }
public:
 Reply reply{};bool hasReply=false;
 explicit NativeWaypoint(NativeApi& native):api(native){}
 bool Pending()const{return phase!=0;}
 void Reset(){phase=0;hasReply=false;ped=blip=0;}
 void Abort(const char* reason){
  if(phase!=0){
   Call(WaypointN::ScreenFadeIn,{0});
   Finish(false,reason);
  }
 }
 void Submit(const Command& value,const Frame& frame,std::uint64_t now){
  if(phase!=0)throw std::runtime_error("Teleporte ja em andamento");
  command=value;hasReply=false;
  if(value.action!=Action::TeleportWaypoint||!Valid(value)||!Fresh(frame,now)){Finish(false,"Pedido de teleporte invalido");return;}
  blip=static_cast<int>(Call(WaypointN::FirstBlip,{8}));
  if(blip<=0||!Call(WaypointN::BlipExists,{static_cast<std::uint64_t>(blip)})){Finish(false,"Marque um ponto no mapa primeiro");return;}
  destination=api.Vector(WaypointN::BlipCoords,{static_cast<std::uint64_t>(blip)});
  if(!Sane(destination)){Finish(false,"Coordenadas do marcador invalidas");return;}
  ped=static_cast<int>(Call(N::PlayerPed));
  // Escurece a tela 300ms para esconder a queda do teleporte
  Call(WaypointN::ScreenFadeOut,{300});
  phase=1;phaseStart=now;
 }
 void Tick(const Frame& frame,std::uint64_t now,std::uint64_t cancelled){
  if(phase==0)return;
  if(cancelled==command.id||!Fresh(frame,now)){
   Call(WaypointN::ScreenFadeIn,{0});
   Finish(false,"Teleporte cancelado");return;
  }
  if(phase==1){
   // Aguarda 350ms (tela escura) antes de mover o ped
   if(now-phaseStart<350)return;
   DoTeleport();
   phase=2;phaseStart=now;
  } else if(phase==2){
   // Aguarda mais 300ms com tela escura apos teleportar, depois clareia
   if(now-phaseStart<300)return;
   Call(WaypointN::ScreenFadeIn,{600});
   Finish(true,"Teleporte confirmado");
  }
 }
};
}
