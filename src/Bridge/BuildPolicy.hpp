#pragma once
#include <cstdint>

namespace MenuBridge::BuildPolicy {
inline constexpr bool Supported(std::uint32_t build) {
    return build == 3258 || build == 3889;
}
inline constexpr std::uint32_t image3889Size=65387520u;
inline constexpr std::uint32_t image3889First=1793798832u,image3889Second=1793965275u;
// These are the two executable revisions already recorded by Build3889Compatibility.
// An unknown revision stays blocked even when its filename contains "3889".
inline constexpr bool Image3889(std::uint32_t timestamp, std::uint32_t size) {
    return size == image3889Size && (timestamp == image3889First || timestamp == image3889Second);
}
inline constexpr bool Matches(std::uint32_t executable, std::uint32_t client, std::uint64_t runtime) {
    return Supported(executable) && executable == client && runtime == executable;
}
}
