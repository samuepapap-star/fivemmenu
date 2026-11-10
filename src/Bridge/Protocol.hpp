#pragma once
#include <cstdint>
#include <cstddef>
#include <cmath>
#include <cstring>
#include <string>

namespace MenuBridge {
inline constexpr std::uint32_t magic=0x464d4231, version=7;
inline constexpr std::size_t catalogMax=8192,catalogNameBytes=96;
enum class Action:std::uint32_t { Model=1, Restore, CloneAppearance, SpawnVehicle, Animation, StopAnimation, StopAll, Connect, StartCamera, StopCamera, ObservePlayer, TransferVehicles, VehicleCatalog, RepairVehicle, DeleteClosestVehicle, TeleportWaypoint };
enum State:std::int32_t { Idle=0, Ready=1, Busy=2, Done=3 };
enum CameraMove:std::uint32_t { Forward=1,Backward=2,Left=4,Right=8,Up=16,Down=32,Fast=64 };
struct VehicleModel {std::string name;int vehicleClass=-1;};
struct Vec {float x=0,y=0,z=0;};
struct Command {
 std::uint64_t id=0;Action action=Action::StopAll;std::uint32_t flags=0;
 std::uint64_t targetServerId=0;char text[96]{},text2[96]{};
};
struct Frame {
 std::uint64_t heartbeat=0,localAddress=0;Vec position{},forward{},target{};
 std::uint32_t valid=0,input=0,shoot=0,rpg=0,targetValid=0,vehicleArmor=0;
 std::uint32_t camera=0,cameraMoves=0;float cameraSpeed=10.f;std::uint32_t gameBuild=0;
};
struct Reply {std::uint64_t id=0;std::uint32_t success=0,value=0;char message[240]{};};
struct alignas(8) Shared {
 std::uint32_t signature=magic,protocol=version,pid=0,size=sizeof(Shared);
 std::int32_t service=0,commandState=Idle,frameSequence=0,rpgState=0,cameraState=0;
 std::uint32_t clientPid=0;std::int32_t runtimeBuild=0;
 std::uint64_t cancelId=0;
 Frame frame{};Command command{};Reply reply{};char error[240]{};
 std::int32_t catalogSequence=0;std::uint32_t catalogCount=0;char catalogNames[catalogMax][catalogNameBytes]{};std::uint8_t catalogClasses[catalogMax]{};
};
inline bool Sane(Vec p){return std::isfinite(p.x)&&std::isfinite(p.y)&&std::isfinite(p.z)&&
 std::fabs(p.x)<100000.f&&std::fabs(p.y)<100000.f&&std::fabs(p.z)<100000.f;}
inline bool Fresh(const Frame& f,std::uint64_t now){return f.valid&&f.heartbeat<=now&&now-f.heartbeat<2000&&Sane(f.position)&&Sane(f.forward);}
inline bool Text(const char* s,std::size_t capacity,bool model=false){
 const char* end=static_cast<const char*>(std::memchr(s,0,capacity));if(!end||end==s)return false;
 for(auto p=s;p!=end;p++){unsigned char c=static_cast<unsigned char>(*p);
  if(model){if(!((c>='a'&&c<='z')||(c>='A'&&c<='Z')||(c>='0'&&c<='9')||c=='_'||c=='-'))return false;}
  else if(c<32||c>126)return false;
 }return true;
}
inline bool Valid(const Command& c){
 if(!c.id)return false;
 switch(c.action){
 case Action::Model:case Action::SpawnVehicle:return Text(c.text,sizeof(c.text),true);
 case Action::Animation:return Text(c.text,sizeof(c.text))&&Text(c.text2,sizeof(c.text2));
 case Action::CloneAppearance:case Action::ObservePlayer:case Action::TransferVehicles:return c.targetServerId>0&&c.targetServerId<=0x7fffffffULL;
 case Action::Restore:case Action::StopAnimation:case Action::StopAll:case Action::Connect:case Action::StartCamera:case Action::StopCamera:case Action::VehicleCatalog:case Action::RepairVehicle:case Action::DeleteClosestVehicle:case Action::TeleportWaypoint:return true;
 default:return false;
 }
}
inline std::uint32_t Hash(const char* name){std::uint32_t h=0;for(;*name;name++){
 unsigned char c=static_cast<unsigned char>(*name);if(c>='A'&&c<='Z')c=static_cast<unsigned char>(c+32);
 h+=c;h+=h<<10;h^=h>>6;}h+=h<<3;h^=h>>11;h+=h<<15;return h;}
inline std::uint64_t Float(float value){std::uint32_t bits=0;std::memcpy(&bits,&value,4);return bits;}
inline float AsFloat(std::uint64_t value){std::uint32_t bits=static_cast<std::uint32_t>(value);float out=0;std::memcpy(&out,&bits,4);return out;}
inline void Copy(char* out,std::size_t capacity,const std::string& text){
 if(!capacity)return;const auto n=text.size()<capacity-1?text.size():capacity-1;std::memcpy(out,text.data(),n);out[n]=0;
}
}
