#pragma once
#include <Core/SDK/SDK.hpp>
#include <Core/ExtensionSettings.hpp>
#include <Core/Features/CarreraAdditions.hpp>
#include <array>
#include <chrono>
#include <Core/Features/RollSpinLogic.hpp>
#include <Core/Features/SourceOptionLogic.hpp>
#include <Gui/AdminList.hpp>
#include <Core/Features/XrcVehiclePull.hpp>
#include <cmath>
namespace Core { namespace Features { namespace Extras {
using CarreraAdditions::Read; using CarreraAdditions::Write;
inline const char* status="";
inline D3DXVECTOR3 peek; inline bool peekSaved=false;
inline DWORD peekPid=0; inline uintptr_t peekPed=0;
inline bool Sane(D3DXVECTOR3 p){return std::isfinite(p.x)&&std::isfinite(p.y)&&std::isfinite(p.z)&&std::abs(p.x)<100000.f&&std::abs(p.y)<100000.f&&std::abs(p.z)<100000.f;}
inline CPed* Player(){return reinterpret_cast<CPed*>(CarreraAdditions::LocalPlayer());}
inline void SavePeek(){auto p=Player();if(!p||!Read((uintptr_t)p+0x90,peek)||!Sane(peek)){status="Nao foi possivel salvar a posicao";return;}peekSaved=true;peekPid=Mem.ProcId;peekPed=(uintptr_t)p;status="Posicao salva";}
inline void ReturnPeek(){auto p=Player();if(!p||!peekSaved||peekPid!=Mem.ProcId||peekPed!=(uintptr_t)p){status="Salve uma posicao para este personagem primeiro";return;}status=p->SetPos(peek)?"Retorno solicitado":"Falha no retorno";}
inline bool Edge(int key,bool& last){bool down=key>0&&key<256&&(GetAsyncKeyState(key)&0x8000);bool hit=down&&!last;last=down;return hit;}
template<class T> struct Override {
 uintptr_t address=0;DWORD pid=0;T original{},applied{};
 void Restore(){T current{};if(address&&pid==Mem.ProcId&&Read(address,current)&&current==applied)Write(address,original);address=0;}
 void Set(uintptr_t next,bool enabled,T value,T lo,T hi){
  if(!enabled||next!=address||pid!=Mem.ProcId)Restore();if(!enabled||!next)return;
  T current{};if(!Read(next,current)||!std::isfinite(static_cast<double>(current))||current<lo||current>hi)return;
  if(!address){address=next;pid=Mem.ProcId;original=current;}
  if(Write(next,value))applied=value;
 }
};
struct Bits {
 uintptr_t address=0;DWORD pid=0;uint32_t original=0,mask=0;
 void Clear(){uint32_t current=0;if(address&&pid==Mem.ProcId&&Read(address,current))Write(address,(current&~mask)|(original&mask));address=0;mask=0;}
 void Set(uintptr_t next,uint32_t wanted,uint32_t owned){
  if(next!=address||pid!=Mem.ProcId)Clear();if(!next||!owned){Clear();return;}
  uint32_t current=0;if(!Read(next,current))return;
  if(!address){address=next;pid=Mem.ProcId;original=current;}
  // Keep the first value of each newly acquired bit; restore bits removed by the user.
  original=(original&mask)|(current&~mask);
  uint32_t removed=mask&~owned;current=(current&~removed)|(original&removed);
  mask=owned;Write(next,(current&~owned)|(wanted&owned));
 }
};
inline Override<float> cameraDistance;
inline Override<float> weaponY,weaponZ,reload,shotInterval,traction,suspension,lift,gravity,driftMinimum,driftLoss;
inline Bits frameFlags,aimFlags,modelFlags;
inline Bits passiveFriendlyLock, passiveFriendlyDamage;
// Entra no veiculo selecionado como motorista (P1) localmente.
// Escreve o ped local no slot de driver do veiculo e o veiculo no offset
// m_LastVehicle do ped, seta a flag InVehicle e teleporta pro assento.
inline const char* EnterVehicleAsDriver(CVehicle* veh){
 if(!veh)return "Veiculo invalido";
 CPed* p=Player();
 if(!p)return "Player indisponivel";
 uintptr_t pedAddr=reinterpret_cast<uintptr_t>(p);
 uintptr_t vehAddr=reinterpret_cast<uintptr_t>(veh);
 // 1) Verificar se ja ha motorista — se sim, reportar mas continuar
 // 2) Escrever o ped no slot de driver do veiculo
 if(!Write(vehAddr+g_Offsets.m_VehicleDriver,(uintptr_t)p))return "Falha ao escrever driver no veiculo";
 // 3) Escrever o veiculo no offset m_LastVehicle do ped
 if(!Write(pedAddr+g_Offsets.m_LastVehicle,(uintptr_t)veh))return "Falha ao escrever veiculo no ped";
 // 4) Setar flag InVehicle no ped
 p->SetConfigFlag(ePedConfigFlag::InVehicle,true);
 // 5) Teleportar o ped para a posicao do assento do motorista
 //    O assento P1 fica aprox. na posicao do veiculo (+0x90) ligeiramente deslocado
 D3DXVECTOR3 vehPos{};
 if(!Read(vehAddr+0x90,vehPos)||!Sane(vehPos))return "Posicao do veiculo indisponivel";
 // Sem offset adicional — o jogo ajusta o ped ao entrar
 vehPos.z+=0.5f;
 if(!p->SetPos(vehPos))return "Falha ao teleportar para o assento";
 // 6) Zerar velocidade do ped
 D3DXVECTOR3 zero{0,0,0};
 Write(pedAddr+0x320,zero);
 return "Entrando no P1 localmente";
}
// Ejecta o motorista (P1) de um CVehicle* selecionado na lista de carros.
inline const char* EjectDriverFromVehicle(CVehicle* veh){
 if(!veh)return "Veiculo invalido";
 CPed* driver=veh->GetDriver();
 if(!driver)return "Nenhum motorista no P1";
 uintptr_t driverAddr=reinterpret_cast<uintptr_t>(driver);
 // Ler posicao do veiculo (+0x90 visual pos)
 uintptr_t vehAddr=reinterpret_cast<uintptr_t>(veh);
 D3DXVECTOR3 vehPos{};
 if(!Read(vehAddr+0x90,vehPos)||!Sane(vehPos))return "Posicao do veiculo indisponivel";
 // Usar coluna direita da matriz de rotacao (+0x60) para offset lateral
 D3DXVECTOR3 rightVec{1,0,0};
 Read(vehAddr+0x60,rightVec);
 float len=std::hypot(rightVec.x,rightVec.y);
 if(len>0.1f){rightVec.x/=len;rightVec.y/=len;rightVec.z=0;}
 else{rightVec={1,0,0};}
 D3DXVECTOR3 ejectPos=vehPos+rightVec*4.f;
 ejectPos.z=vehPos.z+1.f;
 if(!Sane(ejectPos))return "Posicao de ejecao invalida";
 if(!driver->SetPos(ejectPos))return "Falha ao mover motorista";
 D3DXVECTOR3 zero{0,0,0};
 Write(driverAddr+0x320,zero);
 return "Motorista ejetado do P1 localmente";
}
// Ejecta o alvo do assento de motorista (P1) do veiculo em que esta.
// Funciona apenas localmente — reposiciona o ped fora do carro com velocidade zerada.
inline const char* EjectFromVehicle(uintptr_t target){
 auto p=Player();if(!p||target==(uintptr_t)p)return "Alvo indisponivel";
 // Verificar se o alvo esta na lista de players
 bool listed=false;
 for(const auto&e:SDK::Game::SnapshotEntities())if((uintptr_t)e.Ped==target&&e.PedType==2){listed=true;break;}
 if(!listed)return "Alvo nao encontrado na lista";
 CPed* targetPed=reinterpret_cast<CPed*>(target);
 // Verificar se o alvo esta dentro de um veiculo
 if(!targetPed->InVehicle())return "Alvo nao esta em um veiculo";
 CVehicle* veh=targetPed->GetLastVehicle();
 if(!veh)return "Veiculo do alvo indisponivel";
 // Verificar se o alvo e o motorista (P1) — compara ponteiro
 CPed* driver=veh->GetDriver();
 if(!driver||reinterpret_cast<uintptr_t>(driver)!=target)return "Alvo nao esta no banco do motorista";
 // Ler posicao atual do veiculo para jogar o ped do lado
 D3DXVECTOR3 vehPos{};
 uintptr_t vehAddr=reinterpret_cast<uintptr_t>(veh);
 if(!Read(vehAddr+0x90,vehPos)||!Sane(vehPos))return "Posicao do veiculo indisponivel";
 // Deslocar 4m para o lado direito do veiculo usando a coluna direita da matriz de rotacao
 D3DXVECTOR3 rightVec{1,0,0};
 Read(vehAddr+0x60,rightVec); // coluna X da rot matrix = direita
 float len=std::hypot(rightVec.x,rightVec.y);
 if(len>0.1f){rightVec.x/=len;rightVec.y/=len;rightVec.z=0;}
 else{rightVec={1,0,0};}
 // Posicao de ejecao: ao lado + 1m acima para evitar clipar no chao
 D3DXVECTOR3 ejectPos=vehPos+rightVec*4.f;
 ejectPos.z=vehPos.z+1.f;
 if(!Sane(ejectPos))return "Posicao de ejecao invalida";
 // Teleportar o ped para fora
 if(!targetPed->SetPos(ejectPos))return "Falha ao mover o alvo";
 // Zerar velocidade do ped para evitar que ele voe
 D3DXVECTOR3 zero{0,0,0};
 Write(target+0x320,zero);
 return "Motorista ejetado do veiculo localmente";
}
inline const char* PullPlayer(uintptr_t target){
 auto p=Player();if(!p||target==(uintptr_t)p)return "Alvo indisponivel";
 bool listed=false;{for(const auto&e:SDK::Game::SnapshotEntities())if((uintptr_t)e.Ped==target&&e.PedType==2){listed=true;break;}}
 auto pos=p->GetPos();D3DXVECTOR3 forward;
 if(!listed||!Sane(pos)||!Read((uintptr_t)p+0x70,forward)||!Sane(forward))return "Alvo ou posicao indisponivel";
 forward.z=0;float length=std::hypot(forward.x,forward.y);if(length<0.1f||length>2.f)return "Direcao indisponivel";forward*=1.f/length;
 auto reader=[](uintptr_t a,auto&v){return Read(a,v);};auto writer=[](uintptr_t a,const auto&v){return Write(a,v);};
 return XrcVehiclePull::Apply(target,(uintptr_t)p,pos+forward*3.f,reader,writer,[](){std::this_thread::sleep_for(std::chrono::milliseconds(40));})?"Posicao do jogador atualizada localmente":"Falha ao puxar jogador";
}
inline void SetMaximumHealth(float maximum){auto p=Player();if(p&&g_Offsets.m_MaxHealth&&std::isfinite(maximum)&&maximum>=100.f&&maximum<=1000.f){float current=0;if(Read((uintptr_t)p+g_Offsets.m_MaxHealth,current)&&std::isfinite(current)&&current>=100.f&&current<=10000.f){if(Write((uintptr_t)p+g_Offsets.m_MaxHealth,maximum)){if(p->GetHealth()>maximum)p->SetHealth(maximum);status="Vida maxima atualizada localmente";return;}}}status="Falha ao alterar vida maxima";}
inline void SetDoors(bool unlock,bool closest){
 auto p=Player();if(!p)return;auto here=p->GetPos();std::vector<SDK::Game::VehicleStructure> cars;
 {std::lock_guard<std::mutex> lock(Core::Threads::g_VehicleList.vehicleListMutex);cars=SDK::Game::VehicleList;}
 std::sort(cars.begin(),cars.end(),[&](const auto&a,const auto&b){auto d=a.Pos-here,e=b.Pos-here;return D3DXVec3LengthSq(&d)<D3DXVec3LengthSq(&e);});
 int count=0;for(const auto& v:cars){if(!v.Pointer||!Sane(v.Pos))continue;auto d=v.Pos-here;if(D3DXVec3LengthSq(&d)>200.f*200.f)continue;
 if(g_Offsets.m_VehicleDoorsLockState&&Write((uintptr_t)v.Pointer+g_Offsets.m_VehicleDoorsLockState,uint32_t(unlock?1:2)))count++;if(closest)break;}
 status=count?"Portas atualizadas localmente":"Nenhum veiculo compativel a ate 200 m";
}
inline void UpdateTaskbar(bool stop){
 static HWND window=nullptr;static LONG_PTR original=0;static bool applied=false;
 HWND next=g_Variables.g_hCheatWindow;
 if(applied&&(!ExtraSettings::HideTaskbar||stop||next!=window)){
  DWORD owner=0;if(IsWindow(window))GetWindowThreadProcessId(window,&owner);
  if(owner==GetCurrentProcessId()){LONG_PTR style=GetWindowLongPtr(window,GWL_EXSTYLE);style=(style&~(WS_EX_APPWINDOW|WS_EX_TOOLWINDOW))|(original&(WS_EX_APPWINDOW|WS_EX_TOOLWINDOW));SetWindowLongPtr(window,GWL_EXSTYLE,style);SetWindowPos(window,nullptr,0,0,0,0,SWP_NOMOVE|SWP_NOSIZE|SWP_NOZORDER|SWP_NOACTIVATE|SWP_FRAMECHANGED);}applied=false;
 }
 if(!stop&&ExtraSettings::HideTaskbar&&!applied&&IsWindow(next)){
  DWORD owner=0;GetWindowThreadProcessId(next,&owner);if(owner!=GetCurrentProcessId())return;
  window=next;original=GetWindowLongPtr(window,GWL_EXSTYLE);SetWindowLongPtr(window,GWL_EXSTYLE,(original|WS_EX_TOOLWINDOW)&~WS_EX_APPWINDOW);applied=true;
 }
}
inline void Tick(bool stop=false){
 UpdateTaskbar(stop);
 using namespace ExtraSettings;
 auto p=Player();float health=0;
 bool ready=!stop&&g_MenuInfo.IsLogged&&p&&Read((uintptr_t)p+0x280,health)&&std::isfinite(health)&&health>100.f;
 bool input=ready&&!g_MenuInfo.IsOpen&&GetForegroundWindow()==g_Variables.g_hGameWindow;
 static bool saveHeld=false,returnHeld=false,espHeld=false;
 if(input){if(Edge(SavePeekKey,saveHeld))SavePeek();if(Edge(ReturnPeekKey,returnHeld))ReturnPeek();if(Edge(EspKey,espHeld)&&ESPHotkeyOn)g_Config.ESP->Enabled=!g_Config.ESP->Enabled;}
 else {saveHeld=returnHeld=espHeld=true;}
 if(peekSaved&&(peekPid!=Mem.ProcId||peekPed!=(uintptr_t)p))peekSaved=false;
 static uintptr_t frozenAmmo=0;static DWORD ammoPid=0;static int ammoValue=0;
 if(!ready||!FreezeAmmo||ammoPid!=Mem.ProcId)frozenAmmo=0;
 if(ready){
  float max=p->GetMaxHealth(),armor=p->GetArmor();
  if(RegenHealth&&std::isfinite(max)&&max>100.f&&max<=10000.f&&health<max)p->SetHealth(max);
  if(RegenArmor&&std::isfinite(armor)&&armor>=0.f&&armor<100.f)p->SetArmor(100.f);
 }
 uintptr_t info=0,weapon=0,object=0,vehicle=0,model=0;
 if(ready){if(g_Offsets.m_PlayerInfo)Read((uintptr_t)p+g_Offsets.m_PlayerInfo,info);auto manager=p->GetWeaponManager();if(manager){weapon=(uintptr_t)manager->GetWeaponInfo();if(g_Offsets.m_CObject)Read((uintptr_t)manager+g_Offsets.m_CObject,object);}if(p->InVehicle()){vehicle=(uintptr_t)p->GetLastVehicle();if(vehicle)Read(vehicle+0x20,model);}}
 if(ready&&FreezeAmmo&&weapon){
  uintptr_t ammo=0,count=0,container=0;int current=0;
  if(Read(weapon+0x60,ammo)&&ammo>=0x10000&&Read(ammo+8,count)&&count>=0x10000&&Read(count,container)&&container>=0x10000&&Read(container+0x18,current)&&current>=0&&current<=100000){
   if(frozenAmmo!=container+0x18){frozenAmmo=container+0x18;ammoPid=Mem.ProcId;ammoValue=current;}
   Write(frozenAmmo,ammoValue);
  }else frozenAmmo=0;
 }
 // Renaissance/Priv XRC: model-owned camera distance, independent from camera FOV.
 uintptr_t pedModel=0;
 if(ready&&!p->InVehicle())Read((uintptr_t)p+0x20,pedModel);
 cameraDistance.Set(pedModel>=0x10000?pedModel+0x38:0,
     ready&&CameraDistanceEnabled&&pedModel>=0x10000,
     SourceOptionLogic::CameraDistance(CameraDistancePercent),-5.f,1.f);
 uint32_t owned=0,wanted=0;
 auto flag=[&](bool enabled,int bit){if(enabled){owned|=1u<<bit;wanted|=1u<<bit;}};
 flag(ready&&input&&SuperJump&&(GetAsyncKeyState(VK_SPACE)&0x8000),14);flag(ready&&input&&BeastJump&&(GetAsyncKeyState(VK_SPACE)&0x8000),28);
 flag(ready&&ExplosiveFist,13);flag(ready&&FireAmmo,12);flag(ready&&ExplosiveAmmo,11);
 frameFlags.Set(info>=0x10000?info+0x270:0,wanted,owned);
 // DisablePlayerLockon, shared with the sources' Bubble/Block Aim variants.
 const uintptr_t pedFlag=ready&&g_Offsets.m_PedFlag?(uintptr_t)p+g_Offsets.m_PedFlag+4:0;
 const bool blockLock = BlockAim || PassiveTargeting;
 aimFlags.Set(pedFlag,1u<<11,ready&&blockLock?1u<<11:0);
 const auto flagBase = pedFlag ? pedFlag - sizeof(uint32_t) : uintptr_t{};
 // Quarntime's AllowPlayerLockOnIfFriendly (266) and TreatAsFriendlyForTargetingAndDamage (423).
 const bool passive = flagBase && PassiveTargeting;
 passiveFriendlyLock.Set(flagBase ? flagBase + (266 / 32) * 4 : 0, 1u << (266 % 32), passive ? 1u << (266 % 32) : 0);
 passiveFriendlyDamage.Set(flagBase ? flagBase + (423 / 32) * 4 : 0, 1u << (423 % 32), passive ? 1u << (423 % 32) : 0);
 const bool scales=ready&&WeaponScaleOn&&object>=0x10000;
 weaponY.Set(object?object+0x7C:0,scales,ImClamp(WeaponScale,0.1f,5.f),-5.f,5.f);
 weaponZ.Set(object?object+0x8C:0,scales,ImClamp(WeaponScale,0.1f,5.f),-5.f,5.f);
 reload.Set(weapon?weapon+0x134:0,ready&&FastReload,ImClamp(ReloadSpeed,1.f,10.f),0.1f,20.f);
 // Rate is a field adaptation; no executable hooks or anticheat patches are imported.
 float interval=0;
 if(weapon&&Read(weapon+0x13C,interval)&&std::isfinite(interval)&&interval>=0.01f&&interval<=10.f){
  float base=shotInterval.address==weapon+0x13C?shotInterval.original:interval;
  shotInterval.Set(weapon+0x13C,ready&&RapidFire,base/ImClamp(FireRate,1.f,10.f),0.001f,10.f);
 }else shotInterval.Restore();
 uintptr_t handling=0;if(vehicle&&g_Offsets.m_Handling)Read(vehicle+g_Offsets.m_Handling,handling);
 traction.Set(handling>=0x10000?handling+0x88:0,ready&&(MaxTractionOn||Drift),Drift?0.9f:ImClamp(MaxTraction,0.1f,10.f),0.01f,50.f);
 suspension.Set(handling>=0x10000?handling+0xBC:0,ready&&SuspensionOn,ImClamp(Suspension,0.1f,10.f),0.01f,50.f);
 lift.Set(handling>=0x10000?handling+0xD0:0,ready&&LiftOn,ImClamp(Lift,-0.5f,1.f),-5.f,5.f);
 gravity.Set(vehicle&&g_Offsets.m_VehicleGravity?vehicle+g_Offsets.m_VehicleGravity:0,ready&&GravityOn,ImClamp(Gravity,0.1f,20.f),0.f,100.f);
 driftMinimum.Set(handling>=0x10000?handling+0x90:0,ready&&Drift,0.75f,0.01f,50.f);
 driftLoss.Set(handling>=0x10000?handling+0xB8:0,ready&&Drift,1.5f,0.f,50.f);
 uint32_t carMask=0; // model flags: preserve every unrelated bit.
 if(ready&&RocketBoost)carMask|=0x40u<<24;if(ready&&JumpCar)carMask|=0x20u<<24;
 modelFlags.Set(model>=0x10000?model+0x588:0,carMask,carMask);
 if(input&&HornBoost&&vehicle&&HornKey>0&&HornKey<256&&(GetAsyncKeyState(HornKey)&0x8000)){
  uint8_t flags=0,boost=0;float power=0;
  if(Read(vehicle+0x29,flags)&&Read(vehicle+0x97F,boost)&&Read(vehicle+0xAD4,power)&&std::isfinite(power)&&std::abs(power)<=10000.f){Write(vehicle+0x29,uint8_t(flags|1));Write(vehicle+0x97F,uint8_t(boost|0x80));Write(vehicle+0xAD4,ImClamp(BoostPower,1.f,100.f));}
 }
 if(input&&PlaningCar&&vehicle){D3DXVECTOR3 velocity;if(Read(vehicle+0x320,velocity)&&Sane(velocity)&&velocity.z<-1.f){velocity.z=-1.f;Write(vehicle+0x320,velocity);}}
 static RollSpinLogic::Gate rollSpin;
 bool spinAllowed=input&&Spin&&!p->InVehicle();
 if(CombatGlitch){
  uint32_t flags=0;
  const bool readAir=spinAllowed&&g_Offsets.m_PedFlag&&Read((uintptr_t)p+g_Offsets.m_PedFlag+(76/32)*sizeof(uint32_t),flags);
  const bool inAir=!readAir||(flags&(1u<<(76%32)))!=0;
  const auto now=std::chrono::duration_cast<std::chrono::milliseconds>(std::chrono::steady_clock::now().time_since_epoch()).count();
  spinAllowed=rollSpin.Update(spinAllowed,inAir,
      (GetAsyncKeyState(VK_RBUTTON)&0x8000)&&(GetAsyncKeyState(VK_SPACE)&0x8000),static_cast<uint64_t>(now));
 }else rollSpin.Reset();
 if(spinAllowed){
  D3DXVECTOR3 f,r,u;uintptr_t nav=0;
  if(Read((uintptr_t)p+0x70,f)&&Read((uintptr_t)p+0x60,r)&&Read((uintptr_t)p+0x80,u)&&Sane(f)&&Sane(r)&&Sane(u)&&Read((uintptr_t)p+0x30,nav)&&nav>=0x10000){
   float angle=ImClamp(SpinSpeed,10.f,360.f)*ImClamp(ImGui::GetIO().DeltaTime,0.f,0.05f)*0.0174532925f;
   float cs=std::cos(angle),sn=std::sin(angle);auto rot=[&](D3DXVECTOR3 v){return D3DXVECTOR3(v.x*cs-v.y*sn,v.x*sn+v.y*cs,v.z);};
   f=rot(f);r=rot(r);Write((uintptr_t)p+0x60,r);Write((uintptr_t)p+0x70,f);Write(nav+0x20,r);Write(nav+0x30,f);
  }
 }
}
inline void DrawRadar(){
 using namespace ExtraSettings;if(!Radar||!g_MenuInfo.IsLogged||!Player())return;
 ImGui::SetNextWindowSize(ImVec2(RadarSize,RadarSize),ImGuiCond_Always);
 ImGui::SetNextWindowPos(ImVec2(20,100),ImGuiCond_FirstUseEver);
 const auto flags=ImGuiWindowFlags_NoResize|ImGuiWindowFlags_NoCollapse|ImGuiWindowFlags_NoScrollbar|
  (g_MenuInfo.IsOpen?0:ImGuiWindowFlags_NoInputs);
 if(ImGui::Begin("Radar##ExtraRadar",nullptr,flags)){
 auto draw=ImGui::GetWindowDrawList();auto origin=ImGui::GetCursorScreenPos();auto avail=ImGui::GetContentRegionAvail();
 const float radius=ImMin(avail.x,avail.y)*0.5f-4.f;const ImVec2 c(origin.x+avail.x*0.5f,origin.y+avail.y*0.5f);
 if(radius>0){draw->AddCircle(c,radius,IM_COL32(150,150,150,150),48);draw->AddLine(ImVec2(c.x-radius,c.y),ImVec2(c.x+radius,c.y),IM_COL32(80,80,80,130));draw->AddLine(ImVec2(c.x,c.y-radius),ImVec2(c.x,c.y+radius),IM_COL32(80,80,80,130));
 auto here=Player()->GetPos();D3DXVECTOR3 f(0,1,0);
 if(RadarRotate){uintptr_t cam=0;if(SDK::Pointers::pCamGamePlayDirector&&Read(SDK::Pointers::pCamGamePlayDirector+0x2C0,cam))Read(cam+0x40,f);}
 float length=std::hypot(f.x,f.y);if(!Sane(f)||length<0.1f){f=D3DXVECTOR3(0,1,0);length=1;}f.x/=length;f.y/=length;
 auto point=[&](D3DXVECTOR3 pos,const char* name,ImU32 color){if(!Sane(pos)||!Sane(here))return;auto d=pos-here;float dist=std::hypot(d.x,d.y);if(dist>RadarRange)return;
 float x=d.x*f.y-d.y*f.x,y=d.x*f.x+d.y*f.y;ImVec2 q(c.x+x/RadarRange*radius,c.y-y/RadarRange*radius);draw->AddCircleFilled(q,3,color);if(RadarNames&&name)draw->AddText(ImVec2(q.x+4,q.y),color,name);};
 auto players=SDK::Game::SnapshotEntities();
 for(const auto&e:players)if(e.Ped&&e.Ped!=Player()&&(RadarNPCs||e.PedType==2)&&(e.PedType==2?e.Health>100.f:e.Health>0.f))point(e.Pos,e.NetworkInfo.UserName.c_str(),e.IsFriend?IM_COL32(80,255,100,255):IM_COL32(255,255,255,255));
 if(RadarVehicles){std::vector<SDK::Game::VehicleStructure> cars;{std::lock_guard<std::mutex> lock(Core::Threads::g_VehicleList.vehicleListMutex);cars=SDK::Game::VehicleList;}for(const auto&v:cars)point(v.Pos,v.Name.c_str(),IM_COL32(100,180,255,255));}
 draw->AddTriangleFilled(ImVec2(c.x,c.y-5),ImVec2(c.x-4,c.y+4),ImVec2(c.x+4,c.y+4),IM_COL32(180,80,255,255));}
 }ImGui::End();
}
inline void DrawWarnings(){
 if(!ExtraSettings::InvisibleWarning||!g_MenuInfo.IsLogged||AdminList::entries.empty())return;
 const std::string message="Possiveis invisiveis: "+std::to_string(AdminList::entries.size());
 ImGui::GetBackgroundDrawList()->AddText(ImVec2(20,70),IM_COL32(255,255,255,255),message.c_str());
}
inline void DrawBackground(){
 using namespace ExtraSettings;if(!Background||!g_MenuInfo.IsOpen||!g_MenuInfo.IsLogged)return;
 auto draw=ImGui::GetWindowDrawList();auto origin=ImGui::GetWindowPos();auto size=ImGui::GetWindowSize();
 const ImVec2 end(origin.x+size.x,origin.y+size.y);draw->PushClipRect(origin,end,true);
 float t=(float)ImGui::GetTime();const ImU32 col=IM_COL32(160,100,230,55);
 auto point=[&](float x,float y){return ImVec2(origin.x+x,origin.y+y);};
 if(Background==3){for(int row=0;row<6;row++){ImVec2 previous=point(0,size.y*.3f+row*35);for(int x=8;x<size.x;x+=8){auto next=point((float)x,size.y*.3f+row*35+std::sin(x*.018f+t+row)*24);draw->AddLine(previous,next,col,1);previous=next;}}}
 else for(int i=0;i<48;i++){
  float x=std::fmod(i*127.13f,ImMax(size.x,1.f)),y=std::fmod(i*83.17f+t*(10.f+i%11),ImMax(size.y,1.f));
  if(Background==1){char text[2]={char('0'+(i%2)),0};draw->AddText(point(x,y),IM_COL32(80,220,120,65),text);}
  else if(Background==2)draw->AddCircleFilled(point(x,y),1.f+i%2,col);
  else if(Background==4){auto a=point(x,y);draw->AddCircleFilled(ImVec2(a.x-3,a.y),4,col);draw->AddCircleFilled(ImVec2(a.x+3,a.y),4,col);draw->AddTriangleFilled(ImVec2(a.x-7,a.y),ImVec2(a.x+7,a.y),ImVec2(a.x,a.y+10),col);}
  else if(Background==5||Background==7){float angle=t*.4f+i*.45f,r=(i+1)*4.f;auto q=point(size.x*.5f+std::cos(angle)*r*1.6f,size.y*.5f+std::sin(angle)*r);draw->AddCircleFilled(q,Background==7?2.f:1.f,col);}
  else if(Background==6&&i<8){auto a=point(x,0),b=point(x+16,size.y*.45f),c=point(x-10,size.y*.55f),d=point(x+12,size.y);draw->AddLine(a,b,col,1);draw->AddLine(b,c,col,1);draw->AddLine(c,d,col,1);}
  else if(Background==8)draw->AddLine(point(x,y),point(x-6,y+18),col,1);
 }
 draw->PopClipRect();
}
inline void DrawMenuEffects(){
 using namespace ExtraSettings;if(!g_MenuInfo.IsLogged||!g_MenuInfo.IsOpen)return;
 if(Rainbow){float r,g,b;ImGui::ColorConvertHSVtoRGB(std::fmod((float)ImGui::GetTime()*RainbowSpeed,1.f),0.8f,1.f,r,g,b);g_Col.Base=ImVec4(r,g,b,1);}
 auto& style=ImGui::GetStyle();style.WindowBorderSize=BorderSize;
 static float originalRounding=style.WindowRounding;style.WindowRounding=Theme==0?originalRounding:Theme==1?12.f:0.f;
 if(Particles){auto draw=ImGui::GetBackgroundDrawList();auto screen=ImGui::GetIO().DisplaySize;float t=(float)ImGui::GetTime()*ParticleSpeed;
 for(int i=0;i<ParticleCount;i++){float x=std::fmod(i*127.13f+t*(10.f+i%7),ImMax(1.f,screen.x)),y=std::fmod(i*83.17f+t*(20.f+i%11),ImMax(1.f,screen.y));draw->AddCircleFilled(ImVec2(x,y),ParticleSize,IM_COL32(150,100,220,90));}}
}
}}}
