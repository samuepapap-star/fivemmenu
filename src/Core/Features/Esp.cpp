#include "ESP.hpp"
#include "PlayerBones.hpp"
#include <Core/Features/ExtraWeaponIcons.hpp>
#include <Core/ExtensionSettings.hpp>
#include <chrono>
#include "ExternalAimMath.hpp"



namespace {
// Rainbow RGB cycle — mesma lógica do AdminList::RgbCycleColor
ImColor EspRgbCycle(float speed = 1.f) {
    const float t = static_cast<float>(GetTickCount64() % 3000) / 3000.f * speed;
    const float h = fmodf(t, 1.f);
    const float s = 1.f, v = 1.f;
    const int i = static_cast<int>(h * 6.f);
    const float f = h*6.f - i, p = v*(1-s), q = v*(1-s*f), tv = v*(1-s*(1-f));
    float r, g, b;
    switch (i % 6) {
        case 0: r=v;  g=tv; b=p;  break; case 1: r=q;  g=v;  b=p;  break;
        case 2: r=p;  g=v;  b=tv; break; case 3: r=p;  g=q;  b=v;  break;
        case 4: r=tv; g=p;  b=v;  break; default: r=v;  g=p;  b=q;  break;
    }
    return ImColor(r, g, b, 1.f);
}
// Each joint is projected once, even when several segments share it.
void DrawTracelessSkeleton(CPed* ped, ImDrawList* draw, ImColor color, const ImVec2& screen) {
    Core::Features::PlayerBones bones;
    if (!bones.Open(ped)) return;
    enum Joint { Pelvis, Neck, LeftClavicle, RightClavicle, LeftArm, RightArm,
                 LeftForearm, RightForearm, LeftHand, RightHand, LeftThigh, RightThigh,
                 LeftCalf, RightCalf, LeftFoot, RightFoot, JointCount };
    constexpr unsigned masks[JointCount] = {0x2e28,0x9995,0xfcd9,0x29d2,0xb1c5,0x9d4d,
        0xeeeb,0x6e5c,0x49d9,0xdead,0xe39f,0xca72,0xf9bb,0x9000,0x3779,0xcc4d};
    constexpr Joint links[][2] = {{Pelvis,Neck},{Neck,LeftClavicle},{Neck,RightClavicle},
        {LeftClavicle,LeftArm},{RightClavicle,RightArm},{LeftArm,LeftForearm},
        {RightArm,RightForearm},{LeftForearm,LeftHand},{RightForearm,RightHand},
        {Pelvis,LeftThigh},{Pelvis,RightThigh},{LeftThigh,LeftCalf},{RightThigh,RightCalf},
        {LeftCalf,LeftFoot},{RightCalf,RightFoot}};
    ImVec2 points[JointCount]{};
    bool visible[JointCount]{};
    for (int i = 0; i < JointCount; ++i) {
        D3DXVECTOR3 bone;
        if (!bones.Position(masks[i], bone)) continue;
        const auto pos = Core::SDK::Game::WorldToScreen(bone);
        visible[i] = ExternalAimMath::Inside(pos.x, pos.y, screen.x, screen.y);
        points[i] = ImVec2(pos.x, pos.y);
    }
    for (const auto& link : links)
        if (visible[link[0]] && visible[link[1]])
            draw->AddLine(points[link[0]], points[link[1]], color, ExtraSettings::SkeletonWidth);
}
}

