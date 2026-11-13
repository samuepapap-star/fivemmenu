#include "../native/NativePlayerNames.hpp"
#include <cassert>
#include <cstring>
using namespace MenuBridge;
struct Fake:NativeApi {
 bool disconnected=false;
 std::uint64_t Call(std::uint64_t hash,const std::vector<std::uint64_t>& args)override{
  if(hash==0xB8DFD30D6973E135ULL)return !disconnected&&args[0]<4;
  assert(hash==Hash("GET_PLAYER_SERVER_ID"));return args[0]==3?0:100+args[0];
 }
 std::string String(std::uint64_t hash,const std::vector<std::uint64_t>& args)override{
  assert(hash==0x6D0DE6A7B5DA71F8ULL);
  if(args[0]==1)return "**Invalid**";
  if(args[0]==2)return std::string(200,'x');
  return "Joao UTF-8 \xc3\xa9";
 }
};
int main(){
 Fake api;auto names=ReadPlayerNicknames(api);
 assert(names.size()==2&&names[0].serverId==100&&names[1].serverId==102);
 assert(std::string(names[0].name)=="Joao UTF-8 \xc3\xa9");
 assert(std::strlen(names[1].name)==127);
 api.disconnected=true;assert(ReadPlayerNicknames(api).empty());
 Command request{};request.id=1;request.action=Action::PlayerNames;assert(Valid(request));
}
