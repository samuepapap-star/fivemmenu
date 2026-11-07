#pragma once
#include "../src/Bridge/Protocol.hpp"
#include <vector>
#include <initializer_list>
#include <stdexcept>
#include <algorithm>

namespace MenuBridge {
namespace N {
constexpr std::uint64_t PlayerId=0x4F8644AF03D0E0D6,PlayerPed=0xD80958FC74E988A6,
 EntityExists=0x7239B21A38F536BA,EntityModel=0x9F47B058362C84B5,InVehicle=0x997ABD671D25CA0B,Attached=0xB346476EF1E64897,Dead=0x5F9532F3B5CC2551,
 ModelValid=0xC0296A2EDF545E92,ModelPed=0x75816577FEA6DAD5,ModelVehicle=0x19AAC8F07BFEC53E,
 RequestModel=0x963D27A58DF860AC,ModelLoaded=0x98A4EB5D89A0C952,ReleaseModel=0xE532F5D78798DAAB,
 SetPlayerModel=0x00A1CADD00108836,ClonePed=0xEF29A16337FACADB,CloneToTarget=0xE952D6431689AD9A,
 Visible=0xEA1C610A04DB6BBB,Collision=0x1A9205C1B9EE827F,Freeze=0x428CA6DBD1094446,
 MissionEntity=0xAD738C3085FE7E11,DeleteEntity=0xAE3CBE5BF394C9C9,
 GetHealth=0xEEF059FAD016D209,SetHealth=0x6B76DC1F3AE6E6A3,GetArmor=0x9483AF821605B1D8,SetArmor=0xCEA04D83135264CC,
 CreateVehicle=0xAF35D0D2583051B0,Heading=0xE83D4F9BA2A38914,EnterVehicle=0xF75B0D629E1C063D,
 PlayerActive=0xB8DFD30D6973E135,PlayerPedByIndex=0x43A66C31C68491C0,
 RequestAnim=0xD3BD40951412FEF6,AnimLoaded=0xD031A9162D01088C,ReleaseAnim=0xF66A602F829E2A06,
 PlayAnim=0xEA47FE3719165B94,PlayingAnim=0x1F0B79228E461EC9,ClearTasks=0xE1EF3C1216AFF2CD,
 RequestWeapon=0x5443438F033E29C3,WeaponLoaded=0x36E353271F0E90EE,ReleaseWeapon=0xAA08EF13F341C8FC,
 ShootBullet=0x867654CBC7606F2C;
}
struct NativeApi {virtual ~NativeApi()=default;virtual std::uint64_t Call(std::uint64_t,const std::vector<std::uint64_t>&)=0;};
class Actions {
 NativeApi& api;Command command{};bool pending=false,animConfirm=false,animRequested=false;std::uint64_t deadline=0,animDeadline=0;
 std::uint32_t model=0;int cloneSource=0,backup=0,expectedPed=0,oldHealth=0,oldArmor=0;
 bool weaponRequested=false;std::uint64_t lastShot=0;
 std::uint64_t Call(std::uint64_t hash,std::initializer_list<std::uint64_t> args={}){return api.Call(hash,std::vector<std::uint64_t>(args));}
 bool Exists(int entity){return entity>0&&Call(N::EntityExists,{static_cast<std::uint64_t>(entity)})!=0;}
 void Delete(int& entity){if(Exists(entity))Call(N::DeleteEntity,{reinterpret_cast<std::uint64_t>(&entity)});entity=0;}
 int Target(std::uint64_t serverId){
  const int index=static_cast<int>(Call(Hash("GET_PLAYER_FROM_SERVER_ID"),{serverId}));
  if(index<0||index>255||!Call(N::PlayerActive,{static_cast<std::uint64_t>(index)}))return 0;
  const int ped=static_cast<int>(Call(N::PlayerPedByIndex,{static_cast<std::uint64_t>(index)}));
  return Exists(ped)?ped:0;
 }
 void EnsureBackup(int ped){
  if(Exists(backup))return;
  backup=static_cast<int>(Call(N::ClonePed,{static_cast<std::uint64_t>(ped),Float(0),0,0}));
  if(backup==ped||!Exists(backup)){backup=0;throw std::runtime_error("Nao foi possivel guardar a aparencia original");}
  Call(N::MissionEntity,{static_cast<std::uint64_t>(backup),1,1});
  Call(N::Visible,{static_cast<std::uint64_t>(backup),0,0});
  Call(N::Collision,{static_cast<std::uint64_t>(backup),0,0});
  Call(N::Freeze,{static_cast<std::uint64_t>(backup),1});
 }
 void Release(){
  if(model)Call(N::ReleaseModel,{model});
  if(animRequested)Call(N::ReleaseAnim,{reinterpret_cast<std::uint64_t>(command.text)});
  model=0;pending=false;animConfirm=false;animRequested=false;
 }
 void Finish(bool ok,const std::string& text,std::uint32_t value=0){
  reply={};reply.id=command.id;reply.success=ok?1u:0u;reply.value=value;Copy(reply.message,sizeof(reply.message),text);hasReply=true;Release();
 }
public:
 Reply reply{};bool hasReply=false;int rpgState=0;
 explicit Actions(NativeApi& native):api(native){}
 bool Pending()const{return pending;}
 void Reset(){pending=false;animConfirm=false;animRequested=false;model=0;backup=0;expectedPed=0;weaponRequested=false;hasReply=false;rpgState=0;}
 void Abort(const std::string& message){if(pending)Finish(false,message);}
 void Submit(const Command& value,std::uint64_t now){
  if(pending)throw std::runtime_error("Outra acao esta em andamento");
  command=value;hasReply=false;model=0;animConfirm=false;animRequested=false;
  if(!Valid(value)){Finish(false,"Pedido invalido");return;}
  pending=true;deadline=now+15000;
  int ped=static_cast<int>(Call(N::PlayerPed));
  if(!Exists(ped)){Finish(false,"Personagem indisponivel");return;}
  const bool change=command.action==Action::Model||command.action==Action::Restore||command.action==Action::CloneAppearance;
  if(change&&(Call(N::InVehicle,{static_cast<std::uint64_t>(ped),0})||Call(N::Attached,{static_cast<std::uint64_t>(ped)}))){Finish(false,"Saia do veiculo ou desvincule o personagem antes de trocar o modelo");return;}
  if(command.action!=Action::Connect&&command.action!=Action::StopAll&&command.action!=Action::StopAnimation&&Call(N::Dead,{static_cast<std::uint64_t>(ped),0})){Finish(false,"Espere o personagem estar vivo");return;}
  if(expectedPed&&ped!=expectedPed){Delete(backup);}expectedPed=ped;
  switch(command.action){
   case Action::Model:model=Hash(command.text);break;
   case Action::CloneAppearance:cloneSource=Target(command.targetServerId);if(!cloneSource){Finish(false,"Jogador selecionado nao encontrado");return;}model=static_cast<std::uint32_t>(Call(N::EntityModel,{static_cast<std::uint64_t>(cloneSource)}));break;
   case Action::Restore:if(!Exists(backup)){Finish(false,"Nao ha aparencia original para restaurar");return;}model=static_cast<std::uint32_t>(Call(N::EntityModel,{static_cast<std::uint64_t>(backup)}));break;
   case Action::SpawnVehicle:model=Hash(command.text);break;
   case Action::Animation:Call(N::RequestAnim,{reinterpret_cast<std::uint64_t>(command.text)});animRequested=true;return;
   case Action::StopAnimation:Call(N::ClearTasks,{static_cast<std::uint64_t>(ped)});Finish(true,"Animacao interrompida");return;
   case Action::StopAll:if(weaponRequested)Call(N::ReleaseWeapon,{Hash("WEAPON_RPG")});weaponRequested=false;rpgState=0;Finish(true,"Acoes continuas interrompidas");return;
   case Action::Connect:Finish(true,"Ponte pronta");return;
  }
  if(!Call(N::ModelValid,{model})||!Call(command.action==Action::SpawnVehicle?N::ModelVehicle:N::ModelPed,{model})){
   Finish(false,"Modelo invalido para esta acao");return;
  }
  if(command.action!=Action::SpawnVehicle){EnsureBackup(ped);oldHealth=static_cast<int>(Call(N::GetHealth,{static_cast<std::uint64_t>(ped)}));oldArmor=static_cast<int>(Call(N::GetArmor,{static_cast<std::uint64_t>(ped)}));}
  Call(N::RequestModel,{model});
 }
 void Tick(const Frame& f,std::uint64_t now,std::uint64_t cancelled){
  const bool fresh=Fresh(f,now);
  int ped=static_cast<int>(Call(N::PlayerPed));
  if(expectedPed&&ped!=expectedPed&&!pending){Delete(backup);expectedPed=ped;}
  if(pending){
   if(cancelled==command.id||now>=deadline||!fresh||!Exists(ped)){Finish(false,"Acao cancelada ou tempo de carregamento esgotado");}
   else if(command.action==Action::Animation){
    if(animConfirm){if(Call(N::PlayingAnim,{static_cast<std::uint64_t>(ped),reinterpret_cast<std::uint64_t>(command.text),reinterpret_cast<std::uint64_t>(command.text2),3}))Finish(true,"Animacao confirmada");else if(now>=animDeadline)Finish(false,"Animacao nao confirmada pelo jogo");}
    else if(Call(N::AnimLoaded,{reinterpret_cast<std::uint64_t>(command.text)})){
     Call(N::PlayAnim,{static_cast<std::uint64_t>(ped),reinterpret_cast<std::uint64_t>(command.text),reinterpret_cast<std::uint64_t>(command.text2),Float(8),Float(-8),static_cast<std::uint64_t>(-1),command.flags&1u,Float(0),0,0,0});animConfirm=true;animDeadline=now+2000;
    }
   }else if(Call(N::ModelLoaded,{model})){
    if(command.action==Action::SpawnVehicle){
     const float heading=AsFloat(Call(N::Heading,{static_cast<std::uint64_t>(ped)}));
     int car=static_cast<int>(Call(N::CreateVehicle,{model,Float(f.position.x+f.forward.x*5),Float(f.position.y+f.forward.y*5),Float(f.position.z+0.3f),Float(heading),(command.flags&1u)?1u:0u,0,0}));
     if(!Exists(car))Finish(false,"O jogo nao confirmou a criacao do veiculo");
     else{if(command.flags&2u)Call(N::EnterVehicle,{static_cast<std::uint64_t>(ped),static_cast<std::uint64_t>(car),static_cast<std::uint64_t>(-1)});Finish(true,(command.flags&1u)?"Veiculo criado; sincronizacao depende do servidor":"Veiculo local criado",static_cast<std::uint32_t>(car));}
    }else if(Call(N::InVehicle,{static_cast<std::uint64_t>(ped),0})||Call(N::Attached,{static_cast<std::uint64_t>(ped)})){
     Finish(false,"Troca de modelo cancelada: personagem entrou em veiculo ou foi vinculado");
    }else{
     const int source=command.action==Action::Restore?backup:cloneSource;
     if(command.action==Action::CloneAppearance&&(!Exists(source)||Target(command.targetServerId)!=source)){Finish(false,"Jogador selecionado saiu durante o carregamento");}
     else{
      Call(N::SetPlayerModel,{Call(N::PlayerId),model});ped=static_cast<int>(Call(N::PlayerPed));expectedPed=ped;
      if(!Exists(ped)||Call(N::EntityModel,{static_cast<std::uint64_t>(ped)})!=model)Finish(false,"O jogo nao confirmou a troca de modelo; backup mantido");
      else{
       if(command.action==Action::Restore||command.action==Action::CloneAppearance)Call(N::CloneToTarget,{static_cast<std::uint64_t>(source),static_cast<std::uint64_t>(ped)});
       Call(N::SetHealth,{static_cast<std::uint64_t>(ped),static_cast<std::uint64_t>((std::max)(oldHealth,101)),0});
       Call(N::SetArmor,{static_cast<std::uint64_t>(ped),static_cast<std::uint64_t>((std::clamp)(oldArmor,0,100))});
       if(command.action==Action::Restore)Delete(backup);
       Finish(true,command.action==Action::CloneAppearance?"Modelo e aparencia do jogador copiados":command.action==Action::Restore?"Personagem original restaurado":"Modelo do personagem confirmado");
      }
     }
    }
   }
  }
  if(fresh&&f.input&&f.rpg&&!pending&&Exists(ped)){
   if(!weaponRequested){Call(N::RequestWeapon,{Hash("WEAPON_RPG"),31,0});weaponRequested=true;}
   rpgState=Call(N::WeaponLoaded,{Hash("WEAPON_RPG")})?2:1;
   if(rpgState==2&&f.shoot&&f.targetValid&&Sane(f.target)&&now-lastShot>=800){
    Call(N::ShootBullet,{Float(f.position.x+f.forward.x),Float(f.position.y+f.forward.y),Float(f.position.z+1),Float(f.target.x),Float(f.target.y),Float(f.target.z),250,1,Hash("WEAPON_RPG"),static_cast<std::uint64_t>(ped),1,0,Float(-1)});lastShot=now;
   }
  }else{if(weaponRequested)Call(N::ReleaseWeapon,{Hash("WEAPON_RPG")});weaponRequested=false;rpgState=0;}
 }
};
}