void Core::Features::cEsp::Draw( )
{
	auto Cfg = g_Config.ESP;
    ImFont* extraFont=ExtraSettings::Font==1?ImGui::GetFont():ExtraSettings::Font==2?g_Variables.m_FontSmaller:g_Variables.m_DrawFont;
    const float extraTextSize=ExtraSettings::TextSize;

    // === AdminEsp noclip pre-pass: roda independente do ESP principal ===
    using NoclipClock = std::chrono::steady_clock;
    struct NoclipObservation {
        NoclipClock::time_point since;
        NoclipClock::time_point lastSeen;
    };
    static std::unordered_map<CPed *, NoclipObservation> noclipObservations;
    const auto now = NoclipClock::now();
    if (!Cfg->AdminEsp)
        noclipObservations.clear();
    for (auto it = noclipObservations.begin(); it != noclipObservations.end();) {
        if (now - it->second.lastSeen > std::chrono::seconds(1))
            it = noclipObservations.erase(it);
        else
            ++it;
    }
    if (Cfg->AdminEsp) {
        auto* noclipDraw = ImGui::GetBackgroundDrawList();
        const auto noclipWin = g_Variables.g_vGameWindowSize;
        for (auto& nclEnt : Core::SDK::Game::SnapshotEntities()) {
            CPed* nclPed = nclEnt.Ped;
            if (!nclPed || nclPed == Core::SDK::Pointers::pLocalPlayer ||
                nclEnt.PedType != 2 || nclEnt.Health <= 101.f) continue;
            const bool candidate = !nclPed->InVehicle() &&
                !nclPed->HasFlag(ePedConfigFlag::IsSwimming) &&
                !nclPed->HasFlag(ePedConfigFlag::IsOnGround) &&
                (nclPed->HasFlag(ePedConfigFlag::IsInTheAir) ||
                 nclPed->HasFlag(ePedConfigFlag::NoCollision));
            if (candidate) {
                auto inserted = noclipObservations.emplace(nclPed, NoclipObservation{now, now});
                auto& obs = inserted.first->second;
                obs.lastSeen = now;
                if (now - obs.since >= std::chrono::seconds(3)) {
                    const auto nclHead = Core::SDK::Game::WorldToScreen(nclPed->GetBonePosDefault(0));
                    if (ExternalAimMath::Inside(nclHead.x, nclHead.y, noclipWin.x, noclipWin.y)) {
                        constexpr const char* nclLabel = "Possivel noclip";
                        const ImVec2 nclTs = ImGui::CalcTextSize(nclLabel);
                        const ImVec2 nclLp(nclHead.x - nclTs.x * 0.5f, nclHead.y - 40.f);
                        noclipDraw->AddText(ImVec2(nclLp.x + 1.f, nclLp.y + 1.f), IM_COL32(0, 0, 0, 255), nclLabel);
                        noclipDraw->AddText(nclLp, EspRgbCycle(), nclLabel);
                    }
                }
            } else {
                noclipObservations.erase(nclPed);
            }
        }
    }

	if ( !Cfg->Enabled )
		return;


	for ( auto & Entity : Core::SDK::Game::SnapshotEntities() )
	{
		CPed * Ped = Entity.Ped;
        if (!Ped) continue;
		auto DrawList = ImGui::GetBackgroundDrawList( );

		float Distance = Entity.Distance;
		float Health = Entity.Health;
		int PedType = Entity.PedType;
		bool IsFriend = Entity.IsFriend;
		bool IsLocalPlayer = Ped == Core::SDK::Pointers::pLocalPlayer;

		if ( !Cfg->ShowLocalPlayer && IsLocalPlayer )
			continue;

		if (Distance > ImClamp(Cfg->MaxDistance, 0, Cfg->DistanceLimit) || (Cfg->HighlightVisible && !Ped->IsVisible()))
			continue;

		if ( Cfg->IgnoreNPCs && PedType != 2 && !IsLocalPlayer )
			continue;

		if ( Cfg->IgnoreDead && !IsLocalPlayer )
		{
			if ( PedType == 2 )
			{
				if ( Health <= 101.f )
					continue;
			}
			else
			{
				if ( Health <= 1.f )
					continue;
			}
		}


        // Supplied source: head/feet geometry and projected model bounds.
        D3DXVECTOR3 HeadPos = Ped->GetBonePosDefault(0);
        const auto head = Core::SDK::Game::WorldToScreen(HeadPos);
        const auto leftFoot = Core::SDK::Game::WorldToScreen(Ped->GetBonePosDefault(3));
        const auto rightFoot = Core::SDK::Game::WorldToScreen(Ped->GetBonePosDefault(4));
        const auto position = Core::SDK::Game::WorldToScreen(Entity.Pos);
        const auto window = g_Variables.g_vGameWindowSize;
        const auto validScreen = [&](D3DXVECTOR2 point) {
            return ExternalAimMath::Inside(point.x, point.y, window.x, window.y);
        };
        if (!validScreen(head) || !validScreen(leftFoot) || !validScreen(rightFoot) || !validScreen(position)) continue;
        const float bodyHeight = (std::max)(leftFoot.y, rightFoot.y) - head.y;
        const float centerY = head.y + bodyHeight * 0.5f;
        float Height = bodyHeight * 1.2f;
        const float fullWidth = bodyHeight / (1.8f + ImClamp(Cfg->BoxSize, 0, 30) / 100.f);
        D3DXVECTOR2 EntityTop(position.x, centerY - Height * 0.5f);
        D3DXVECTOR2 EntityBottom(position.x, centerY + Height * 0.5f);
        float BoxLeft = position.x - fullWidth * 0.5f;
        float BoxRight = position.x + fullWidth * 0.5f;
        if (Cfg->StaticESP) {
            const uintptr_t model = Mem.Read<uintptr_t>(reinterpret_cast<uintptr_t>(Ped) + 0x20);
            if (!model) continue;
            const auto mins = Mem.Read<D3DXVECTOR3>(model + 0x30);
            const auto maxs = Mem.Read<D3DXVECTOR3>(model + 0x40);
            if (!std::isfinite(mins.x) || !std::isfinite(mins.y) || !std::isfinite(mins.z) ||
                !std::isfinite(maxs.x) || !std::isfinite(maxs.y) || !std::isfinite(maxs.z) ||
                mins.x >= maxs.x || mins.y >= maxs.y || mins.z >= maxs.z) continue;
            float x0 = FLT_MAX, y0 = FLT_MAX, x1 = -FLT_MAX, y1 = -FLT_MAX;
            bool valid = true;
            for (int corner = 0; corner < 8; ++corner) {
                const D3DXVECTOR3 point = Entity.Pos + D3DXVECTOR3(
                    corner & 1 ? maxs.x : mins.x, corner & 2 ? maxs.y : mins.y, corner & 4 ? maxs.z : mins.z);
                const auto projected = Core::SDK::Game::WorldToScreen(point);
                if (!validScreen(projected)) { valid = false; break; }
                x0 = (std::min)(x0, projected.x); x1 = (std::max)(x1, projected.x);
                y0 = (std::min)(y0, projected.y); y1 = (std::max)(y1, projected.y);
            }
            if (!valid) continue;
            const float half = (x1 - x0) * 0.5f * (1.80f / (1.80f + ImClamp(Cfg->BoxSize, 0, 30) / 100.f));
            BoxLeft = (x0 + x1) * 0.5f - half; BoxRight = (x0 + x1) * 0.5f + half;
            EntityTop = D3DXVECTOR2((x0 + x1) * 0.5f, y0);
            EntityBottom = D3DXVECTOR2((x0 + x1) * 0.5f, y1);
            Height = y1 - y0;
        }
        if (!std::isfinite(Height) || Height <= 1.f || BoxRight <= BoxLeft) continue;
        ImVec2 BoxMin(BoxLeft, EntityTop.y), BoxMax(BoxRight, EntityBottom.y);
        ImVec2 BoxCenter((BoxLeft + BoxRight) * 0.5f, (EntityTop.y + EntityBottom.y) * 0.5f);



        if (ExtraSettings::PlayerGlow) {
            const ImU32 color = IsFriend ? static_cast<ImU32>(ImColor(g_Col.Base))
                                         : ImGui::GetColorU32(Cfg->BoxCol.Value);
            DrawList->AddShadowRect(BoxMin, BoxMax, color, 50.f, ImVec2(0, 0));
        }
		if ( Cfg->FriendsMarker && !IsLocalPlayer )
		{
			float radius = 2 / Distance;

			const static int FovRadius = 60;
			const static int Delay = 300;
			int Key = Cfg->FriendsMarkerBind;
			//std::string PlayerSetFriend = xorstr( "Press ") + std::string(keys[ Key ]) + xorstr(" to Add Friend" );
			std::string PlayerSetFriend = xorstr("Add Friend" );
			std::string PlayerUnFriend = xorstr( "Unfriend" );


			int FovSize = std::hypot( BoxCenter.x - g_Variables.g_vGameWindowCenter.x, BoxCenter.y - g_Variables.g_vGameWindowCenter.y );

			ImGui::PushFont( g_Variables.m_FontSmaller );

			if ( IsFriend && FovSize < FovRadius )
			{
				DrawList->AddCircleFilled( ImVec2( BoxCenter.x, BoxCenter.y ), radius + Distance < 1 ? 1 : 4, ImColor( 0, 0, 0 ), 999 );
				DrawList->AddCircleFilled( ImVec2( BoxCenter.x, BoxCenter.y ), radius + Distance < 1 ? 0 : 3, ImColor( g_Col.Base ), 999 );

				//ImVec2 TextSize = g_Variables.m_FontSmaller->CalcTextSizeA( g_Variables.m_FontSmaller->FontSize, FLT_MAX, 0.0f, PlayerUnFriend.c_str( ) );
				//DrawList->AddText( ImVec2( BoxCenter.x - ( TextSize.x / 2 ), BoxCenter.y ), ImColor( 0, 0, 0 ), PlayerUnFriend.c_str( ) );
				//DrawList->AddText( ImVec2( BoxCenter.x - ( TextSize.x / 2 ), BoxCenter.y - 2 ), ImColor( 255, 255, 255 ), PlayerUnFriend.c_str( ) );

				if ( Utils::KeyPressedWithDelay( Key, Delay ) )
				{
					IsFriend = false;
					Core::SDK::Game::FriendMap[ Ped ] = IsFriend;
				}
			}
			else if( !IsFriend && FovSize < FovRadius )
			{
				DrawList->AddCircleFilled( ImVec2( BoxCenter.x, BoxCenter.y ), radius + Distance < 1 ? 1 : 4, ImColor( 0, 0, 0 ), 999 );
				DrawList->AddCircleFilled( ImVec2( BoxCenter.x, BoxCenter.y ), radius + Distance < 1 ? 0 : 3, ImColor( 255, 255, 255 ), 999 );

				//ImVec2 TextSize = g_Variables.m_FontSmaller->CalcTextSizeA( g_Variables.m_FontSmaller->FontSize, FLT_MAX, 0.0f, PlayerSetFriend.c_str( ) );
				//DrawList->AddText( ImVec2( BoxCenter.x - ( TextSize.x / 2 ), BoxCenter.y ), ImColor( 0, 0, 0 ), PlayerSetFriend.c_str( ) );
				//DrawList->AddText( ImVec2( BoxCenter.x - ( TextSize.x / 2 ), BoxCenter.y - 1 ), ImColor( 255, 255, 255 ), PlayerSetFriend.c_str( ) );

				if ( Utils::KeyPressedWithDelay( Key, Delay ) )
				{
					IsFriend = true;
					Core::SDK::Game::FriendMap[ Ped ] = IsFriend;
				}
			}
			ImGui::PopFont( );
		}

		const auto color = [&](ImColor normal) -> ImColor { return IsFriend ? Cfg->FriendCol : normal; };
        if (Cfg->SnapLines && !IsLocalPlayer) {
            const float snapY = Cfg->SnapLinesFrom == 1 ? 0.f : window.y;
            DrawList->AddLine(ImVec2(window.x * 0.5f, snapY), ImVec2(BoxCenter.x, Cfg->SnapLinesFrom == 1 ? BoxMin.y : BoxMax.y), color(Cfg->SnapLinesCol), 1.f);
        }
        if (Cfg->Box3D) {
            const float bottom = (std::min)(Ped->GetBonePosDefault(3).z, Ped->GetBonePosDefault(4).z);
            const float top = HeadPos.z + 0.2f;
            const float radius = (top - bottom) * 0.25f;
            if (radius > 0.f && radius < 2.f) {
                D3DXVECTOR2 corners[8];
                bool visible[8];
                for (int i = 0; i < 8; ++i) {
                    const D3DXVECTOR3 corner(Entity.Pos.x + ((i & 1) ? radius : -radius),
                        Entity.Pos.y + ((i & 2) ? radius : -radius), (i & 4) ? top : bottom);
                    corners[i] = SDK::Game::WorldToScreen(corner);
                    visible[i] = validScreen(corners[i]);
                }
                for (int i = 0; i < 8; ++i) {
                    for (int axis : {1, 2, 4}) {
                        const int next = i ^ axis;
                        if (i < next && visible[i] && visible[next])
                            DrawList->AddLine(ImVec2(corners[i].x,corners[i].y), ImVec2(corners[next].x,corners[next].y),
                                              color(Cfg->BoxCol), ExtraSettings::BoxWidth);
                    }
                }
            }
        } else if (Cfg->Box) {
            if (Cfg->FilledBox) {
                ImVec4 fill = color(Cfg->FilledBoxCol).Value;
                fill.w = Cfg->FilledBoxCol.Value.w;
                DrawList->AddRectFilled(BoxMin, BoxMax, ImGui::ColorConvertFloat4ToU32(fill));
            }
            DrawList->AddRect(BoxMin, BoxMax, color(Cfg->BoxCol), 0, 0, ExtraSettings::BoxWidth);
        }
        if (Cfg->CornerBox && !Cfg->Box3D) {
            const float length = (std::min)(15.f, (BoxRight - BoxLeft) * 0.5f);
            for (int i = 0; i < 4; ++i) {
                const float x = i & 1 ? BoxRight : BoxLeft;
                const float y = i & 2 ? EntityBottom.y : EntityTop.y;
                const float dx = i & 1 ? -length : length;
                const float dy = i & 2 ? -length : length;
                DrawList->AddLine(ImVec2(x,y), ImVec2(x+dx,y), color(Cfg->CornerColor), ExtraSettings::BoxWidth);
                DrawList->AddLine(ImVec2(x,y), ImVec2(x,y+dy), color(Cfg->CornerColor), ExtraSettings::BoxWidth);
            }
        }

        if (Cfg->Skeleton) {
            DrawTracelessSkeleton(Ped, DrawList, color(Cfg->SkeletonCol), window);
        }

        if(ExtraSettings::ChinaHat && HeadPos!=D3DXVECTOR3(0,0,0)) {
            auto apex=SDK::Game::WorldToScreen(HeadPos+D3DXVECTOR3(0,0,0.35f));
            D3DXVECTOR2 last{};
            for(int side=0;side<=24;side++){
                float angle=side*6.2831853f/24.f;
                auto ring=SDK::Game::WorldToScreen(HeadPos+D3DXVECTOR3(std::cos(angle)*ExtraSettings::HatSize,std::sin(angle)*ExtraSettings::HatSize,0.1f));
                if(validScreen(ring)&&validScreen(apex)){
                    if(side&&validScreen(last))DrawList->AddLine(ImVec2(last.x,last.y),ImVec2(ring.x,ring.y),Cfg->SkeletonCol,1.f);
                    if(side%3==0)DrawList->AddLine(ImVec2(apex.x,apex.y),ImVec2(ring.x,ring.y),Cfg->SkeletonCol,1.f);
                }last=ring;
            }
        }

        if(ExtraSettings::WeaponIcon){
            auto manager=Ped->GetWeaponManager();auto info=manager?manager->GetWeaponInfo():nullptr;
            if(info){auto texture=ExtraWeaponIcons::Find(info->GetName());if(texture)DrawList->AddImage(texture,ImVec2(BoxCenter.x-24,BoxMax.y+3.f*(extraTextSize+4.f)+16.f),ImVec2(BoxCenter.x+24,BoxMax.y+3.f*(extraTextSize+4.f)+40.f));}
        }

		if (Cfg->HeadCircle)
            DrawList->AddCircle(ImVec2(head.x, head.y), Cfg->HeadSize, color(Cfg->SkeletonCol), 12, 1.f);

        auto label = [&](const std::string& text, float y, ImColor ink) {
            const float width = extraFont->CalcTextSizeA(extraTextSize, FLT_MAX, 0.f, text.c_str()).x;
            const ImVec2 pos(BoxCenter.x - width * 0.5f, y);
            DrawList->AddText(extraFont, extraTextSize, ImVec2(pos.x + 1.f, pos.y + 1.f), ImColor(0.f,0.f,0.f,ink.Value.w), text.c_str());
            DrawList->AddText(extraFont, extraTextSize, pos, ink, text.c_str());
        };
        {
            const uintptr_t pedAddr = reinterpret_cast<uintptr_t>(Ped);
            const bool globalNames = Cfg->UserNames;
            const bool pinned = PedType == 2 && Core::Features::EspName::IsPinned(pedAddr);
            if (globalNames || pinned) {
                std::string nameLabel;
                if (PedType == 2) {
                    const std::string& uname = Entity.NetworkInfo.UserName;
                    nameLabel = (uname.empty() ? ("Jogador #" + std::to_string(Entity.Id)) : uname)
                                + " [" + std::to_string(Entity.Id) + "]";
                } else {
                    nameLabel = "NPC";
                }
                // Peds pinados individualmente aparecem em amarelo, global usa cor configurada
                const ImColor nameColor = pinned ? ImColor(255,220,50,255) : color(Cfg->UserNamesCol);
                label(nameLabel, BoxMin.y - extraTextSize - 2.f, nameColor);
            }
        }
        float y = BoxMax.y + 2.f;
        if (Cfg->WeaponName && !Entity.WeaponName.empty()) {
            label(Entity.WeaponName, y, Cfg->WeaponNameCol);
            y += extraTextSize;
        }
        if (Cfg->DistanceFromMe && !IsLocalPlayer)
            label(std::to_string(static_cast<int>(Distance)) + "m", y, Cfg->DistanceCol);
        // Barra vertical (esquerda/direita)
        auto bar = [&](float x, float width, float fraction, ImU32 ink) {
            DrawList->AddRectFilled(ImVec2(x, BoxMin.y), ImVec2(x+width, BoxMax.y), IM_COL32(0,0,0,150), 2.f);
            DrawList->AddRectFilled(ImVec2(x, BoxMax.y - Height * ImClamp(fraction,0.f,1.f)), ImVec2(x+width,BoxMax.y), ink, 2.f);
        };
        // Barra horizontal (cima/baixo)
        auto hbar = [&](float y, float barH, float fraction, ImU32 ink) {
            DrawList->AddRectFilled(ImVec2(BoxMin.x, y), ImVec2(BoxMax.x, y+barH), IM_COL32(0,0,0,150), 2.f);
            DrawList->AddRectFilled(ImVec2(BoxMin.x, y), ImVec2(BoxMin.x + (BoxMax.x - BoxMin.x) * ImClamp(fraction,0.f,1.f), y+barH), ink, 2.f);
        };
        if (Cfg->HealthBar) {
            // Usa MaxHealth lido do ped; se vier 0 ou negativo (offset errado),
            // assume 200 como referência padrão do GTA5.
            const float maxHp = Entity.MaxHealth > 50.f ? Entity.MaxHealth : 200.f;
            const float hp = ImClamp(Health / maxHp, 0.f, 1.f);
            const ImU32 hpCol = hp > 0.7f ? IM_COL32(0,255,0,255) : hp > 0.3f ? IM_COL32(255,255,0,255) : IM_COL32(255,0,0,255);
            switch (Cfg->HealthBarState) {
                default:
                case 0: bar(BoxMin.x - 3.f - Cfg->HealthBarWidth, Cfg->HealthBarWidth, hp, hpCol); break; // esquerda
                case 1: bar(BoxMax.x + 3.f, Cfg->HealthBarWidth, hp, hpCol); break;                       // direita
                case 2: hbar(BoxMin.y - 3.f - Cfg->HealthBarWidth, Cfg->HealthBarWidth, hp, hpCol); break; // cima
                case 3: hbar(BoxMax.y + 3.f, Cfg->HealthBarWidth, hp, hpCol); break;                       // baixo
            }
        }
        if (Cfg->ArmorBar) {
            const float arm = ImClamp(Entity.Armor / 100.f, 0.f, 1.f);
            const ImU32 armCol = IM_COL32(0,150,255,255);
            switch (Cfg->ArmorBarState) {
                default:
                case 0: bar(BoxMin.x - 3.f - Cfg->ArmorBarWidth - (Cfg->HealthBar && Cfg->HealthBarState == 0 ? Cfg->HealthBarWidth + 2.f : 0.f), Cfg->ArmorBarWidth, arm, armCol); break; // esquerda
                case 1: bar(BoxMax.x + 3.f + (Cfg->HealthBar && Cfg->HealthBarState == 1 ? Cfg->HealthBarWidth + 2.f : 0.f), Cfg->ArmorBarWidth, arm, armCol); break; // direita
                case 2: hbar(BoxMin.y - 3.f - Cfg->ArmorBarWidth - (Cfg->HealthBar && Cfg->HealthBarState == 2 ? Cfg->HealthBarWidth + 2.f : 0.f), Cfg->ArmorBarWidth, arm, armCol); break; // cima
                case 3: hbar(BoxMax.y + 3.f + (Cfg->HealthBar && Cfg->HealthBarState == 3 ? Cfg->HealthBarWidth + 2.f : 0.f), Cfg->ArmorBarWidth, arm, armCol); break; // baixo
            }
        }
    }
}

