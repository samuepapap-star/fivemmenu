#pragma once
// Independent reference values checked against citizenfx/natives on 2026-11-07.
// This catches transcription errors that state-machine mocks cannot detect.
// https://raw.githubusercontent.com/citizenfx/natives/master/STREAMING/HasAnimDictLoaded.md
static_assert(MenuBridge::N::AnimLoaded == 0xD031A9162D01088CULL, "Native hash mismatch: AnimLoaded");
// https://raw.githubusercontent.com/citizenfx/natives/master/ENTITY/IsEntityAttached.md
static_assert(MenuBridge::N::Attached == 0xB346476EF1A64897ULL, "Native hash mismatch: Attached");
// https://raw.githubusercontent.com/citizenfx/natives/master/TASK/ClearPedTasks.md
static_assert(MenuBridge::N::ClearTasks == 0xE1EF3C1216AFF2CDULL, "Native hash mismatch: ClearTasks");
// https://raw.githubusercontent.com/citizenfx/natives/master/PED/ClonePed.md
static_assert(MenuBridge::N::ClonePed == 0xEF29A16337FACADBULL, "Native hash mismatch: ClonePed");
// https://raw.githubusercontent.com/citizenfx/natives/master/PED/ClonePedToTarget.md
static_assert(MenuBridge::N::CloneToTarget == 0xE952D6431689AD9AULL, "Native hash mismatch: CloneToTarget");
// https://raw.githubusercontent.com/citizenfx/natives/master/ENTITY/SetEntityCollision.md
static_assert(MenuBridge::N::Collision == 0x1A9205C1B9EE827FULL, "Native hash mismatch: Collision");
// https://raw.githubusercontent.com/citizenfx/natives/master/VEHICLE/CreateVehicle.md
static_assert(MenuBridge::N::CreateVehicle == 0xAF35D0D2583051B0ULL, "Native hash mismatch: CreateVehicle");
// https://raw.githubusercontent.com/citizenfx/natives/master/ENTITY/IsEntityDead.md
static_assert(MenuBridge::N::Dead == 0x5F9532F3B5CC2551ULL, "Native hash mismatch: Dead");
// https://raw.githubusercontent.com/citizenfx/natives/master/ENTITY/DeleteEntity.md
static_assert(MenuBridge::N::DeleteEntity == 0xAE3CBE5BF394C9C9ULL, "Native hash mismatch: DeleteEntity");
// https://raw.githubusercontent.com/citizenfx/natives/master/PED/SetPedIntoVehicle.md
static_assert(MenuBridge::N::EnterVehicle == 0xF75B0D629E1C063DULL, "Native hash mismatch: EnterVehicle");
// https://raw.githubusercontent.com/citizenfx/natives/master/ENTITY/DoesEntityExist.md
static_assert(MenuBridge::N::EntityExists == 0x7239B21A38F536BAULL, "Native hash mismatch: EntityExists");
// https://raw.githubusercontent.com/citizenfx/natives/master/ENTITY/GetEntityModel.md
static_assert(MenuBridge::N::EntityModel == 0x9F47B058362C84B5ULL, "Native hash mismatch: EntityModel");
// https://raw.githubusercontent.com/citizenfx/natives/master/ENTITY/FreezeEntityPosition.md
static_assert(MenuBridge::N::Freeze == 0x428CA6DBD1094446ULL, "Native hash mismatch: Freeze");
// https://raw.githubusercontent.com/citizenfx/natives/master/PED/GetPedArmour.md
static_assert(MenuBridge::N::GetArmor == 0x9483AF821605B1D8ULL, "Native hash mismatch: GetArmor");
// https://raw.githubusercontent.com/citizenfx/natives/master/ENTITY/GetEntityHealth.md
static_assert(MenuBridge::N::GetHealth == 0xEEF059FAD016D209ULL, "Native hash mismatch: GetHealth");
// https://raw.githubusercontent.com/citizenfx/natives/master/ENTITY/GetEntityHeading.md
static_assert(MenuBridge::N::Heading == 0xE83D4F9BA2A38914ULL, "Native hash mismatch: Heading");
// https://raw.githubusercontent.com/citizenfx/natives/master/PED/IsPedInAnyVehicle.md
static_assert(MenuBridge::N::InVehicle == 0x997ABD671D25CA0BULL, "Native hash mismatch: InVehicle");
// https://raw.githubusercontent.com/citizenfx/natives/master/ENTITY/SetEntityAsMissionEntity.md
static_assert(MenuBridge::N::MissionEntity == 0xAD738C3085FE7E11ULL, "Native hash mismatch: MissionEntity");
// https://raw.githubusercontent.com/citizenfx/natives/master/STREAMING/HasModelLoaded.md
static_assert(MenuBridge::N::ModelLoaded == 0x98A4EB5D89A0C952ULL, "Native hash mismatch: ModelLoaded");
// https://raw.githubusercontent.com/citizenfx/natives/master/STREAMING/IsModelAPed.md
static_assert(MenuBridge::N::ModelPed == 0x75816577FEA6DAD5ULL, "Native hash mismatch: ModelPed");
// https://raw.githubusercontent.com/citizenfx/natives/master/STREAMING/IsModelValid.md
static_assert(MenuBridge::N::ModelValid == 0xC0296A2EDF545E92ULL, "Native hash mismatch: ModelValid");
// https://raw.githubusercontent.com/citizenfx/natives/master/STREAMING/IsModelAVehicle.md
static_assert(MenuBridge::N::ModelVehicle == 0x19AAC8F07BFEC53EULL, "Native hash mismatch: ModelVehicle");
// https://raw.githubusercontent.com/citizenfx/natives/master/TASK/TaskPlayAnim.md
static_assert(MenuBridge::N::PlayAnim == 0xEA47FE3719165B94ULL, "Native hash mismatch: PlayAnim");
// https://raw.githubusercontent.com/citizenfx/natives/master/NETWORK/NetworkIsPlayerActive.md
static_assert(MenuBridge::N::PlayerActive == 0xB8DFD30D6973E135ULL, "Native hash mismatch: PlayerActive");
// https://raw.githubusercontent.com/citizenfx/natives/master/PLAYER/PlayerId.md
static_assert(MenuBridge::N::PlayerId == 0x4F8644AF03D0E0D6ULL, "Native hash mismatch: PlayerId");
// https://raw.githubusercontent.com/citizenfx/natives/master/PLAYER/PlayerPedId.md
static_assert(MenuBridge::N::PlayerPed == 0xD80958FC74E988A6ULL, "Native hash mismatch: PlayerPed");
// https://raw.githubusercontent.com/citizenfx/natives/master/PLAYER/GetPlayerPed.md
static_assert(MenuBridge::N::PlayerPedByIndex == 0x43A66C31C68491C0ULL, "Native hash mismatch: PlayerPedByIndex");
// https://raw.githubusercontent.com/citizenfx/natives/master/ENTITY/IsEntityPlayingAnim.md
static_assert(MenuBridge::N::PlayingAnim == 0x1F0B79228E461EC9ULL, "Native hash mismatch: PlayingAnim");
// https://raw.githubusercontent.com/citizenfx/natives/master/STREAMING/RemoveAnimDict.md
static_assert(MenuBridge::N::ReleaseAnim == 0xF66A602F829E2A06ULL, "Native hash mismatch: ReleaseAnim");
// https://raw.githubusercontent.com/citizenfx/natives/master/STREAMING/SetModelAsNoLongerNeeded.md
static_assert(MenuBridge::N::ReleaseModel == 0xE532F5D78798DAABULL, "Native hash mismatch: ReleaseModel");
// https://raw.githubusercontent.com/citizenfx/natives/master/WEAPON/RemoveWeaponAsset.md
static_assert(MenuBridge::N::ReleaseWeapon == 0xAA08EF13F341C8FCULL, "Native hash mismatch: ReleaseWeapon");
// https://raw.githubusercontent.com/citizenfx/natives/master/STREAMING/RequestAnimDict.md
static_assert(MenuBridge::N::RequestAnim == 0xD3BD40951412FEF6ULL, "Native hash mismatch: RequestAnim");
// https://raw.githubusercontent.com/citizenfx/natives/master/STREAMING/RequestModel.md
static_assert(MenuBridge::N::RequestModel == 0x963D27A58DF860ACULL, "Native hash mismatch: RequestModel");
// https://raw.githubusercontent.com/citizenfx/natives/master/WEAPON/RequestWeaponAsset.md
static_assert(MenuBridge::N::RequestWeapon == 0x5443438F033E29C3ULL, "Native hash mismatch: RequestWeapon");
// https://raw.githubusercontent.com/citizenfx/natives/master/PED/SetPedArmour.md
static_assert(MenuBridge::N::SetArmor == 0xCEA04D83135264CCULL, "Native hash mismatch: SetArmor");
// https://raw.githubusercontent.com/citizenfx/natives/master/ENTITY/SetEntityHealth.md
static_assert(MenuBridge::N::SetHealth == 0x6B76DC1F3AE6E6A3ULL, "Native hash mismatch: SetHealth");
// https://raw.githubusercontent.com/citizenfx/natives/master/PLAYER/SetPlayerModel.md
static_assert(MenuBridge::N::SetPlayerModel == 0x00A1CADD00108836ULL, "Native hash mismatch: SetPlayerModel");
// https://raw.githubusercontent.com/citizenfx/natives/master/MISC/ShootSingleBulletBetweenCoords.md
static_assert(MenuBridge::N::ShootBullet == 0x867654CBC7606F2CULL, "Native hash mismatch: ShootBullet");
// https://raw.githubusercontent.com/citizenfx/natives/master/ENTITY/SetEntityVisible.md
static_assert(MenuBridge::N::Visible == 0xEA1C610A04DB6BBBULL, "Native hash mismatch: Visible");
// https://raw.githubusercontent.com/citizenfx/natives/master/WEAPON/HasWeaponAssetLoaded.md
static_assert(MenuBridge::N::WeaponLoaded == 0x36E353271F0E90EEULL, "Native hash mismatch: WeaponLoaded");

