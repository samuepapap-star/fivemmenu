#pragma once
#include "NativeApi.hpp"
#include <cmath>
#include <initializer_list>
namespace MenuBridge {
namespace TransferN {
constexpr std::uint64_t GetEntityCoords=0x3FEF770D40960D5AULL;
constexpr std::uint64_t SetEntityCoordsNoOffset=0x239A3351AC1DA385ULL;
constexpr std::uint64_t SetEntityVelocity=0x1C99BB7B6E96D16FULL;
constexpr std::uint64_t NetworkHasControlOfEntity=0x01BF60A500E28887ULL;
constexpr std::uint64_t NetworkGetEntityIsNetworked=0xC7827959479DCC78ULL;
constexpr std::uint64_t GetVehicleNumberOfPassengers=0x24CB2137731FFE89ULL;
constexpr std::uint64_t IsVehicleSeatFree=0x22AC59A870E6A669ULL;
constexpr std::uint64_t ActivatePhysics=0x710311ADF0E20730ULL;
constexpr std::uint64_t PlayerActive=0xB8DFD30D6973E135,PlayerPedByIndex=0x43A66C31C68491C0,EntityExists=0x7239B21A38F536BA;
}
class NativeVehicleTransfer {
 NativeApi& api;Command command{};bool pending=false;std::uint64_t verifyAt=0;
 struct Move{int vehicle;Vec goal;};std::vector<Move> moves;
 int uncontrolled=0,occupied=0,outside=0,confirmed=0,rejected=0;
 std::uint64_t Call(std::uint64_t h,std::initializer_list<std::uint64_t> a={}){return api.Call(h,std::vector<std::uint64_t>(a));}
 Vec Position(int entity){return api.Vector(TransferN::GetEntityCoords,{static_cast<std::uint64_t>(entity),0});}
 int Player(){
  int index=static_cast<int>(Call(Hash("GET_PLAYER_FROM_SERVER_ID"),{command.targetServerId}));
  if(index<0||index>255||!Call(TransferN::PlayerActive,{static_cast<std::uint64_t>(index)}))return 0;
  int ped=static_cast<int>(Call(TransferN::PlayerPedByIndex,{static_cast<std::uint64_t>(index)}));
  return ped>0&&Call(TransferN::EntityExists,{static_cast<std::uint64_t>(ped)})?ped:0;
 }
 static float Distance2(Vec a,Vec b){float x=a.x-b.x,y=a.y-b.y,z=a.z-b.z;return x*x+y*y+z*z;}
 void Finish(bool ok,const std::string& text){
  reply={};reply.id=command.id;reply.success=ok?1u:0u;reply.value=static_cast<std::uint32_t>(confirmed);Copy(reply.message,sizeof(reply.message),text);pending=false;hasReply=true;
 }
 std::string Summary()const{return "Confirmados localmente: "+std::to_string(confirmed)+"/"+std::to_string(moves.size())+". Sem controle: "+std::to_string(uncontrolled)+". Ocupados: "+std::to_string(occupied)+". Fora do alcance: "+std::to_string(outside)+". Nao confirmados: "+std::to_string(rejected)+".";}
 struct Enumerator{
  NativeApi& api;int handle;
  ~Enumerator(){if(handle>=0)try{api.Call(Hash("END_FIND_VEHICLE"),{static_cast<std::uint64_t>(handle)});}catch(...){}}
 };
public:
 Reply reply{};bool hasReply=false;
 explicit NativeVehicleTransfer(NativeApi& native):api(native){}
 bool Pending()const{return pending;}
 void Reset(){pending=false;hasReply=false;moves.clear();}
 void Abort(){if(pending)Finish(false,"Verificacao cancelada; movimentos ja executados nao sao desfeitos.");}
 void Submit(const Command& c,const Frame& f,std::uint64_t now){
  if(pending)throw std::runtime_error("Transferencia em andamento");
  command=c;hasReply=false;moves.clear();uncontrolled=occupied=outside=confirmed=rejected=0;
  if(!Valid(c)||c.action!=Action::TransferVehicles||!Fresh(f,now)){Finish(false,"Pedido de veiculos invalido");return;}
  int target=Player();if(!target){Finish(false,"Jogador selecionado nao esta carregado");return;}
  Vec destination=Position(target);if(!Sane(destination)){Finish(false,"Posicao do jogador indisponivel");return;}
  int vehicle=0;int find=static_cast<int>(Call(Hash("FIND_FIRST_VEHICLE"),{reinterpret_cast<std::uint64_t>(&vehicle)}));Enumerator iterator{api,find};
  for(int scanned=0;find>=0&&vehicle>0&&scanned<512&&moves.size()<10;scanned++){
   auto id=static_cast<std::uint64_t>(vehicle);
   if(Call(TransferN::EntityExists,{id})){
    Vec current=Position(vehicle);
    if(!Sane(current)||Distance2(current,f.position)>200.f*200.f)outside++;
    else if(Call(TransferN::GetVehicleNumberOfPassengers,{id})||!Call(TransferN::IsVehicleSeatFree,{id,static_cast<std::uint64_t>(-1),1}))occupied++;
    else if(Call(TransferN::NetworkGetEntityIsNetworked,{id})&&!Call(TransferN::NetworkHasControlOfEntity,{id}))uncontrolled++;
    else{
     float angle=static_cast<float>(moves.size()+1)*2.399963f;
     Vec goal{destination.x+std::cos(angle)*5.f,destination.y+std::sin(angle)*5.f,destination.z+((c.flags&1u)?15.f:0.5f)};
     Call(TransferN::SetEntityCoordsNoOffset,{id,Float(goal.x),Float(goal.y),Float(goal.z),0,0,1});
     Call(TransferN::ActivatePhysics,{id});
     Call(TransferN::SetEntityVelocity,{id,Float(0),Float(0),Float((c.flags&1u)?-5.f:0.f)});
     moves.push_back({vehicle,goal});
    }
   }
   if(moves.size()>=10||!Call(Hash("FIND_NEXT_VEHICLE"),{static_cast<std::uint64_t>(find),reinterpret_cast<std::uint64_t>(&vehicle)}))break;
  }
  if(moves.empty()){Finish(false,Summary());return;}
  pending=true;verifyAt=now+100;
 }
 void Tick(const Frame& f,std::uint64_t now,std::uint64_t cancelled){
  if(!pending)return;
  if(cancelled==command.id||!Fresh(f,now)){Abort();return;}
  if(now<verifyAt)return;
  for(const auto& move:moves){
   auto id=static_cast<std::uint64_t>(move.vehicle);
   if(Call(TransferN::EntityExists,{id})){Vec current=Position(move.vehicle);if(Sane(current)&&Distance2(current,move.goal)<=49.f){confirmed++;continue;}}
   rejected++;
  }
  Finish(confirmed>0,Summary());
 }
};
}

