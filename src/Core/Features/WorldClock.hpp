#pragma once
#include <Core/Features/ClockLogic.hpp>
#include <Core/Features/ExtraFeatures.hpp>
#include <array>
#include <chrono>

namespace Core { namespace Features { namespace WorldClock {
inline bool requested=false;
inline const char* status="Horario local ainda nao solicitado";
inline void Request() { requested=true; }
inline uintptr_t Locate() {
    if(g_Offsets.CurrentBuild!=3258||!Mem.ModBase||!Mem.ModBaseSize)return 0;
    IMAGE_DOS_HEADER dos{};IMAGE_NT_HEADERS64 nt{};
    if(!Extras::Read(Mem.ModBase,dos)||dos.e_magic!=IMAGE_DOS_SIGNATURE||
       dos.e_lfanew<=0||dos.e_lfanew>0x100000||
       !Extras::Read(Mem.ModBase+dos.e_lfanew,nt)||nt.Signature!=IMAGE_NT_SIGNATURE||
       nt.FileHeader.Machine!=IMAGE_FILE_MACHINE_AMD64||
       !ClockLogic::Profile(g_Offsets.CurrentBuild,nt.FileHeader.TimeDateStamp,nt.OptionalHeader.SizeOfImage)||
       Mem.ModBaseSize<nt.OptionalHeader.SizeOfImage)return 0;
    std::array<std::uint8_t,20> instruction{};
    if(!Extras::Read(Mem.ModBase+ClockLogic::instructionRva,instruction)||
       !ClockLogic::Instruction(instruction.data(),instruction.size()))return 0;
    return Mem.ModBase+ClockLogic::clockRva+0x10;
}
inline void Tick(bool stop=false) {
    using namespace ExtraSettings;
    if(stop){requested=false;status="Escritas de horario encerradas";return;}
    if(!requested&&!FreezeClock)return;
    static auto next=std::chrono::steady_clock::time_point{};
    const auto now=std::chrono::steady_clock::now();
    if(!requested&&now<next)return;
    requested=false;next=now+std::chrono::milliseconds(200);
    auto ped=Extras::Player();float health=0;
    if(!g_MenuInfo.IsLogged||!ped||!Extras::Read((uintptr_t)ped+0x280,health)||
       !std::isfinite(health)||health<=100.f) {status="Carregue um personagem vivo para alterar o horario";return;}
    const auto address=Locate();
    if(!address){status="Rotina de horario nao validada para esta revisao do FiveM";return;}
    const ClockLogic::Time wanted{ClockHour,ClockMinute,0};
    const auto result=ClockLogic::Apply(wanted,
        [&](ClockLogic::Time& value){return Extras::Read(address,value);},
        [&](const ClockLogic::Time& value){return Extras::Write(address,value);});
    switch(result) {
    case ClockLogic::Result::Applied:status="Horario confirmado por leitura local";break;
    case ClockLogic::Result::ChangedByGame:status="Horario alterado pelo jogo durante a confirmacao";break;
    case ClockLogic::Result::Unconfirmed:status="Escrita enviada; leitura de confirmacao falhou";break;
    case ClockLogic::Result::InvalidTime:status="Hora ou minuto fora do intervalo";break;
    case ClockLogic::Result::ReadFailed:status="Relogio indisponivel; nenhuma escrita solicitada";break;
    case ClockLogic::Result::RollbackFailed:status="Falha ao recuperar escrita parcial; horario continuo desligado";FreezeClock=false;break;
    default:status="Falha ao escrever o horario local";FreezeClock=false;break;
    }
}
}}}

