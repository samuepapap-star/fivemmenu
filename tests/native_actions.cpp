#include "../native/NativeActions.hpp"
#include "native_hashes_reference.hpp"
#include "../native/NativeDispatch.hpp"
#include <cassert>
#include <unordered_map>
#include <limits>
using namespace MenuBridge;
struct Fake:NativeApi{
 int player=1,next=100,sets=0,clones=0,shots=0,enters=0,releases=0,animations=0,modelRequests=0;
 bool loaded=false,inVehicle=false,attached=false,playing=false,validModel=true,denyChange=false;
 bool denyVehicle=false,delayVehicle=false;int vehicleCalls=0,vehicleNetwork=-1,vehicleMission=-1;
 std::unordered_map<int,std::uint32_t> entities{{1,Hash("mp_m_freemode_01")},{2,Hash("s_m_y_cop_01")}};
 std::uint64_t Call(std::uint64_t hash,const std::vector<std::uint64_t>& a)override{
  auto entity=[&](){return static_cast<int>(a.at(0));};
  if(hash==N::PlayerPed)return static_cast<std::uint64_t>(player);
  if(hash==N::PlayerId)return 0;
  if(hash==N::EntityExists)return entities.count(entity());
  if(hash==N::EntityModel)return entities.at(entity());
  if(hash==N::ModelValid||hash==N::ModelPed||hash==N::ModelVehicle)return validModel;
  if(hash==N::InVehicle)return inVehicle;
  if(hash==N::Attached)return attached;
  if(hash==N::Dead)return 0;
  if(hash==N::ModelLoaded||hash==N::AnimLoaded)return loaded;
  if(hash==N::RequestModel){modelRequests++;return 0;}
  if(hash==N::ClonePed){entities[next]=entities.at(entity());return static_cast<std::uint64_t>(next++);}
  if(hash==N::SetPlayerModel){sets++;if(!denyChange){player=next++;entities[player]=static_cast<std::uint32_t>(a.at(1));}return 0;}
  if(hash==N::CloneToTarget){clones++;return 0;}
  if(hash==N::DeleteEntity){auto p=reinterpret_cast<int*>(a.at(0));entities.erase(*p);*p=0;return 0;}
  if(hash==N::GetHealth)return 200;
  if(hash==N::GetArmor)return 80;
  if(hash==N::PlayerActive)return a.at(0)==1;
  if(hash==N::PlayerPedByIndex)return 2;
  if(hash==Hash("GET_PLAYER_FROM_SERVER_ID"))return a.at(0)==5137?1:static_cast<std::uint64_t>(-1);
  if(hash==N::Heading)return Float(90);
  if(hash==N::CreateVehicle){assert(a.size()==8);assert(AsFloat(a.at(1))==10.f);vehicleCalls++;vehicleNetwork=static_cast<int>(a.at(5));vehicleMission=static_cast<int>(a.at(6));if(denyVehicle)return 0;if(!delayVehicle)entities[50]=static_cast<std::uint32_t>(a.at(0));return 50;}
  if(hash==N::EnterVehicle){enters++;return 0;}
  if(hash==N::ReleaseModel||hash==N::ReleaseAnim){releases++;return 0;}
  if(hash==N::PlayAnim){animations++;playing=true;return 0;}
  if(hash==N::PlayingAnim)return playing;
  if(hash==N::ClearTasks){playing=false;return 0;}
  if(hash==N::WeaponLoaded)return 1;
  if(hash==N::ShootBullet){assert(a.size()==13);shots++;return 0;}
  return 0;
 }
};
Frame FrameAt(std::uint64_t now){Frame f{};f.heartbeat=now;f.valid=1;f.position={10,20,30};f.forward={0,1,0};return f;}
Command Cmd(Action action,const char* text="a_c_chimp"){Command c{};c.id=1;c.action=action;Copy(c.text,sizeof(c.text),text);return c;}
int main(){
 {int calls=0;std::uint64_t result=0;CfxHandler custom=[&](fx::ScriptContext& c){calls++;*static_cast<std::uint64_t*>(c.resultBuffer)=7;};NativeSelection selected{nullptr,&custom};RageContext context{};context.result=&result;
  bool rejected=false;try{selected.InvokeInScript(context,0,0);}catch(const std::runtime_error&){rejected=true;}assert(rejected&&calls==0);
  rejected=false;try{selected.InvokeInScript(context,2,1);}catch(const std::runtime_error&){rejected=true;}assert(rejected&&calls==0);
  selected.InvokeInScript(context,1,1);assert(calls==1&&result==7);
  const auto error=FailedNative(0x963D27A58DF860ACULL,0xc0000005u);assert(error.find("963D27A58DF860AC")!=std::string::npos&&error.find("C0000005")!=std::string::npos&&error.find("indisponivel")==std::string::npos);
 }
 {std::uint64_t args[2]{5137,7},result=0;bool customCalled=false,gameResolved=false;
  CfxHandler custom=[&](fx::ScriptContext& c){customCalled=true;assert(c.argumentCount==2&&c.resultCount==0);assert(static_cast<std::uint64_t*>(c.argumentBuffer)[0]==5137);*static_cast<std::uint64_t*>(c.resultBuffer)=42;};
  auto selected=SelectNative(1,[&](std::uint64_t)->RageHandler{gameResolved=true;return nullptr;},[&](std::uint64_t){return &custom;});
  RageContext c{};c.arguments=args;c.result=&result;c.count=2;assert(selected);selected.Invoke(c);assert(customCalled&&!gameResolved&&result==42);
  selected=SelectNative(1,[](std::uint64_t)->RageHandler{return [](RageContext* context){*static_cast<std::uint64_t*>(context->result)=99;};},[](std::uint64_t)->CfxHandler*{return nullptr;});
  selected.Invoke(c);assert(result==99);
  selected=SelectNative(1,[](std::uint64_t)->RageHandler{return nullptr;},[](std::uint64_t)->CfxHandler*{return nullptr;});assert(!selected);assert(MissingNative(0x1234).find("0000000000001234")!=std::string::npos);
 }
 {auto c=Cmd(Action::CloneAppearance);c.targetServerId=0;assert(!Valid(c));c.targetServerId=5137;assert(Valid(c));c.targetServerId=0x80000000ULL;assert(!Valid(c));}
 assert(Hash("GET_PLAYER_FROM_SERVER_ID")==0x344ea166u);
 assert(Float(1.f)==0x3f800000&&AsFloat(Float(-2.f))==-2.f);
 {auto c=Cmd(Action::Model);std::memset(c.text,'a',sizeof(c.text));assert(!Valid(c));c=Cmd(Action::Model,"a_c_chimp;os.execute");assert(!Valid(c));}
 {Fake api;Actions actions(api);auto c=Cmd(Action::Animation);std::memset(c.text,'x',sizeof(c.text));actions.Submit(c,1000);assert(actions.hasReply&&!actions.reply.success&&api.releases==0);}
 {Fake api;Actions actions(api);actions.Submit(Cmd(Action::Model),1000);assert(actions.Pending()&&api.sets==0);
  actions.Tick(FrameAt(1100),1100,0);assert(api.sets==0);api.loaded=true;actions.Tick(FrameAt(1200),1200,0);assert(actions.reply.success&&api.sets==1);
  auto restore=Cmd(Action::Restore);restore.id=2;actions.Submit(restore,1300);actions.Tick(FrameAt(1400),1400,0);assert(actions.reply.success&&api.clones==1&&api.entities.at(api.player)==Hash("mp_m_freemode_01"));}
 {Fake api;api.inVehicle=true;Actions actions(api);actions.Submit(Cmd(Action::Model),1000);assert(!actions.reply.success&&!actions.Pending()&&api.sets==0);}
 {Fake api;api.attached=true;Actions actions(api);auto c=Cmd(Action::CloneAppearance);c.targetServerId=5137;actions.Submit(c,1000);assert(actions.hasReply&&!actions.reply.success&&!actions.Pending()&&api.sets==0);}
 {Fake api;Actions actions(api);auto c=Cmd(Action::CloneAppearance);c.targetServerId=5137;actions.Submit(c,1000);api.attached=true;api.loaded=true;actions.Tick(FrameAt(1100),1100,0);assert(actions.hasReply&&!actions.reply.success&&api.sets==0);}
 {Fake api;Actions actions(api);actions.Submit(Cmd(Action::Model),1000);api.inVehicle=true;api.loaded=true;actions.Tick(FrameAt(1100),1100,0);assert(!actions.reply.success&&api.sets==0);}
 {Fake api;api.loaded=true;api.denyChange=true;Actions actions(api);actions.Submit(Cmd(Action::Model),1000);actions.Tick(FrameAt(1100),1100,0);assert(!actions.reply.success);}
 {Fake api;Actions actions(api);actions.Submit(Cmd(Action::Model),1000);actions.Tick(FrameAt(1100),1100,1);assert(!actions.Pending()&&!actions.reply.success&&api.sets==0);}
 {Fake api;Actions actions(api);actions.Submit(Cmd(Action::Model),1000);actions.Tick(FrameAt(17000),17000,0);assert(!actions.reply.success&&api.sets==0);}
 {Fake api;Actions actions(api);actions.Submit(Cmd(Action::Restore),1000);assert(!actions.reply.success);}
 {Fake api;api.loaded=true;Actions actions(api);auto c=Cmd(Action::CloneAppearance);c.targetServerId=5137;actions.Submit(c,1000);actions.Tick(FrameAt(1100),1100,0);assert(actions.reply.success&&api.sets==1&&api.clones==1&&api.entities.at(api.player)==Hash("s_m_y_cop_01"));}
 {Fake api;api.entities[2]=api.entities[1];Actions actions(api);auto c=Cmd(Action::CloneAppearance);c.targetServerId=5137;actions.Submit(c,1000);assert(actions.hasReply&&actions.reply.success&&!actions.Pending()&&api.sets==0&&api.clones==1&&api.modelRequests==0&&api.releases==0);c=Cmd(Action::Restore);c.id=2;actions.Submit(c,1100);assert(actions.reply.success&&api.clones==2&&api.sets==0&&api.modelRequests==0&&api.releases==0);}
 {Fake api;Actions actions(api);auto c=Cmd(Action::CloneAppearance);c.targetServerId=5137;actions.Submit(c,1000);api.entities.erase(2);api.loaded=true;actions.Tick(FrameAt(1100),1100,0);assert(!actions.reply.success&&api.sets==0);}
 {Fake api;Actions actions(api);auto c=Cmd(Action::CloneAppearance);c.targetServerId=9999;actions.Submit(c,1000);assert(actions.hasReply&&!actions.reply.success&&api.sets==0);}
 {Fake api;api.loaded=true;Actions actions(api);auto c=Cmd(Action::SpawnVehicle,"adder");c.flags=2;actions.Submit(c,1000);actions.Tick(FrameAt(1100),1100,0);assert(actions.reply.success&&actions.reply.value==50&&api.enters==1);}
 {Fake api;api.loaded=true;Actions actions(api);auto c=Cmd(Action::SpawnVehicle,"sultan");actions.Submit(c,1000);actions.Tick(FrameAt(1100),1100,0);assert(actions.reply.success&&api.vehicleCalls==1&&api.vehicleNetwork==0&&api.vehicleMission==1&&api.entities.at(50)==Hash("sultan"));}
 {Fake api;api.loaded=true;api.denyVehicle=true;Actions actions(api);auto c=Cmd(Action::SpawnVehicle,"zentorno");c.flags=1;actions.Submit(c,1000);actions.Tick(FrameAt(1100),1100,0);assert(!actions.reply.success&&api.vehicleCalls==1&&api.vehicleNetwork==1&&std::string(actions.reply.message).find("zentorno")!=std::string::npos&&std::string(actions.reply.message).find("rede")!=std::string::npos);}
 {Fake api;api.loaded=true;api.delayVehicle=true;Actions actions(api);auto c=Cmd(Action::SpawnVehicle,"adder");actions.Submit(c,1000);actions.Tick(FrameAt(1100),1100,0);assert(actions.Pending()&&api.vehicleCalls==1);api.entities[50]=Hash("adder");actions.Tick(FrameAt(1200),1200,0);assert(actions.reply.success&&api.vehicleCalls==1);}
 {Fake api;api.loaded=true;api.delayVehicle=true;Actions actions(api);actions.Submit(Cmd(Action::SpawnVehicle,"adder"),1000);actions.Tick(FrameAt(1100),1100,0);actions.Tick(FrameAt(3200),3200,0);assert(!actions.Pending()&&!actions.reply.success&&api.vehicleCalls==1);}
 {Fake api;api.loaded=true;Actions actions(api);auto c=Cmd(Action::Animation,"missminuteman_1ig_2");Copy(c.text2,sizeof(c.text2),"handsup_base");actions.Submit(c,1000);actions.Tick(FrameAt(1100),1100,0);assert(actions.Pending()&&api.animations==1);actions.Tick(FrameAt(1200),1200,0);assert(actions.reply.success);}
 {Fake api;Actions actions(api);auto f=FrameAt(1000);f.input=f.rpg=f.shoot=f.targetValid=1;f.target={100,100,10};actions.Tick(f,1000,0);assert(api.shots==1);f.heartbeat=1100;actions.Tick(f,1100,0);assert(api.shots==1);f.heartbeat=1800;actions.Tick(f,1800,0);assert(api.shots==2);actions.Tick(f,4000,0);assert(api.shots==2&&actions.rpgState==0);}
 {auto f=FrameAt(1000);f.position.x=std::numeric_limits<float>::quiet_NaN();assert(!Fresh(f,1000));assert(!Fresh(FrameAt(1001),1000));}
}

