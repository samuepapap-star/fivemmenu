#include "../src/Core/Features/ClockLogic.hpp"
#include <array>
#include <cassert>
#include <cstring>
int main() {
    using namespace ClockLogic;
    assert(Valid({23,59,59}));assert(!Valid({24,0,0}));assert(!Valid({0,-1,0}));assert(!Valid({0,0,60}));
    assert(Profile(3258,1793965266u,73740288u));
    assert(!Profile(3889,1793965266u,73740288u));
    assert(!Profile(3258,1793965267u,73740288u));
    std::array<std::uint8_t,20> bytes{0x48,0x8d,0x0d,0,0,0,0,0xba,0,0,0,0,0xe8,0,0,0,0,0x44,0x8b,0x05};
    const auto displacement=static_cast<std::int32_t>(clockRva-instructionRva-7);
    std::memcpy(bytes.data()+3,&displacement,4);
    assert(Instruction(bytes.data(),bytes.size()));assert(!Instruction(bytes.data(),6));
    bytes[3]^=1;assert(!Instruction(bytes.data(),bytes.size()));
    bytes[3]^=1;bytes[12]=0x90;assert(!Instruction(bytes.data(),bytes.size()));
    Time memory{4,58,20},original=memory,wanted{12,30,0};int writes=0;
    auto read=[&](Time& value){value=memory;return true;};
    auto write=[&](Time value){writes++;memory=value;return true;};
    assert(Apply(Time{25,0,0},read,write)==Result::InvalidTime&&writes==0);
    auto failedRead=[](Time&){return false;};
    assert(Apply(wanted,failedRead,write)==Result::ReadFailed&&writes==0);
    assert(Apply(wanted,read,write)==Result::Applied&&Equal(memory,wanted));
    memory=original;writes=0;
    auto partial=[&](Time value){writes++;if(writes==1){memory.hour=value.hour;return false;}memory=value;return true;};
    assert(Apply(wanted,read,partial)==Result::WriteFailed&&writes==2&&Equal(memory,original));
    memory=original;writes=0;
    auto concurrent=[&](Time){writes++;memory={18,15,42};return false;};
    assert(Apply(wanted,read,concurrent)==Result::WriteFailed&&writes==1&&memory.hour==18);
    memory=original;writes=0;
    auto failedRollback=[&](Time value){writes++;memory.hour=value.hour;return false;};
    assert(Apply(wanted,read,failedRollback)==Result::RollbackFailed&&writes==2);
}

