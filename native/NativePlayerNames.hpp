#pragma once
#include "NativeApi.hpp"
namespace MenuBridge {
inline std::vector<PlayerNickname> ReadPlayerNicknames(NativeApi& api) {
 std::vector<PlayerNickname> result;
 for(std::uint64_t player=0;player<playerNicknameMax;player++) {
  // NETWORK_IS_PLAYER_ACTIVE; GET_PLAYER_SERVER_ID; GET_PLAYER_NAME.
  if(!api.Call(0xB8DFD30D6973E135ULL,{player}))continue;
  const auto id=api.Call(Hash("GET_PLAYER_SERVER_ID"),{player});
  if(!id||id>65535)continue;
  const auto name=api.String(0x6D0DE6A7B5DA71F8ULL,{player});
  if(name.empty()||name=="**Invalid**")continue;
  PlayerNickname entry{};entry.serverId=static_cast<std::uint32_t>(id);
  Copy(entry.name,sizeof(entry.name),name);result.push_back(entry);
 }
 return result;
}
}
