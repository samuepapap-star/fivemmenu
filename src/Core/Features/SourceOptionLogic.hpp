#pragma once
#include <algorithm>
#include <cmath>
namespace SourceOptionLogic {
// Traceless caps the configured distance by a successfully read weapon range.
inline int DistanceLimit(int configured, bool automatic, bool readOk, float weaponRange) {
    const int manual = (std::max)(1, configured);
    if (!automatic || !readOk || !std::isfinite(weaponRange) || weaponRange < 1.f || weaponRange >= 10000.f)
        return manual;
    return (std::min)(manual, static_cast<int>(weaponRange));
}
inline bool KnownWeaponRangeLayout(int build) {
    switch(build) {
    case 2060: case 2189: case 2372: case 2545: case 2612: case 2699: case 2802: return true;
    default: return false;
    }
}
inline float CameraDistance(float percent) {
    if (!std::isfinite(percent)) percent = 100.f;
    return -5.f + (std::clamp)(percent, 0.f, 100.f) * 0.06f;
}
}
