#pragma once
#include <string>
#include <algorithm>
#include <cctype>
namespace ClientBuild {
inline int Detect(std::string name) {
    std::transform(name.begin(),name.end(),name.begin(),[](unsigned char c){return char(std::tolower(c));});
    if(name=="fivem_gameprocess.exe" || name=="fivem_gtaprocess.exe") return 1604;
    const int supported[]={1604,2060,2189,2372,2545,2612,2699,2802,2944,3095,3258};
    for(int build:supported) {
        const std::string prefix="fivem_b"+std::to_string(build)+"_";
        if(name==prefix+"gameprocess.exe" || name==prefix+"gtaprocess.exe") return build;
    }
    return 0;
}
}
