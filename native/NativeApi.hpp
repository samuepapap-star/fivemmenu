#pragma once
#include "../src/Bridge/Protocol.hpp"
#include <vector>
namespace MenuBridge {
struct NativeApi {virtual ~NativeApi()=default;virtual std::uint64_t Call(std::uint64_t,const std::vector<std::uint64_t>&)=0;};
}

