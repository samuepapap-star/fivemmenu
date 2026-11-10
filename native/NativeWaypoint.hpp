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
}
class NativeWaypoint {
 NativeApi& api;
 Command command{};
 bool pending=false,scene=false,moved=false;
 std::uint64_t started=0,nextProbe=0;
 int ped=0,blip=0,probes=0;
 Vec destination{};
 std::uint64_t Call(std::uint64_t hash,std::initializer_list<std::uint64_t> args={}) {
  return api.Call(hash,std::vector<std::uint64_t>(args));
 }
 void StopScene(){if(scene){scene=false;Call(WaypointN::SceneStop);}}
 void Finish(bool success,const char* message){
  pending=false;StopScene();
  reply={};reply.id=command.id;reply.success=success?1u:0u;
  Copy(reply.message,sizeof(reply.message),message);hasReply=true;
 }
 void DoTeleport(float z){
  const auto entity=static_cast<std::uint64_t>(ped);
  Call(WaypointN::SetCoords,{entity,Float(destination.x),Float(destination.y),Float(z),1,1,1});
  moved=true;
  Finish(true,"Teleporte confirmado");
 }
public:
 Reply reply{};bool hasReply=false;
 explicit NativeWaypoint(NativeApi& native):api(native){}
 bool Pending()const{return pending;}
 void Reset(){pending=false;scene=false;moved=false;hasReply=false;ped=blip=probes=0;}
 void Abort(const char* reason){if(pending||scene)Finish(false,reason);}
 void Submit(const Command& value,const Frame& frame,std::uint64_t now){
  if(pending)throw std::runtime_error("Teleporte ja em andamento");
  command=value;hasReply=false;moved=false;probes=0;
  if(value.action!=Action::TeleportWaypoint||!Valid(value)||!Fresh(frame,now)){Finish(false,"Pedido de teleporte invalido");return;}
  blip=static_cast<int>(Call(WaypointN::FirstBlip,{8}));
  if(blip<=0||!Call(WaypointN::BlipExists,{static_cast<std::uint64_t>(blip)})){Finish(false,"Marque um ponto no mapa primeiro");return;}
  destination=api.Vector(WaypointN::BlipCoords,{static_cast<std::uint64_t>(blip)});
  if(!Sane(destination)){Finish(false,"Coordenadas do marcador invalidas");return;}
  ped=static_cast<int>(Call(N::PlayerPed));
  pending=true;started=now;nextProbe=now;
  // Tenta probe rapido de terreno sem cena primeiro
  float ground=std::numeric_limits<float>::quiet_NaN();
  const bool gotGround=Call(WaypointN::Ground,{Float(destination.x),Float(destination.y),Float(1500.f),
      reinterpret_cast<std::uint64_t>(&ground),0,0})!=0;
  if(gotGround&&std::isfinite(ground)&&ground>-500.f&&ground<1499.f){
   DoTeleport(ground+1.f);return;
  }
  // Terreno nao disponivel: inicia cena de streaming e aguarda no Tick
  scene=!Call(WaypointN::SceneActive)&&
        Call(WaypointN::SceneStart,{Float(destination.x),Float(destination.y),Float(destination.z),Float(1500.f),0})!=0;
  if(!scene){
   // Sem cena disponivel: teleporta com Z seguro
   DoTeleport(100.f);
  }
 }
 void Tick(const Frame& frame,std::uint64_t now,std::uint64_t cancelled){
  if(!pending||moved)return;
  if(cancelled==command.id||!Fresh(frame,now)||now<started||now-started>=10000){
   DoTeleport(100.f);return; // fallback com Z seguro ao inves de falhar
  }
  if(now<nextProbe)return;
  nextProbe=now+100;
  if(++probes>100){DoTeleport(100.f);return;}
  if(scene&&!Call(WaypointN::SceneLoaded))return;
  float ground=std::numeric_limits<float>::quiet_NaN();
  const bool gotGround=Call(WaypointN::Ground,{Float(destination.x),Float(destination.y),Float(1500.f),
      reinterpret_cast<std::uint64_t>(&ground),0,0})!=0;
  if(!gotGround||!std::isfinite(ground)){return;}
  if(ground<-500.f||ground>1499.f)ground=100.f;
  DoTeleport(ground+1.f);
 }
};
}
