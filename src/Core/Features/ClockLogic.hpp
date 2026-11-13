#pragma once
#include <cstdint>
#include <cstddef>
#include <cstring>

namespace ClockLogic {
struct Time { std::int32_t hour=0, minute=0, second=0; };
static_assert(sizeof(Time)==12,"Clock fields must be contiguous 32-bit integers");
inline bool Valid(Time t) {
    return t.hour>=0&&t.hour<24&&t.minute>=0&&t.minute<60&&t.second>=0&&t.second<60;
}
inline bool Equal(Time a,Time b) {
    return a.hour==b.hour&&a.minute==b.minute&&a.second==b.second;
}
inline bool OwnedFields(Time current,Time before,Time applied) {
    return (current.hour==before.hour||current.hour==applied.hour)&&
        (current.minute==before.minute||current.minute==applied.minute)&&
        (current.second==before.second||current.second==applied.second);
}
// The read-only report from this client identified this exact image and signature.
inline constexpr std::uint64_t instructionRva=0x600c11, clockRva=0x1dbc220;
inline bool Profile(int build,std::uint32_t timestamp,std::uint32_t imageSize) {
    return build==3258&&timestamp==1793965266u&&imageSize==73740288u;
}
inline bool Instruction(const std::uint8_t* b,std::size_t size) {
    const int pattern[]={0x48,0x8d,0x0d,-1,-1,-1,-1,0xba,-1,-1,-1,-1,0xe8,-1,-1,-1,-1,0x44,0x8b,0x05};
    if(!b||size<sizeof(pattern)/sizeof(pattern[0]))return false;
    for(std::size_t i=0;i<sizeof(pattern)/sizeof(pattern[0]);i++)
        if(pattern[i]>=0&&b[i]!=pattern[i])return false;
    std::int32_t displacement=0;std::memcpy(&displacement,b+3,4);
    return static_cast<std::int64_t>(instructionRva)+7+displacement==static_cast<std::int64_t>(clockRva);
}
enum class Result { InvalidTime, ReadFailed, WriteFailed, RollbackFailed, Unconfirmed, ChangedByGame, Applied };
template<class Read,class Write> Result Apply(Time wanted,Read read,Write write) {
    if(!Valid(wanted))return Result::InvalidTime;
    Time before{};if(!read(before)||!Valid(before))return Result::ReadFailed;
    if(!write(wanted)) {
        Time current{};
        // Roll back only fields still attributable to this transfer; preserve a concurrent game update.
        if(!read(current))return Result::RollbackFailed;
        if(!Equal(current,before)&&OwnedFields(current,before,wanted)&&!write(before))return Result::RollbackFailed;
        return Result::WriteFailed;
    }
    Time after{};if(!read(after))return Result::Unconfirmed;
    return Equal(after,wanted)?Result::Applied:Result::ChangedByGame;
}
}

