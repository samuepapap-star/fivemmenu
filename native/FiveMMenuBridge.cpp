#define WIN32_LEAN_AND_MEAN
#define NOMINMAX
#include <Windows.h>
#include <array>
#include <atomic>
#include <climits>
#include <memory>
#include <mutex>
#include <unordered_map>
#include "cfx/component.h"
#include "cfx/event.h"
#include "NativeActions.hpp"
#include "NativeDispatch.hpp"
#include "cfx/ScriptThread.hpp"
#include "NativeCamera.hpp"
#include "NativeVehicleTransfer.hpp"
#include "../src/Bridge/ModelCatalog.hpp"

extern "C" __declspec(dllexport) DWORD WINAPI FiveMMenuBridgeStart(void*);

namespace {
using namespace MenuBridge;
struct ManagerView:fx::fwRefCountable,fx::ComponentHolderImpl<ManagerView>{fx::fwEvent<> tick;fx::fwEvent<> reset;};
static_assert(sizeof(fx::fwEvent<>)==16&&sizeof(ManagerView)==64,"Unsupported CFX event ABI");
static_assert(sizeof(LONG)==sizeof(std::int32_t),"IPC state size");
using Context=RageContext;
using Handler=void(*)(Context*);using GetHandler=Handler(*)(std::uint64_t);using GetManager=ManagerView*(*)(bool);
Shared* shared=nullptr;HANDLE mapping=nullptr;std::atomic<bool> initialized{false};ManagerView* manager=nullptr;
using GetCustom=CfxHandler*(*)(std::uint64_t);
GetHandler resolve=nullptr;GetCustom resolveCustom=nullptr;bool nativeFault=false;
using CreateScript=void(*)(GtaThread*);using ResetScript=int(*)(GtaThread*,std::uint32_t,void*,std::uint32_t);using ActiveScript=void*(*)();
CreateScript createScript=nullptr;ResetScript resetScript=nullptr;ActiveScript activeScript=nullptr;
GtaThread* ownedThread=nullptr;std::uint64_t registeredAt=0,lastScriptTick=0;
std::int32_t ReadState(std::int32_t& s){return InterlockedCompareExchange(reinterpret_cast<volatile LONG*>(&s),0,0);}
void Store(std::int32_t& s,std::int32_t value){InterlockedExchange(reinterpret_cast<volatile LONG*>(&s),value);}
bool SafeHandler(const NativeSelection* function,Context* context,DWORD* fault){__try{function->InvokeInScript(*context,reinterpret_cast<std::uintptr_t>(activeScript()),reinterpret_cast<std::uintptr_t>(ownedThread));return true;}__except((*fault=GetExceptionCode()),EXCEPTION_EXECUTE_HANDLER){return false;}}
bool SafeCopyCatalog(void* target,const void* source,std::size_t size){__try{std::memcpy(target,source,size);return true;}__except(EXCEPTION_EXECUTE_HANDLER){return false;}}
Handler SafeResolve(GetHandler function,std::uint64_t hash){__try{return function(hash);}__except(EXCEPTION_EXECUTE_HANDLER){return nullptr;}}
CfxHandler* SafeCustom(GetCustom function,std::uint64_t hash){__try{return function(hash);}__except(EXCEPTION_EXECUTE_HANDLER){return nullptr;}}
ManagerView* SafeManager(GetManager function){__try{return function(true);}__except(EXCEPTION_EXECUTE_HANDLER){return nullptr;}}
bool Profile(HMODULE module,std::uint32_t timestamp,std::uint32_t size){
 if(!module)return false;
 __try{auto base=reinterpret_cast<const std::uint8_t*>(module);auto dos=reinterpret_cast<const IMAGE_DOS_HEADER*>(base);
  if(dos->e_magic!=IMAGE_DOS_SIGNATURE||dos->e_lfanew<=0||dos->e_lfanew>0x100000)return false;
  auto nt=reinterpret_cast<const IMAGE_NT_HEADERS64*>(base+dos->e_lfanew);
  return nt->Signature==IMAGE_NT_SIGNATURE&&nt->FileHeader.Machine==IMAGE_FILE_MACHINE_AMD64&&nt->FileHeader.TimeDateStamp==timestamp&&nt->OptionalHeader.SizeOfImage==size;
 }__except(EXCEPTION_EXECUTE_HANDLER){return false;}
}
bool Readable(const void* p,std::size_t bytes){MEMORY_BASIC_INFORMATION m{};if(!p||VirtualQuery(p,&m,sizeof(m))!=sizeof(m)||m.State!=MEM_COMMIT||(m.Protect&(PAGE_NOACCESS|PAGE_GUARD)))return false;
 auto start=reinterpret_cast<std::uintptr_t>(p),base=reinterpret_cast<std::uintptr_t>(m.BaseAddress);return start>=base&&bytes<=m.RegionSize&&start-base<=m.RegionSize-bytes;}
bool FrameSnapshot(Frame& out){for(int tries=0;tries<3;tries++){auto before=ReadState(shared->frameSequence);if(before&1)continue;MemoryBarrier();std::memcpy(&out,&shared->frame,sizeof(out));MemoryBarrier();if(before==ReadState(shared->frameSequence))return true;}return false;}
class Api final:public NativeApi{
 std::unordered_map<std::uint64_t,NativeSelection> handlers;
public:
 void Clear(){handlers.clear();}
 std::array<std::uint64_t,4> Invoke(std::uint64_t hash,const std::vector<std::uint64_t>& input){
  const bool cleanup=hash==CameraN::RenderScriptCams||hash==CameraN::DoesCamExist||hash==CameraN::DestroyCam||hash==CameraN::ClearFocus||hash==Hash("END_FIND_VEHICLE");
  if((nativeFault&&!cleanup)||input.size()>32)throw std::runtime_error("Execucao de natives indisponivel");
  auto it=handlers.find(hash);auto handler=it==handlers.end()?SelectNative(hash,[](std::uint64_t h){return SafeResolve(resolve,h);},[](std::uint64_t h){return SafeCustom(resolveCustom,h);}):it->second;
  if(!handler)throw std::runtime_error(MissingNative(hash));
  handlers[hash]=handler;std::array<std::uint64_t,32> arguments{};std::array<std::uint64_t,4> result{};
  std::copy(input.begin(),input.end(),arguments.begin());Context context{};context.result=result.data();context.count=static_cast<std::uint32_t>(input.size());context.arguments=arguments.data();
  DWORD fault=0;
  if(!SafeHandler(&handler,&context,&fault)){nativeFault=true;throw std::runtime_error(FailedNative(hash,fault));}return result;
 }
 std::uint64_t Call(std::uint64_t hash,const std::vector<std::uint64_t>& input)override{return Invoke(hash,input)[0];}
 Vec Vector(std::uint64_t hash,const std::vector<std::uint64_t>& input)override{return DecodeNativeVector(Invoke(hash,input));}
 std::vector<std::string> Strings(std::uint64_t hash,const std::vector<std::uint64_t>& input)override{
  auto result=Invoke(hash,input);const auto size=static_cast<std::size_t>(result[1]);
  if(!result[0]||!size||size>catalogInputLimit)throw std::runtime_error("Retorno do catalogo invalido");
  std::vector<std::uint8_t> bytes(size);
  if(!SafeCopyCatalog(bytes.data(),reinterpret_cast<const void*>(result[0]),size))throw std::runtime_error("Nao foi possivel ler o catalogo");
  return ParseModelCatalog(bytes.data(),bytes.size());
 }
};
Api api;Actions actions(api);NativeCamera cameraActions(api);NativeVehicleTransfer vehicleTransfer(api);
void Publish(const Reply& reply){shared->reply=reply;MemoryBarrier();Store(shared->commandState,Done);}
void Failure(const char* text){Copy(shared->error,sizeof(shared->error),text);MemoryBarrier();Store(shared->service,-1);}
void WriteCatalog(const std::vector<std::string>& models){
 InterlockedIncrement(reinterpret_cast<volatile LONG*>(&shared->catalogSequence));MemoryBarrier();
 const auto count=(std::min)(models.size(),catalogMax);
 for(std::size_t i=0;i<count;i++)Copy(shared->catalogNames[i],catalogNameBytes,models[i]);
 shared->catalogCount=static_cast<std::uint32_t>(count);MemoryBarrier();
 InterlockedIncrement(reinterpret_cast<volatile LONG*>(&shared->catalogSequence));
}
void OnReset(){
 if(!shared)return;
 const auto state=ReadState(shared->commandState);
 if(state==Busy||state==Ready){Reply r{};r.id=shared->command.id;Copy(r.message,sizeof(r.message),"Sessao reiniciada; acao cancelada");Publish(r);}
 actions.Reset();vehicleTransfer.Reset();cameraActions.Reset();api.Clear();WriteCatalog({});Store(shared->rpgState,0);Store(shared->cameraState,0);
}
void Tick(){
 if(!shared||ReadState(shared->service)!=1)return;
 try{
  Frame frame{};if(!FrameSnapshot(frame))frame={};const auto now=GetTickCount64();
  if(!Fresh(frame,now)&&!actions.Pending()&&!vehicleTransfer.Pending()&&actions.rpgState==0&&!cameraActions.Active())return;
  if(!actions.Pending()&&!vehicleTransfer.Pending()&&InterlockedCompareExchange(reinterpret_cast<volatile LONG*>(&shared->commandState),Busy,Ready)==Ready){
   Command command=shared->command;MemoryBarrier();
   if(!Fresh(frame,now)){Reply r{};r.id=command.id;Copy(r.message,sizeof(r.message),"Carregue o personagem e mantenha o menu conectado");Publish(r);}
   else if(command.action==Action::StartCamera||command.action==Action::StopCamera||command.action==Action::ObservePlayer)Publish(cameraActions.Submit(command,frame,now));
   else if(command.action==Action::TransferVehicles)vehicleTransfer.Submit(command,frame,now);
   else if(command.action==Action::VehicleCatalog){
    auto models=api.Strings(Hash("GET_ALL_VEHICLE_MODELS"),{});WriteCatalog(models);
    Reply reply{};reply.id=command.id;reply.success=models.empty()?0u:1u;reply.value=static_cast<std::uint32_t>(models.size());
    Copy(reply.message,sizeof(reply.message),"Lista atualizada: "+std::to_string(models.size())+" modelos registrados, incluindo originais e personalizados.");Publish(reply);
   }
   else{
    if(command.action==Action::Model||command.action==Action::Restore||command.action==Action::CloneAppearance||command.action==Action::StopAll)cameraActions.Stop();
    actions.Submit(command,now);
   }
  }
  actions.Tick(frame,now,static_cast<std::uint64_t>(InterlockedCompareExchange64(reinterpret_cast<volatile LONG64*>(&shared->cancelId),0,0)));
  cameraActions.Tick(frame,now);Store(shared->cameraState,cameraActions.State());
  vehicleTransfer.Tick(frame,now,static_cast<std::uint64_t>(InterlockedCompareExchange64(reinterpret_cast<volatile LONG64*>(&shared->cancelId),0,0)));
  if(vehicleTransfer.hasReply){Publish(vehicleTransfer.reply);vehicleTransfer.hasReply=false;}
  Store(shared->rpgState,actions.rpgState);
  if(actions.hasReply){Publish(actions.reply);actions.hasReply=false;}
 }catch(const std::exception& error){
  try{cameraActions.Stop();}catch(...){}Store(shared->cameraState,0);
  vehicleTransfer.Reset();
  Reply r{};r.id=shared->command.id;Copy(r.message,sizeof(r.message),error.what());Publish(r);
  if(nativeFault){Failure(error.what());Store(shared->rpgState,0);}else{
   // Drop the operation before accepting another request; never report an exception as success.
   try{actions.Abort(error.what());actions.hasReply=false;}catch(...){Failure("Falha ao encerrar a acao");}
  }
 }catch(...){Failure("Excecao na ponte; nenhuma nova acao sera executada");}
}
bool SafeTick(){__try{Tick();return true;}__except(EXCEPTION_EXECUTE_HANDLER){return false;}}
void GuardedTick(){if(!SafeTick()&&shared)Failure("ABI do runtime indisponivel; ponte interrompida");}
class BridgeScript final:public CfxThread {
public:
 bool registered=false;
 void Reset()override{if(registered)OnReset();}
 void Kill()override{if(registered)OnReset();}
 void DoRun()override{lastScriptTick=GetTickCount64();GuardedTick();}
};
static_assert(sizeof(CfxThread)==16,"Unsupported CFX script thread ABI");
BridgeScript* script=nullptr;
void ResourcePump(){
 if(!shared||ReadState(shared->service)!=1)return;
 if(!script){
  Frame frame{};
  if(ReadState(shared->commandState)!=Ready||!FrameSnapshot(frame)||!Fresh(frame,GetTickCount64()))return;
  // Allocate and initialize on the game thread; CfxThread supplies its own RAGE context.
  script=new BridgeScript();ownedThread=script->GetThread();
  if(!ownedThread)throw std::runtime_error("Thread de script nao foi criada pelo Cfx");
  resetScript(ownedThread,Hash("fivem_menu_actions"),nullptr,0);
  createScript(ownedThread);script->registered=true;registeredAt=GetTickCount64();
 }
 if(registeredAt&&GetTickCount64()-registeredAt>3000&&!lastScriptTick)Failure("A thread de script nao entrou no agendador do jogo");
}
void PumpCpp(){try{ResourcePump();}catch(const std::exception& error){Failure(error.what());}catch(...){Failure("Falha ao registrar a thread de script");}}
bool SafePump(){__try{PumpCpp();return true;}__except(EXCEPTION_EXECUTE_HANDLER){return false;}}
void GuardedPump(){if(!SafePump()&&shared)Failure("Falha na inicializacao da thread de script Cfx");}
}

