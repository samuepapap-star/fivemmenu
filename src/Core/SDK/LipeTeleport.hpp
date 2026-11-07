#pragma once
#include <cstdint>
#include <cmath>

namespace LipeTeleport {
inline uintptr_t Entity(uintptr_t ped,bool insideVehicle,uintptr_t lastVehicle) {
    // A remembered vehicle must never be moved while the player is on foot.
    const uintptr_t entity=insideVehicle?lastVehicle:ped;
    return entity>=0x10000?entity:0;
}
template<class Vec> inline bool Sane(const Vec& value) {
    return std::isfinite(value.x) && std::isfinite(value.y) && std::isfinite(value.z) &&
        std::fabs(value.x)<100000.f && std::fabs(value.y)<100000.f && std::fabs(value.z)<100000.f;
}
struct Result {bool motion=false,navigation=false,visual=false;};
template<class Vec,class Reader,class Writer>
inline Result Move(uintptr_t entity,const Vec& destination,Reader read,Writer write) {
    Result result;
    if(entity<0x10000 || !Sane(destination))return result;
    uintptr_t navigation=0;
    Vec previousMotion{},previousNavigation{},previousVisual{};
    if(!read(entity+0x30,navigation) || navigation<0x10000 ||
        !read(navigation+0x30,previousMotion) || !Sane(previousMotion) ||
        !read(navigation+0x50,previousNavigation) || !Sane(previousNavigation) ||
        !read(entity+0x90,previousVisual) || !Sane(previousVisual))return result;
    Vec stopped{};stopped.x=stopped.y=stopped.z=0.f;
    // lipeee's navigation reset, plus the project's synchronized navigation
    // position. No retry loop or attempt to defeat server-side corrections.
    result.motion=write(navigation+0x30,stopped);
    if(!result.motion)return result;
    result.navigation=write(navigation+0x50,destination);
    if(!result.navigation) {
        write(navigation+0x30,previousMotion);
        return result;
    }
    result.visual=write(entity+0x90,destination);
    if(!result.visual) {
        write(navigation+0x50,previousNavigation);
        write(navigation+0x30,previousMotion);
    }
    return result;
}
}
