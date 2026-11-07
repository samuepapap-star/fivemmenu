#include "../native/NativeCamera.hpp"
#include <cassert>
#include <limits>
using namespace MenuBridge;
// https://raw.githubusercontent.com/citizenfx/natives/master/CAM/DestroyCam.md
static_assert(CameraN::DestroyCam==0x865908C81A2C22E9ULL, "CFX camera hash mismatch");
// https://raw.githubusercontent.com/citizenfx/natives/master/CAM/DoesCamExist.md
static_assert(CameraN::DoesCamExist==0xA7A932170592B50EULL, "CFX camera hash mismatch");
// https://raw.githubusercontent.com/citizenfx/natives/master/CAM/CreateCamWithParams.md
static_assert(CameraN::CreateCamWithParams==0xB51194800B257161ULL, "CFX camera hash mismatch");
// https://raw.githubusercontent.com/citizenfx/natives/master/CAM/RenderScriptCams.md
static_assert(CameraN::RenderScriptCams==0x07E5B515DB0636FCULL, "CFX camera hash mismatch");
// https://raw.githubusercontent.com/citizenfx/natives/master/CAM/SetCamCoord.md
static_assert(CameraN::SetCamCoord==0x4D41783FB745E42EULL, "CFX camera hash mismatch");
// https://raw.githubusercontent.com/citizenfx/natives/master/PAD/DisableAllControlActions.md
static_assert(CameraN::DisableAllControlActions==0x5F4B6931816E599BULL, "CFX camera hash mismatch");
// https://raw.githubusercontent.com/citizenfx/natives/master/PAD/GetDisabledControlNormal.md
static_assert(CameraN::GetDisabledControlNormal==0x11E65974A982637CULL, "CFX camera hash mismatch");
// https://raw.githubusercontent.com/citizenfx/natives/master/STREAMING/SetFocusPosAndVel.md
static_assert(CameraN::SetFocusPosAndVel==0xBB7454BAFF08FE25ULL, "CFX camera hash mismatch");
// https://raw.githubusercontent.com/citizenfx/natives/master/STREAMING/ClearFocus.md
static_assert(CameraN::ClearFocus==0x31B73D1EA9F01DA2ULL, "CFX camera hash mismatch");
// https://raw.githubusercontent.com/citizenfx/natives/master/CAM/SetCamRot.md
static_assert(CameraN::SetCamRot==0x85973643155D0B07ULL, "CFX camera hash mismatch");
struct CameraFake:NativeApi {
 int player=1,creates=0,destroys=0,renders=0,unrenders=0,clears=0,controls=0,moves=0;
 bool exists=false,deny=false;float mouseX=0,mouseY=0;
 std::uint64_t Call(std::uint64_t h,const std::vector<std::uint64_t>& a)override{
  if(h==CameraN::PlayerPed)return static_cast<std::uint64_t>(player);
  if(h==CameraN::CreateCamWithParams){assert(a.size()==10&&a.at(8)==1&&a.at(9)==2);creates++;exists=!deny;return deny?0:77;}
  if(h==CameraN::DoesCamExist)return exists;
  if(h==CameraN::RenderScriptCams){assert(a.size()==6);if(a.at(0))renders++;else unrenders++;return 0;}
  if(h==CameraN::DestroyCam){assert(a.at(0)==77&&a.at(1)==1);destroys++;exists=false;return 0;}
  if(h==CameraN::ClearFocus){clears++;return 0;}
  if(h==CameraN::SetFocusPosAndVel){assert(a.size()==6);return 0;}
  if(h==CameraN::DisableAllControlActions){controls++;return 0;}
  if(h==CameraN::GetDisabledControlNormal)return Float(a.at(1)==1?mouseX:mouseY);
  if(h==CameraN::SetCamCoord){moves++;for(std::size_t i=1;i<a.size();i++)assert(std::isfinite(AsFloat(a[i])));return 0;}
  if(h==CameraN::SetCamRot)return 0;
  assert(false);return 0;
 }
};
Frame FrameAt(std::uint64_t now){Frame f{};f.heartbeat=now;f.valid=f.camera=1;f.position={10,20,30};f.forward={0,1,0};return f;}
Command Start(){Command c{};c.id=1;c.action=Action::StartCamera;return c;}
int main(){
 {CameraFake api;NativeCamera camera(api);api.deny=true;auto r=camera.Submit(Start(),FrameAt(1000),1000);assert(!r.success&&!camera.Active()&&api.renders==0);}
 {CameraFake api;NativeCamera camera(api);auto f=FrameAt(1000);auto r=camera.Submit(Start(),f,1000);assert(r.success&&camera.Active()&&api.renders==1);f=FrameAt(1100);f.input=1;f.cameraMoves=Forward;f.cameraSpeed=10;float before=camera.Pose().position.y;camera.Tick(f,1100);assert(camera.Pose().position.y>before&&api.controls==1);camera.Tick(f,4000);assert(!camera.Active()&&api.unrenders==1&&api.destroys==1&&api.clears==1);}
 {CameraFake api;NativeCamera camera(api);auto f=FrameAt(1000);camera.Submit(Start(),f,1000);f=FrameAt(1400);f.camera=0;camera.Tick(f,1400);assert(!camera.Active()&&api.unrenders==1);}
 {CameraFake api;NativeCamera camera(api);auto f=FrameAt(1000);camera.Submit(Start(),f,1000);api.player=2;camera.Tick(FrameAt(1100),1100);assert(!camera.Active());}
 {CameraFake api;NativeCamera camera(api);camera.Submit(Start(),FrameAt(1000),1000);Command c=Start();c.action=Action::StopCamera;auto r=camera.Submit(c,FrameAt(1100),1100);assert(r.success&&!camera.Active());camera.Stop();assert(api.unrenders==1&&api.destroys==1);}
 {CameraFake api;NativeCamera camera(api);camera.Submit(Start(),FrameAt(1000),1000);auto f=FrameAt(1100);f.input=0;f.cameraMoves=Forward;auto before=camera.Pose().position;camera.Tick(f,1100);assert(camera.Pose().position.y==before.y&&api.moves==0);}
 {CameraPose slow{},fast{},diagonal{};auto f=FrameAt(1000);f.cameraMoves=Forward;f.cameraSpeed=2;MoveCamera(slow,f,0,0,0.05f);f.cameraSpeed=20;MoveCamera(fast,f,0,0,0.05f);assert(fast.position.y>slow.position.y*9.f);f.cameraMoves=Forward|Right;MoveCamera(diagonal,f,0,0,0.05f);float d=std::sqrt(diagonal.position.x*diagonal.position.x+diagonal.position.y*diagonal.position.y);assert(std::fabs(d-1.f)<0.001f);f.cameraSpeed=std::numeric_limits<float>::quiet_NaN();MoveCamera(slow,f,0,100,1);assert(Sane(slow.position)&&slow.pitch>=-89&&slow.pitch<=89);}
}

