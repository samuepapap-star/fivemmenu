#include "../native/NativeVehicleSeats.hpp"
#include <cassert>
using namespace MenuBridge;
struct Fake:NativeApi{
 std::uint64_t occupant=0;bool control=true,apply=true;int warps=0,leaves=0,closed=0;
 std::uint64_t Call(std::uint64_t h,const std::vector<std::uint64_t>& a)override{
  if(h==SeatN::PlayerPed)return 10;
  if(h==SeatN::Exists)return 1;
  if(h==Hash("FIND_FIRST_VEHICLE")){*reinterpret_cast<int*>(a[0])=20;return 1;}
  if(h==Hash("FIND_NEXT_VEHICLE"))return 0;
  if(h==Hash("END_FIND_VEHICLE")){closed++;return 0;}
  if(h==Hash("GET_ENTITY_ADDRESS"))return 0x10000;
  if(h==SeatN::Occupant)return occupant;
  if(h==SeatN::Networked)return 1;
  if(h==SeatN::Control)return control;
  if(h==SeatN::Warp){assert(a[2]==static_cast<std::uint64_t>(-1));warps++;if(apply)occupant=10;return 0;}
  if(h==SeatN::Leave){assert(a[0]==30&&a[1]==20&&a[2]==16);leaves++;if(apply)occupant=0;return 0;}
  if(h==0xA3EE4A07279BB9DBULL)return occupant!=0;
  assert(false);return 0;
 }
 Vec Vector(std::uint64_t,const std::vector<std::uint64_t>&)override{return {};}
};
int main(){
 Fake api;VehicleSeats seats(api);Command c{};c.id=1;c.action=Action::EnterDriver;c.targetEntityAddress=0x10000;
 assert(Valid(c));seats.Submit(c,0);assert(seats.Pending());seats.Tick(1);assert(seats.reply.success&&api.warps==1&&api.closed==1);
 api.occupant=30;seats.Submit(c,2);assert(!seats.Pending()&&!seats.reply.success&&api.warps==1);
 c.action=Action::EjectSeat;c.flags=1;assert(Valid(c));api.control=false;
 seats.Submit(c,3);assert(!seats.Pending()&&!seats.reply.success&&api.leaves==0);
 api.control=true;seats.Submit(c,4);seats.Tick(5);assert(seats.reply.success&&api.leaves==1);
 api.occupant=30;api.apply=false;c.flags=0;seats.Submit(c,6);seats.Tick(7);assert(seats.Pending());seats.Tick(4007);assert(!seats.Pending()&&!seats.reply.success);
 c.flags=2;assert(!Valid(c));c.flags=0;c.targetEntityAddress=0;assert(!Valid(c));
}
