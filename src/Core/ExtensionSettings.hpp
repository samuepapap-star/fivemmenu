#pragma once
#include <Includes/Includes.hpp>
#include <cmath>
namespace ExtraSettings {
#define EXTRA_BOOLS(X) \
 X(FreezeAmmo) X(WeaponIcon) X(SecondMonitor) X(MaxTractionOn) X(SuspensionOn) X(LiftOn) X(GravityOn) X(Drift) X(RegenHealth) X(RegenArmor) X(SuperJump) X(BeastJump) X(ExplosiveFist) X(FireAmmo) X(ExplosiveAmmo) X(BlockAim) \
 X(FreezeClock) X(CameraDistanceEnabled) X(SilentAutoDistance) X(AimHumanization) X(SilentPrediction) X(SilentDynamicFov) X(SilentForceDriver) X(Spin) X(WeaponScaleOn) X(FastReload) X(RapidFire) X(HornBoost) X(RocketBoost) X(JumpCar) X(PlaningCar) \
 X(Radar) X(RadarVehicles) X(RadarNames) X(RadarRotate) X(RadarNPCs) X(VehicleMarker) X(IgnoreOccupied) \
 X(PassiveTargeting) X(PlayerGlow) X(ChinaHat) X(InvisibleWarning) X(Particles) X(Rainbow) X(HideTaskbar) X(ESPHotkeyOn)
#define DECL_BOOL(n) inline bool n=false;
EXTRA_BOOLS(DECL_BOOL)
#undef DECL_BOOL
inline int ClockHour=12,ClockMinute=0;
inline int Background=0;
inline int SilentHitbox=0,TriggerHitbox=0,SilentMinFov=10;
inline float CameraDistancePercent=100.f;
inline float AimLeadSeconds=0.08f;
inline float PredictionSeconds=0.08f;
inline int SavePeekKey=0, ReturnPeekKey=0, EspKey=0, HornKey=0x45, Theme=0, Font=0, CrossType=0, ParticleCount=40;
inline float MaxTraction=2.5f,Suspension=2.f,Lift=0.f,Gravity=9.8f;
inline float SpinSpeed=90, WeaponScale=1, ReloadSpeed=2, FireRate=2, BoostPower=30;
inline float RadarSize=200, RadarRange=150, TextSize=13, SkeletonWidth=1.5f, BoxWidth=1, HatSize=0.4f;
inline float CrossSize=5, CrossWidth=1, RainbowSpeed=0.15f, BorderSize=1, ParticleSpeed=0.25f, ParticleSize=2;
inline ImVec4 CrossColor{1,1,1,1};
#define EXTRA_INTS(X) X(ClockHour,0,23) X(ClockMinute,0,59) X(Background,0,8) X(SilentHitbox,0,2) X(TriggerHitbox,0,2) X(SilentMinFov,1,180) X(SavePeekKey,0,255) X(ReturnPeekKey,0,255) X(EspKey,0,255) X(HornKey,0,255) X(Theme,0,2) X(Font,0,2) X(CrossType,0,3) X(ParticleCount,0,150)
#define EXTRA_FLOATS(X) X(CameraDistancePercent,0.f,100.f) X(AimLeadSeconds,0.f,0.3f) X(MaxTraction,0.1f,10.f) X(Suspension,0.1f,10.f) X(Lift,-0.5f,1.f) X(Gravity,0.1f,20.f) X(PredictionSeconds,0.f,0.3f) X(SpinSpeed,10.f,360.f) X(WeaponScale,0.1f,5.f) X(ReloadSpeed,1.f,10.f) X(FireRate,1.f,10.f) X(BoostPower,1.f,100.f) X(RadarSize,120.f,400.f) X(RadarRange,20.f,1000.f) X(TextSize,8.f,24.f) X(SkeletonWidth,0.5f,5.f) X(BoxWidth,0.5f,4.f) X(HatSize,0.1f,1.f) X(CrossSize,1.f,30.f) X(CrossWidth,0.5f,5.f) X(RainbowSpeed,0.01f,1.f) X(BorderSize,0.f,4.f) X(ParticleSpeed,0.05f,2.f) X(ParticleSize,1.f,5.f)
inline nlohmann::json Save() {
 nlohmann::json j=nlohmann::json::object();
#define SAVE_BOOL(n) j[#n]=n;
 EXTRA_BOOLS(SAVE_BOOL)
#undef SAVE_BOOL
#define SAVE_NUMBER(n,l,h) j[#n]=n;
 EXTRA_INTS(SAVE_NUMBER) EXTRA_FLOATS(SAVE_NUMBER)
#undef SAVE_NUMBER
 j["CrossColor"]={CrossColor.x,CrossColor.y,CrossColor.z,CrossColor.w};return j;
}
inline void Load(const nlohmann::json& j) {
 if(!j.is_object())return;
#define LOAD_BOOL(n) if(j.contains(#n)&&j[#n].is_boolean())n=j[#n].get<bool>();
 EXTRA_BOOLS(LOAD_BOOL)
#undef LOAD_BOOL
#define LOAD_INT(n,l,h) if(j.contains(#n)&&j[#n].is_number_integer())n=ImClamp(j[#n].get<int>(),l,h);
 EXTRA_INTS(LOAD_INT)
#undef LOAD_INT
#define LOAD_FLOAT(n,l,h) if(j.contains(#n)&&j[#n].is_number()){float v=j[#n].get<float>();if(std::isfinite(v))n=ImClamp(v,l,h);}
 EXTRA_FLOATS(LOAD_FLOAT)
#undef LOAD_FLOAT
 if(j.contains("CrossColor")&&j["CrossColor"].is_array()&&j["CrossColor"].size()==4){
  ImVec4 next=CrossColor;float* p=&next.x;bool valid=true;
  for(int i=0;i<4;i++){if(!j["CrossColor"][i].is_number()){valid=false;break;}float v=j["CrossColor"][i].get<float>();if(!std::isfinite(v)){valid=false;break;}p[i]=ImClamp(v,0.f,1.f);}if(valid)CrossColor=next;
 }
}
}
