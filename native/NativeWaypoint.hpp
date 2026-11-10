#pragma once
#include "NativeActions.hpp"
#include <limits>

namespace MenuBridge {
namespace WaypointN {
// citizenfx/natives: HUD, STREAMING, MISC, ENTITY and VEHICLE.
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
 std::uint64_t started=0,nextProbe=0,confirmAt=0;
 int ped=0,entity=0,blip=0,probes=0;
 Vec destination{};
 std::uint64_t Call(std::uint64_t hash,std::initializer_list<std::uint64_t> args={}) {
  return api.Call(hash,std::vector<std::uint64_t>(args));
 }
 bool Exists(int id) {return id>0 && Call(N::EntityExists,{static_cast<std::uint64_t>(id)})!=0;}
 int CurrentEntity(int player) {
  const auto p=static_cast<std::uint64_t>(player);
  if(!Exists(player)||Call(N::Dead,{p,0})||Call(N::Attached,{p}))return 0;
  if(!Call(N::InVehicle,{p,0}))return player;
  const auto vehicle=Call(N::CurrentVehicle,{p,0});
  if(!vehicle||vehicle>0x7fffffffULL||!Exists(static_cast<int>(vehicle))||
     Call(WaypointN::Driver,{vehicle,static_cast<std::uint64_t>(-1),0})!=p||
     Call(N::Passengers,{vehicle})!=0)return 0;
  return static_cast<int>(vehicle);
 }
 bool Controlled() {
  const auto id=static_cast<std::uint64_t>(entity);
  return !Call(N::Networked,{id})||Call(N::HasControl,{id});
 }
 void StopScene() {
  if(scene){scene=false;Call(WaypointN::SceneStop);}
 }
 void Finish(bool success,const char* message) {
  pending=false;
  StopScene();
  reply={};reply.id=command.id;reply.success=success?1u:0u;
  Copy(reply.message,sizeof(reply.message),message);hasReply=true;
 }
 bool SameWaypoint() {
  if(Call(WaypointN::FirstBlip,{8})!=static_cast<std::uint64_t>(blip)||
     !Call(WaypointN::BlipExists,{static_cast<std::uint64_t>(blip)}))return false;
  const auto current=api.Vector(WaypointN::BlipCoords,{static_cast<std::uint64_t>(blip)});
  return Sane(current)&&std::fabs(current.x-destination.x)<0.1f&&std::fabs(current.y-destination.y)<0.1f;
 }
public:
 Reply reply{};bool hasReply=false;
 explicit NativeWaypoint(NativeApi& native):api(native){}
 bool Pending()const{return pending;}
 // Session reset invalidates old handles and the game's streaming scene.
 void Reset(){pending=false;scene=false;moved=false;hasReply=false;ped=entity=blip=probes=0;}
 void Abort(const char* reason){if(pending||scene)Finish(false,reason);}
 void Submit(const Command& value,const Frame& frame,std::uint64_t now) {
  if(pending)throw std::runtime_error("Teleporte ja em andamento");
  command=value;hasReply=false;moved=false;probes=0;
  if(value.action!=Action::TeleportWaypoint||!Valid(value)||!Fresh(frame,now)){Finish(false,"Pedido de teleporte invalido");return;}
  pending=true;started=now;nextProbe=now;
  ped=static_cast<int>(Call(N::PlayerPed));entity=CurrentEntity(ped);
  if(!entity){Finish(false,"Use a pe ou como motorista sem passageiros; personagem vivo e desvinculado");return;}
  if(!Controlled()){Finish(false,"Sem controle local; teleporte nao aplicado");return;}
  blip=static_cast<int>(Call(WaypointN::FirstBlip,{8}));
  if(blip<=0||!Call(WaypointN::BlipExists,{static_cast<std::uint64_t>(blip)})){Finish(false,"Marque um ponto no mapa primeiro");return;}
  destination=api.Vector(WaypointN::BlipCoords,{static_cast<std::uint64_t>(blip)});
  if(!Sane(destination)){Finish(false,"Coordenadas do marcador invalidas");return;}
  if(Call(WaypointN::SceneActive)){Finish(false,"Outro carregamento de mapa esta em andamento");return;}
  // Do not move or freeze the player to probe terrain, and do not replace an existing scene.
  scene=Call(WaypointN::SceneStart,{Float(destination.x),Float(destination.y),Float(destination.z),Float(1500.f),0})!=0;
  if(!scene)Finish(false,"O jogo recusou carregar o destino; teleporte nao aplicado");
 }
 void Tick(const Frame& frame,std::uint64_t now,std::uint64_t cancelled) {
  if(!pending)return;
  if(cancelled==command.id||!Fresh(frame,now)||now<started||now-started>=5000){
   Finish(false,moved?"Teleporte sem confirmacao; nenhuma nova tentativa":"Teleporte cancelado ou terreno indisponivel");return;
  }
  if(Call(N::PlayerPed)!=static_cast<std::uint64_t>(ped)||CurrentEntity(ped)!=entity||!Controlled()){
   Finish(false,"Personagem, veiculo ou controle mudou; teleporte interrompido");return;
  }
  if(moved){
   if(now<confirmAt)return;
   const auto current=api.Vector(N::EntityCoords,{static_cast<std::uint64_t>(entity),0});
   const float dx=current.x-destination.x,dy=current.y-destination.y,dz=current.z-destination.z;
   const bool confirmed=Sane(current)&&dx*dx+dy*dy+dz*dz<=25.f;
   Finish(confirmed,confirmed?"Teleporte confirmado localmente":"O jogo nao confirmou o destino; sem repetir ou contrariar correcoes");return;
  }
  if(!SameWaypoint()){Finish(false,"Marcador alterado ou removido; teleporte cancelado");return;}
  if(now<nextProbe)return;
  nextProbe=now+100;
  if(++probes>50){Finish(false,"Limite de consultas ao terreno atingido");return;}
  if(!Call(WaypointN::SceneLoaded))return;
  float ground=std::numeric_limits<float>::quiet_NaN();
  if(!Call(WaypointN::Ground,{Float(destination.x),Float(destination.y),Float(1500.f),
      reinterpret_cast<std::uint64_t>(&ground),0,0}))return;
  if(!std::isfinite(ground)||ground< -500.f||ground>1499.f){Finish(false,"Altura do terreno invalida");return;}
  destination.z=ground+1.f;
  Call(WaypointN::SetCoords,{static_cast<std::uint64_t>(entity),Float(destination.x),Float(destination.y),
      Float(destination.z),1,1,1});
  moved=true;confirmAt=now+250;
 }
};
}
