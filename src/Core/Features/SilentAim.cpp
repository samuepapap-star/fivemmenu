#include "SilentAim.hpp"
#include "SharkSilentAdapted.hpp"

void Core::Features::cSilentAim::RestoreSilent() {
    g_SharkSilentState.Restore();
}

const char* Core::Features::cSilentAim::StatusText() const {
    return g_SharkSilentState.StatusText();
}

void Core::Features::cSilentAim::HookSilent() {
    while (true) {
        g_SharkSilentState.Tick();
        std::this_thread::sleep_for(std::chrono::milliseconds(2));
    }
}
