#pragma once
#ifndef NOMINMAX
#define NOMINMAX
#endif
#include <Windows.h>
#include <TlHelp32.h>
#include <filesystem>
#include <thread>
#include <mutex>
#include <condition_variable>
#include <optional>
#include <atomic>
#include <stdexcept>
#include <vector>
#include "Protocol.hpp"

namespace MenuBridge {
class Client {
 std::mutex mutex;std::condition_variable wake;std::thread worker;
 bool stop=false,working=false;std::optional<Command> job;
 DWORD pid=0,blockedPid=0;HANDLE process=nullptr,mapping=nullptr;Shared* shared=nullptr;
 Frame lastFrame{};std::string status="Aguardando acao";std::uint64_t serial=0;
 static std::int32_t State(std::int32_t& s){return InterlockedCompareExchange(reinterpret_cast<volatile LONG*>(&s),0,0);}
 struct Module {std::uintptr_t base=0;std::uint32_t size=0;std::wstring path;};
 static Module Find(DWORD target,const std::wstring& name){
  HANDLE snapshot=CreateToolhelp32Snapshot(TH32CS_SNAPMODULE|TH32CS_SNAPMODULE32,target);Module out;
  if(snapshot==INVALID_HANDLE_VALUE)return out;
  MODULEENTRY32W m{};m.dwSize=sizeof(m);
  if(Module32FirstW(snapshot,&m))do{if(_wcsicmp(m.szModule,name.c_str())==0){out={reinterpret_cast<std::uintptr_t>(m.modBaseAddr),m.modBaseSize,m.szExePath};break;}}while(Module32NextW(snapshot,&m));CloseHandle(snapshot);return out;
 }
 template<class T>static bool Read(HANDLE h,std::uintptr_t p,T& out){SIZE_T bytes=0;return p&&ReadProcessMemory(h,reinterpret_cast<LPCVOID>(p),&out,sizeof(out),&bytes)&&bytes==sizeof(out);}
 static bool SameImage(HANDLE h,Module remote,HMODULE local){
  if(!remote.base||!local)return false;
  auto b=reinterpret_cast<const std::uint8_t*>(local);auto d=reinterpret_cast<const IMAGE_DOS_HEADER*>(b);
  if(d->e_magic!=IMAGE_DOS_SIGNATURE||d->e_lfanew<=0||d->e_lfanew>0x100000)return false;
  auto n=reinterpret_cast<const IMAGE_NT_HEADERS64*>(b+d->e_lfanew);IMAGE_DOS_HEADER rd{};IMAGE_NT_HEADERS64 rn{};
  return Read(h,remote.base,rd)&&rd.e_magic==IMAGE_DOS_SIGNATURE&&rd.e_lfanew>0&&rd.e_lfanew<0x100000&&
   Read(h,remote.base+rd.e_lfanew,rn)&&rn.Signature==IMAGE_NT_SIGNATURE&&rn.FileHeader.Machine==IMAGE_FILE_MACHINE_AMD64&&
   rn.FileHeader.TimeDateStamp==n->FileHeader.TimeDateStamp&&rn.OptionalHeader.SizeOfImage==n->OptionalHeader.SizeOfImage;
 }
 void Message(const std::string& value){std::lock_guard<std::mutex> lock(mutex);status=value;}
 bool Stopping(){std::lock_guard<std::mutex> lock(mutex);return stop;}
 bool Wait(HANDLE thread,DWORD milliseconds){
  const auto end=GetTickCount64()+milliseconds;
  while(GetTickCount64()<end){if(WaitForSingleObject(thread,25)==WAIT_OBJECT_0)return true;if(Stopping())return false;}
  return false;
 }
 bool Map(DWORD target){
  const auto name=L"Local\\FiveMMenuActions_"+std::to_wstring(target);
  HANDLE handle=OpenFileMappingW(FILE_MAP_ALL_ACCESS,FALSE,name.c_str());if(!handle)return false;
  auto view=static_cast<Shared*>(MapViewOfFile(handle,FILE_MAP_ALL_ACCESS,0,0,sizeof(Shared)));
  if(!view){CloseHandle(handle);return false;}
  if(view->signature!=magic||view->protocol!=version||view->pid!=target||view->size!=sizeof(Shared)){
   UnmapViewOfFile(view);CloseHandle(handle);throw std::runtime_error("Outra versao da ponte esta carregada; reinicie o FiveM");
  }
  if(view->clientPid&&view->clientPid!=GetCurrentProcessId()){
   HANDLE owner=OpenProcess(SYNCHRONIZE,FALSE,view->clientPid);
   const bool running=owner&&WaitForSingleObject(owner,0)==WAIT_TIMEOUT;if(owner)CloseHandle(owner);
   if(running){UnmapViewOfFile(view);CloseHandle(handle);throw std::runtime_error("Outra instancia do menu usa esta ponte");}
  }
  std::lock_guard<std::mutex> lock(mutex);
  if(pid!=target){UnmapViewOfFile(view);CloseHandle(handle);throw std::runtime_error("Processo do jogo alterado");}
  mapping=handle;shared=view;view->clientPid=GetCurrentProcessId();
  InterlockedIncrement(reinterpret_cast<volatile LONG*>(&view->frameSequence));MemoryBarrier();view->frame=lastFrame;MemoryBarrier();InterlockedIncrement(reinterpret_cast<volatile LONG*>(&view->frameSequence));return true;
 }
 void Connect(DWORD target,HANDLE h){
  {std::lock_guard<std::mutex> lock(mutex);if(shared)return;}
  if(Map(target))return;
  if(blockedPid==target)throw std::runtime_error("Carregamento anterior pendente; reinicie o FiveM antes de tentar novamente");
  wchar_t filename[32768]{};if(!GetModuleFileNameW(nullptr,filename,32768))throw std::runtime_error("Pasta do menu indisponivel");
  const auto dll=std::filesystem::path(filename).parent_path()/L"FiveMMenuBridge.dll";
  if(!std::filesystem::exists(dll))throw std::runtime_error("Coloque FiveMMenuBridge.dll junto do EXE extraido do ZIP novo");
  HMODULE local=LoadLibraryExW(dll.c_str(),nullptr,DONT_RESOLVE_DLL_REFERENCES);
  if(!local)throw std::runtime_error("DLL da ponte invalida");
  const auto start=GetProcAddress(local,"FiveMMenuBridgeStart");
  if(!start){FreeLibrary(local);throw std::runtime_error("DLL sem a entrada da ponte");}
  const auto startRva=reinterpret_cast<std::uintptr_t>(start)-reinterpret_cast<std::uintptr_t>(local);
  auto module=Find(target,L"FiveMMenuBridge.dll");
  if(!module.base){
   auto load=GetProcAddress(GetModuleHandleW(L"kernel32.dll"),"LoadLibraryW");HMODULE owner=nullptr;
   if(!load||!GetModuleHandleExW(GET_MODULE_HANDLE_EX_FLAG_FROM_ADDRESS|GET_MODULE_HANDLE_EX_FLAG_UNCHANGED_REFCOUNT,reinterpret_cast<LPCWSTR>(load),&owner)){FreeLibrary(local);throw std::runtime_error("Carregador do Windows indisponivel");}
   wchar_t ownerPath[32768]{};GetModuleFileNameW(owner,ownerPath,32768);
   auto remoteOwner=Find(target,std::filesystem::path(ownerPath).filename().wstring());
   if(!SameImage(h,remoteOwner,owner)){FreeLibrary(local);throw std::runtime_error("Carregador do processo nao corresponde ao Windows atual");}
   const auto text=dll.wstring();const auto bytes=(text.size()+1)*sizeof(wchar_t);
   void* buffer=VirtualAllocEx(h,nullptr,bytes,MEM_COMMIT|MEM_RESERVE,PAGE_READWRITE);SIZE_T written=0;
   if(!buffer||!WriteProcessMemory(h,buffer,text.c_str(),bytes,&written)||written!=bytes){if(buffer)VirtualFreeEx(h,buffer,0,MEM_RELEASE);FreeLibrary(local);throw std::runtime_error("Falha ao preparar a ponte no processo");}
   const auto remoteLoad=remoteOwner.base+reinterpret_cast<std::uintptr_t>(load)-reinterpret_cast<std::uintptr_t>(owner);
   HANDLE thread=CreateRemoteThread(h,nullptr,0,reinterpret_cast<LPTHREAD_START_ROUTINE>(remoteLoad),buffer,0,nullptr);
   if(!thread){VirtualFreeEx(h,buffer,0,MEM_RELEASE);FreeLibrary(local);throw std::runtime_error("Windows recusou carregar a ponte");}
   const bool ended=Wait(thread,10000);CloseHandle(thread);
   // A still-running loader owns this memory until the target exits; do not free it on timeout.
   if(ended)VirtualFreeEx(h,buffer,0,MEM_RELEASE);
   if(!ended){blockedPid=target;FreeLibrary(local);throw std::runtime_error("Carregamento ainda pendente; reinicie o FiveM antes de tentar novamente");}
   module=Find(target,L"FiveMMenuBridge.dll");
  }
  const bool match=SameImage(h,module,local)&&startRva<module.size;FreeLibrary(local);
  if(!match)throw std::runtime_error("Outra DLL da ponte esta carregada; reinicie o FiveM");
  HANDLE thread=CreateRemoteThread(h,nullptr,0,reinterpret_cast<LPTHREAD_START_ROUTINE>(module.base+startRva),nullptr,0,nullptr);
  if(!thread)throw std::runtime_error("Windows recusou iniciar a ponte");
  const bool ended=Wait(thread,10000);CloseHandle(thread);
  if(!ended){blockedPid=target;throw std::runtime_error("Inicializacao pendente; reinicie o FiveM antes de tentar novamente");}
  if(!Map(target))throw std::runtime_error("A ponte nao disponibilizou o canal de acoes");
 }
 void Run(){
  for(;;){
   Command command;DWORD target=0;HANDLE h=nullptr;
   {std::unique_lock<std::mutex> lock(mutex);wake.wait(lock,[&](){return stop||job.has_value();});if(stop)return;
    command=*job;job.reset();target=pid;
    if(!process||!DuplicateHandle(GetCurrentProcess(),process,GetCurrentProcess(),&h,0,FALSE,DUPLICATE_SAME_ACCESS)){working=false;status="Processo do jogo indisponivel";continue;}}
   try{
    Connect(target,h);
    {std::lock_guard<std::mutex> lock(mutex);
     if(pid!=target||!shared||State(shared->service)!=1)throw std::runtime_error(shared?shared->error:"Ponte indisponivel");
     const auto state=State(shared->commandState);if(state==MenuBridge::Busy||state==Ready)throw std::runtime_error("Outra acao esta em andamento na ponte");
     shared->command=command;MemoryBarrier();InterlockedExchange(reinterpret_cast<volatile LONG*>(&shared->commandState),Ready);
    }
    bool done=false;const auto end=GetTickCount64()+20000;
    while(!Stopping()&&GetTickCount64()<end){
     {std::lock_guard<std::mutex> lock(mutex);
      if(pid!=target)throw std::runtime_error("Processo do jogo alterado; acao cancelada");
      if(shared&&State(shared->commandState)==Done&&shared->reply.id==command.id){if(command.action!=Action::PlayerNames)status=shared->reply.message;InterlockedExchange(reinterpret_cast<volatile LONG*>(&shared->commandState),Idle);done=true;break;}
      if(shared&&State(shared->service)<0)throw std::runtime_error(shared->error);
     }
     if(WaitForSingleObject(h,25)==WAIT_OBJECT_0)throw std::runtime_error("FiveM encerrado durante a acao");
    }
    if(!done){std::lock_guard<std::mutex> lock(mutex);if(shared)InterlockedExchange64(reinterpret_cast<volatile LONG64*>(&shared->cancelId),static_cast<LONG64>(command.id));status="Acao cancelada ou sem confirmacao do jogo";}
   }catch(const std::exception& e){Message(e.what());}
   CloseHandle(h);{std::lock_guard<std::mutex> lock(mutex);working=false;}
  }
 }
public:
 Client():serial(GetTickCount64()*65536ULL+(GetCurrentProcessId()&65535u)){}Client(const Client&)=delete;Client& operator=(const Client&)=delete;
 ~Client(){Stop();}
 void Tick(DWORD target,HANDLE h,const Frame& frame){
  std::lock_guard<std::mutex> lock(mutex);lastFrame=frame;
  if(pid&&pid!=target&&shared){InterlockedExchange64(reinterpret_cast<volatile LONG64*>(&shared->cancelId),static_cast<LONG64>(shared->command.id));UnmapViewOfFile(shared);shared=nullptr;CloseHandle(mapping);mapping=nullptr;}
  pid=target;process=h;
  if(shared){InterlockedIncrement(reinterpret_cast<volatile LONG*>(&shared->frameSequence));MemoryBarrier();shared->frame=frame;MemoryBarrier();InterlockedIncrement(reinterpret_cast<volatile LONG*>(&shared->frameSequence));}
 }
 bool Submit(Command value){
  std::lock_guard<std::mutex> lock(mutex);if(stop||working||!process||!pid)return false;
  value.id=++serial;if(!Valid(value)){status="Valores invalidos para a acao";return false;}
  if(!Fresh(lastFrame,GetTickCount64())){status="Carregue seu personagem primeiro";return false;}
  if(!worker.joinable())worker=std::thread([this](){Run();});
  job=value;working=true;if(value.action!=Action::PlayerNames)status="Preparando acao...";wake.notify_one();return true;
 }
 bool Busy(){std::lock_guard<std::mutex> lock(mutex);return working;}
 std::string Status(){std::lock_guard<std::mutex> lock(mutex);return status;}
 std::string Nickname(int serverId){
  std::lock_guard<std::mutex> lock(mutex);if(!shared||State(shared->service)!=1||serverId<=0)return {};
  for(int retry=0;retry<3;retry++){
   const auto before=State(shared->namesSequence);if(before&1)continue;MemoryBarrier();
   const auto updated=shared->namesUpdated;const auto now=GetTickCount64();
   if(!updated||updated>now||now-updated>10000||shared->namesCount>playerNicknameMax)return {};
   std::string result;
   for(std::size_t i=0;i<shared->namesCount;i++){
    const auto entry=shared->names[i];if(entry.serverId!=static_cast<std::uint32_t>(serverId))continue;
    const auto end=static_cast<const char*>(std::memchr(entry.name,0,sizeof(entry.name)));
    if(end)result.assign(entry.name,static_cast<std::size_t>(end-entry.name));break;
   }
   MemoryBarrier();if(before==State(shared->namesSequence))return result;
  }return {};
 }
 int RuntimeBuild(){std::lock_guard<std::mutex> lock(mutex);return shared?State(shared->runtimeBuild):0;}
 int RpgState(){std::lock_guard<std::mutex> lock(mutex);return shared?State(shared->rpgState):0;}
 int CameraState(){std::lock_guard<std::mutex> lock(mutex);return shared?State(shared->cameraState):0;}
 std::uint64_t CatalogToken(){std::lock_guard<std::mutex> lock(mutex);return shared?(static_cast<std::uint64_t>(pid)<<32)|static_cast<std::uint32_t>(State(shared->catalogSequence)):0;}
 std::vector<VehicleModel> Models(){
  std::lock_guard<std::mutex> lock(mutex);if(!shared)return {};
  for(int retry=0;retry<3;retry++){
   const auto before=State(shared->catalogSequence);if(before&1)continue;MemoryBarrier();
   const auto count=shared->catalogCount;if(count>catalogMax)return {};
   std::vector<VehicleModel> result;result.reserve(count);
   for(std::size_t i=0;i<count;i++){
    char name[catalogNameBytes]{};std::memcpy(name,shared->catalogNames[i],sizeof(name));
    const char* end=static_cast<const char*>(std::memchr(name,0,sizeof(name)));
    if(!end)return {};const auto vehicleClass=shared->catalogClasses[i];result.push_back({std::string(name,static_cast<std::size_t>(end-name)),vehicleClass<=22?static_cast<int>(vehicleClass):-1});
   }
   MemoryBarrier();if(before==State(shared->catalogSequence))return result;
  }return {};
 }
 void Cancel(){std::lock_guard<std::mutex> lock(mutex);if(shared)InterlockedExchange64(reinterpret_cast<volatile LONG64*>(&shared->cancelId),static_cast<LONG64>(shared->command.id));}
 void Stop(){
  {std::lock_guard<std::mutex> lock(mutex);stop=true;if(shared){InterlockedIncrement(reinterpret_cast<volatile LONG*>(&shared->frameSequence));MemoryBarrier();shared->frame.rpg=0;shared->frame.input=0;shared->frame.camera=0;shared->frame.cameraMoves=0;MemoryBarrier();InterlockedIncrement(reinterpret_cast<volatile LONG*>(&shared->frameSequence));InterlockedExchange64(reinterpret_cast<volatile LONG64*>(&shared->cancelId),static_cast<LONG64>(shared->command.id));}wake.notify_all();}
  if(worker.joinable())worker.join();
  std::lock_guard<std::mutex> lock(mutex);if(shared){UnmapViewOfFile(shared);shared=nullptr;}if(mapping){CloseHandle(mapping);mapping=nullptr;}
 }
};
inline Client& Connection(){static Client value;return value;}
}
