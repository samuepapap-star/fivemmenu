#pragma once
#include <cstdint>
#include <unordered_map>
namespace VisibilityEvidence {
class Filter {
    struct Observation {int playerId=0;std::uint64_t since=0,last=0;};
    std::unordered_map<std::uintptr_t,Observation> observations;
public:
    bool Observe(std::uintptr_t ped,int playerId,unsigned visibility,unsigned alpha,bool insideVehicle,std::uint64_t now) {
        // Lack of the visibility bit alone is common for occluded entities.
        // Occupants may be hidden by vehicle rendering; that is not evidence.
        if(!ped || playerId<=0 || (visibility&1)!=0 || alpha!=0 || insideVehicle) {
            observations.erase(ped);
            return false;
        }
        auto found=observations.find(ped);
        if(found==observations.end() || found->second.playerId!=playerId || now<found->second.last || now-found->second.last>500) {
            observations[ped]={playerId,now,now};
            return false;
        }
        found->second.last=now;
        return now-found->second.since>=1000;
    }
    void Prune(std::uint64_t now) {
        for(auto it=observations.begin();it!=observations.end();) {
            if(now<it->second.last || now-it->second.last>500)it=observations.erase(it);
            else ++it;
        }
    }
    void Clear(){observations.clear();}
};
}
