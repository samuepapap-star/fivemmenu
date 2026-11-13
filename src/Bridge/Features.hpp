#pragma once
#include <Bridge/Client.hpp>
#include <Bridge/BuildPolicy.hpp>
#include <Core/Features/ExtraFeatures.hpp>
#include <Core/Features/ExtraCamera.hpp>
#include <Core/Features/NoclipVisibility.hpp>
#include <Core/Features/SelfGodMode.hpp>
#include <Core/Features/ExternalCameraOptions.hpp>

namespace BridgeFeatures {
inline bool vehicleArmor=false;
inline bool silentRpg=false;
inline bool cameraRequested=false;inline std::uint64_t cameraRequestedAt=0;
inline std::string localStatus;
inline void PrepareModel(){
 silentRpg=false;
 cameraRequested=false;
 auto p=Core::Features::Extras::Player();
 if(p&&g_Config.Player->NoClipEnabled)p->FreezePed(false);
 g_Config.Player->NoClipEnabled=false;g_Config.Player->NoCollisionEnabled=false;
 g_Config.Player->EnableGodMode=false;g_Config.Player->FastRun=false;g_Config.Player->InfiniteStamina=false;
 g_Config.Player->NoRagDollEnabled=false;g_Config.Player->ShrinkEnabled=false;g_Config.Player->AntiHSEnabled=false;
 g_Config.Player->InfiniteCombatRoll=false;g_Config.Player->NoRecoilEnabled=false;g_Config.Player->NoSpreadEnabled=false;
 g_Config.Player->NoReloadEnabled=false;g_Config.Player->InfiniteAmmoEnabled=false;
 Core::Features::ExtraCamera::Stop();Core::Features::NoclipVisibility::Tick(true);
 Core::Features::SelfGodMode::Tick(true);Core::Features::ExternalCameraOptions::Tick(true);
 Core::Features::Extras::Tick(true);Core::Features::CarreraAdditions::TickCollision();
}
inline bool Submit(MenuBridge::Command command,bool modelChange=false){
 if(!MenuBridge::BuildPolicy::Supported(g_Offsets.CurrentBuild)){localStatus="Ponte disponivel para 3258 e 3889; build atual nao suportada";return false;}
 if(MenuBridge::Connection().Busy()){localStatus="Aguarde a acao em andamento";return false;}
 auto p=Core::Features::Extras::Player();if(!p){localStatus="Personagem indisponivel";return false;}
 if(modelChange&&p->InVehicle()){localStatus="Saia do veiculo antes de trocar o personagem";return false;}
 if(modelChange)PrepareModel();
 localStatus.clear();const bool submitted=MenuBridge::Connection().Submit(command);
 if(!submitted)localStatus=MenuBridge::Connection().Status();return submitted;
}
inline void Model(const char* model){MenuBridge::Command c{};c.action=MenuBridge::Action::Model;MenuBridge::Copy(c.text,sizeof(c.text),model);Submit(c,true);}
inline void Restore(){MenuBridge::Command c{};c.action=MenuBridge::Action::Restore;Submit(c,true);}
inline void Clone(int serverId){MenuBridge::Command c{};c.action=MenuBridge::Action::CloneAppearance;c.targetServerId=static_cast<std::uint64_t>(serverId);Submit(c,true);}
inline void Spawn(const char* model,bool network,bool enter){MenuBridge::Command c{};c.action=MenuBridge::Action::SpawnVehicle;c.flags=(network?1u:0u)|(enter?2u:0u);MenuBridge::Copy(c.text,sizeof(c.text),model);Submit(c);}
inline void TransferVehicles(int serverId,bool rain){MenuBridge::Command c{};c.action=MenuBridge::Action::TransferVehicles;c.targetServerId=static_cast<std::uint64_t>(serverId);c.flags=rain?1u:0u;Submit(c);}
inline void DeleteClosestVehicle(){MenuBridge::Command c{};c.action=MenuBridge::Action::DeleteClosestVehicle;Submit(c);}
inline void TeleportWaypoint(){
 if(cameraRequested||MenuBridge::Connection().CameraState()||g_Config.Player->NoClipEnabled||g_Config.Player->NoCollisionEnabled){localStatus="Desative camera livre, NoClip e sem colisao antes de teleportar";return;}
 MenuBridge::Command c{};c.action=MenuBridge::Action::TeleportWaypoint;Submit(c);
}
inline void VehicleSeat(std::uintptr_t vehicle,bool enter,bool passenger=false){MenuBridge::Command c{};c.action=enter?MenuBridge::Action::EnterDriver:MenuBridge::Action::EjectSeat;c.targetEntityAddress=vehicle;c.flags=passenger?1u:0u;Submit(c);}
inline void RepairVehicle(){MenuBridge::Command c{};c.action=MenuBridge::Action::RepairVehicle;Submit(c);}
inline void LoadVehicleCatalog(){MenuBridge::Command c{};c.action=MenuBridge::Action::VehicleCatalog;Submit(c);}
inline void ApplyWeaponSkin(int tintIndex){MenuBridge::Command c{};c.action=MenuBridge::Action::WeaponSkin;c.flags=static_cast<std::uint32_t>(tintIndex);Submit(c);}
inline void Animate(const char* dictionary,const char* animation,bool loop){MenuBridge::Command c{};c.action=MenuBridge::Action::Animation;c.flags=loop?1u:0u;MenuBridge::Copy(c.text,sizeof(c.text),dictionary);MenuBridge::Copy(c.text2,sizeof(c.text2),animation);Submit(c);}
inline void StopAnimation(){MenuBridge::Command c{};c.action=MenuBridge::Action::StopAnimation;Submit(c);}
inline void ToggleCamera(){
 MenuBridge::Command c{};
 if(cameraRequested||MenuBridge::Connection().CameraState()){cameraRequested=false;c.action=MenuBridge::Action::StopCamera;Submit(c);}
 else{if(g_Config.Player->NoClipEnabled||g_Config.Player->NoCollisionEnabled){localStatus="Desative NoClip e sem colisao antes de iniciar a camera livre";return;}Core::Features::ExtraCamera::Stop();silentRpg=false;c.action=MenuBridge::Action::StartCamera;if(Submit(c)){cameraRequested=true;cameraRequestedAt=GetTickCount64();}}
}
inline void Observe(int serverId){
 if(g_Config.Player->NoClipEnabled||g_Config.Player->NoCollisionEnabled){localStatus="Desative NoClip e sem colisao antes de observar";return;}
 MenuBridge::Command c{};c.action=MenuBridge::Action::ObservePlayer;c.targetServerId=static_cast<std::uint64_t>(serverId);
 Core::Features::ExtraCamera::Stop();silentRpg=false;if(Submit(c)){cameraRequested=true;cameraRequestedAt=GetTickCount64();}
}
inline void Tick(){
 MenuBridge::Frame f{};f.heartbeat=GetTickCount64();f.gameBuild=g_Offsets.CurrentBuild;
 if(cameraRequested&&((GetForegroundWindow()==g_Variables.g_hGameWindow&&(GetAsyncKeyState(VK_ESCAPE)&0x8000))||(!MenuBridge::Connection().Busy()&&MenuBridge::Connection().CameraState()==0&&f.heartbeat-cameraRequestedAt>1000)))cameraRequested=false;
 auto p=Core::Features::Extras::Player();float health=0;D3DXVECTOR3 pos{},direction{};
 if(g_MenuInfo.IsLogged&&p&&Core::Features::Extras::Read((uintptr_t)p+0x280,health)&&std::isfinite(health)&&health>0&&
    Core::Features::Extras::Read((uintptr_t)p+0x90,pos)&&Core::Features::Extras::Sane(pos)&&
    Core::Features::Extras::Read((uintptr_t)p+0x70,direction)&&Core::Features::Extras::Sane(direction)){
  float length=D3DXVec3Length(&direction);if(length>0.1f&&length<2.f){direction*=1.f/length;
   f.vehicleArmor=vehicleArmor?1u:0u;
   f.valid=1;f.localAddress=(uintptr_t)p;f.position={pos.x,pos.y,pos.z};f.forward={direction.x,direction.y,direction.z};
   f.input=!g_MenuInfo.IsOpen&&GetForegroundWindow()==g_Variables.g_hGameWindow;
   f.shoot=f.input&&(GetAsyncKeyState(VK_LBUTTON)&0x8000);f.rpg=silentRpg&&!cameraRequested?1u:0u;
   f.camera=cameraRequested?1u:0u;f.cameraSpeed=Core::Features::ExtraCamera::speed;
   if(f.camera&&f.input){
    if(GetAsyncKeyState('W')&0x8000)f.cameraMoves|=MenuBridge::Forward;
    if(GetAsyncKeyState('S')&0x8000)f.cameraMoves|=MenuBridge::Backward;
    if(GetAsyncKeyState('A')&0x8000)f.cameraMoves|=MenuBridge::Left;
    if(GetAsyncKeyState('D')&0x8000)f.cameraMoves|=MenuBridge::Right;
    if(GetAsyncKeyState(VK_SPACE)&0x8000)f.cameraMoves|=MenuBridge::Up;
    if(GetAsyncKeyState(VK_CONTROL)&0x8000)f.cameraMoves|=MenuBridge::Down;
    if(GetAsyncKeyState(VK_SHIFT)&0x8000)f.cameraMoves|=MenuBridge::Fast;
   }
   float closest=static_cast<float>(g_Config.SilentAim->FOV);bool found=false;
   if(silentRpg&&f.input)for(const auto& entity:Core::SDK::Game::SnapshotEntities()){
    if(!entity.Ped||entity.Ped==p||entity.IsFriend||entity.Health<=101.f||entity.Distance>g_Config.SilentAim->MaxDistance||
       (g_Config.SilentAim->IgnoreNPCs&&entity.PedType!=2)||(g_Config.SilentAim->OnlyVisible&&!entity.Ped->IsVisible()))continue;
    auto bone=entity.Ped->GetBonePosDefault(0);if(!Core::Features::Extras::Sane(bone))continue;
    auto screen=Core::SDK::Game::WorldToScreen(bone);if(!Core::SDK::Game::IsOnScreen(screen))continue;
    float distance=std::hypot(screen.x-g_Variables.g_vGameWindowCenter.x,screen.y-g_Variables.g_vGameWindowCenter.y);
    if(distance<closest){closest=distance;f.target={bone.x,bone.y,bone.z};found=true;}
   }
   f.targetValid=found?1u:0u;
  }
 }
 if(silentRpg)g_Config.SilentAim->Enabled=false;
 MenuBridge::Connection().Tick(Core::Mem.ProcId,Core::Mem.ProcHandle,f);
 static std::uint64_t lastNamesRequest=0;
 if(g_Config.ESP->Enabled&&g_Config.ESP->UserNames&&f.valid&&f.heartbeat-lastNamesRequest>=3000&&!MenuBridge::Connection().Busy()){
  lastNamesRequest=f.heartbeat;MenuBridge::Command request{};request.action=MenuBridge::Action::PlayerNames;Submit(request);
 }

}
inline void Stop(){vehicleArmor=false;silentRpg=false;cameraRequested=false;MenuBridge::Connection().Stop();}
inline void Status(){
 const auto runtime=MenuBridge::Connection().RuntimeBuild();
 if(runtime)ImGui::TextDisabled("Ponte: build %d confirmada pelo runtime | protocolo %u",runtime,MenuBridge::version);
 const auto text=localStatus.empty()?MenuBridge::Connection().Status():localStatus;ImGui::TextWrapped("%s",text.c_str());
 if(silentRpg){const auto state=MenuBridge::Connection().RpgState();ImGui::TextDisabled(state==2?"RPG pronto; botao esquerdo dispara no alvo dentro do FOV":state==1?"Carregando o RPG":"RPG aguardando conexao/alvo e menu fechado");}
}
inline void EnableRpg(){MenuBridge::Command c{};c.action=MenuBridge::Action::Connect;if(!Submit(c))silentRpg=false;}
}
