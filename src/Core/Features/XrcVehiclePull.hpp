#pragma once
#include <cmath>
#include <cstdint>

// Adapted from XRC Updated Another ImGui: Pull Vehicle To Me and TeleportToObject.
// Reader/writer return true only for complete transfers; pause is injected for tests.
namespace XrcVehiclePull {
template<class Vec, class Reader, class Writer, class Pause>
bool Apply(uintptr_t vehicle, uintptr_t ped, Vec target, Reader read, Writer write, Pause pause) {
    auto sane = [](Vec v) { return std::isfinite(v.x) && std::isfinite(v.y) && std::isfinite(v.z) &&
        std::abs(v.x)<100000.f && std::abs(v.y)<100000.f && std::abs(v.z)<100000.f; };
    uintptr_t nav=0, model=0, net=0, localNet=0;
    Vec oldVisual{}, oldNav{}, oldVelocity{};
    float collision=0;
    uint8_t owner=0, oldOwner=0, pending=0, arbitration=0;
    uint16_t flags=0;
    if (vehicle<0x10000 || ped<0x10000 || !sane(target) ||
        !read(vehicle+0x30,nav) || nav<0x10000 ||
        !read(vehicle+0x20,model) || model<0x10000 ||
        !read(model+0x2C,collision) || !std::isfinite(collision) || std::abs(collision)>1000000.f ||
        !read(vehicle+0x90,oldVisual) || !sane(oldVisual) ||
        !read(nav+0x50,oldNav) || !sane(oldNav) ||
        !read(vehicle+0x320,oldVelocity) || !sane(oldVelocity) ||
        !read(vehicle+0xD0,net)) return false;
    // Local-only entities may have no network object.
    if (net && (net<0x10000 || !read(ped+0xD0,localNet) || localNet<0x10000 ||
        !read(localNet+0x49,owner) || owner==0xFF ||
        !read(net+0x49,oldOwner) || !read(net+0x4A,pending) ||
        !read(net+0x4B,arbitration) || !read(net+0x4E,flags))) return false;
    auto restoreNetwork = [&]() {
        if (net) { write(net+0x49,oldOwner);write(net+0x4A,pending);
            write(net+0x4B,arbitration);write(net+0x4E,flags); }
    };
    if (net && (!write(net+0x4E,static_cast<uint16_t>((flags & ~1u)|8u)) ||
        !write(net+0x49,owner) || !write(net+0x4A,uint8_t(0xFF)) || !write(net+0x4B,uint8_t(0)))) {
        restoreNetwork(); return false;
    }
    if (!write(model+0x2C,0.f)) { restoreNetwork();return false; }
    const bool moved = write(vehicle+0x90,target) && write(nav+0x50,target) &&
        write(vehicle+0x320,Vec(0,0,0));
    pause(); // XRC holds this temporary model value for 40 ms.
    const bool restored = write(model+0x2C,collision);
    if (!moved || !restored) {
        write(vehicle+0x90,oldVisual);write(nav+0x50,oldNav);
        write(vehicle+0x320,oldVelocity);restoreNetwork();return false;
    }
    Vec visual{}, navigation{};
    if (!read(vehicle+0x90,visual) || !read(nav+0x50,navigation)) return false;
    const auto isNearDestination = [&](Vec v) {const auto d=v-target;return sane(v) && d.x*d.x+d.y*d.y+d.z*d.z<=4.f;};
    return isNearDestination(visual) && isNearDestination(navigation);
}
}
