#pragma once
#include <cstdint>
#include <functional>
#include <cstdio>
#include <string>
#include <stdexcept>

// Layout from scripting-gta/include/ScriptEngine.h; separate from the RAGE context.
namespace fx {
class ScriptContext {
public:
 void* argumentBuffer;void* resultBuffer;int argumentCount;int resultCount;
};
}
namespace MenuBridge {
struct RageContext {void* result;std::uint32_t count,pad0;void* arguments;std::uint32_t vectors,pad1;std::uint8_t vectorSpace[512];};
using RageHandler=void(*)(RageContext*);
using CfxHandler=std::function<void(fx::ScriptContext&)>;
struct NativeSelection {
 RageHandler game=nullptr;CfxHandler* custom=nullptr;
 explicit operator bool()const{return game||(custom&&static_cast<bool>(*custom));}
 void Invoke(RageContext& context)const{
  if(custom&&*custom){fx::ScriptContext c{context.arguments,context.result,static_cast<int>(context.count),0};(*custom)(c);}
  else if(game)game(&context);
 }
 void InvokeInScript(RageContext& context,std::uintptr_t active,std::uintptr_t owned)const{
  if(!owned||active!=owned)throw std::runtime_error("Contexto da thread de script indisponivel");
  Invoke(context);
 }
};
template<class GameResolver,class CustomResolver>
NativeSelection SelectNative(std::uint64_t hash,GameResolver game,CustomResolver custom){
 auto handler=custom(hash);if(handler&&*handler)return {nullptr,handler};return {game(hash),nullptr};
}
inline std::string MissingNative(std::uint64_t hash){
 char text[120]{};std::snprintf(text,sizeof(text),"Native indisponivel: 0x%016llX",static_cast<unsigned long long>(hash));return text;
}
inline std::string FailedNative(std::uint64_t hash,std::uint32_t code){
 char text[160]{};std::snprintf(text,sizeof(text),"Native 0x%016llX falhou durante execucao (SEH 0x%08X); ponte interrompida",static_cast<unsigned long long>(hash),static_cast<unsigned int>(code));return text;
}
}

