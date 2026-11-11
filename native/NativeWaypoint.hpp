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
constexpr std::uint64_t ScreenFadeOut=0xD82958B4FD20EAB4ULL;
constexpr std::uint64_t ScreenFadeIn =0xD4E8E24955024033ULL;
// REQUEST_COLLISION_AT_COORD — forca o engine a carregar colisao na area
constexpr std::uint64_t RequestCollision=0x07503F7948F491A7ULL;
// LOAD_ALL_OBJECTS_NOW — forca carregamento imediato
constexpr std::uint64_t LoadObjects=0x4448EB75B4904BDB ULL;
}
class NativeWaypoint {
 NativeApi& api;
 Command command{};
 int phase=0; // 0=inativo,1=fade out,2=probe+streaming,3=fade in
 std::uint64_t phaseStart=0,nextProbe=0;
 int ped=0,blip=0,probes=0;
 bool scene=false;
 Vec destination{};
 std::uint64_t Call(std::uint64_t hash,std::initializer_list<std::uint64_t> args={}) {
  return api.Call(hash,std::vector<std::uint64_t>(args));
 }
 void StopScene(){if(scene){scene=false;Call(WaypointN::SceneStop);}}
 void Finish(bool success,const char* message){
  StopScene();phase=0;
  reply={};reply.id=command.id;reply.success=success?1u:0u;
  Copy(reply.message,sizeof(reply.message),message);hasReply=true;
 }
 float ProbeGround(){
  float g=std::numeric_limits<float>::quiet_NaN();
  const bool ok=Call(WaypointN::Ground,{Float(destination.x),Float(destination.y),Float(1000.f),
      reinterpret_cast<std::uint64_t>(&g),0,0})!=0;
  return (ok&&std::isfinite(g)&&g>-500.f&&g<999.f)?g:std::numeric_limits<float>::quiet_NaN();
 }
 void TeleportTo(float z){
  Call(WaypointN::SetCoords,{static_cast<std::uint64_t>(ped),
      Float(destination.x),Float(destination.y),Float(z),0,0,1});
 }
public:
 Reply reply{};bool hasReply=false;
 explicit NativeWaypoint(NativeApi& native):api(native){}
 bool Pending()const{return phase!=0;}
 void Reset(){StopScene();phase=0;hasReply=false;ped=blip=probes=0;}
 void Abort(const char* reason){
  if(phase!=0){Call(WaypointN::ScreenFadeIn,{0});Finish(false,reason);}
 }
 void Submit(const Command& value,const Frame& frame,std::uint64_t now){
  if(phase!=0)throw std::runtime_error("Teleporte ja em andamento");
  command=value;hasReply=false;probes=0;scene=false;
  if(value.action!=Action::TeleportWaypoint||!Valid(value)||!Fresh(frame,now)){Finish(false,"Pedido de teleporte invalido");return;}
  blip=static_cast<int>(Call(WaypointN::FirstBlip,{8}));
  if(blip<=0||!Call(WaypointN::BlipExists,{static_cast<std::uint64_t>(blip)})){Finish(false,"Marque um ponto no mapa primeiro");return;}
  destination=api.Vector(WaypointN::BlipCoords,{static_cast<std::uint64_t>(blip)});
  if(!Sane(destination)){Finish(false,"Coordenadas do marcador invalidas");return;}
  ped=static_cast<int>(Call(N::PlayerPed));
  // Forca carregamento do chunk imediatamente
  Call(WaypointN::RequestCollision,{Float(destination.x),Float(destination.y),Float(destination.z)});
  Call(WaypointN::LoadObjects);
  // Inicia streaming de cena para carregar o terreno mais rapido
  if(!Call(WaypointN::SceneActive))
   scene=Call(WaypointN::SceneStart,{Float(destination.x),Float(destination.y),Float(destination.z),Float(500.f),0})!=0;
  Call(WaypointN::ScreenFadeOut,{300});
  phase=1;phaseStart=now;nextProbe=now;
 }
 void Tick(const Frame& frame,std::uint64_t now,std::uint64_t cancelled){
  if(phase==0)return;
  if(cancelled==command.id||!Fresh(frame,now)){
   Call(WaypointN::ScreenFadeIn,{0});Finish(false,"Teleporte cancelado");return;
  }
  if(phase==1){
   if(now-phaseStart<350)return;
   // Probe rapido enquanto tela ainda escurece
   const float g=ProbeGround();
   if(std::isfinite(g)){TeleportTo(g);StopScene();phase=3;phaseStart=now;return;}
   phase=2;phaseStart=now;nextProbe=now;
   return;
  }
  if(phase==2){
   // Probe a cada 80ms com tela preta; timeout 6s
   if(now<nextProbe)return;
   nextProbe=now+80;++probes;
   // Continua forcando carregamento a cada probe
   Call(WaypointN::RequestCollision,{Float(destination.x),Float(destination.y),Float(destination.z)});
   const float g=ProbeGround();
   if(std::isfinite(g)){TeleportTo(g);StopScene();phase=3;phaseStart=now;return;}
   if(now-phaseStart>=6000){
    // Timeout: usa keepOnGround=1 com Z=0, o engine coloca no chao
    TeleportTo(0.f);StopScene();phase=3;phaseStart=now;
   }
   return;
  }
  if(phase==3){
   if(now-phaseStart<400)return;
   Call(WaypointN::ScreenFadeIn,{500});
   Finish(true,"Teleporte confirmado");
  }
 }
};
}
