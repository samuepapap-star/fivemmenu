#include "Aimbot.hpp"

void Core::Features::cAimbot::SetViewAngles( CPed * Ped, D3DXVECTOR3 BonePos )
{
	if (!Core::SDK::Pointers::pCamGamePlayDirector || !Core::SDK::Pointers::pLocalPlayer) return;
	uintptr_t CamFollowPedCamera = Mem.Read<uintptr_t>( Core::SDK::Pointers::pCamGamePlayDirector + 0x2C0 );
    if (!CamFollowPedCamera) return;
	D3DXVECTOR3 CamPos = Mem.Read<D3DXVECTOR3>( CamFollowPedCamera + 0x60 );

	D3DXVECTOR3 CurrentViewAngles = Mem.Read<D3DXVECTOR3>( CamFollowPedCamera + 0x40 );
	D3DXVec3Normalize( &CurrentViewAngles, &CurrentViewAngles );

	uintptr_t CamFollowVehicle = Mem.Read<uintptr_t>( CamFollowPedCamera + 0x10 );

	if ( !CamFollowPedCamera || !CamFollowVehicle ) return;

	if ( Core::SDK::Pointers::pLocalPlayer->InVehicle( ) )
	{
		if ( Mem.Read<float>( CamFollowVehicle + 0x2AC ) == -2.f )
		{
			Mem.Write<float>( CamFollowVehicle + 0x2AC, 0.f );
			Mem.Write<float>( CamFollowVehicle + 0x2C0, 111.f );
			Mem.Write<float>( CamFollowVehicle + 0x2C4, 111.f );
		}
	}

	D3DXVECTOR3 TargetViewAngles = BonePos - CamPos;
	D3DXVec3Normalize( &TargetViewAngles, &TargetViewAngles );	
	
    const auto settings = g_Config.Aimbot;
    float horizontal = static_cast<float>(settings->SeparateSmooth ? settings->SmoothHorizontal : settings->AimbotSpeed);
    float vertical = static_cast<float>(settings->SeparateSmooth ? settings->SmoothVertical : settings->AimbotSpeed);
    if (settings->LegitMode) {
        // Adapted from carrera: the mode changes smoothing, not license/anti-cheat state.
        const float legitFactor = (100.f / 15.f) * 0.57f;
        horizontal *= legitFactor;
        vertical *= legitFactor;
    }
    horizontal = ImClamp(horizontal, 1.f, 1000.f);
    vertical = ImClamp(vertical, 1.f, 1000.f);
    const D3DXVECTOR3 delta = TargetViewAngles - CurrentViewAngles;
    D3DXVECTOR3 finalAngles(CurrentViewAngles.x + delta.x / horizontal,
        CurrentViewAngles.y + delta.y / horizontal, CurrentViewAngles.z + delta.z / vertical);
    D3DXVec3Normalize(&finalAngles, &finalAngles);
    Mem.Write<D3DXVECTOR3>(CamFollowPedCamera + 0x40, finalAngles);
    Mem.Write<D3DXVECTOR3>(CamFollowPedCamera + 0x3D0, finalAngles);
}

void Core::Features::cAimbot::Start( )
{
	while ( true )
	{

		if (g_Config.Aimbot->Enabled && !g_MenuInfo.IsOpen &&
            GetForegroundWindow() == g_Variables.g_hGameWindow &&
            (g_Config.Aimbot->KeyBind == 0 ||
             (GetAsyncKeyState(g_Config.Aimbot->KeyBind) & 0x8000) != 0))
		{
			CPed * Ped = Core::SDK::Game::GetClosestPed( g_Config.Aimbot->MaxDistance, g_Config.Aimbot->IgnoreNPCs, g_Config.Aimbot->OnlyVisible );
			if ( !Ped ) continue;

			static const int bones[] = {0, 8};
            const int selection = g_Config.Aimbot->Hitbox;
            const int bone = selection >= 0 && selection < 2 ? bones[selection] : 0;
            D3DXVECTOR3 HeadPos = Ped->GetBonePosDefault(bone);
			D3DXVECTOR2 ScreenHeadPos = Core::SDK::Game::WorldToScreen( HeadPos );

			if ( Core::SDK::Game::IsOnScreen( ScreenHeadPos ) )
			{
				int Fov = std::hypot( ScreenHeadPos.x - g_Variables.g_vGameWindowCenter.x, ScreenHeadPos.y - g_Variables.g_vGameWindowCenter.y );
				if ( Fov < g_Config.Aimbot->FOV )
				{
					SetViewAngles( Ped, HeadPos + D3DXVECTOR3(0, 0, bone == 0 ? 0.08f : 0.f) );
				}
			}
		}
		std::this_thread::sleep_for( std::chrono::nanoseconds( 1 ) );
	}
}