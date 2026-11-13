#pragma once
#include <Includes/Includes.hpp>
#include <Core/Offsets.hpp>
#include <Core/SDK/SDK.hpp>
#include <unordered_set>
#include <mutex>

extern const char * keys[ ];

namespace Core
{
	namespace Features
	{
		class cEsp
		{
		public:
			void Draw( );
			void DrawVehicle();
		};

		inline cEsp g_Esp;

        // Peds com ESP de nome individual ativado via lista de jogadores
        namespace EspName {
            inline std::unordered_set<uintptr_t> pinnedPeds;
            inline std::mutex mtx;
            inline bool IsPinned(uintptr_t ped){std::lock_guard<std::mutex> lk(mtx);return pinnedPeds.count(ped)>0;}
            inline void Toggle(uintptr_t ped){std::lock_guard<std::mutex> lk(mtx);if(pinnedPeds.count(ped))pinnedPeds.erase(ped);else pinnedPeds.insert(ped);}
            inline bool GetAndSet(uintptr_t ped, bool& state){std::lock_guard<std::mutex> lk(mtx);state=pinnedPeds.count(ped)>0;return state;}
        }
	}
}