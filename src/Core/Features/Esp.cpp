#include "Esp.hpp"
#include <Core/ExtensionSettings.hpp>
#include <chrono>
#include "ExternalAimMath.hpp"



void Core::Features::cEsp::Draw( )
{
	auto Cfg = g_Config.ESP;
    ImFont* extraFont=ExtraSettings::Font==1?ImGui::GetFont():g_Variables.m_DrawFont;
    const float extraTextSize=ExtraSettings::TextSize;

	if ( !Cfg->Enabled )
		return;

	static std::unordered_map<CPed *, Core::SDK::Game::EspAnim> vEspAnimations;
    using NoclipClock = std::chrono::steady_clock;
    struct NoclipObservation {
        NoclipClock::time_point since;
        NoclipClock::time_point lastSeen;
    };
    static std::unordered_map<CPed *, NoclipObservation> noclipObservations;
    const auto now = NoclipClock::now();
    if (!Cfg->PossibleNoclip)
        noclipObservations.clear();
    for (auto it = noclipObservations.begin(); it != noclipObservations.end();) {
        if (now - it->second.lastSeen > std::chrono::seconds(1))
            it = noclipObservations.erase(it);
        else
            ++it;
    }


	for ( auto & Entity : Core::SDK::Game::EntityList )
	{
		CPed * Ped = Entity.Ped;
        if (!Ped) continue;
		auto DrawList = ImGui::GetBackgroundDrawList( );

		auto & CurrentESPAnim = vEspAnimations[ Ped ];
		CurrentESPAnim.CanFadeOut = false;

		float Distance = Entity.Distance;
		float Health = Entity.Health;
		int PedType = Entity.PedType;
		bool IsFriend = Entity.IsFriend;
		bool IsLocalPlayer = Ped == Core::SDK::Pointers::pLocalPlayer;

		if ( !Cfg->ShowLocalPlayer && IsLocalPlayer )
			continue;

		if ( Distance > Cfg->MaxDistance )
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


        bool possibleNoclip = false;
        if (Cfg->PossibleNoclip && Ped && !IsLocalPlayer && PedType == 2 && Health > 101.f) {
            const bool candidate = !Ped->InVehicle() &&
                !Ped->HasFlag(ePedConfigFlag::IsSwimming) &&
                !Ped->HasFlag(ePedConfigFlag::IsOnGround) &&
                (Ped->HasFlag(ePedConfigFlag::IsInTheAir) ||
                 Ped->HasFlag(ePedConfigFlag::NoCollision));
            if (candidate) {
                auto inserted = noclipObservations.emplace(Ped, NoclipObservation{now, now});
                auto & observation = inserted.first->second;
                observation.lastSeen = now;
                possibleNoclip = now - observation.since >= std::chrono::seconds(3);
            } else {
                noclipObservations.erase(Ped);
            }
        } else {
            noclipObservations.erase(Ped);
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
        float Height = std::fabs(head.y - (leftFoot.y + rightFoot.y) * 0.5f) * 1.35f;
        float fullWidth = Height / (1.80f + ImClamp(Cfg->BoxSize, 0, 30) / 100.f);
        D3DXVECTOR2 EntityTop(position.x, position.y - Height * 0.5f);
        D3DXVECTOR2 EntityBottom(position.x, position.y + Height * 0.5f);
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
        float Width = (BoxRight - BoxLeft) * 0.5f;
        ImVec2 BoxMin(BoxLeft, EntityTop.y), BoxMax(BoxRight, EntityBottom.y);
        ImVec2 BoxCenter((BoxLeft + BoxRight) * 0.5f, (EntityTop.y + EntityBottom.y) * 0.5f);



        if (possibleNoclip) {
            const char * label = "Possivel noclip";
            const ImVec2 textSize = ImGui::CalcTextSize(label);
            const ImVec2 labelPos(BoxCenter.x - textSize.x * 0.5f, BoxMin.y - 40.f);
            DrawList->AddText(ImVec2(labelPos.x + 1.f, labelPos.y + 1.f), IM_COL32(0, 0, 0, 255), label);
            DrawList->AddText(labelPos, IM_COL32(255, 190, 60, 255), label);
        }
		float FirstTextBoxTop = BoxMin.y - 18;

		float FirstTextBoxBottom = BoxMax.y + 4;
		float SecondTextBoxBottom = BoxMax.y + ( 4 * 2 ) + 8; //8 is padding
		float ThirdTextBoxBottom = BoxMax.y + ( 4 * 3 ) + ( 8 * 2 );

		if ( Cfg->HealthBarState == 2 && Cfg->ArmorBarState == 2 ) {
			FirstTextBoxBottom += 16;
			SecondTextBoxBottom += 16;
			ThirdTextBoxBottom += 16;
		}
		else if ( Cfg->HealthBarState == 2 ) {
			FirstTextBoxBottom += 8;
			SecondTextBoxBottom += 8;
			ThirdTextBoxBottom += 8;
		}
		else if ( Cfg->ArmorBarState == 2 ) {
			FirstTextBoxBottom += 16;
			SecondTextBoxBottom += 16;
			ThirdTextBoxBottom += 16;
		}

		if ( Cfg->HealthBarState == 0 && Cfg->ArmorBarState == 0 ) {
			FirstTextBoxTop -= 4;
		}
		else if ( Cfg->HealthBarState == 0 ) {
			FirstTextBoxTop -= 4;
		}
		else if ( Cfg->ArmorBarState == 0 ) {
			FirstTextBoxTop -= 4;
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

		if ( IsFriend )
			continue;

		if ( Cfg->SnapLines )
		{
			DrawList->AddLine( ImVec2( g_Variables.g_vGameWindowCenter.x, g_Variables.g_vGameWindowPos.y + g_Variables.g_vGameWindowSize.y ), ImVec2( BoxCenter.x, BoxMax.y ), ImColor( 0, 0, 0 ), 2.5 );
			DrawList->AddLine( ImVec2( g_Variables.g_vGameWindowCenter.x, g_Variables.g_vGameWindowPos.y + g_Variables.g_vGameWindowSize.y ), ImVec2( BoxCenter.x, BoxMax.y ), Cfg->SnapLinesCol, 1.5 );
		}

        if ((Cfg->Box || Cfg->CornerBox) && Cfg->FilledBox)
            DrawList->AddRectFilled(BoxMin, BoxMax, Cfg->FilledBoxCol);
        if (Cfg->Box) {
            DrawList->AddRect(BoxMin, BoxMax, IM_COL32(0,0,0,220), 0, 0, 3.f);
            DrawList->AddRect(BoxMin, BoxMax, Cfg->BoxCol, 0, 0, ExtraSettings::BoxWidth);
        }
        if (Cfg->CornerBox) {
            const float length = (BoxRight - BoxLeft) * 0.25f;
            for (int i = 0; i < 4; ++i) {
                const float x = i & 1 ? BoxRight : BoxLeft;
                const float y = i & 2 ? EntityBottom.y : EntityTop.y;
                const float dx = i & 1 ? -length : length;
                const float dy = i & 2 ? -length : length;
                DrawList->AddLine(ImVec2(x,y), ImVec2(x+dx,y), IM_COL32(0,0,0,220), 3.f);
                DrawList->AddLine(ImVec2(x,y), ImVec2(x,y+dy), IM_COL32(0,0,0,220), 3.f);
                DrawList->AddLine(ImVec2(x,y), ImVec2(x+dx,y), Cfg->CornerColor, ExtraSettings::BoxWidth);
                DrawList->AddLine(ImVec2(x,y), ImVec2(x,y+dy), Cfg->CornerColor, ExtraSettings::BoxWidth);
            }
        }

        if (Cfg->Skeleton) {
            // Cached bone pairs used by the supplied ESP.
            static const int pairs[][2] = {{0,7},{7,6},{7,5},{7,8},{8,3},{8,4}};
            for (const auto& pair : pairs) {
                const auto a = Core::SDK::Game::WorldToScreen(Ped->GetBonePosDefault(pair[0]));
                const auto b = Core::SDK::Game::WorldToScreen(Ped->GetBonePosDefault(pair[1]));
                if (validScreen(a) && validScreen(b))
                    DrawList->AddLine(ImVec2(a.x,a.y), ImVec2(b.x,b.y), Cfg->SkeletonCol, ExtraSettings::SkeletonWidth);
            }
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

		if ( Cfg->HeadCircle )
		{
			if ( HeadPos != D3DXVECTOR3( 0, 0, 0 ) ) {
				D3DXVECTOR2 head_screen_pos = SDK::Game::WorldToScreen( HeadPos );
				if ( head_screen_pos != D3DXVECTOR2( 0, 0 ) ) {

					int CircleValue = Height / 15;
					if ( CircleValue <= 10 ) { Height / 20; }

					ImVec2 center( head_screen_pos.x, head_screen_pos.y );
					DrawList->AddCircle( center, CircleValue, Cfg->SkeletonCol, 999, 1.5 );
				}
			}
		}

		if ( Cfg->DistanceFromMe )
		{

			std::string DistStr = std::to_string( ( int ) Distance ) + xorstr( "m" );

			ImVec2 TextSize = extraFont->CalcTextSizeA( extraTextSize, FLT_MAX, 0.0f, DistStr.c_str( ) );
			ImVec2 TextPos = ImVec2( BoxCenter.x - ( TextSize.x / 2 ), FirstTextBoxBottom );

			switch ( Cfg->DistanceFromMeState )
			{

			case 0: //Top

				if ( Cfg->UserNames && Cfg->UserNamesState == 0 )
				{
					TextPos.y = FirstTextBoxTop - 15;
				}

				if ( Cfg->WeaponName && Cfg->WeaponNameState == 0 )
				{
					TextPos.y = FirstTextBoxTop - 15;
				}

				if ( Cfg->WeaponName && Cfg->WeaponNameState == 0 && Cfg->UserNames && Cfg->UserNamesState == 0 )
				{
					TextPos.y = FirstTextBoxTop - 30;
				}

				if ( Cfg->WeaponNameState == 2 && Cfg->UserNamesState == 2 )
				{
					TextPos.y = FirstTextBoxTop;
				}


				if ( !Cfg->UserNames && !Cfg->WeaponName )
				{
					TextPos.y = FirstTextBoxTop;
				}

				break;
			case 2: //Bottom
				if ( Cfg->UserNames && Cfg->UserNamesState == 2 )
				{
					TextPos.y = FirstTextBoxBottom + 15;
				}

				if ( Cfg->WeaponName && Cfg->WeaponNameState == 2 )
				{
					TextPos.y = FirstTextBoxBottom + 15;
				}

				if ( Cfg->WeaponName && Cfg->WeaponNameState == 2 && Cfg->UserNames && Cfg->UserNamesState == 2 )
				{
					TextPos.y = FirstTextBoxBottom + 30;
				}

				if ( !Cfg->UserNames && !Cfg->WeaponName )
				{
					TextPos.y = FirstTextBoxBottom;
				}

				break;
			default:
				TextPos.y = FirstTextBoxBottom;
				break;
			}

			/*DrawList->AddText( extraFont, extraTextSize, ImVec2( TextPos.x + 1, TextPos.y + 1 ), ImColor( 0, 0, 0, 100 ), DistStr.c_str( ) );
			DrawList->AddText( extraFont, extraTextSize, ImVec2( TextPos.x - 1, TextPos.y + 1 ), ImColor( 0, 0, 0, 100 ), DistStr.c_str( ) );
			DrawList->AddText( extraFont, extraTextSize, ImVec2( TextPos.x + 1, TextPos.y - 1 ), ImColor( 0, 0, 0, 100 ), DistStr.c_str( ) );
			DrawList->AddText( extraFont, extraTextSize, ImVec2( TextPos.x - 1, TextPos.y - 1 ), ImColor( 0, 0, 0, 100 ), DistStr.c_str( ) );*/

			DrawList->AddText( extraFont, extraTextSize, ImVec2( TextPos.x + 1, TextPos.y + 1 ), ImColor( 0.f, 0.f, 0.f, Cfg->DistanceCol.Value.w ), DistStr.c_str( ) );
			DrawList->AddText( extraFont, extraTextSize, TextPos, Cfg->DistanceCol, DistStr.c_str( ) );

		}

		if ( Cfg->WeaponName ) {

			std::string weaponName = Entity.WeaponName;

			auto to_lower = [ ] ( const char * str ) {
				std::string message = str;
				if ( !message.empty( ) ) {
					std::transform( message.begin( ), message.end( ), message.begin( ), ::tolower );
					message[ 0 ] = std::toupper( message[ 0 ] );
				}
				return message;
				};

			if ( weaponName.empty( ) )
				weaponName = xorstr( "Unarmed" );

			ImVec2 TextSize = extraFont->CalcTextSizeA( extraTextSize, FLT_MAX, 0.0f, to_lower( weaponName.c_str( ) ).c_str( ) );
			ImVec2 TextPos = ImVec2( BoxCenter.x - ( TextSize.x / 2 ), FirstTextBoxBottom );

			if ( Cfg->DistanceFromMe )
				TextPos.y = FirstTextBoxBottom;

			if ( Cfg->UserNamesState == 1 )
				TextPos.y = SecondTextBoxBottom;

			switch ( Cfg->WeaponNameState )
			{

			case 0: //Top
				if ( Cfg->UserNamesState == 0 && Cfg->UserNames )
				{
					TextPos.y = FirstTextBoxTop - 15;
				}
				else
				{
					TextPos.y = FirstTextBoxTop;
				}

				break;
			case 2: //Bottom
				if ( Cfg->UserNamesState == 2 && Cfg->UserNames )
				{
					TextPos.y = FirstTextBoxBottom + 15;
				}
				else
				{
					TextPos.y = FirstTextBoxBottom;
				}
				break;
			default:
				TextPos.y = FirstTextBoxBottom;
				break;
			}

			std::string WeaponNameUnder = to_lower( weaponName.c_str( ) );

			/*DrawList->AddText( extraFont, extraTextSize, ImVec2( TextPos.x + 1, TextPos.y + 1 ), ImColor( 0, 0, 0, 100 ), WeaponNameUnder.c_str( ) );
			DrawList->AddText( extraFont, extraTextSize, ImVec2( TextPos.x - 1, TextPos.y + 1 ), ImColor( 0, 0, 0, 100 ), WeaponNameUnder.c_str( ) );
			DrawList->AddText( extraFont, extraTextSize, ImVec2( TextPos.x + 1, TextPos.y - 1 ), ImColor( 0, 0, 0, 100 ), WeaponNameUnder.c_str( ) );
			DrawList->AddText( extraFont, extraTextSize, ImVec2( TextPos.x - 1, TextPos.y - 1 ), ImColor( 0, 0, 0, 100 ), WeaponNameUnder.c_str( ) );*/

			DrawList->AddText( extraFont, extraTextSize, ImVec2( TextPos.x + 1, TextPos.y + 1 ), ImColor( 0.f, 0.f, 0.f, Cfg->WeaponNameCol.Value.w ), to_lower( weaponName.c_str( ) ).c_str( ) );
			DrawList->AddText( extraFont, extraTextSize, TextPos, Cfg->WeaponNameCol, WeaponNameUnder.c_str( ) );
		}

		if ( Cfg->UserNames )
		{

			std::string PlayerName = Entity.NetworkInfo.UserName;

			ImVec2 TextSize = extraFont->CalcTextSizeA( extraTextSize, FLT_MAX, 0.0f, PlayerName.c_str( ) );
			ImVec2 TextPos = ImVec2( BoxCenter.x - ( TextSize.x / 2 ), FirstTextBoxTop );

			switch ( Cfg->UserNamesState )
			{

			case 0: //Top
				TextPos.y = FirstTextBoxTop;
				break;
			case 2: //Bottom
				TextPos.y = FirstTextBoxBottom;
				break;
			default:
				TextPos.y = FirstTextBoxTop;
				break;
			}

			/*DrawList->AddText( extraFont, extraTextSize, ImVec2( TextPos.x + 1, TextPos.y + 1 ), ImColor( 0, 0, 0, 100 ), PlayerName.c_str( ) );
			DrawList->AddText( extraFont, extraTextSize, ImVec2( TextPos.x - 1, TextPos.y + 1 ), ImColor( 0, 0, 0, 100 ), PlayerName.c_str( ) );
			DrawList->AddText( extraFont, extraTextSize, ImVec2( TextPos.x + 1, TextPos.y - 1 ), ImColor( 0, 0, 0, 100 ), PlayerName.c_str( ) );
			DrawList->AddText( extraFont, extraTextSize, ImVec2( TextPos.x - 1, TextPos.y - 1 ), ImColor( 0, 0, 0, 100 ), PlayerName.c_str( ) );*/

			DrawList->AddText( extraFont, extraTextSize, ImVec2( TextPos.x + 1, TextPos.y + 1 ), ImColor( 0.f, 0.f, 0.f, Cfg->UserNamesCol.Value.w ), PlayerName.c_str( ) );
			DrawList->AddText( extraFont, extraTextSize, TextPos, Cfg->UserNamesCol, PlayerName.c_str( ) );
		}

		const auto DrawHealthBarV = [ &DrawList ] ( ImVec2 pos, ImVec2 dim, ImColor col, int background ) {
			if ( background == 1 ) { //Bar
				DrawList->AddRectFilled( ImVec2( pos.x, pos.y ), ImVec2( pos.x + dim.x, pos.y - ( dim.y + 1 ) ), col );
			}
			else { //BackGround
				DrawList->AddRectFilled( ImVec2( pos.x - 1, pos.y + 1 ), ImVec2( pos.x + dim.x + 1, pos.y - ( dim.y + 2 ) ), ImColor( 0, 0, 0, 255 ) );
				DrawList->AddRectFilled( ImVec2( pos.x, pos.y - 1 ), ImVec2( pos.x + dim.x, pos.y - ( dim.y + 2 ) ), ImColor( 80, 80, 80, 125 ) );
			}
			};
		const auto DrawHealthBarH = [ &DrawList ] ( ImVec2 pos, ImVec2 dim, ImColor col, int background ) {
			if ( background == 1 ) { //Bar
				DrawList->AddRectFilled( ImVec2( pos.x, pos.y ), ImVec2( pos.x + dim.y, pos.y + dim.x ), col );
			}
			else { //BackGround
				DrawList->AddRectFilled( ImVec2( pos.x - 1, pos.y - 1 ), ImVec2( pos.x + dim.y + 1, pos.y + dim.x + 1 ), ImColor( 0, 0, 0, 255 ) );
				DrawList->AddRectFilled( ImVec2( pos.x, pos.y ), ImVec2( pos.x + dim.y, pos.y + dim.x ), ImColor( 80, 80, 80, 125 ) );
			}
			};

		if ( Cfg->HealthBar )
		{
			const float healthBase = PedType == 2 ? 100.f : 0.f;
            float MaxHealth = (std::max)(1.f, Entity.MaxHealth - healthBase);
            const float displayHealth = ImClamp(Health - healthBase, 0.f, MaxHealth);

			CurrentESPAnim.Health = ImLerp( CurrentESPAnim.Health, displayHealth, ImClamp(ImGui::GetIO().DeltaTime * 4.f, 0.f, 1.f) );
			float AnimHealth = ImClamp(CurrentESPAnim.Health, 0.f, MaxHealth);

			float FullHealthBar = Height / 100 * 100;
			float DecreaseHealthBar = FullHealthBar * ( AnimHealth / MaxHealth );

			float Width2 = BoxRight - BoxLeft;
			float FullHealthBarH = Width2 / 100 * 100;
			float DecreaseHealthBarH = FullHealthBarH * ( AnimHealth / MaxHealth );

			if ( DecreaseHealthBarH > FullHealthBarH )
			{
				DecreaseHealthBarH = FullHealthBarH;
			}

			if ( DecreaseHealthBar > FullHealthBar )
			{
				DecreaseHealthBar = FullHealthBar;
			}

            const float healthRatio = displayHealth / MaxHealth;
            ImColor BarColor = healthRatio > 0.5f ? ImColor(81,252,141,255) :
                (healthRatio > 0.25f ? ImColor(245,135,66,255) : ImColor(242,0,0,255));
            ImColor FullHealth(80,80,80,200);

			switch ( Cfg->HealthBarState )
			{
			case 0: //Top
				DrawHealthBarH( ImVec2( BoxMin.x, BoxMin.y - 4 ), ImVec2( 2, FullHealthBarH ),( ImVec4( FullHealth ) ), 0 );
				DrawHealthBarH( ImVec2( BoxMin.x, BoxMin.y - 4 ), ImVec2( 2, DecreaseHealthBarH ),( ImVec4( BarColor ) ), 1 );
				break;
			case 1: //Right
				DrawHealthBarV( ImVec2( BoxMax.x + 6, BoxMax.y ), ImVec2( 2, FullHealthBar ),( ImVec4( FullHealth ) ), 0 );
				DrawHealthBarV( ImVec2( BoxMax.x + 6, BoxMax.y ), ImVec2( 2, DecreaseHealthBar ),( ImVec4( BarColor ) ), 1 );
				break;
			case 2: //Bottom
				DrawHealthBarH( ImVec2( BoxMin.x, BoxMax.y + 6 ), ImVec2( 2, FullHealthBarH ),( ImVec4( FullHealth ) ), 0 );
				DrawHealthBarH( ImVec2( BoxMin.x, BoxMax.y + 6 ), ImVec2( 2, DecreaseHealthBarH ),( ImVec4( BarColor ) ), 1 );
				break;
			case 3: //Left
				DrawHealthBarV( ImVec2( BoxMin.x - 6, BoxMax.y ), ImVec2( 2, FullHealthBar ),( ImVec4( FullHealth ) ), 0 );
				DrawHealthBarV( ImVec2( BoxMin.x - 6, BoxMax.y ), ImVec2( 2, DecreaseHealthBar ),( ImVec4( BarColor ) ), 1 );
				break;

			default:
				DrawHealthBarV( ImVec2( BoxMin.x - 6, BoxMax.y ), ImVec2( 2, FullHealthBar ),( ImVec4( FullHealth ) ), 0 );
				DrawHealthBarV( ImVec2( BoxMin.x - 6, BoxMax.y ), ImVec2( 2, DecreaseHealthBar ),( ImVec4( BarColor ) ), 1 );
				break;
			}
		}

		if ( Cfg->ArmorBar )
		{
			float MaxHealth = 100.f;
			float Armor = Entity.Armor;

			if ( Armor > 0 )
			{
				MaxHealth = 100.f;

				CurrentESPAnim.Armor = ImLerp( CurrentESPAnim.Armor, Armor, ImClamp(ImGui::GetIO().DeltaTime * 4.f, 0.f, 1.f) );
				float AnimArmor = ImClamp(CurrentESPAnim.Armor, 0.f, MaxHealth);

				float FullArmorBar = Height / 100 * 100;
				float DecreaseHealthBar = FullArmorBar * ( AnimArmor / MaxHealth );

				float Width2 = BoxRight - BoxLeft;
				float FullArmorBarH = Width2 / 100 * 100;
				float DecreaseArmorBarH = FullArmorBarH * ( AnimArmor / MaxHealth );


				if ( DecreaseHealthBar > FullArmorBar )
				{
					DecreaseHealthBar = FullArmorBar;
				}

				if ( DecreaseArmorBarH > FullArmorBarH )
				{
					DecreaseArmorBarH = FullArmorBarH;
				}

				ImColor BarColor;
				ImColor FullArmor = ImVec4( ImColor( 80, 80, 80, 200 ) );

				if ( Armor > ( MaxHealth / 2 ) ) {
					BarColor = ImVec4( ImColor( 85, 128, 200, 255 ) );
				}
				else if ( Armor <= ( MaxHealth / 2 ) && MaxHealth == 200 ? Armor > 50 : Armor > 150 ) {
					BarColor = ImVec4( ImColor( 69, 102, 157, 255 ) );
				}
				else {
					BarColor = ImVec4( ImColor( 45, 62, 92, 255 ) );
				}

				switch ( Cfg->ArmorBarState )
				{

				case 0: //Top
					if ( Cfg->HealthBar && Cfg->HealthBarState == Cfg->ArmorBarState )
					{
						DrawHealthBarH( ImVec2( BoxMin.x, BoxMin.y - 8 ), ImVec2( 2, FullArmorBarH ), FullArmor, 0 );
						DrawHealthBarH( ImVec2( BoxMin.x, BoxMin.y - 8 ), ImVec2( 2, DecreaseArmorBarH ), BarColor, 1 );
					}
					else
					{
						DrawHealthBarH( ImVec2( BoxMin.x, BoxMin.y - 4 ), ImVec2( 2, FullArmorBarH ), FullArmor, 0 );
						DrawHealthBarH( ImVec2( BoxMin.x, BoxMin.y - 4 ), ImVec2( 2, DecreaseArmorBarH ), BarColor, 1 );
					}
					break;
				case 1: //Right
					if ( Cfg->HealthBar && Cfg->HealthBarState == Cfg->ArmorBarState )
					{
						DrawHealthBarV( ImVec2( BoxMax.x + 12, BoxMax.y ), ImVec2( 2, FullArmorBar ), FullArmor, 0 );
						DrawHealthBarV( ImVec2( BoxMax.x + 12, BoxMax.y ), ImVec2( 2, DecreaseHealthBar ), BarColor, 1 );
					}
					else
					{
						DrawHealthBarV( ImVec2( BoxMax.x + 6, BoxMax.y ), ImVec2( 2, FullArmorBar ), FullArmor, 0 );
						DrawHealthBarV( ImVec2( BoxMax.x + 6, BoxMax.y ), ImVec2( 2, DecreaseHealthBar ), BarColor, 1 );
					}
					break;
				case 2: //Bottom
					if ( Cfg->HealthBar && Cfg->HealthBarState == Cfg->ArmorBarState )
					{
						DrawHealthBarH( ImVec2( BoxMin.x, BoxMax.y + 12 ), ImVec2( 2, FullArmorBarH ), FullArmor, 0 );
						DrawHealthBarH( ImVec2( BoxMin.x, BoxMax.y + 12 ), ImVec2( 2, DecreaseArmorBarH ), BarColor, 1 );
					}
					else
					{
						DrawHealthBarH( ImVec2( BoxMin.x, BoxMax.y + 6 ), ImVec2( 2, FullArmorBarH ), FullArmor, 0 );
						DrawHealthBarH( ImVec2( BoxMin.x, BoxMax.y + 6 ), ImVec2( 2, DecreaseArmorBarH ), BarColor, 1 );
					}
					break;
				case 3: //Left
					if ( Cfg->HealthBar && Cfg->HealthBarState == Cfg->ArmorBarState )
					{
						DrawHealthBarV( ImVec2( BoxMin.x - 12, BoxMax.y ), ImVec2( 2, FullArmorBar ), FullArmor, 0 );
						DrawHealthBarV( ImVec2( BoxMin.x - 12, BoxMax.y ), ImVec2( 2, DecreaseHealthBar ), BarColor, 1 );
					}
					else
					{
						DrawHealthBarV( ImVec2( BoxMin.x - 6, BoxMax.y ), ImVec2( 2, FullArmorBar ), FullArmor, 0 );
						DrawHealthBarV( ImVec2( BoxMin.x - 6, BoxMax.y ), ImVec2( 2, DecreaseHealthBar ), BarColor, 1 );
					}
					break;

				default:
					DrawHealthBarV( ImVec2( BoxMin.x - 6, BoxMax.y ), ImVec2( 2, FullArmorBar ), FullArmor, 0 );
					DrawHealthBarV( ImVec2( BoxMin.x - 6, BoxMax.y ), ImVec2( 2, DecreaseHealthBar ), BarColor, 1 );
					break;
				}

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

		if(ExtraSettings::VehicleMarker)DrawList->AddCircleFilled(ImVec2(VehicleLocation.x,VehicleLocation.y),4,IM_COL32(180,80,255,255));

        if ( Cfg->VehName )
		{
			ImVec2 text_size = g_Variables.m_DrawFont->CalcTextSizeA( g_Variables.m_DrawFont->FontSize, FLT_MAX, 0.0f, Entity.Name.c_str( ) );

			DrawList->AddText( ImVec2( VehicleLocation.x - ( text_size.x / 2 ), VehicleLocation.y + 2 ), ImColor( 0, 0, 0 ), Entity.Name.c_str( ) );
			DrawList->AddText( ImVec2( VehicleLocation.x - ( text_size.x / 2 ), VehicleLocation.y ), ImColor( 255, 255, 255 ), Entity.Name.c_str( ) );
		}

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

		if ( Cfg->DistanceFromMe )
		{
			std::string vehicleDistance = std::to_string( int( Distance ) ) + xorstr( "m" );
			ImVec2 text_size = g_Variables.m_DrawFont->CalcTextSizeA( g_Variables.m_DrawFont->FontSize, FLT_MAX, 0.0f, vehicleDistance.c_str( ) );

			DrawList->AddText( ImVec2( VehicleLocation.x - ( text_size.x / 2 ), VehicleLocation.y + 16 ), ImColor( 0, 0, 0 ), vehicleDistance.c_str( ) );
			DrawList->AddText( ImVec2( VehicleLocation.x - ( text_size.x / 2 ), VehicleLocation.y + 14 ), ImColor( 255, 255, 255 ), vehicleDistance.c_str( ) );
		}

	}
}
