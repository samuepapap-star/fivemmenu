#pragma once
#include "Protocol.hpp"
#include <vector>
#include <algorithm>
#include <stdexcept>
namespace MenuBridge {
inline constexpr std::size_t catalogInputLimit=2*1024*1024;
inline std::string LowerModel(std::string value){for(char& c:value)if(c>='A'&&c<='Z')c=static_cast<char>(c+32);return value;}
inline bool ModelMatches(const std::string& value,const std::string& filter){return LowerModel(value).find(LowerModel(filter))!=std::string::npos;}
// GET_ALL_VEHICLE_MODELS returns fx::scrObject containing a MessagePack string array.
// Only the documented array/string types are accepted; lengths and allocations are bounded.
inline std::vector<std::string> ParseModelCatalog(const std::uint8_t* data,std::size_t length){
 if(!data||!length||length>catalogInputLimit)throw std::runtime_error("Tamanho do catalogo invalido");
 std::size_t offset=0;
 auto read=[&](){if(offset>=length)throw std::runtime_error("Catalogo incompleto");return data[offset++];};
 auto number=[&](int bytes){std::uint32_t n=0;for(int i=0;i<bytes;i++)n=(n<<8)|read();return n;};
 const auto root=read();std::uint32_t count=0;
 if((root&0xf0)==0x90)count=root&15;else if(root==0xdc)count=number(2);else if(root==0xdd)count=number(4);else throw std::runtime_error("Formato do catalogo invalido");
 if(count>32768)throw std::runtime_error("Catalogo excedeu o limite de leitura");
 std::vector<std::string> result;result.reserve((std::min)(static_cast<std::size_t>(count),catalogMax));
 for(std::uint32_t i=0;i<count;i++){
  const auto tag=read();std::uint32_t size=0;
  if((tag&0xe0)==0xa0)size=tag&31;else if(tag==0xd9)size=number(1);else if(tag==0xda)size=number(2);else if(tag==0xdb)size=number(4);else throw std::runtime_error("Modelo nao textual no catalogo");
  if(size>length-offset)throw std::runtime_error("Nome de modelo incompleto");
  if(size>0&&size<catalogNameBytes){
   std::string name(reinterpret_cast<const char*>(data+offset),size);
   if(name.find('\0')==std::string::npos&&Text(name.c_str(),name.size()+1,true))result.push_back(LowerModel(name));
  }
  offset+=size;
 }
 if(offset!=length)throw std::runtime_error("Dados extras no catalogo");
 std::sort(result.begin(),result.end());result.erase(std::unique(result.begin(),result.end()),result.end());
 if(result.size()>catalogMax)result.resize(catalogMax);
 return result;
}
}