void Core::Features::cEsp::DrawVehicle( )
{
	auto Cfg = g_Config.VehicleESP;

	if ( !Cfg->Enabled )
		return;

	auto extraVehicles=SDK::Game::SnapshotVehicles();
    for ( auto Entity : extraVehicles )
	{
		CVehicle * Vehicle = Entity.Pointer;
        if(!Vehicle || (ExtraSettings::IgnoreOccupied && Entity.Driver))continue;
		auto DrawList = ImGui::GetBackgroundDrawList( );

		float Distance = Entity.Dist;

		if ( Distance > Cfg->MaxDistance )
			continue;

		D3DXVECTOR2 VehicleLocation = Core::SDK::Game::WorldToScreen(
			Vehicle->GetPos( ) //Usei aqui pra não ter esse delay do esp por causa da thread
		);

		if ( !VehicleLocation )
			continue;

		if ( !SDK::Game::IsOnScreen( VehicleLocation ) )
			continue;

		if ( Cfg->SnapLines )
		{
			DrawList->AddLine( ImVec2( g_Variables.g_vGameWindowCenter.x, g_Variables.g_vGameWindowPos.y + g_Variables.g_vGameWindowSize.y ), ImVec2( VehicleLocation.x, VehicleLocation.y + 30 ), ImColor( 0, 0, 0 ), 2.5 );
			DrawList->AddLine( ImVec2( g_Variables.g_vGameWindowCenter.x, g_Variables.g_vGameWindowPos.y + g_Variables.g_vGameWindowSize.y ), ImVec2( VehicleLocation.x, VehicleLocation.y + 30 ), Cfg->SnapLinesCol, 1.5 );
		}

		float labelY = VehicleLocation.y;
        if (ExtraSettings::VehicleMarker) {
            const ImVec2 center(VehicleLocation.x, labelY);
            DrawList->AddCircleFilled(center, 4.f, IM_COL32_BLACK);
            DrawList->AddCircleFilled(center, 3.f, Cfg->SnapLinesCol);
            labelY += 12.f;
        }
        auto label = [&](const std::string& value) {
            const float x = VehicleLocation.x - ImGui::CalcTextSize(value.c_str()).x * 0.5f;
            DrawList->AddText(ImVec2(x + 1.f, labelY + 1.f), IM_COL32_BLACK, value.c_str());
            DrawList->AddText(ImVec2(x, labelY), IM_COL32_WHITE, value.c_str());
            labelY += 12.f;
        };
        if (Cfg->VehName) label(Entity.Name);
        if (Cfg->DistanceFromMe) label(std::to_string(static_cast<int>(Distance)) + "m");

        if ( Cfg->ShowLockUnlock ) {
			bool IsLocked = Entity.IsLocked;
			float radius = 2 / Distance;

			const static std::string VehicleLocked = xorstr( "Press L to Unlock" );
			const static std::string VehicleUnlocked = xorstr( "Press L to Lock" );
			const static int fovRadius = 40;
			const static int L = 0x4C;
			const static int Delay = 500;


			int FovSize = std::hypot( VehicleLocation.x - g_Variables.g_vGameWindowCenter.x, VehicleLocation.y - g_Variables.g_vGameWindowCenter.y );

			if ( IsLocked )
			{
				DrawList->AddCircleFilled( ImVec2( VehicleLocation.x, VehicleLocation.y - 6 ), radius + Distance < 1 ? 1 : 4, ImColor( 0, 0, 0 ), 999 );
				DrawList->AddCircleFilled( ImVec2( VehicleLocation.x, VehicleLocation.y - 6 ), radius + Distance < 1 ? 0 : 3, ImColor( 250, 72, 62, 255 ), 999 );

				if ( FovSize < fovRadius )
				{
					ImVec2 text_size = g_Variables.m_DrawFont->CalcTextSizeA( g_Variables.m_DrawFont->FontSize, FLT_MAX, 0.0f, VehicleLocked.c_str( ) );

					DrawList->AddText( ImVec2( VehicleLocation.x - ( text_size.x / 2 ), VehicleLocation.y - 28 ), ImColor( 0, 0, 0 ), VehicleLocked.c_str( ) );
					DrawList->AddText( ImVec2( VehicleLocation.x - ( text_size.x / 2 ), VehicleLocation.y - 30 ), ImColor( 255, 255, 255 ), VehicleLocked.c_str( ) );

					if ( Utils::KeyPressedWithDelay( L, Delay ) )
						Vehicle->DoorState( true );
				}



			}
			else if ( !IsLocked )
			{
				DrawList->AddCircleFilled( ImVec2( VehicleLocation.x, VehicleLocation.y - 6 ), radius + Distance < 1 ? 1 : 4, ImColor( 0, 0, 0 ), 999 );
				DrawList->AddCircleFilled( ImVec2( VehicleLocation.x, VehicleLocation.y - 6 ), radius + Distance < 1 ? 0 : 3, ImColor( 102, 255, 133, 255 ), 999 );

				if ( FovSize < fovRadius )
				{
					ImVec2 text_size = g_Variables.m_DrawFont->CalcTextSizeA( g_Variables.m_DrawFont->FontSize, FLT_MAX, 0.0f, VehicleUnlocked.c_str( ) );

					DrawList->AddText( ImVec2( VehicleLocation.x - ( text_size.x / 2 ), VehicleLocation.y - 28 ), ImColor( 0, 0, 0 ), VehicleUnlocked.c_str( ) );
					DrawList->AddText( ImVec2( VehicleLocation.x - ( text_size.x / 2 ), VehicleLocation.y - 30 ), ImColor( 255, 255, 255 ), VehicleUnlocked.c_str( ) );

					if ( Utils::KeyPressedWithDelay( L, Delay ) )
						Vehicle->DoorState( false );
				}
			}
		}

	}
}
