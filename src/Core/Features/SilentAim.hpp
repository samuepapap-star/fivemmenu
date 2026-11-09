#pragma once
#include <Includes/Includes.hpp>
namespace Core { namespace Features {
class cSilentAim {
public:
    void RestoreSilent();
    void HookSilent();
    const char* StatusText() const;
};
inline cSilentAim g_SilentAim;
}}
