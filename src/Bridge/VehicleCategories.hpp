#pragma once
#include "ModelCatalog.hpp"
#include <map>
#include <sstream>
namespace MenuBridge {
enum class VehicleGroup { General, Vip, Police, Ambulance, Army, Emergency };
using VehicleAssignments=std::map<std::string,VehicleGroup>;
inline std::string TrimCategory(std::string s){const auto a=s.find_first_not_of(" \t\r");if(a==std::string::npos)return {};return s.substr(a,s.find_last_not_of(" \t\r")-a+1);}
// Explicit server classifications only; never infer VIP/family membership from names.
inline VehicleAssignments ParseVehicleAssignments(const std::string& text){
 if(text.size()>1024*1024)throw std::runtime_error("Arquivo de categorias excede 1 MB");
 VehicleAssignments out;std::istringstream input(text);std::string line;std::size_t count=0;
 while(std::getline(input,line)){
  line=TrimCategory(line);if(line.empty()||line[0]=='#')continue;
  if(++count>catalogMax)throw std::runtime_error("Muitas categorias de veiculos");
  const auto separator=line.find(';');if(separator==std::string::npos||line.find(';',separator+1)!=std::string::npos)throw std::runtime_error("Use modelo;categoria no arquivo de categorias");
  const auto model=LowerModel(TrimCategory(line.substr(0,separator))),group=LowerModel(TrimCategory(line.substr(separator+1)));
  if(model.empty()||model.find('\0')!=std::string::npos||model.size()>=catalogNameBytes||!Text(model.c_str(),model.size()+1,true))throw std::runtime_error("Modelo invalido no arquivo de categorias");
  VehicleGroup value=VehicleGroup::General;
  if(group=="vip")value=VehicleGroup::Vip;else if(group=="policia")value=VehicleGroup::Police;
  else if(group=="ambulancia")value=VehicleGroup::Ambulance;else if(group=="exercito")value=VehicleGroup::Army;
  else if(group=="emergencia")value=VehicleGroup::Emergency;else if(group!="geral")throw std::runtime_error("Categoria desconhecida no arquivo");
  if(!out.emplace(model,value).second)throw std::runtime_error("Modelo duplicado no arquivo de categorias");
 }return out;
}
inline VehicleGroup GroupFor(const VehicleModel& model,const VehicleAssignments& assignments){
 const auto explicitGroup=assignments.find(model.name);if(explicitGroup!=assignments.end())return explicitGroup->second;
 // Known base models; customized models require the server's explicit assignment.
 if(model.name=="ambulance")return VehicleGroup::Ambulance;
 if(model.name=="police"||model.name=="police2"||model.name=="police3"||model.name=="police4"||model.name=="policeb"||model.name=="policet"||model.name=="policeold1"||model.name=="policeold2"||model.name=="sheriff"||model.name=="sheriff2"||model.name=="fbi"||model.name=="fbi2"||model.name=="pranger"||model.name=="riot"||model.name=="riot2"||model.name=="polmav"||model.name=="predator")return VehicleGroup::Police;
 if(model.vehicleClass==19)return VehicleGroup::Army;
 if(model.vehicleClass==18)return VehicleGroup::Emergency;
 return VehicleGroup::General;
}
// 0 all, 1 road vehicles, 2 motorcycles, 3 other (including unclassified).
inline bool VehicleTypeMatches(int vehicleClass,int type){
 const bool motorcycle=vehicleClass==8;
 const bool road=(vehicleClass>=0&&vehicleClass<=7)||(vehicleClass>=9&&vehicleClass<=12)||(vehicleClass>=17&&vehicleClass<=20)||vehicleClass==22;
 return type==0||(type==1&&road)||(type==2&&motorcycle)||(type==3&&!road&&!motorcycle);
}
inline bool VehicleCategoryMatches(const VehicleModel& model,int type,int group,const VehicleAssignments& assignments){
 return VehicleTypeMatches(model.vehicleClass,type)&&(group==0||static_cast<int>(GroupFor(model,assignments))+1==group);
}
}
