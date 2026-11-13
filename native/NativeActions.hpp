#pragma once
#include "../src/Bridge/Protocol.hpp"
#include <vector>
#include <initializer_list>
#include <stdexcept>
#include <algorithm>
#include "NativeApi.hpp"
#include "NativeVehicleSeats.hpp"

namespace MenuBridge {
namespace N {
constexpr std::uint64_t PlayerId=0x4F8644AF03D0E0D6,PlayerPed=0xD80958FC74E988A6,
 EntityExists=0x7239B21A38F536BA,EntityModel=0x9F47B058362C84B5,InVehicle=0x997ABD671D25CA0B,Attached=0xB346476EF1A64897,Dead=0x5F9532F3B5CC2551,
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
constexpr std::uint64_t CurrentVehicle=0x9A9112A0FE9A4713,FixVehicle=0x115722B1B9C14C1C,FixDeformation=0x953DA1E1B12C0491,
 GetEngineHealth=0xC45D23BAF168AAB8,GetBodyHealth=0xF271147EB7B40F12,
 EngineHealth=0x45F6D8EEF34ABEF1,BodyHealth=0xB77D05AC8C78AADB,Undriveable=0x8ABA6AF54B942B95,DoorDamaged=0xB8E181E559464527,
 Networked=0xC7827959479DCC78,HasControl=0x01BF60A500E28887,
 NetworkId=0xA11700682F3AD45C,NetworkIdExists=0x38CE16C96BD11344,
 ClosestVehicle=0xF73EB622C4F1689B,EntityCoords=0x3FEF770D40960D5A,DeleteVehicle=0xEA386986E786A54F,
 Passengers=0x24CB2137731FFE89,SeatFree=0x22AC59A870E6A669;
}
class Actions {
 VehicleSeats seats;
 NativeApi& api;Command command{};bool pending=false,animConfirm=false,animRequested=false;std::uint64_t deadline=0,animDeadline=0;
 std::uint32_t model=0;int cloneSource=0,backup=0,expectedPed=0,oldHealth=0,oldArmor=0;
 bool weaponRequested=false;std::uint64_t lastShot=0;
 int spawned=0;bool spawnStarted=false;std::uint64_t spawnDeadline=0;
 // Estado para delete em massa com request control assincrono
 std::vector<int> massDeleteTargets{};int massDeletePhase=0;std::uint64_t massDeleteDeadline=0;
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
  seats.Reset();
  if(model)Call(N::ReleaseModel,{model});
  if(animRequested)Call(N::ReleaseAnim,{reinterpret_cast<std::uint64_t>(command.text)});
  model=0;pending=false;animConfirm=false;animRequested=false;spawned=0;spawnStarted=false;
  massDeleteTargets.clear();massDeletePhase=0;
 }
 void Finish(bool ok,const std::string& text,std::uint32_t value=0){
  reply={};reply.id=command.id;reply.success=ok?1u:0u;reply.value=value;Copy(reply.message,sizeof(reply.message),text);hasReply=true;Release();
 }
 bool ConfirmVehicle(int ped){
  std::uint64_t networkId=0;
  if(command.flags&1u){
   if(!Call(N::Networked,{static_cast<std::uint64_t>(spawned)}))return false;
   networkId=Call(N::NetworkId,{static_cast<std::uint64_t>(spawned)});
   if(!networkId||networkId>0x7fffffffULL||!Call(N::NetworkIdExists,{networkId}))return false;
  }
  if(command.flags&2u)Call(N::EnterVehicle,{static_cast<std::uint64_t>(ped),static_cast<std::uint64_t>(spawned),static_cast<std::uint64_t>(-1)});
  const std::string message=(command.flags&1u)?"Registro em rede confirmado (ID "+std::to_string(networkId)+"): "+command.text+". Visibilidade depende do servidor.":"Veiculo LOCAL criado: "+std::string(command.text)+". Visivel somente para voce.";
  Finish(true,message,static_cast<std::uint32_t>(spawned));return true;
 }
public:
 Reply reply{};bool hasReply=false;int rpgState=0;
 explicit Actions(NativeApi& native):seats(native),api(native){}
 bool Pending()const{return pending;}
 void Reset(){seats.Reset();pending=false;animConfirm=false;animRequested=false;model=0;backup=0;expectedPed=0;weaponRequested=false;hasReply=false;rpgState=0;spawned=0;spawnStarted=false;}
 void Abort(const std::string& message){if(pending)Finish(false,message);}
 void Submit(const Command& value,std::uint64_t now){
  if(pending)throw std::runtime_error("Outra acao esta em andamento");
  command=value;hasReply=false;model=0;animConfirm=false;animRequested=false;spawned=0;spawnStarted=false;
  if(!Valid(value)){Finish(false,"Pedido invalido");return;}
  pending=true;deadline=now+15000;
  int ped=static_cast<int>(Call(N::PlayerPed));
  if(!Exists(ped)){Finish(false,"Personagem indisponivel");return;}
  const bool change=command.action==Action::Model||command.action==Action::Restore||command.action==Action::CloneAppearance;
  if(change&&(Call(N::InVehicle,{static_cast<std::uint64_t>(ped),0})||Call(N::Attached,{static_cast<std::uint64_t>(ped)}))){Finish(false,"Saia do veiculo ou desvincule o personagem antes de trocar o modelo");return;}
  if(command.action!=Action::Connect&&command.action!=Action::StopAll&&command.action!=Action::StopAnimation&&Call(N::Dead,{static_cast<std::uint64_t>(ped),0})){Finish(false,"Espere o personagem estar vivo");return;}
  if(expectedPed&&ped!=expectedPed){Delete(backup);}expectedPed=ped;
  switch(command.action){
   case Action::EnterDriver:case Action::EjectSeat:
    seats.Submit(command,now);if(!seats.Pending())Finish(seats.reply.success!=0,seats.reply.message);return;
   case Action::Model:model=Hash(command.text);break;
   case Action::CloneAppearance:cloneSource=Target(command.targetServerId);if(!cloneSource){Finish(false,"Jogador selecionado nao encontrado");return;}model=static_cast<std::uint32_t>(Call(N::EntityModel,{static_cast<std::uint64_t>(cloneSource)}));break;
   case Action::Restore:if(!Exists(backup)){Finish(false,"Nao ha aparencia original para restaurar");return;}model=static_cast<std::uint32_t>(Call(N::EntityModel,{static_cast<std::uint64_t>(backup)}));break;
   case Action::SpawnVehicle:model=Hash(command.text);break;
   case Action::Animation:Call(N::RequestAnim,{reinterpret_cast<std::uint64_t>(command.text)});animRequested=true;return;
   case Action::StopAnimation:Call(N::ClearTasks,{static_cast<std::uint64_t>(ped)});Finish(true,"Animacao interrompida");return;
   case Action::StopAll:if(weaponRequested)Call(N::ReleaseWeapon,{Hash("WEAPON_RPG")});weaponRequested=false;rpgState=0;Finish(true,"Acoes continuas interrompidas");return;
   case Action::DeleteClosestVehicle:{
    const Vec pos=api.Vector(N::EntityCoords,{static_cast<std::uint64_t>(ped),0});
    if(!Sane(pos)){Finish(false,"Posicao indisponivel");return;}
    const auto own=Call(N::CurrentVehicle,{static_cast<std::uint64_t>(ped),0});
    // flags==0 -> deletar o mais proximo em 15 m (comportamento original)
    // flags>0  -> deletar TODOS no raio de flags metros (modo em massa)
    const bool massDelete=command.flags>0;
    const float radius=massDelete?static_cast<float>(command.flags):15.f;
    const float radiusSq=radius*radius;
    int vehicle=0,best=0,uncontrolled=0,outside=0;float bestDistance=radiusSq;
    {
     const int handle=static_cast<int>(Call(Hash("FIND_FIRST_VEHICLE"),{reinterpret_cast<std::uint64_t>(&vehicle)}));
     struct Iterator{NativeApi& api;int handle;~Iterator(){if(handle>=0)try{api.Call(Hash("END_FIND_VEHICLE"),{static_cast<std::uint64_t>(handle)});}catch(...){}}} iterator{api,handle};
     for(int scanned=0;handle>=0&&vehicle>0&&scanned<512;scanned++){
      const auto id=static_cast<std::uint64_t>(vehicle);
      if(Exists(vehicle)&&id!=own){
       const Vec current=api.Vector(N::EntityCoords,{id,0});
       const float x=current.x-pos.x,y=current.y-pos.y,z=current.z-pos.z,distance=x*x+y*y+z*z;
       if(!Sane(current)||distance>radiusSq){outside++;goto next_vehicle;}
       if(Call(N::Networked,{id})&&!Call(N::HasControl,{id})){uncontrolled++;goto next_vehicle;}
       if(massDelete){
        // Fase 0: coletar handle e pedir controle
        Call(Hash("NETWORK_REQUEST_CONTROL_OF_ENTITY"),{id});
        massDeleteTargets.push_back(vehicle);
        goto next_vehicle;
       }
       if(distance<=bestDistance){best=vehicle;bestDistance=distance;}
      }
      next_vehicle:
      if(!Call(Hash("FIND_NEXT_VEHICLE"),{static_cast<std::uint64_t>(handle),reinterpret_cast<std::uint64_t>(&vehicle)}))break;
     }
    }
    if(massDelete){
     if(massDeleteTargets.empty()){Finish(false,"Nenhum veiculo encontrado em "+std::to_string(command.flags)+" m. Fora do raio: "+std::to_string(outside)+".");return;}
     // Aguardar controle no Tick (fase 1 -> 2)
     massDeletePhase=1;massDeleteDeadline=now+600;return; // pending ja e true
    }
    if(!best){Finish(false,"Nenhum veiculo elegivel em 15 m. Sem controle: "+std::to_string(uncontrolled)+". Fora do raio: "+std::to_string(outside)+". Seu veiculo atual e ignorado.");return;}
    const auto id=static_cast<std::uint64_t>(best);
    if(!Exists(best)||id==Call(N::CurrentVehicle,{static_cast<std::uint64_t>(ped),0})){Finish(false,"Veiculo indisponivel ou atual; exclusao cancelada");return;}
    if(Call(N::Networked,{id})&&!Call(N::HasControl,{id})){Finish(false,"Sem controle local deste veiculo; exclusao cancelada");return;}
    Call(N::MissionEntity,{id,1,1});Call(N::DeleteVehicle,{reinterpret_cast<std::uint64_t>(&best)});
    const bool removed=!Call(N::EntityExists,{id});
    Finish(removed,removed?"Veiculo proximo apagado; ausencia confirmada localmente":"O jogo nao confirmou a exclusao do veiculo");return;
   }
   case Action::RepairVehicle:{
    const auto vehicle=Call(N::CurrentVehicle,{static_cast<std::uint64_t>(ped),0});
    if(!vehicle||!Call(N::EntityExists,{vehicle})){Finish(false,"Entre no veiculo para reparar");return;}
    if(Call(N::Networked,{vehicle})&&!Call(N::HasControl,{vehicle})){Finish(false,"O cliente nao controla este veiculo; reparo nao aplicado");return;}
    // Health alone does not restore detached doors; use the full repair native.
    Call(N::EngineHealth,{vehicle,Float(1000.f)});Call(N::FixVehicle,{vehicle});Call(N::FixDeformation,{vehicle});
    Call(N::BodyHealth,{vehicle,Float(1000.f)});Call(N::Undriveable,{vehicle,0});
    if(!Call(N::EntityExists,{vehicle})||Call(N::CurrentVehicle,{static_cast<std::uint64_t>(ped),0})!=vehicle||
       (Call(N::Networked,{vehicle})&&!Call(N::HasControl,{vehicle}))){Finish(false,"Veiculo ou controle mudou durante o reparo; sem confirmacao");return;}
    int damaged=0;for(std::uint64_t door=0;door<8;door++)if(Call(N::DoorDamaged,{vehicle,door}))damaged++;
    const float engine=AsFloat(Call(N::GetEngineHealth,{vehicle})),body=AsFloat(Call(N::GetBodyHealth,{vehicle}));
    const bool confirmed=std::isfinite(engine)&&std::isfinite(body)&&engine>=999.f&&engine<=1000.f&&body>=999.f&&body<=1000.f&&damaged==0;
    Finish(confirmed,confirmed?"Reparo confirmado localmente: motor, carroceria e portas":"O jogo nao confirmou o reparo completo; nenhuma nova tentativa");return;
   }
   case Action::Connect:Finish(true,"Ponte pronta");return;
   case Action::WeaponSkin:case Action::PlayerNames:
   case Action::TeleportWaypoint:case Action::StartCamera:case Action::StopCamera:case Action::ObservePlayer:case Action::TransferVehicles:case Action::VehicleCatalog:Finish(false,"Comando deve ser tratado pela ponte");return;
  }
  if(!Call(N::ModelValid,{model})||!Call(command.action==Action::SpawnVehicle?N::ModelVehicle:N::ModelPed,{model})){
   Finish(false,"Modelo invalido para esta acao");return;
  }
  if(command.action!=Action::SpawnVehicle){EnsureBackup(ped);oldHealth=static_cast<int>(Call(N::GetHealth,{static_cast<std::uint64_t>(ped)}));oldArmor=static_cast<int>(Call(N::GetArmor,{static_cast<std::uint64_t>(ped)}));}
  if((command.action==Action::CloneAppearance||command.action==Action::Restore)&&Call(N::EntityModel,{static_cast<std::uint64_t>(ped)})==model){
   // Clothing/props on the same base model do not require destroying the player ped.
   Call(N::CloneToTarget,{static_cast<std::uint64_t>(command.action==Action::Restore?backup:cloneSource),static_cast<std::uint64_t>(ped)});
   if(command.action==Action::Restore)Delete(backup);
   model=0;Finish(true,command.action==Action::Restore?"Aparencia original restaurada sem trocar o modelo":"Aparencia copiada sem trocar o modelo do personagem");return;
  }
  Call(N::RequestModel,{model});
 }
 void Tick(const Frame& f,std::uint64_t now,std::uint64_t cancelled){
  const bool fresh=Fresh(f,now);
  int ped=static_cast<int>(Call(N::PlayerPed));
  if(expectedPed&&ped!=expectedPed&&!pending){Delete(backup);expectedPed=ped;}
  if(pending){
   if(cancelled==command.id||now>=deadline||!fresh||!Exists(ped)){Finish(false,"Acao cancelada ou tempo de carregamento esgotado");}
   else if(command.action==Action::EnterDriver||command.action==Action::EjectSeat){
    seats.Tick(now);if(!seats.Pending())Finish(seats.reply.success!=0,seats.reply.message);
   }
   else if(command.action==Action::SpawnVehicle&&spawnStarted){
    if(Exists(spawned)){if(!ConfirmVehicle(ped)&&now>=spawnDeadline)Finish(false,"Veiculo existe localmente, mas o registro em rede nao foi confirmado. Sem sucesso de sincronizacao.");}
    else if(now>=spawnDeadline)Finish(false,"Veiculo recebeu handle, mas nao apareceu em 2 segundos; sem nova tentativa automatica");
   }
   else if(command.action==Action::Animation){
    if(animConfirm){if(Call(N::PlayingAnim,{static_cast<std::uint64_t>(ped),reinterpret_cast<std::uint64_t>(command.text),reinterpret_cast<std::uint64_t>(command.text2),3}))Finish(true,"Animacao confirmada");else if(now>=animDeadline)Finish(false,"Animacao nao confirmada pelo jogo");}
    else if(Call(N::AnimLoaded,{reinterpret_cast<std::uint64_t>(command.text)})){
     Call(N::PlayAnim,{static_cast<std::uint64_t>(ped),reinterpret_cast<std::uint64_t>(command.text),reinterpret_cast<std::uint64_t>(command.text2),Float(8),Float(-8),static_cast<std::uint64_t>(-1),command.flags&1u,Float(0),0,0,0});animConfirm=true;animDeadline=now+2000;
    }
   }else if(command.action==Action::DeleteClosestVehicle&&massDeletePhase==1){
    // Fase 1: aguardar controle ou timeout, depois deletar todos
    const bool timedOut=now>=massDeleteDeadline;
    // Re-pedir controle dos que ainda nao tem (ate timeout)
    if(!timedOut){for(int vh:massDeleteTargets){const auto id=static_cast<std::uint64_t>(vh);if(Exists(vh)&&Call(N::Networked,{id})&&!Call(N::HasControl,{id}))Call(Hash("NETWORK_REQUEST_CONTROL_OF_ENTITY"),{id});}}
    // Checkar se todos ja tem controle ou se chegou timeout
    bool allReady=true;
    for(int vh:massDeleteTargets){const auto id=static_cast<std::uint64_t>(vh);if(Exists(vh)&&Call(N::Networked,{id})&&!Call(N::HasControl,{id})){allReady=false;break;}}
    if(!allReady&&!timedOut)return; // ainda esperando
    // Fase 2: deletar
    int deleted=0,noControl=0;
    for(int vh:massDeleteTargets){
     if(!Exists(vh))continue;
     const auto id=static_cast<std::uint64_t>(vh);
     if(Call(N::Networked,{id})&&!Call(N::HasControl,{id})){++noControl;continue;}
     Call(N::MissionEntity,{id,1,1});
     Call(N::DeleteVehicle,{reinterpret_cast<std::uint64_t>(&vh)});
     if(!Call(N::EntityExists,{id}))++deleted;
    }
    massDeleteTargets.clear();massDeletePhase=0;
    const std::string msg="Deletados: "+std::to_string(deleted)+(noControl?" | Sem controle apos espera: "+std::to_string(noControl):"");
    Finish(deleted>0,deleted>0?msg:"Nenhum veiculo deletado. Sem controle: "+std::to_string(noControl)+".");return;
   }else if(Call(N::ModelLoaded,{model})){
    if(command.action==Action::SpawnVehicle){
     const float heading=AsFloat(Call(N::Heading,{static_cast<std::uint64_t>(ped)}));
     spawned=static_cast<int>(Call(N::CreateVehicle,{model,Float(f.position.x+f.forward.x*5),Float(f.position.y+f.forward.y*5),Float(f.position.z+0.3f),Float(heading),(command.flags&1u)?1u:0u,1,0}));
     if(spawned<=0)Finish(false,std::string("CREATE_VEHICLE retornou 0 para ")+command.text+((command.flags&1u)?" (rede). Teste desmarcando a opcao em rede; a sessao pode recusar entidades.":" (local). O jogo nao criou a entidade."));
     else{spawnStarted=true;spawnDeadline=now+8000;if(Exists(spawned))ConfirmVehicle(ped);}
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
