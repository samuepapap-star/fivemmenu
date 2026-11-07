#include "../src/Bridge/ModelCatalog.hpp"
#include <cassert>
using namespace MenuBridge;
std::vector<std::uint8_t> Pack(const std::vector<std::string>& names){
 std::vector<std::uint8_t> b{0xdc,static_cast<std::uint8_t>(names.size()>>8),static_cast<std::uint8_t>(names.size()&255)};
 for(const auto& n:names){if(n.size()<32)b.push_back(static_cast<std::uint8_t>(0xa0|n.size()));else{b.push_back(0xd9);b.push_back(static_cast<std::uint8_t>(n.size()));}b.insert(b.end(),n.begin(),n.end());}return b;
}
bool Rejected(std::vector<std::uint8_t> b){try{ParseModelCatalog(b.data(),b.size());return false;}catch(const std::runtime_error&){return true;}}
int main(){
 {auto b=Pack({"Sultan","rmod-x6","adder","SULTAN"});auto names=ParseModelCatalog(b.data(),b.size());assert((names==std::vector<std::string>{"adder","rmod-x6","sultan"}));assert(ModelMatches("RMod-X6","x6")&&ModelMatches("bmw_custom","BMW")&&!ModelMatches("adder","bmw"));}
 {std::vector<std::uint8_t> b{0x92,0xa3,'t','2','0',0xd9,3,'b','m','w'};auto n=ParseModelCatalog(b.data(),b.size());assert(n.size()==2&&n[0]=="bmw");}
 {std::vector<std::uint8_t> b{0xdd,0,0,0,1,0xda,0,3,'t','2','0'};auto n=ParseModelCatalog(b.data(),b.size());assert(n.size()==1&&n[0]=="t20");}
 {auto b=Pack({"",std::string(100,'a'),"bad;code",std::string("a\0b",3),"valid_01"});auto n=ParseModelCatalog(b.data(),b.size());assert(n.size()==1&&n[0]=="valid_01");}
 assert(Rejected({0x91,0xa5,'a'}));assert(Rejected({0x91,0x01}));assert(Rejected({0x90,0}));assert(Rejected({0xdd,0,1,0,0}));assert(Rejected({0x91,0xdb,255,255,255,255}));
 {std::vector<std::string> input;for(std::size_t i=0;i<catalogMax+10;i++)input.push_back("car_"+std::to_string(i));auto b=Pack(input);auto n=ParseModelCatalog(b.data(),b.size());assert(n.size()==catalogMax);}
 {Command c{};c.id=1;c.action=Action::SpawnVehicle;Copy(c.text,sizeof(c.text),"rmod-x6");assert(Valid(c));c.action=Action::VehicleCatalog;assert(Valid(c));}
}

