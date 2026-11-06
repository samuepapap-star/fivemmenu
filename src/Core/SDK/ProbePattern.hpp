#pragma once
#include <cstdint>
#include <cstddef>
#include <vector>
#include <cstring>
namespace ProbePattern {
inline bool Match(const std::uint8_t* bytes,std::size_t size,std::size_t at,const std::vector<int>& pattern) {
    if(pattern.empty() || at>size || pattern.size()>size-at)return false;
    for(std::size_t i=0;i<pattern.size();++i)
        if(pattern[i]>=0 && bytes[at+i]!=static_cast<std::uint8_t>(pattern[i]))return false;
    return true;
}
inline bool Rip(const std::uint8_t* bytes,std::size_t size,std::size_t at,
    std::uint64_t instructionRva,std::uint64_t imageSize,std::uint64_t& result) {
    if(at>size || size-at<7)return false;
    std::int32_t displacement=0;std::memcpy(&displacement,bytes+at+3,4);
    const std::int64_t target=static_cast<std::int64_t>(instructionRva)+7+displacement;
    if(target<0 || static_cast<std::uint64_t>(target)>=imageSize)return false;
    result=static_cast<std::uint64_t>(target);return true;
}
}
