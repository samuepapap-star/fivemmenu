#pragma once
#include <cstddef>
#include <cstdint>
#include <cstring>
namespace NativeCandidateLogic {
// The registration signature is LEA RCX,[RIP+disp32]; CALL rel32.
// A table address alone cannot identify the resolver paired with it.
inline bool Resolver(const std::uint8_t* bytes,std::size_t size,std::uint64_t instruction,
    std::uint64_t imageSize,std::uint64_t& target) {
    if(!imageSize||imageSize>0x20000000ULL||!bytes||size<12||bytes[0]!=0x48||bytes[1]!=0x8d||bytes[2]!=0x0d||bytes[7]!=0xe8||
       instruction>=imageSize||imageSize-instruction<12)return false;
    std::int32_t displacement=0;std::memcpy(&displacement,bytes+8,4);
    const auto resolved=static_cast<std::int64_t>(instruction)+12+displacement;
    if(resolved<0||static_cast<std::uint64_t>(resolved)>=imageSize)return false;
    target=static_cast<std::uint64_t>(resolved);return true;
}
inline bool Pointer(std::uint64_t address) {return address>=0x10000&&address<=0x7fffffffffffULL;}
}

