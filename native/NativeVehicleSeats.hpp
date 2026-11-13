#pragma once
#include "NativeApi.hpp"
#include <algorithm>
namespace MenuBridge {
namespace SeatN {
constexpr std::uint64_t PlayerPed=0xD80958FC74E988A6, Exists=0x7239B21A38F536BA,
 Occupant=0xBB40DD2270B65366, Networked=0xC7827959479DCC78, Control=0x01BF60A500E28887,
 Coords=0x3FEF770D40960D5A, Warp=0xF75B0D629E1C063D, Leave=0xD3DBCE61A490BE02;
}
class VehicleSeats {
 NativeApi& api;Command request{};std::uint64_t vehicle=0,occupant=0,seat=0,deadline=0;bool pending=false;
 std::uint64_t Call(std::uint64_t hash,std::initializer_list<std::uint64_t> args){return api.Call(hash,std::vector<std::uint64_t>(args));}
 bool Controlled(std::uint64_t entity){return !Call(SeatN::Networked,{entity})||Call(SeatN::Control,{entity});}
 void Finish(bool success,const char* message){reply={};reply.id=request.id;reply.success=success?1u:0u;Copy(reply.message,sizeof(reply.message),message);pending=false;}
public:
 Reply reply{};
 explicit VehicleSeats(NativeApi& value):api(value){}
 void Reset(){pending=false;vehicle=0;occupant=0;}
 bool Pending()const{return pending;}
 void Submit(const Command& command,std::uint64_t now){
  Reset();request=command;seat=command.flags?0:static_cast<std::uint64_t>(-1);
  const auto ped=Call(SeatN::PlayerPed,{});
  if(!ped||!Call(SeatN::Exists,{ped})){Finish(false,"Personagem indisponivel");return;}
  int candidate=0;
  const int iterator=static_cast<int>(Call(Hash("FIND_FIRST_VEHICLE"),{reinterpret_cast<std::uint64_t>(&candidate)}));
  struct Guard{NativeApi& api;int handle;~Guard(){if(handle>=0)try{api.Call(Hash("END_FIND_VEHICLE"),{static_cast<std::uint64_t>(handle)});}catch(...){}}} guard{api,iterator};
  for(int count=0;iterator>=0&&candidate>0&&count<4096;count++){
   if(Call(Hash("GET_ENTITY_ADDRESS"),{static_cast<std::uint64_t>(candidate)})==command.targetEntityAddress){vehicle=static_cast<std::uint64_t>(candidate);break;}
   if(!Call(Hash("FIND_NEXT_VEHICLE"),{static_cast<std::uint64_t>(iterator),reinterpret_cast<std::uint64_t>(&candidate)}))break;
  }
  if(!vehicle||!Call(SeatN::Exists,{vehicle})){Finish(false,"Veiculo selecionado nao esta carregado");return;}
  const auto p=api.Vector(SeatN::Coords,{ped,0}),v=api.Vector(SeatN::Coords,{vehicle,0});
  const float x=p.x-v.x,y=p.y-v.y,z=p.z-v.z;
  if(!Sane(p)||!Sane(v)||x*x+y*y+z*z>900.f){Finish(false,"Aproxime-se do veiculo selecionado (ate 30 m)");return;}
  occupant=Call(SeatN::Occupant,{vehicle,seat,0});
  if(command.action==Action::EnterDriver){
   if(occupant==ped){Finish(true,"Voce ja esta no P1");return;}
   if(occupant){Finish(false,"P1 ocupado; retire o motorista antes de entrar");return;}
   Call(SeatN::Warp,{ped,vehicle,seat});occupant=ped;
  }else{
   if(!occupant){Finish(false,command.flags?"Nenhum ocupante no P2":"Nenhum ocupante no P1");return;}
   if(!Call(SeatN::Exists,{occupant})){Finish(false,"Ocupante deixou de estar disponivel");return;}
   if(!Controlled(occupant)){
    const bool player=Call(0x12534C348C6CB68BULL,{occupant})!=0;
    Finish(false,player?"Outro jogador controla este personagem. Retirada requer uma acao autorizada pelo servidor.":"Sem controle local do NPC; retirada nao aplicada");return;
   }
   Call(SeatN::Leave,{occupant,vehicle,16});
  }
  pending=true;deadline=now+4000;
 }
 void Tick(std::uint64_t now){
  if(!pending)return;
  if(!Call(SeatN::Exists,{vehicle})){Finish(false,"Veiculo deixou de estar disponivel");return;}
  if(Call(Hash("GET_ENTITY_ADDRESS"),{vehicle})!=request.targetEntityAddress){Finish(false,"Veiculo selecionado mudou; acao cancelada");return;}
  const auto current=Call(SeatN::Occupant,{vehicle,seat,0});
  if(request.action==Action::EnterDriver){if(current==occupant){Finish(true,"Entrada no P1 confirmada");return;}}
  else if(!current&&Call(SeatN::Exists,{occupant})&&!Call(0xA3EE4A07279BB9DBULL,{occupant,vehicle,0})){
   Finish(true,request.flags?"Retirada do P2 confirmada":"Retirada do P1 confirmada");return;
  }
  if(now>=deadline)Finish(false,"O jogo nao confirmou a acao; o servidor pode ter recusado");
 }
};
}
