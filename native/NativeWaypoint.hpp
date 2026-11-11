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
constexpr std::uint64_t RequestCollision=0x07503F7948F491A7ULL;
}
class NativeWaypoint {
 NativeApi& api;
 Command command{};
 bool pending=false;
 std::uint64_t started=0,nextProbe=0;
 int ped=0,blip=0,probes=0;
 bool scene=false;
 Vec destination{};
 std::uint64_t Call(std::uint64_t hash,std::initializer_list<std::uint64_t> args={}) {
  return api.Call(hash,std::vector<std::uint64_t>(args));
 }
 void StopScene(){if(scene){scene=false;Call(WaypointN::SceneStop);}}
 void Finish(bool success,const char* message){
  StopScene();pending=false;
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
 bool Pending()const{return pending;}
 void Reset(){StopScene();pending=false;hasReply=false;ped=blip=probes=0;}
 void Abort(const char* reason){if(pending)Finish(false,reason);}
 void Submit(const Command& value,const Frame& frame,std::uint64_t now){
  if(pending)throw std::runtime_error("Teleporte ja em andamento");
  command=value;hasReply=false;probes=0;scene=false;
  if(value.action!=Action::TeleportWaypoint||!Valid(value)||!Fresh(frame,now)){Finish(false,"Pedido de teleporte invalido");return;}
  blip=static_cast<int>(Call(WaypointN::FirstBlip,{8}));
  if(blip<=0||!Call(WaypointN::BlipExists,{static_cast<std::uint64_t>(blip)})){Finish(false,"Marque um ponto no mapa primeiro");return;}
  destination=api.Vector(WaypointN::BlipCoords,{static_cast<std::uint64_t>(blip)});
  if(!Sane(destination)){Finish(false,"Coordenadas do marcador invalidas");return;}
  ped=static_cast<int>(Call(N::PlayerPed));
  // Forca carregamento do chunk no destino
  Call(WaypointN::RequestCollision,{Float(destination.x),Float(destination.y),Float(destination.z)});
  // Inicia streaming de cena para carregar terreno mais rapido
  if(!Call(WaypointN::SceneActive))
   scene=Call(WaypointN::SceneStart,{Float(destination.x),Float(destination.y),Float(destination.z),Float(500.f),0})!=0;
  // Tenta probe imediato
  const float g=ProbeGround();
  if(std::isfinite(g)){TeleportTo(g);StopScene();Finish(true,"Teleporte confirmado");return;}
  // Aguarda no Tick
  pending=true;started=now;nextProbe=now;
 }
 void Tick(const Frame& frame,std::uint64_t now,std::uint64_t cancelled){
  if(!pending)return;
  if(cancelled==command.id||!Fresh(frame,now)){Finish(false,"Teleporte cancelado");return;}
  if(now<nextProbe)return;
  nextProbe=now+80;++probes;
  Call(WaypointN::RequestCollision,{Float(destination.x),Float(destination.y),Float(destination.z)});
  const float g=ProbeGround();
  if(std::isfinite(g)){TeleportTo(g);StopScene();Finish(true,"Teleporte confirmado");return;}
  // Timeout 6s: keepOnGround=1 com Z=0
  if(now-started>=6000){TeleportTo(0.f);StopScene();Finish(true,"Teleporte confirmado");}
 }
};
}
