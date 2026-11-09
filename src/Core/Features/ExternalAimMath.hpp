#pragma once
#include <cmath>
namespace ExternalAimMath {
inline float Step(float current, float target, float smooth) {
    if (!std::isfinite(current) || !std::isfinite(target)) return current;
    if (smooth <= 1.f) return target;
    const float threshold = 0.5f / (smooth * 20.f);
    const float delta = target - current;
    return current + (std::fabs(delta) <= threshold ? delta : (delta > 0 ? threshold : -threshold));
}
inline bool Inside(float x, float y, float width, float height) {
    return std::isfinite(x) && std::isfinite(y) && width > 0 && height > 0 &&
        x >= 0 && y >= 0 && x <= width && y <= height && (x != 0 || y != 0);
}
inline int Bone(int selection) {
    // Use the destination SDK's cached bone indices, not the DLL's mixed bone hashes.
    static const int indices[] = {0, 8, 3, 4, 6, 5};
    return indices[selection >= 0 && selection < 6 ? selection : 0];
}
}
