#include "../native/NativeVehicleTransfer.hpp"
#include "../native/NativeDispatch.hpp"
#include <cassert>
#include <unordered_map>
#include <unordered_set>
using namespace MenuBridge;
// https://raw.githubusercontent.com/citizenfx/natives/master/ENTITY/GetEntityCoords.md
static_assert(TransferN::GetEntityCoords==0x3FEF770D40960D5AULL,"CFX transfer native mismatch");
// https://raw.githubusercontent.com/citizenfx/natives/master/ENTITY/SetEntityCoordsNoOffset.md
static_assert(TransferN::SetEntityCoordsNoOffset==0x239A3351AC1DA385ULL,"CFX transfer native mismatch");
// https://raw.githubusercontent.com/citizenfx/natives/master/ENTITY/SetEntityVelocity.md
static_assert(TransferN::SetEntityVelocity==0x1C99BB7B6E96D16FULL,"CFX transfer native mismatch");
// https://raw.githubusercontent.com/citizenfx/natives/master/NETWORK/NetworkHasControlOfEntity.md
static_assert(TransferN::NetworkHasControlOfEntity==0x01BF60A500E28887ULL,"CFX transfer native mismatch");
// https://raw.githubusercontent.com/citizenfx/natives/master/NETWORK/NetworkGetEntityIsNetworked.md
static_assert(TransferN::NetworkGetEntityIsNetworked==0xC7827959479DCC78ULL,"CFX transfer native mismatch");
// https://raw.githubusercontent.com/citizenfx/natives/master/VEHICLE/GetVehicleNumberOfPassengers.md
static_assert(TransferN::GetVehicleNumberOfPassengers==0x24CB2137731FFE89ULL,"CFX transfer native mismatch");
// https://raw.githubusercontent.com/citizenfx/natives/master/VEHICLE/IsVehicleSeatFree.md
static_assert(TransferN::IsVehicleSeatFree==0x22AC59A870E6A669ULL,"CFX transfer native mismatch");
// https://raw.githubusercontent.com/citizenfx/natives/master/PHYSICS/ActivatePhysics.md
static_assert(TransferN::ActivatePhysics==0x710311ADF0E20730ULL,"CFX transfer native mismatch");
struct FakeTransfer:NativeApi {
 std::vector<int> vehicles{10,11};std::unordered_map<int,Vec> positions{{2,{100,0,0}},{10,{20,0,0}},{11,{30,0,0}}};
 std::unordered_set<int> noControl,occupied;std::size_t index=0;
 int writes=0,ends=0,physics=0;bool reject=false,throwOnMove=false;float velocityZ=0;
 Vec Vector(std::uint64_t hash,const std::vector<std::uint64_t>& a)override{assert(hash==TransferN::GetEntityCoords);return positions.at(static_cast<int>(a.at(0)));}
 std::uint64_t Call(std::uint64_t hash,const std::vector<std::uint64_t>& a)override {
  auto entity=[&](){return static_cast<int>(a.at(0));};
  if(hash==Hash("GET_PLAYER_FROM_SERVER_ID"))return a.at(0)==5137?1:static_cast<std::uint64_t>(-1);
  if(hash==TransferN::PlayerActive)return a.at(0)==1;
  if(hash==TransferN::PlayerPedByIndex)return 2;
  if(hash==TransferN::EntityExists)return positions.count(entity());
  if(hash==Hash("FIND_FIRST_VEHICLE")){index=0;*reinterpret_cast<int*>(a.at(0))=vehicles.empty()?0:vehicles[0];return vehicles.empty()?static_cast<std::uint64_t>(-1):4;}
  if(hash==Hash("FIND_NEXT_VEHICLE")){++index;if(index>=vehicles.size())return 0;*reinterpret_cast<int*>(a.at(1))=vehicles[index];return 1;}
  if(hash==Hash("END_FIND_VEHICLE")){ends++;return 0;}
  if(hash==TransferN::GetVehicleNumberOfPassengers)return occupied.count(entity());
  if(hash==TransferN::IsVehicleSeatFree)return occupied.count(entity())?0:1;
  if(hash==TransferN::NetworkGetEntityIsNetworked)return 1;
  if(hash==TransferN::NetworkHasControlOfEntity)return noControl.count(entity())?0:1;
  if(hash==TransferN::SetEntityCoordsNoOffset){assert(!occupied.count(entity())&&!noControl.count(entity()));assert(a.size()==7);if(throwOnMove)throw std::runtime_error("test failure");writes++;if(!reject)positions[entity()]={AsFloat(a.at(1)),AsFloat(a.at(2)),AsFloat(a.at(3))};return 0;}
  if(hash==TransferN::ActivatePhysics){physics++;return 0;}
  if(hash==TransferN::SetEntityVelocity){velocityZ=AsFloat(a.at(3));return 0;}
  assert(false);return 0;
 }
};
Frame FrameAt(std::uint64_t now){Frame f{};f.valid=1;f.heartbeat=now;f.forward={0,1,0};return f;}
Command Request(bool rain=true){Command c{};c.id=1;c.action=Action::TransferVehicles;c.targetServerId=5137;c.flags=rain?1u:0u;return c;}
int main(){
 static_assert(alignof(RageContext)==16&&offsetof(RageContext,vectorSpace)==32,"CFX scrThread.h context alignment");
 {std::array<std::uint64_t,4> result{Float(1.25f),Float(-3.5f),Float(9.f),0};auto v=DecodeNativeVector(result);assert(v.x==1.25f&&v.y==-3.5f&&v.z==9.f);}
 {FakeTransfer api;NativeVehicleTransfer transfer(api);transfer.Submit(Request(),FrameAt(1000),1000);assert(transfer.Pending()&&api.writes==2&&api.ends==1&&api.physics==2&&api.velocityZ<0);transfer.Tick(FrameAt(1050),1050,0);assert(!transfer.hasReply);transfer.Tick(FrameAt(1100),1100,0);assert(transfer.hasReply&&transfer.reply.success&&transfer.reply.value==2);}
 {FakeTransfer api;api.noControl.insert(10);api.occupied.insert(11);NativeVehicleTransfer transfer(api);transfer.Submit(Request(),FrameAt(1000),1000);assert(transfer.hasReply&&!transfer.reply.success&&api.writes==0&&api.ends==1);}
 {FakeTransfer api;api.reject=true;NativeVehicleTransfer transfer(api);transfer.Submit(Request(),FrameAt(1000),1000);transfer.Tick(FrameAt(1100),1100,0);assert(transfer.hasReply&&!transfer.reply.success&&transfer.reply.value==0&&api.writes==2);}
 {FakeTransfer api;api.vehicles.clear();for(int i=10;i<40;i++){api.vehicles.push_back(i);api.positions[i]={static_cast<float>(i),0,0};}NativeVehicleTransfer transfer(api);transfer.Submit(Request(false),FrameAt(1000),1000);transfer.Tick(FrameAt(1100),1100,0);assert(transfer.reply.success&&api.writes==10&&api.ends==1&&api.velocityZ==0);}
 {FakeTransfer api;api.throwOnMove=true;NativeVehicleTransfer transfer(api);bool caught=false;try{transfer.Submit(Request(),FrameAt(1000),1000);}catch(const std::runtime_error&){caught=true;}assert(caught&&api.ends==1);}
 {FakeTransfer api;NativeVehicleTransfer transfer(api);transfer.Submit(Request(),FrameAt(1000),1000);transfer.Tick(FrameAt(1100),1100,1);assert(!transfer.Pending()&&!transfer.reply.success&&api.writes==2);}
 {FakeTransfer api;auto c=Request();c.targetServerId=0;NativeVehicleTransfer transfer(api);transfer.Submit(c,FrameAt(1000),1000);assert(transfer.hasReply&&!transfer.reply.success&&api.writes==0&&api.ends==0);}
 {FakeTransfer api;api.positions[10]={500,0,0};api.positions[11]={600,0,0};NativeVehicleTransfer transfer(api);transfer.Submit(Request(),FrameAt(1000),1000);assert(transfer.hasReply&&!transfer.reply.success&&api.writes==0);}
}

