#include "../native/NativeWaypoint.hpp"
#include <cassert>
#include <limits>
#include <iostream>
using namespace MenuBridge;
struct Fake final:NativeApi {
 int player=1,vehicle=0,driver=1,passengers=0,blip=7,moves=0,starts=0,stops=0,groundCalls=0;
 bool exists=true,dead=false,attached=false,controlled=true,networked=true;
 bool active=false,startAllowed=true,loaded=true,groundAvailable=true,acceptMove=true,throwGround=false;
 float ground=30.f;
 Vec marker{100,200,0},position{1,2,3};
 Vec Vector(std::uint64_t hash,const std::vector<std::uint64_t>&)override {
  if(hash==WaypointN::BlipCoords)return marker;
  assert(hash==N::EntityCoords);return position;
 }
 std::uint64_t Call(std::uint64_t hash,const std::vector<std::uint64_t>& a)override {
  if(hash==N::PlayerPed)return static_cast<std::uint64_t>(player);
  if(hash==N::EntityExists)return exists;
  if(hash==N::Dead)return dead;
  if(hash==N::Attached)return attached;
  if(hash==N::InVehicle)return vehicle!=0;
  if(hash==N::CurrentVehicle)return static_cast<std::uint64_t>(vehicle);
  if(hash==WaypointN::Driver){assert(a.at(1)==static_cast<std::uint64_t>(-1));return static_cast<std::uint64_t>(driver);}
  if(hash==N::Passengers)return static_cast<std::uint64_t>(passengers);
  if(hash==N::Networked)return networked;
  if(hash==N::HasControl)return controlled;
  if(hash==WaypointN::FirstBlip){assert(a.at(0)==8);return static_cast<std::uint64_t>(blip);}
  if(hash==WaypointN::BlipExists)return blip!=0;
  if(hash==WaypointN::SceneActive)return active;
  if(hash==WaypointN::SceneStart){starts++;if(startAllowed)active=true;return startAllowed;}
  if(hash==WaypointN::SceneLoaded)return loaded;
  if(hash==WaypointN::SceneStop){stops++;active=false;return 0;}
  if(hash==WaypointN::Ground){
   if(throwGround)throw std::runtime_error("missing native");
   groundCalls++;assert(a.size()==6&&AsFloat(a.at(2))==1500.f&&a.at(4)==0);
   *reinterpret_cast<float*>(a.at(3))=ground;return groundAvailable;
  }
  if(hash==WaypointN::SetCoords){
   moves++;assert(a.size()==7&&a.at(0)==static_cast<std::uint64_t>(vehicle?vehicle:player));
   if(acceptMove)position={AsFloat(a.at(1)),AsFloat(a.at(2)),AsFloat(a.at(3))};
   return 0;
  }
  // No ownership requests, memory writes, fallback heights or unknown natives.
  assert(false);return 0;
 }
};
Frame frame(std::uint64_t now){Frame f{};f.valid=1;f.heartbeat=now;f.gameBuild=3889;return f;}
Command command(){Command c{};c.id=42;c.action=Action::TeleportWaypoint;return c;}
void step(NativeWaypoint& w,std::uint64_t now){w.Tick(frame(now),now,0);}
int main(){
 for(const auto build:{3258u,3889u}){
  Fake api;NativeWaypoint w(api);auto f=frame(1000);f.gameBuild=build;
  w.Submit(command(),f,1000);assert(w.Pending()&&api.moves==0);
  step(w,1000);assert(api.moves==1&&w.Pending()&&api.position.z==31.f);
  step(w,1200);assert(!w.hasReply);
  step(w,1250);assert(w.reply.success&&!w.Pending()&&api.stops==1);
  step(w,1300);assert(api.moves==1&&api.stops==1);
 }
 {Fake api;api.vehicle=50;NativeWaypoint w(api);w.Submit(command(),frame(1000),1000);step(w,1000);step(w,1250);assert(w.reply.success&&api.moves==1);}
 {Fake api;api.driver=2;api.vehicle=50;NativeWaypoint w(api);w.Submit(command(),frame(1000),1000);assert(!w.reply.success&&api.starts==0&&api.moves==0);}
 {Fake api;api.vehicle=50;api.passengers=1;NativeWaypoint w(api);w.Submit(command(),frame(1000),1000);assert(!w.reply.success&&api.starts==0);}
 {Fake api;api.dead=true;NativeWaypoint w(api);w.Submit(command(),frame(1000),1000);assert(!w.reply.success&&api.starts==0);}
 {Fake api;api.attached=true;NativeWaypoint w(api);w.Submit(command(),frame(1000),1000);assert(!w.reply.success&&api.starts==0);}
 {Fake api;api.exists=false;NativeWaypoint w(api);w.Submit(command(),frame(1000),1000);assert(!w.reply.success&&api.starts==0);}
 {Fake api;api.controlled=false;NativeWaypoint w(api);w.Submit(command(),frame(1000),1000);assert(!w.reply.success&&api.starts==0);}
 {Fake api;api.blip=0;NativeWaypoint w(api);w.Submit(command(),frame(1000),1000);assert(!w.reply.success&&api.starts==0);}
 for(auto value:{std::numeric_limits<float>::quiet_NaN(),std::numeric_limits<float>::infinity(),100000.f}){
  Fake api;api.marker.x=value;NativeWaypoint w(api);w.Submit(command(),frame(1000),1000);assert(!w.reply.success&&api.starts==0);
 }
 {Fake api;api.active=true;NativeWaypoint w(api);w.Submit(command(),frame(1000),1000);assert(!w.reply.success&&api.starts==0&&api.stops==0&&api.active);}
 {Fake api;api.startAllowed=false;NativeWaypoint w(api);w.Submit(command(),frame(1000),1000);assert(!w.reply.success&&!w.Pending()&&api.stops==0);}
 {Fake api;api.loaded=false;NativeWaypoint w(api);w.Submit(command(),frame(1000),1000);
  for(std::uint64_t now=1000;now<=6000;now+=100)step(w,now);
  assert(!w.reply.success&&!w.Pending()&&api.moves==0&&api.stops==1&&api.groundCalls==0);}
 {Fake api;api.groundAvailable=false;NativeWaypoint w(api);w.Submit(command(),frame(1000),1000);
  for(std::uint64_t now=1000;now<=6000;now+=100)step(w,now);
  assert(!w.reply.success&&api.moves==0&&api.stops==1&&api.groundCalls<=50);}
 for(auto value:{std::numeric_limits<float>::quiet_NaN(),std::numeric_limits<float>::infinity(),-501.f,1500.f}){
  Fake api;api.ground=value;NativeWaypoint w(api);w.Submit(command(),frame(1000),1000);step(w,1000);assert(!w.reply.success&&api.moves==0&&api.stops==1);
 }
 {Fake api;api.ground=0.f;NativeWaypoint w(api);w.Submit(command(),frame(1000),1000);step(w,1000);step(w,1250);assert(w.reply.success&&api.position.z==1.f);}
 {Fake api;NativeWaypoint w(api);w.Submit(command(),frame(1000),1000);api.marker.x+=10;step(w,1100);assert(!w.reply.success&&api.moves==0&&api.stops==1);}
 {Fake api;NativeWaypoint w(api);w.Submit(command(),frame(1000),1000);api.blip=0;step(w,1100);assert(!w.reply.success&&api.moves==0&&api.stops==1);}
 {Fake api;NativeWaypoint w(api);w.Submit(command(),frame(1000),1000);api.player=2;step(w,1100);assert(!w.reply.success&&api.moves==0&&api.stops==1);}
 {Fake api;NativeWaypoint w(api);w.Submit(command(),frame(1000),1000);api.vehicle=50;step(w,1100);assert(!w.reply.success&&api.moves==0&&api.stops==1);}
 {Fake api;NativeWaypoint w(api);w.Submit(command(),frame(1000),1000);api.controlled=false;step(w,1100);assert(!w.reply.success&&api.moves==0&&api.stops==1);}
 {Fake api;NativeWaypoint w(api);w.Submit(command(),frame(1000),1000);w.Tick(frame(1100),1100,42);assert(!w.reply.success&&api.moves==0&&api.stops==1);}
 {Fake api;NativeWaypoint w(api);w.Submit(command(),frame(1000),1000);w.Tick(frame(1000),3000,0);assert(!w.reply.success&&api.moves==0&&api.stops==1);}
 {Fake api;NativeWaypoint w(api);w.Submit(command(),frame(1000),1000);step(w,999);assert(!w.reply.success&&api.moves==0&&api.stops==1);}
 {Fake api;api.acceptMove=false;NativeWaypoint w(api);w.Submit(command(),frame(1000),1000);step(w,1000);step(w,1250);step(w,1500);assert(!w.reply.success&&api.moves==1&&api.stops==1);}
 {Fake api;NativeWaypoint w(api);w.Submit(command(),frame(1000),1000);step(w,1000);api.position={1,2,3};step(w,1250);assert(!w.reply.success&&api.moves==1);}
 {Fake api;NativeWaypoint w(api);w.Submit(command(),frame(1000),1000);w.Abort("exception");w.Abort("again");assert(!w.reply.success&&!w.Pending()&&api.moves==0&&api.stops==1);}
 {Fake api;api.throwGround=true;NativeWaypoint w(api);w.Submit(command(),frame(1000),1000);
  try{step(w,1000);assert(false);}catch(const std::runtime_error&){w.Abort("missing native");}
  assert(!w.reply.success&&!w.Pending()&&api.moves==0&&api.stops==1);}
 {Fake api;NativeWaypoint w(api);w.Submit(command(),frame(1000),1000);w.Reset();step(w,1100);assert(!w.Pending()&&!w.hasReply&&api.moves==0);}
 {Fake api;NativeWaypoint w(api);auto invalid=command();invalid.id=0;w.Submit(invalid,frame(1000),1000);assert(!w.reply.success&&api.starts==0);}
 std::cout<<"Waypoint success, cancellation, ownership, ground and confirmation tests passed\n";
}