static DWORD StartImplementation(){
 if(initialized.exchange(true))return 1;
 const auto name=L"Local\\FiveMMenuActions_"+std::to_wstring(GetCurrentProcessId());
 mapping=CreateFileMappingW(INVALID_HANDLE_VALUE,nullptr,PAGE_READWRITE,0,sizeof(MenuBridge::Shared),name.c_str());
 if(!mapping){initialized=false;return 0;}
 if(GetLastError()==ERROR_ALREADY_EXISTS){CloseHandle(mapping);mapping=nullptr;initialized=false;return 0;}
 auto view=MapViewOfFile(mapping,FILE_MAP_ALL_ACCESS,0,0,sizeof(MenuBridge::Shared));
 if(!view){CloseHandle(mapping);mapping=nullptr;initialized=false;return 0;}
 shared=new(view) MenuBridge::Shared{};shared->pid=GetCurrentProcessId();
 auto core=GetModuleHandleW(L"citizen-resources-core.dll"),rage=GetModuleHandleW(L"rage-scripting-five.dll");
 auto scripting=GetModuleHandleW(L"scripting-gta.dll");
 if(!Profile(core,1793965266u,782336u)||!Profile(rage,1793798977u,1720320u)){Failure("Revisao dos componentes CFX nao validada");return 0;}
 if(!Profile(scripting,1793799053u,286720u)){Failure("Revisao do scripting-gta nao validada");return 0;}
 auto current=reinterpret_cast<GetManager>(GetProcAddress(core,"?GetCurrent@ResourceManager@fx@@SAPEAV12@_N@Z"));
 resolve=reinterpret_cast<GetHandler>(GetProcAddress(rage,"?GetNativeHandler@scrEngine@rage@@SAP6AXPEAVscrNativeCallContext@2@@Z_K@Z"));
 resolveCustom=reinterpret_cast<GetCustom>(GetProcAddress(scripting,"?GetNativeHandlerPtr@ScriptEngine@fx@@SAPEAV?$function@$$A6AXAEAVScriptContext@fx@@@Z@std@@_K@Z"));
 createScript=reinterpret_cast<CreateScript>(GetProcAddress(rage,"?CreateThread@scrEngine@rage@@SAXPEAVGtaThread@@@Z"));
 resetScript=reinterpret_cast<ResetScript>(GetProcAddress(rage,"?Reset@GtaThread@@QEAA?AW4eThreadState@rage@@IPEAXI@Z"));
 activeScript=reinterpret_cast<ActiveScript>(GetProcAddress(rage,"?GetActiveThread@scrEngine@rage@@SAPEAVscrThread@2@XZ"));
 if(!current||!resolve||!resolveCustom||!createScript||!resetScript||!activeScript){Failure("Exports publicos do runtime nao encontrados");return 0;}
 manager=SafeManager(current);if(!Readable(manager,sizeof(ManagerView))){Failure("Gerenciador de resources indisponivel");return 0;}
 // Keep the module and the manager alive for callbacks already owned by the game.
 HMODULE pinned=nullptr;if(!GetModuleHandleExW(GET_MODULE_HANDLE_EX_FLAG_FROM_ADDRESS|GET_MODULE_HANDLE_EX_FLAG_PIN,reinterpret_cast<LPCWSTR>(&FiveMMenuBridgeStart),&pinned)){Failure("Nao foi possivel manter a ponte carregada");return 0;}
 manager->AddRef();manager->tick.Connect([](){GuardedPump();},INT_MAX);manager->reset.Connect([](){OnReset();},INT_MAX);
 Copy(shared->error,sizeof(shared->error),"Ponte pronta; aguardando acao");MemoryBarrier();Store(shared->service,1);return 1;
}
extern "C" __declspec(dllexport) DWORD WINAPI FiveMMenuBridgeStart(void*){
 __try{return StartImplementation();}
 __except(EXCEPTION_EXECUTE_HANDLER){if(shared)Failure("Nao foi possivel validar a interface do runtime");return 0;}
}
BOOL WINAPI DllMain(HINSTANCE module,DWORD reason,LPVOID){if(reason==DLL_PROCESS_ATTACH)DisableThreadLibraryCalls(module);return TRUE;}

