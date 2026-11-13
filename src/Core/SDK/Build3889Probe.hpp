#pragma once
#include <Windows.h>
#include <TlHelp32.h>
#include <vector>
#include <string>
#include <fstream>
#include <sstream>
#include <filesystem>
#include <algorithm>
#include <cmath>
#include <set>
#include <chrono>
#include "ProbePattern.hpp"
#include "NativeCandidateLogic.hpp"
#include <array>

namespace Build3889Probe {
struct Handle {
    HANDLE value=nullptr;
    ~Handle(){if(value && value!=INVALID_HANDLE_VALUE)CloseHandle(value);}
};
template<class T> inline bool Read(HANDLE process,uintptr_t at,T& value) {
    SIZE_T bytes=0;
    return at && ReadProcessMemory(process,reinterpret_cast<LPCVOID>(at),&value,sizeof(value),&bytes) && bytes==sizeof(value);
}
inline std::string Hex(std::uint64_t value) {std::ostringstream s;s<<"0x"<<std::hex<<value;return s.str();}
inline const char* Bool(bool value){return value?"true":"false";}
struct Vec {float x=0,y=0,z=0;};
inline bool Sane(const Vec& v) {
    return std::isfinite(v.x)&&std::isfinite(v.y)&&std::isfinite(v.z)&&
        std::fabs(v.x)<100000.f&&std::fabs(v.y)<100000.f&&std::fabs(v.z)<100000.f;
}
inline bool Matrix(HANDLE process,uintptr_t at) {
    float values[16]={};if(!Read(process,at,values))return false;
    int nonzero=0;for(float v:values){if(!std::isfinite(v)||std::fabs(v)>1e8f)return false;if(std::fabs(v)>1e-6f)++nonzero;}
    return nonzero>=4;
}
struct Hit {std::uint64_t rva=0,target=0;bool resolved=false;};
struct Spec {const char* name;std::vector<int> bytes;std::vector<Hit> hits;std::set<std::uint64_t> seen;};
inline std::string Inspect(HANDLE process,uintptr_t base,std::uint64_t imageSize,const Spec& spec,const Hit& hit) {
    if(!hit.resolved)return "{}";
    const uintptr_t global=base+hit.target;
    uintptr_t pointer=0;const bool pointerRead=Read(process,global,pointer);
    std::ostringstream s;s<<"{\"pointer_read\":"<<Bool(pointerRead)<<",\"pointer_nonzero\":"<<Bool(pointer!=0);
    const std::string name=spec.name;
    if(name=="source_clock_direct"||name=="source_clock_pointer") {
        const uintptr_t clock=name=="source_clock_direct"?global:(pointerRead?pointer:0);
        int hour=-1,minute=-1,second=-1;
        const bool clockRead=clock>=0x10000&&Read(process,clock+0x10,hour)&&
            Read(process,clock+0x14,minute)&&Read(process,clock+0x18,second);
        const bool sane=clockRead&&hour>=0&&hour<24&&minute>=0&&minute<60&&second>=0&&second<60;
        s<<",\"clock_values_read\":"<<Bool(clockRead)<<",\"clock_values_in_range\":"<<Bool(sane);
        if(sane)s<<",\"hour\":"<<hour<<",\"minute\":"<<minute<<",\"second\":"<<second;
    }
    if(name=="source_native_registration") {
        uintptr_t first=0;
        s<<",\"first_bucket_read\":"<<Bool(Read(process,global,first))
         <<",\"first_bucket_nonzero\":"<<Bool(first!=0);
        std::array<std::uint8_t,12> caller{};std::uint64_t resolver=0;
        const bool callOk=Read(process,base+hit.rva,caller)&&
            NativeCandidateLogic::Resolver(caller.data(),caller.size(),hit.rva,imageSize,resolver);
        s<<",\"resolver_resolved\":"<<Bool(callOk);
        if(callOk) {
            s<<",\"resolver_rva\":\""<<Hex(resolver)<<"\"";
            auto hexBytes=[](const auto& bytes) {
                const char* digits="0123456789abcdef";std::string out;out.reserve(bytes.size()*2);
                for(auto value:bytes){out.push_back(digits[value>>4]);out.push_back(digits[value&15]);}return out;
            };
            std::array<std::uint8_t,512> code{};
            const bool codeOk=resolver<imageSize&&imageSize-resolver>=code.size()&&Read(process,base+resolver,code);
            s<<",\"resolver_code_read\":"<<Bool(codeOk);
            if(codeOk)s<<",\"resolver_code_hex\":\""<<hexBytes(code)<<"\"";
            std::array<std::uint8_t,128> node{};
            const bool nodeOk=NativeCandidateLogic::Pointer(first)&&Read(process,first,node);
            s<<",\"first_bucket_node_read\":"<<Bool(nodeOk);
            if(nodeOk)s<<",\"first_bucket_node_hex\":\""<<hexBytes(node)<<"\"";
        }
    }
    if(name=="world"||name=="world_short") {
        uintptr_t ped=0,nav=0,info=0,weapon=0;
        Vec pos{},navPos{};float health=0,maxHealth=0,armor=0;std::uint32_t type=0;
        const bool pedOk=pointerRead&&pointer&&Read(process,pointer+8,ped)&&ped;
        const bool posOk=pedOk&&Read(process,ped+0x90,pos)&&Sane(pos)&&!(pos.x==0&&pos.y==0&&pos.z==0);
        const bool navOk=pedOk&&Read(process,ped+0x30,nav)&&nav&&Read(process,nav+0x50,navPos)&&Sane(navPos);
        const bool hpOk=pedOk&&Read(process,ped+0x280,health)&&std::isfinite(health)&&health>=0&&health<=10000;
        const bool maxOk=pedOk&&Read(process,ped+0x284,maxHealth)&&std::isfinite(maxHealth)&&maxHealth>0&&maxHealth<=10000;
        const bool armorOk=pedOk&&Read(process,ped+0x150C,armor)&&std::isfinite(armor)&&armor>=0&&armor<=1000;
        const bool typeOk=pedOk&&Read(process,ped+0x1098,type);
        const bool infoOk=pedOk&&Read(process,ped+0x10A8,info)&&info;
        const bool weaponOk=pedOk&&Read(process,ped+0x10B8,weapon)&&weapon;
        s<<",\"local_ped\":"<<Bool(pedOk)<<",\"position_sane\":"<<Bool(posOk)<<",\"navigation_sane\":"<<Bool(navOk)
         <<",\"matrix_60\":"<<Bool(pedOk&&Matrix(process,ped+0x60))
         <<",\"layout_hypotheses\":{\"health_280_sane\":"<<Bool(hpOk)<<",\"max_health_284_sane\":"<<Bool(maxOk)
         <<",\"armor_150c_sane\":"<<Bool(armorOk)<<",\"ped_type_1098\":"<<(typeOk?int((type<<11)>>25):-1)
         <<",\"player_info_10a8_nonzero\":"<<Bool(infoOk)<<",\"weapon_manager_10b8_nonzero\":"<<Bool(weaponOk)<<"}";
        // Values are hypotheses to verify, not a declaration that this layout is supported.
    } else if(name=="replay"||name=="replay_alt") {
        auto layout=[&](uintptr_t object){
            uintptr_t iface=0,list=0;int capacity=0;
            const bool ok=object&&Read(process,object+0x18,iface)&&iface&&Read(process,iface+0x100,list)&&list&&
                Read(process,iface+0x108,capacity)&&capacity>0&&capacity<=4096;
            return ok?capacity:0;
        };
        s<<",\"pointer_layout_capacity\":"<<layout(pointer)<<",\"direct_layout_capacity\":"<<layout(global);
    } else if(name=="viewport") {
        s<<",\"matrix_24c\":"<<Bool(pointer&&Matrix(process,pointer+0x24C))
         <<",\"matrix_250\":"<<Bool(pointer&&Matrix(process,pointer+0x250));
    } else if(name=="camera") {
        uintptr_t follow=0;Vec origin{},direction{};
        const bool followOk=pointer&&Read(process,pointer+0x2C0,follow)&&follow;
        const bool originOk=followOk&&Read(process,follow+0x60,origin)&&Sane(origin);
        const bool directionOk=followOk&&Read(process,follow+0x40,direction)&&Sane(direction);
        s<<",\"follow_2c0\":"<<Bool(followOk)<<",\"origin_60_sane\":"<<Bool(originOk)
         <<",\"direction_40_sane\":"<<Bool(directionOk);
    }
    s<<"}";return s.str();
}
inline void Run(DWORD pid, int build=3889) {
    // This path returns before game-offset initialization or any feature thread starts.
    const std::wstring title=L"Coleta de compatibilidade "+std::to_wstring(build);
    const std::wstring intro=L"Esta versao vai coletar dados de compatibilidade da build "+std::to_wstring(build)+
        L".\nEla nao ativa o menu e nao escreve na memoria do jogo.\nMantenha o personagem carregado na cidade e aguarde a coleta.";
    MessageBoxW(nullptr,intro.c_str(),title.c_str(),MB_OK|MB_ICONINFORMATION);
    const auto started=std::chrono::steady_clock::now();
    Handle process;process.value=OpenProcess(PROCESS_VM_READ|PROCESS_QUERY_LIMITED_INFORMATION,FALSE,pid);
    DWORD error=process.value?0:GetLastError();
    MODULEENTRY32W module{};module.dwSize=sizeof(module);
    Handle snapshot;snapshot.value=CreateToolhelp32Snapshot(TH32CS_SNAPMODULE|TH32CS_SNAPMODULE32,pid);
    bool moduleOk=snapshot.value!=INVALID_HANDLE_VALUE&&Module32FirstW(snapshot.value,&module);
    if(!moduleOk && !error)error=GetLastError();
    const uintptr_t base=moduleOk?reinterpret_cast<uintptr_t>(module.modBaseAddr):0;
    IMAGE_DOS_HEADER dos{};IMAGE_NT_HEADERS64 nt{};
    bool peOk=process.value&&base&&Read(process.value,base,dos)&&dos.e_magic==IMAGE_DOS_SIGNATURE&&
        dos.e_lfanew>0&&dos.e_lfanew<0x100000&&Read(process.value,base+dos.e_lfanew,nt)&&
        nt.Signature==IMAGE_NT_SIGNATURE&&nt.FileHeader.Machine==IMAGE_FILE_MACHINE_AMD64&&
        nt.FileHeader.NumberOfSections>0&&nt.FileHeader.NumberOfSections<=96&&nt.OptionalHeader.SizeOfImage>0 && nt.OptionalHeader.SizeOfImage<0x20000000;
    if(!peOk&&!error){error=GetLastError();if(!error)error=ERROR_BAD_EXE_FORMAT;}
    std::vector<Spec> specs={
        {"source_native_registration",{0x48,0x8D,0x0D,-1,-1,-1,-1,0xE8,-1,-1,-1,-1,0x48,0x85,0xC0,0x75}},
        {"source_clock_direct",{0x48,0x8D,0x0D,-1,-1,-1,-1,0xBA,-1,-1,-1,-1,0xE8,-1,-1,-1,-1,0x44,0x8B,0x05}},
        {"source_clock_pointer",{0x48,0x8B,0x05,-1,-1,-1,-1,0x0F,0xB6,0x48,0x10}},
        {"world",{0x48,0x8B,0x05,-1,-1,-1,-1,0x33,0xD2,0x48,0x8B,0x40,0x08,0x8A,0xCA,0x48,0x85,0xC0,0x74,0x16,0x48,0x8B}},
        {"world_short",{0x48,0x8B,0x05,-1,-1,-1,-1,0x48,0x8B,0x48,0x08,0x48,0x85,0xC9,0x74}},
        {"replay",{0x48,0x8D,0x0D,-1,-1,-1,-1,0x48,0x8B,0xD7,0xE8,-1,-1,-1,-1,0x48,0x8D,0x0D,-1,-1,-1,-1,0x8A,0xD8}},
        {"replay_alt",{0x48,0x8D,0x0D,-1,-1,-1,-1,0x48,-1,-1,0xE8,-1,-1,-1,-1,0x48,0x8D,0x0D,-1,-1,-1,-1,0x8A,0xD8}},
        {"viewport",{0x48,0x8B,0x15,-1,-1,-1,-1,0x48,0x8D,0x2D,-1,-1,-1,-1,0x48,0x8B,0xCD}},
        {"camera",{0x4C,0x8B,0x35,-1,-1,-1,-1,0x33,0xFF,0x32,0xDB}},
        {"blip_list",{0x4C,0x8D,0x05,-1,-1,-1,-1,0x0F,0xB7,0xC1}}
    };
    std::uint64_t scanned=0,failed=0;
    if(peOk) {
        const uintptr_t sections=base+dos.e_lfanew+sizeof(DWORD)+sizeof(IMAGE_FILE_HEADER)+nt.FileHeader.SizeOfOptionalHeader;
        for(unsigned section=0;section<nt.FileHeader.NumberOfSections;++section) {
            IMAGE_SECTION_HEADER header{};
            if(!Read(process.value,sections+section*sizeof(header),header))continue;
            if(!(header.Characteristics&IMAGE_SCN_MEM_EXECUTE)||!header.Misc.VirtualSize)continue;
            const std::uint64_t start=header.VirtualAddress;
            if(start>=nt.OptionalHeader.SizeOfImage)continue;
            const std::uint64_t length=(std::min)(std::uint64_t(header.Misc.VirtualSize),std::uint64_t(nt.OptionalHeader.SizeOfImage)-start);
            std::vector<std::uint8_t> previous;
            for(std::uint64_t done=0;done<length;done+=4096) {
                const SIZE_T wanted=SIZE_T((std::min)(std::uint64_t(4096),length-done));
                std::vector<std::uint8_t> page(wanted);SIZE_T bytes=0;
                const BOOL ok=ReadProcessMemory(process.value,reinterpret_cast<LPCVOID>(base+start+done),page.data(),wanted,&bytes);
                if(!ok){++failed;error=GetLastError();}
                if(!bytes){previous.clear();continue;}
                scanned+=bytes;page.resize(bytes);
                const std::uint64_t rva=start+done-previous.size();
                previous.insert(previous.end(),page.begin(),page.end());
                for(auto& spec:specs) {
                    for(std::size_t offset=0;offset<previous.size();++offset) {
                        if(!ProbePattern::Match(previous.data(),previous.size(),offset,spec.bytes))continue;
                        const std::uint64_t instruction=rva+offset;
                        if(!spec.seen.insert(instruction).second)continue;
                        if(spec.hits.size()>=64)continue;
                        Hit hit;hit.rva=instruction;
                        hit.resolved=ProbePattern::Rip(previous.data(),previous.size(),offset,instruction,nt.OptionalHeader.SizeOfImage,hit.target);
                        spec.hits.push_back(hit);
                    }
                }
                if(ok && bytes==wanted) {
                    const std::size_t keep=(std::min)(std::size_t(63),previous.size());
                    previous=std::vector<std::uint8_t>(previous.end()-keep,previous.end());
                } else previous.clear();
            }
        }
    }
    std::ostringstream report;
    report<<"{\"schema\":3,\"build\":"<<build<<",\"pid\":"<<pid<<",\"read_only\":true,\"memory_writes\":0,\"features_enabled\":false,"
          <<"\"pe_valid\":"<<Bool(peOk)<<",\"image_size\":"<<(peOk?nt.OptionalHeader.SizeOfImage:0)
          <<",\"pe_timestamp\":"<<(peOk?nt.FileHeader.TimeDateStamp:0)<<",\"scanned_bytes\":"<<scanned
          <<",\"failed_pages\":"<<failed<<",\"read_error\":"<<error<<",\"patterns\":{";
    bool firstSpec=true;
    for(const auto& spec:specs) {
        if(!firstSpec)report<<",";firstSpec=false;
        report<<"\""<<spec.name<<"\":{\"match_count\":"<<spec.seen.size()<<",\"matches\":[";
        bool firstHit=true;
        for(const auto& hit:spec.hits) {
            if(!firstHit)report<<",";firstHit=false;
            report<<"{\"instruction_rva\":\""<<Hex(hit.rva)<<"\",\"resolved\":"<<Bool(hit.resolved)
                  <<",\"target_rva\":\""<<Hex(hit.target)<<"\",\"checks\":"<<Inspect(process.value,base,nt.OptionalHeader.SizeOfImage,spec,hit)<<"}";
        }
        report<<"]}";
    }
    const auto elapsed=std::chrono::duration_cast<std::chrono::milliseconds>(std::chrono::steady_clock::now()-started).count();
    report<<"},\"elapsed_ms\":"<<elapsed<<"}";
    wchar_t folder[MAX_PATH]={};
    const DWORD folderLength=GetTempPathW(MAX_PATH,folder);
    if(!folderLength||folderLength>=MAX_PATH) {
        MessageBoxW(nullptr,L"Nao foi possivel localizar a pasta temporaria.",L"Coleta de recursos",MB_ICONERROR);return;
    }
    const std::filesystem::path path=std::filesystem::path(folder)/(L"ALTOS_PODERES_"+std::to_wstring(build)+L".json");
    std::ofstream output(path,std::ios::binary|std::ios::trunc);
    output<<report.str();output.close();
    if(!output) {
        MessageBoxW(nullptr,L"Nao foi possivel salvar o relatorio da coleta.",L"Coleta de recursos",MB_ICONERROR);return;
    }
    const std::wstring message=L"Coleta concluida. Envie este arquivo:\n\n"+path.wstring()+
        L"\n\nEsta coleta nao habilitou o menu nem alterou a memoria do jogo. A compatibilidade ainda precisa ser validada.";
    MessageBoxW(nullptr,message.c_str(),L"Relatorio de recursos salvo",MB_ICONINFORMATION);
}
}
