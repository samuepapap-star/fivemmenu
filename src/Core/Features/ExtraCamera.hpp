#pragma once
#include <Core/Features/ExtraFeatures.hpp>
#include <array>
namespace Core { namespace Features { namespace ExtraCamera {
inline bool freecam=false;inline float speed=10.f;
inline uintptr_t target=0,site=0,camera=0,pedOwner=0;inline DWORD pid=0;
inline std::array<uint8_t,32> saved{},replacement{};
inline bool patched=false;
inline D3DXVECTOR3 position{};
inline const char* status="Desativada";
inline Extras::Override<float> frozenModel;
inline bool Bytes(uintptr_t address,void* out,size_t count){SIZE_T n=0;return ReadProcessMemory(Mem.ProcHandle,(LPCVOID)address,out,count,&n)&&n==count;}
inline bool WriteBytes(uintptr_t address,const void* data,size_t count){
 DWORD old=0;if(!VirtualProtectEx(Mem.ProcHandle,(LPVOID)address,count,PAGE_EXECUTE_READWRITE,&old))return false;
 SIZE_T n=0;bool ok=WriteProcessMemory(Mem.ProcHandle,(LPVOID)address,data,count,&n)&&n==count;
 DWORD ignored=0;bool restored=VirtualProtectEx(Mem.ProcHandle,(LPVOID)address,count,old,&ignored)!=0;
 FlushInstructionCache(Mem.ProcHandle,(LPCVOID)address,count);return ok&&restored;
}
inline bool Match(const uint8_t* b){
 // HyperX's follow-camera position writer; validate all three displacement operands.
 const uint8_t pattern[]={0xf3,0x0f,0x11,0x47,0x60,0xf3,0x0f,0x10,0x44,0x24,0,0xf3,0x0f,0x11,0x4f,0x64,0xf3,0x0f,0x10,0x4c,0x24,0,0xf3,0x0f,0x11,0x47,0x68,0xf3,0x0f,0x11,0x4f,0};
 for(size_t i=0;i<32;i++)if(i!=10&&i!=21&&i!=31&&b[i]!=pattern[i])return false;return true;
}
inline uintptr_t Find(){
 IMAGE_DOS_HEADER dos{};IMAGE_NT_HEADERS64 nt{};
 if(!Bytes(Mem.ModBase,&dos,sizeof(dos))||dos.e_magic!=IMAGE_DOS_SIGNATURE||dos.e_lfanew<=0||dos.e_lfanew>0x100000||!Bytes(Mem.ModBase+dos.e_lfanew,&nt,sizeof(nt))||nt.Signature!=IMAGE_NT_SIGNATURE||nt.FileHeader.NumberOfSections>96)return 0;
 uintptr_t headers=Mem.ModBase+dos.e_lfanew+sizeof(DWORD)+sizeof(IMAGE_FILE_HEADER)+nt.FileHeader.SizeOfOptionalHeader,result=0;
 for(unsigned i=0;i<nt.FileHeader.NumberOfSections;i++){
  IMAGE_SECTION_HEADER section{};if(!Bytes(headers+i*sizeof(section),&section,sizeof(section))||!(section.Characteristics&IMAGE_SCN_MEM_EXECUTE)||section.VirtualAddress>=Mem.ModBaseSize)continue;
  size_t length=(std::min)(size_t(section.Misc.VirtualSize),size_t(Mem.ModBaseSize-section.VirtualAddress));
  for(size_t offset=0;offset<length;offset+=4096){size_t size=(std::min)(size_t(4096+31),length-offset);std::vector<uint8_t> b(size);SIZE_T n=0;ReadProcessMemory(Mem.ProcHandle,(LPCVOID)(Mem.ModBase+section.VirtualAddress+offset),b.data(),size,&n);
   for(size_t at=0;at+32<=n&&at<4096;at++)if(Match(b.data()+at)){uintptr_t candidate=Mem.ModBase+section.VirtualAddress+offset+at;if(result&&result!=candidate)return 0;result=candidate;}
  }
 }return result;
}
inline void Stop(){
 if(patched){if(pid!=Mem.ProcId){patched=false;site=0;}else{
  std::array<uint8_t,32> current{};if(!Bytes(site,current.data(),current.size())){status="Falha ao ler camera para restaurar";return;}
  if(current==replacement){if(!WriteBytes(site,saved.data(),saved.size())){status="Falha ao restaurar camera";return;}}
  else if(current!=saved){
   bool ownPartial=true;for(size_t i=0;i<current.size();i++)if(current[i]!=saved[i]&&current[i]!=replacement[i]){ownPartial=false;break;}
   if(!ownPartial||!WriteBytes(site,saved.data(),saved.size())){status="Camera alterada por outro modulo; restauracao pendente";return;}
  }
  patched=false;site=0;
 }}
 frozenModel.Restore();freecam=false;target=0;camera=0;pedOwner=0;status="Desativada";
}
inline bool Start(uintptr_t spectate=0){
 Stop();if(patched)return false;
 auto ped=Extras::Player();float health=0;uintptr_t cam=0;
 if(!ped||!g_MenuInfo.IsLogged||!Extras::Read((uintptr_t)ped+0x280,health)||health<=100.f||g_Config.Player->NoClipEnabled||g_Config.Player->NoCollisionEnabled||
 !Extras::Read(SDK::Pointers::pCamGamePlayDirector+0x2C0,cam)||cam<0x10000||!Extras::Read(cam+0x60,position)||!Extras::Sane(position)){status="Camera indisponivel ou voo/sem colisao ativo";return false;}
 site=Find();if(!site||!Bytes(site,saved.data(),saved.size())||!Match(saved.data())){site=0;status="Rotina de camera nao identificada nesta revisao";return false;}
 replacement=saved;for(int at:{0,11,22})for(int i=0;i<5;i++)replacement[at+i]=0x90;
 pid=Mem.ProcId;camera=cam;pedOwner=(uintptr_t)ped;
 // Mark before writing so an unsuccessful/partial transfer is never forgotten.
 patched=true;if(!WriteBytes(site,replacement.data(),replacement.size())){Stop();status="Falha ao ativar camera";return false;}
 target=spectate;freecam=!spectate;status=spectate?"Observando jogador":"Camera livre: WASD, espaco, Ctrl; Shift acelera";return true;
}
inline void Tick(bool stop=false){
 if(stop){Stop();return;}if(!patched)return;
 auto p=Extras::Player();float health=0;uintptr_t cam=0;
 if(!g_MenuInfo.IsLogged||pid!=Mem.ProcId||pedOwner!=(uintptr_t)p||!p||!Extras::Read((uintptr_t)p+0x280,health)||health<=100.f||!Extras::Read(SDK::Pointers::pCamGamePlayDirector+0x2C0,cam)||cam!=camera||p->InVehicle()||g_Config.Player->NoClipEnabled||g_Config.Player->NoCollisionEnabled){Stop();return;}
 DWORD foreground=0;GetWindowThreadProcessId(GetForegroundWindow(),&foreground);
 if(GetForegroundWindow()!=g_Variables.g_hGameWindow&&!(g_MenuInfo.IsOpen&&foreground==GetCurrentProcessId())){Stop();return;}
 uintptr_t model=0;Extras::Read((uintptr_t)p+0x20,model);frozenModel.Set(model>=0x10000?model+0x2C:0,true,0.f,-100.f,100.f);
 D3DXVECTOR3 direction;if(!Extras::Read(camera+0x40,direction)||!Extras::Sane(direction)){Stop();return;}
 float length=D3DXVec3Length(&direction);if(length<0.1f||length>2.f){Stop();return;}direction*=1.f/length;
 if(target){
  bool exists=false;{for(const auto&e:SDK::Game::SnapshotEntities())if((uintptr_t)e.Ped==target){exists=true;break;}}
  D3DXVECTOR3 pos;if(!exists||!Extras::Read(target+0x90,pos)||!Extras::Sane(pos)){Stop();return;}position=pos-direction*6.f+D3DXVECTOR3(0,0,2.f);
 }else if(!g_MenuInfo.IsOpen){
  float movement=ImClamp(speed,1.f,50.f)*ImClamp(ImGui::GetIO().DeltaTime,0.f,0.05f);if(GetAsyncKeyState(VK_SHIFT)&0x8000)movement*=2;
  if(GetAsyncKeyState('W')&0x8000)position+=direction*movement;if(GetAsyncKeyState('S')&0x8000)position-=direction*movement;
  D3DXVECTOR3 right(direction.y,-direction.x,0);
  if(GetAsyncKeyState('D')&0x8000)position+=right*movement;if(GetAsyncKeyState('A')&0x8000)position-=right*movement;
  if(GetAsyncKeyState(VK_SPACE)&0x8000)position.z+=movement;if(GetAsyncKeyState(VK_CONTROL)&0x8000)position.z-=movement;
 }
 if(!Extras::Sane(position)||!Extras::Write(camera+0x60,position)){Stop();status="Falha ao mover camera";}
}
}}}
