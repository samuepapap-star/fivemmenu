#pragma once
#include "NativeApi.hpp"
#include <algorithm>
#include <cmath>
#include <initializer_list>
#include <stdexcept>
namespace MenuBridge {
namespace CameraN {
constexpr std::uint64_t DestroyCam=0x865908C81A2C22E9ULL;
constexpr std::uint64_t DoesCamExist=0xA7A932170592B50EULL;
constexpr std::uint64_t CreateCamWithParams=0xB51194800B257161ULL;
constexpr std::uint64_t RenderScriptCams=0x07E5B515DB0636FCULL;
constexpr std::uint64_t SetCamCoord=0x4D41783FB745E42EULL;
constexpr std::uint64_t DisableAllControlActions=0x5F4B6931816E599BULL;
constexpr std::uint64_t GetDisabledControlNormal=0x11E65974A982637CULL;
constexpr std::uint64_t SetFocusPosAndVel=0xBB7454BAFF08FE25ULL;
constexpr std::uint64_t ClearFocus=0x31B73D1EA9F01DA2ULL;
constexpr std::uint64_t SetCamRot=0x85973643155D0B07ULL;
constexpr std::uint64_t PlayerPed=0xD80958FC74E988A6;
}
struct CameraPose {Vec position{};float yaw=0,pitch=0;};
inline void MoveCamera(CameraPose& pose,const Frame& f,float mouseX,float mouseY,float dt){
 constexpr float radians=0.0174532925199433f;
 if(std::isfinite(mouseX))pose.yaw=std::remainder(pose.yaw-mouseX*8.f,360.f);
 if(std::isfinite(mouseY))pose.pitch=(std::clamp)(pose.pitch-mouseY*8.f,-89.f,89.f);
 float yaw=pose.yaw*radians,pitch=pose.pitch*radians;
 Vec direction{-std::sin(yaw)*std::cos(pitch),std::cos(yaw)*std::cos(pitch),std::sin(pitch)};
 Vec right{std::cos(yaw),std::sin(yaw),0};
 float forward=((f.cameraMoves&Forward)?1.f:0.f)-((f.cameraMoves&Backward)?1.f:0.f);
 float side=((f.cameraMoves&Right)?1.f:0.f)-((f.cameraMoves&Left)?1.f:0.f);
 float up=((f.cameraMoves&Up)?1.f:0.f)-((f.cameraMoves&Down)?1.f:0.f);
 Vec velocity{direction.x*forward+right.x*side,direction.y*forward+right.y*side,direction.z*forward+static_cast<float>(up)};
 float length=std::sqrt(velocity.x*velocity.x+velocity.y*velocity.y+velocity.z*velocity.z);
 float speed=std::isfinite(f.cameraSpeed)?(std::clamp)(f.cameraSpeed,1.f,50.f):10.f;
 float distance=speed*(std::clamp)(dt,0.f,0.05f)*((f.cameraMoves&Fast)?2.f:1.f);
 if(length>0.f){pose.position.x+=velocity.x/length*distance;pose.position.y+=velocity.y/length*distance;pose.position.z+=velocity.z/length*distance;}
}
class NativeCamera {
 NativeApi& api;int handle=0,ped=0;CameraPose pose{};std::uint64_t started=0,last=0;
 std::uint64_t Call(std::uint64_t hash,std::initializer_list<std::uint64_t> values={}){return api.Call(hash,std::vector<std::uint64_t>(values));}
 void Focus(){Call(CameraN::SetFocusPosAndVel,{Float(pose.position.x),Float(pose.position.y),Float(pose.position.z),Float(0),Float(0),Float(0)});}
public:
 explicit NativeCamera(NativeApi& native):api(native){}
 bool Active()const{return handle>0;}
 const CameraPose& Pose()const{return pose;}
 void Reset(){handle=0;ped=0;started=last=0;}
 void Stop(){
  int previous=handle;Reset();if(!previous)return;
  // Always restore the gameplay camera before destroying the camera we own.
  Call(CameraN::RenderScriptCams,{0,0,0,1,0,0});
  if(Call(CameraN::DoesCamExist,{static_cast<std::uint64_t>(previous)}))Call(CameraN::DestroyCam,{static_cast<std::uint64_t>(previous),1});
  Call(CameraN::ClearFocus);
 }
 Reply Submit(const Command& c,const Frame& f,std::uint64_t now){
  Reply reply{};reply.id=c.id;
  if(!Valid(c)){Copy(reply.message,sizeof(reply.message),"Comando de camera invalido");return reply;}
  if(c.action==Action::StopCamera){Stop();reply.success=1;Copy(reply.message,sizeof(reply.message),"Camera livre desativada");return reply;}
  if(c.action!=Action::StartCamera||!Fresh(f,now)){Copy(reply.message,sizeof(reply.message),"Personagem indisponivel para iniciar camera");return reply;}
  if(Active()){reply.success=1;Copy(reply.message,sizeof(reply.message),"Camera livre ja esta ativa");return reply;}
  ped=static_cast<int>(Call(CameraN::PlayerPed));
  if(ped<=0){Copy(reply.message,sizeof(reply.message),"Personagem indisponivel para iniciar camera");return reply;}
  pose.position={f.position.x+f.forward.x*0.3f,f.position.y+f.forward.y*0.3f,f.position.z+1.6f};
  pose.yaw=std::atan2(-f.forward.x,f.forward.y)*57.2957795130823f;pose.pitch=0;
  const char* name="DEFAULT_SCRIPTED_CAMERA";
  handle=static_cast<int>(Call(CameraN::CreateCamWithParams,{reinterpret_cast<std::uint64_t>(name),Float(pose.position.x),Float(pose.position.y),Float(pose.position.z),Float(0),Float(0),Float(pose.yaw),Float(60),1,2}));
  if(!Active()||!Call(CameraN::DoesCamExist,{static_cast<std::uint64_t>(handle)})){Reset();Copy(reply.message,sizeof(reply.message),"O jogo nao criou a camera livre");return reply;}
  Call(CameraN::RenderScriptCams,{1,0,0,1,0,0});Focus();started=last=now;reply.success=1;
  Copy(reply.message,sizeof(reply.message),"Camera livre: feche o menu; WASD, mouse, Espaco/Ctrl, Shift. Esc encerra.");return reply;
 }
 void Tick(const Frame& f,std::uint64_t now){
  if(!Active())return;
  if(!Fresh(f,now)||(!f.camera&&now-started>250)||static_cast<int>(Call(CameraN::PlayerPed))!=ped||!Call(CameraN::DoesCamExist,{static_cast<std::uint64_t>(handle)})){Stop();return;}
  float dt=now>=last?static_cast<float>(now-last)/1000.f:0.f;last=now;
  Call(CameraN::DisableAllControlActions,{0});
  if(!f.input)return;
  const float mouseX=AsFloat(Call(CameraN::GetDisabledControlNormal,{0,1}));
  const float mouseY=AsFloat(Call(CameraN::GetDisabledControlNormal,{0,2}));
  MoveCamera(pose,f,mouseX,mouseY,dt);
  if(!Sane(pose.position)){Stop();throw std::runtime_error("Posicao da camera invalida");}
  Call(CameraN::SetCamCoord,{static_cast<std::uint64_t>(handle),Float(pose.position.x),Float(pose.position.y),Float(pose.position.z)});
  Call(CameraN::SetCamRot,{static_cast<std::uint64_t>(handle),Float(pose.pitch),Float(0),Float(pose.yaw),2});Focus();
 }
};
}

