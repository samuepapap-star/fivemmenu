#pragma once
#include "../src/Bridge/Protocol.hpp"
#include <vector>
#include <array>
#include <stdexcept>
namespace MenuBridge {
inline Vec DecodeNativeVector(const std::array<std::uint64_t,4>& values){return {AsFloat(values[0]),AsFloat(values[1]),AsFloat(values[2])};}
struct NativeApi {
 virtual ~NativeApi()=default;
 virtual std::uint64_t Call(std::uint64_t,const std::vector<std::uint64_t>&)=0;
 virtual Vec Vector(std::uint64_t,const std::vector<std::uint64_t>&){throw std::runtime_error("Retorno vetorial nao implementado");}
 virtual std::vector<std::string> Strings(std::uint64_t,const std::vector<std::uint64_t>&){throw std::runtime_error("Catalogo nao implementado");}
};
}

