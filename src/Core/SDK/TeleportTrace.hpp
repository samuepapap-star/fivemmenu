#pragma once
#include <windows.h>
#include <d3dx9math.h>
#include <fstream>
#include <sstream>
#include <mutex>
#include <memory>
#include <thread>
#include <chrono>
#include <atomic>

namespace AltosDiagnostics {
    inline void Log(const std::string& message) noexcept {
        try {
            static std::mutex mutex;
            std::lock_guard<std::mutex> guard(mutex);
            char folder[MAX_PATH]={};
            const DWORD size=GetTempPathA(MAX_PATH,folder);
            if(!size || size>=MAX_PATH)return;
            std::ofstream file(std::string(folder)+"ALTOS_PODERES_teleporte.log",std::ios::app);
            SYSTEMTIME time{};GetLocalTime(&time);
            file<<time.wYear<<"-"<<time.wMonth<<"-"<<time.wDay<<" "<<time.wHour<<":"
                <<time.wMinute<<":"<<time.wSecond<<"."<<time.wMilliseconds<<" "<<message<<"\n";
        } catch(...) {}
    }
    struct TraceState {
        HANDLE process=nullptr;
        uintptr_t entity=0;
        unsigned long long id=0;
        ~TraceState(){if(process)CloseHandle(process);}
        void Sample(const char* stage) noexcept {
            try {
                uintptr_t navigation=0;D3DXVECTOR3 visual{},nav{};
                SIZE_T bytes=0;
                const bool p=ReadProcessMemory(process,reinterpret_cast<LPCVOID>(entity+0x90),
                    &visual,sizeof(visual),&bytes) && bytes==sizeof(visual);
                bytes=0;
                const bool n=ReadProcessMemory(process,reinterpret_cast<LPCVOID>(entity+0x30),
                    &navigation,sizeof(navigation),&bytes) && bytes==sizeof(navigation) && navigation;
                bytes=0;
                const bool v=n && ReadProcessMemory(process,reinterpret_cast<LPCVOID>(navigation+0x50),
                    &nav,sizeof(nav),&bytes) && bytes==sizeof(nav);
                std::ostringstream text;
                text<<"request="<<id<<" stage="<<stage<<" visual_ok="<<p<<" visual=("
                    <<visual.x<<","<<visual.y<<","<<visual.z<<") nav_ok="<<v<<" nav=("
                    <<nav.x<<","<<nav.y<<","<<nav.z<<")";
                Log(text.str());
            } catch(...) {}
        }
    };
    class TeleportTrace {
        std::shared_ptr<TraceState> state;
        bool delayed=true;
    public:
        TeleportTrace(HANDLE process,uintptr_t entity,const D3DXVECTOR3& target,int build,
            const char* action="personagem",bool delayedSamples=true) noexcept :delayed(delayedSamples) {
            try {
                static std::atomic<unsigned long long> sequence{0};
                state=std::make_shared<TraceState>();
                state->id=++sequence;
                state->entity=entity;
                std::ostringstream text;
                text<<"request="<<state->id<<" action="<<action<<" build="<<build
                    <<" target=("<<target.x<<","<<target.y<<","<<target.z<<")";
                Log(text.str());
                if(!process || !entity || !DuplicateHandle(GetCurrentProcess(),process,GetCurrentProcess(),
                    &state->process,PROCESS_VM_READ,FALSE,0)) {
                    Log("request="+std::to_string(state->id)+" observer_unavailable");
                    state.reset();return;
                }
                state->Sample("before");
            } catch(...) {state.reset();}
        }
        void Results(bool visual,bool navigation) noexcept {
            try {if(state)Log("request="+std::to_string(state->id)+" visual_write="+
                std::to_string(visual)+" navigation_write="+std::to_string(navigation));}catch(...){}
        }
        ~TeleportTrace() noexcept {
            if(!state)return;
            state->Sample("immediate");
            if(!delayed)return;
            try {
                auto copy=state;
                std::thread([copy]{
                    std::this_thread::sleep_for(std::chrono::milliseconds(250));copy->Sample("250ms");
                    std::this_thread::sleep_for(std::chrono::milliseconds(750));copy->Sample("1000ms");
                }).detach();
            }catch(...){Log("observer_thread_unavailable");}
        }
    };
}
