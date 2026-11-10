#pragma once
#include "NativeActions.hpp"

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
 bool pending=false;
 std::uint64_t Call(std::uint64_t hash,std::initializer_list<std::uint64_t> args={}) {
  return api.Call(hash,std::vector<std::uint64_t>(args));
 }
 void Finish(bool success,const char* message) {
  pending=false;
  reply={};reply.id=command.id;reply.success=success?1u:0u;
  Copy(reply.message,sizeof(reply.message),message);hasReply=true;
 }
public:
 Reply reply{};bool hasReply=false;
 explicit NativeWaypoint(NativeApi& native):api(native){}
 bool Pending()const{return pending;}
 void Reset(){pending=false;hasReply=false;}
 void Abort(const char* reason){if(pending)Finish(false,reason);}
 void Submit(const Command& value,const Frame& frame,std::uint64_t now) {
  if(pending)throw std::runtime_error("Teleporte ja em andamento");
  command=value;hasReply=false;
  if(value.action!=Action::TeleportWaypoint||!Valid(value)||!Fresh(frame,now)){Finish(false,"Pedido de teleporte invalido");return;}
  const auto blip=static_cast<int>(Call(WaypointN::FirstBlip,{8}));
  if(blip<=0||!Call(WaypointN::BlipExists,{static_cast<std::uint64_t>(blip)})){Finish(false,"Marque um ponto no mapa primeiro");return;}
  const auto dest=api.Vector(WaypointN::BlipCoords,{static_cast<std::uint64_t>(blip)});
  if(!Sane(dest)){Finish(false,"Coordenadas do marcador invalidas");return;}
  // Teleporta direto, igual ao SetPos de memoria — sem checar veiculo, terreno ou passageiros
  const auto ped=static_cast<std::uint64_t>(Call(N::PlayerPed));
  Call(WaypointN::SetCoords,{ped,Float(dest.x),Float(dest.y),Float(dest.z+1.f),1,1,1});
  Finish(true,"Teleporte aplicado");
 }
 void Tick(const Frame&,std::uint64_t,std::uint64_t){} // sincrono, nada a fazer no tick
};
}
