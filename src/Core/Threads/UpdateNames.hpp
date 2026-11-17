#pragma once
#include <Includes/Includes.hpp>
#include <Core/SDK/SDK.hpp>
#include <Bridge/Client.hpp>
#include <Core/SDK/ServerDiscovery.hpp>
#include <fstream>
#include <filesystem>
#include <mutex>
#include <set>

#define CURL_STATIC_LIB
#include <Security/Api/curl/curl.h>
#pragma comment(lib,"ws2_32.lib")
#pragma comment(lib,"Normaliz.lib")
#pragma comment(lib,"Crypt32.lib")
#pragma comment(lib,"Wldap32.lib")
#pragma comment(lib,"libcurl.lib")
using json = nlohmann::json;

namespace Core { namespace Threads {
class cUpdateNames {
    std::mutex namesMutex;
    std::unordered_map<int,SDK::Game::NetworkInfo> networkMap;
    std::set<std::string> discoveredVips;   // VIPs únicos vistos neste servidor
    std::string serverIdentity;
    std::string status="Aguardando consulta dos nomes.";
    std::string debugRawKeys;   // chaves brutas do 1º player (para debug)
    struct Response {std::string body,url;long http=0;CURLcode result=CURLE_FAILED_INIT;};
    static size_t Receive(void* data,size_t size,size_t count,void* target) {
        const size_t bytes=size*count;
        auto& text=*static_cast<std::string*>(target);
        if (bytes>4*1024*1024 || text.size()>4*1024*1024-bytes) return 0;
        text.append(static_cast<char*>(data),bytes);
        return bytes;
    }
    static Response Request(const std::string& url) {
        Response response;
        CURL* handle=curl_easy_init();
        if (!handle) return response;
        curl_easy_setopt(handle,CURLOPT_URL,url.c_str());
        curl_easy_setopt(handle,CURLOPT_WRITEFUNCTION,Receive);
        curl_easy_setopt(handle,CURLOPT_WRITEDATA,&response.body);
        curl_easy_setopt(handle,CURLOPT_USERAGENT,"AltosPoderes/1.0");
        curl_easy_setopt(handle,CURLOPT_FOLLOWLOCATION,1L);
        curl_easy_setopt(handle,CURLOPT_MAXREDIRS,3L);
        curl_easy_setopt(handle,CURLOPT_CONNECTTIMEOUT,3L);
        curl_easy_setopt(handle,CURLOPT_TIMEOUT,6L);
        curl_easy_setopt(handle,CURLOPT_NOSIGNAL,1L);
        response.result=curl_easy_perform(handle);
        curl_easy_getinfo(handle,CURLINFO_RESPONSE_CODE,&response.http);
        char* effective=nullptr;
        curl_easy_getinfo(handle,CURLINFO_EFFECTIVE_URL,&effective);
        if(effective)response.url=effective;
        curl_easy_cleanup(handle);
        return response;
    }
    void SetStatus(const std::string& text) {
        std::lock_guard<std::mutex> lock(namesMutex);
        status=text;
    }
    static nlohmann::json Players(const Response& response) {
        if (response.result!=CURLE_OK || response.http<200 || response.http>=300) return nullptr;
        auto data=nlohmann::json::parse(response.body,nullptr,false);
        if (data.is_discarded()) return nullptr;
        // array direto
        if (data.is_array()) return data;
        if (!data.is_object()) return nullptr;
        // busca wrapper de players — vários servidores BR usam nomes diferentes
        for(const auto& wrapKey:{"players","Players","jogadores","lista","list","result","results"}) {
            if(data.contains(wrapKey) && data[wrapKey].is_array()) return data[wrapKey];
        }
        // wrapper aninhado: { "Data":{"players":[]} } ou { "data":{"players":[]} }
        for(const auto& outerKey:{"Data","data","response","payload","body"}) {
            if(!data.contains(outerKey) || !data[outerKey].is_object()) continue;
            const auto& inner = data[outerKey];
            for(const auto& wrapKey:{"players","Players","jogadores","lista","list","result","results"}) {
                if(inner.contains(wrapKey) && inner[wrapKey].is_array()) return inner[wrapKey];
            }
            if(inner.is_array()) return inner;
        }
        return nullptr;
    }
    // ----- helpers de extração -----
    static int ExtractInt(const nlohmann::json& v) {
        if(v.is_number_integer()) return v.get<int>();
        if(v.is_number_float())   return static_cast<int>(v.get<double>());
        if(v.is_string()) { try{ return std::stoi(v.get<std::string>()); }catch(...){} }
        return -1;
    }
    static std::string ExtractStr(const nlohmann::json& v) {
        if(v.is_string()) return v.get<std::string>();
        if(v.is_number_integer()) return std::to_string(v.get<int>());
        if(v.is_number_float()) return std::to_string(static_cast<int>(v.get<double>()));
        return {};
    }
    // Tenta ler uma string de um objeto — aceita string direta ou objeto com sub-chave de label
    static std::string TryStringFromObj(const nlohmann::json& obj, const char* key) {
        if(!obj.contains(key)) return {};
        const auto& v = obj[key];
        if(v.is_string()) return v.get<std::string>();
        // objeto aninhado: job = { "label":"Policia", "name":"police" }
        if(v.is_object()) {
            for(const auto& sub:{"label","name","titulo","nome","valor","tag","display"}) {
                if(v.contains(sub) && v[sub].is_string()) return v[sub].get<std::string>();
            }
        }
        return {};
    }
    static int TryIntFromObj(const nlohmann::json& obj, const char* key) {
        if(!obj.contains(key)) return -1;
        return ExtractInt(obj[key]);
    }
    static bool TryBoolFromObj(const nlohmann::json& obj, const char* key) {
        if(!obj.contains(key)) return false;
        const auto& v = obj[key];
        if(v.is_boolean()) return v.get<bool>();
        if(v.is_number())  return v.get<int>() != 0;
        if(v.is_string())  { const auto s=v.get<std::string>(); return s=="1"||s=="true"||s=="sim"; }
        return false;
    }
    // Varre um objeto genérico buscando VIP/horas/job/afk — retorna true se achou algo
    static bool ScanObject(const nlohmann::json& obj, SDK::Game::NetworkInfo& info) {
        bool found = false;
        // ---- PlayTime ----
        if(info.PlayTime < 0) {
            static const char* hoursKeys[] = {
                "playTime","playtime","play_time","hours","play_hours","time_played",
                "horasJogadas","horas","horas_jogadas","horasServidor","horas_servidor",
                "tempo","tempoJogado","tempo_jogado","tempoServidor","tempo_servidor",
                "minutos","minutosJogados","minutos_jogados",
                "totalHours","total_hours","totalTime","total_time",
                "timePlayed","timeOnServer","time_on_server",
                "hoursPlayed","hours_played",
                nullptr
            };
            for(int i=0;hoursKeys[i];i++) {
                int v=TryIntFromObj(obj,hoursKeys[i]);
                if(v>=0){ info.PlayTime=v; found=true; break; }
            }
        }
        // ---- VipName ----
        if(info.VipName.empty()) {
            static const char* vipKeys[] = {
                "vip","vipStatus","vip_status","vipName","vip_name","vipNome","vip_nome",
                "vipTipo","vip_tipo","tipoVip","tipo_vip",
                "vipGrupo","vip_grupo","grupoVip","grupo_vip","grupo",
                "rank","rankNome","rank_nome","rankName","rank_name",
                "nivel","nivelVip","nivel_vip","nivelSocial","nivel_social",
                "classe","classeVip","classe_vip",
                "titulo","tituloVip","titulo_vip",
                "status","statusVip","status_vip","vipStatus",
                "tag","tagVip","tag_vip",
                "plano","planoVip","plano_vip",
                "categoria","categoriaVip",
                "group","groupName","group_name",
                "cargo","cargoVip","cargo_vip",
                "nomeVip","nome_vip",
                "social","socialLevel","social_level",
                nullptr
            };
            for(int i=0;vipKeys[i];i++) {
                auto s=TryStringFromObj(obj,vipKeys[i]);
                if(!s.empty()){ info.VipName=s; found=true; break; }
            }
        }
        // ---- JobName ----
        if(info.JobName.empty()) {
            static const char* jobKeys[] = {
                "job","jobName","job_name","jobLabel","job_label",
                "gang","gangName","gang_name","gangLabel","gang_label",
                "faction","factionName","faction_name",
                "cargo","cargoNome","cargo_nome","cargoName","cargo_name",
                "profissao","profissão","profissaoNome",
                "trabalho","trabalhoNome","trabalho_nome",
                "emprego","empregoNome","emprego_nome",
                "funcao","função","funcaoNome","funcao_nome",
                "ocupacao","ocupacaoNome",
                "trabalhoAtual","trabalho_atual",
                "currentJob","current_job",
                "work","workName","work_name",
                nullptr
            };
            for(int i=0;jobKeys[i];i++) {
                auto s=TryStringFromObj(obj,jobKeys[i]);
                if(!s.empty()){ info.JobName=s; found=true; break; }
            }
        }
        // ---- AFK ----
        if(!info.IsAfk) {
            static const char* afkKeys[] = {
                "afk","isAfk","is_afk","idle","ausente","inativo",
                "away","isAway","is_away",
                nullptr
            };
            for(int i=0;afkKeys[i];i++) {
                if(obj.contains(afkKeys[i])) {
                    info.IsAfk=TryBoolFromObj(obj,afkKeys[i]);
                    if(info.IsAfk){ found=true; break; }
                }
            }
        }
        return found;
    }
    // Varre o array "variables" do FiveM: [{"name":"vip","value":"GOLD"}, ...]
    // ou objeto direto: {"vip":"GOLD","job":"police"}
    static void ScanVariables(const nlohmann::json& player, SDK::Game::NetworkInfo& info) {
        if(!player.contains("variables")) return;
        const auto& vars = player["variables"];
        if(vars.is_object()) {
            ScanObject(vars, info);
            return;
        }
        if(vars.is_array()) {
            // array de {name:X, value:Y}
            nlohmann::json flat = nlohmann::json::object();
            for(const auto& entry:vars) {
                if(!entry.is_object()) continue;
                for(const auto& nameKey:{"name","key","k","campo"}) {
                    if(!entry.contains(nameKey) || !entry[nameKey].is_string()) continue;
                    const auto k = entry[nameKey].get<std::string>();
                    for(const auto& valKey:{"value","val","v","valor","data"}) {
                        if(!entry.contains(valKey)) continue;
                        flat[k] = entry[valKey];
                        break;
                    }
                    break;
                }
            }
            if(!flat.empty()) ScanObject(flat, info);
        }
    }
    bool Publish(const nlohmann::json& players) {
        if (!players.is_array()) return false;
        std::unordered_map<int,SDK::Game::NetworkInfo> next;
        for (const auto& player:players) {
            if (!player.is_object() || !player.contains("id") || !player.contains("name") ||
                !player["name"].is_string()) continue;
            int id=0;
            if(player["id"].is_number_integer()) id=player["id"].get<int>();
            else if(player["id"].is_string()) {
                const auto value=player["id"].get<std::string>();
                if(value.empty() || value.size()>5 || value.find_first_not_of("0123456789")!=std::string::npos)continue;
                id=std::stoi(value);
            } else continue;
            const auto name=player["name"].get<std::string>();
            if(id<=0 || id>65535 || name.empty()) continue;
            SDK::Game::NetworkInfo info{name,"",""};
            if(player.contains("identifiers") && player["identifiers"].is_array()) {
                for(const auto& identifier:player["identifiers"]) {
                    if(!identifier.is_string())continue;
                    const auto value=identifier.get<std::string>();
                    if(value.rfind("discord:",0)==0)info.DiscordId=value.substr(8);
                    if(value.rfind("steam:",0)==0)info.SteamId=value.substr(6);
                }
            }
            // 1) Scan nível raiz do objeto player
            ScanObject(player, info);
            // 2) Scan variables array/object (FiveM built-in state)
            ScanVariables(player, info);
            // 3) Scan objetos aninhados comuns: metadata, data, extra, info, player_data, etc.
            static const char* nestedKeys[] = {
                "metadata","data","extra","info","playerData","player_data",
                "userData","user_data","charData","char_data","characterData",
                "dados","dadosJogador","dados_jogador",
                "perfil","perfilJogador","perfil_jogador",
                "stats","playerStats","player_stats",
                "details","playerDetails","player_details",
                "account","accountData","account_data",
                "character","characterInfo","character_info",
                nullptr
            };
            for(int i=0;nestedKeys[i];i++) {
                if(player.contains(nestedKeys[i]) && player[nestedKeys[i]].is_object()) {
                    ScanObject(player[nestedKeys[i]], info);
                    // Também tenta variables dentro do nested
                    ScanVariables(player[nestedKeys[i]], info);
                }
            }
            next[id]=std::move(info);
        }
        // Coleta chaves brutas do 1º player para debug
        std::string rawKeys;
        for(const auto& player:players) {
            if(!player.is_object()) continue;
            for(auto it=player.begin();it!=player.end();++it) {
                rawKeys += it.key();
                // mostra subchaves de objetos aninhados
                if(it.value().is_object()) {
                    rawKeys += "{";
                    for(auto jt=it.value().begin();jt!=it.value().end();++jt)
                        rawKeys += jt.key() + ",";
                    if(!rawKeys.empty() && rawKeys.back()==',') rawKeys.pop_back();
                    rawKeys += "}";
                }
                rawKeys += " | ";
            }
            break;
        }
        if(next.empty())return false;
        std::lock_guard<std::mutex> lock(namesMutex);
        networkMap=std::move(next);
        debugRawKeys = rawKeys;
        for(const auto& [id,ni]:networkMap) {
            if(!ni.VipName.empty())
                discoveredVips.insert(ni.VipName);
        }
        status="Nomes recebidos: "+std::to_string(networkMap.size());
        return true;
    }
    void Refresh() {
        wchar_t folder[8192]{};DWORD bytes=sizeof(folder);
        if(RegGetValueW(HKEY_CURRENT_USER,L"Software\\CitizenFX\\FiveM",L"Last Run Location",
            RRF_RT_REG_SZ,nullptr,folder,&bytes)!=ERROR_SUCCESS) {
            SetStatus("Nao foi possivel localizar os dados de conexao do FiveM.");return;
        }
        auto root=std::filesystem::path(folder);
        if(root.extension()==L".exe" || root.extension()==L".EXE")root=root.parent_path();
        std::ifstream stream(root/L"data"/L"cache"/L"crashometry",std::ios::binary);
        if(!stream){SetStatus("Arquivo de conexao do FiveM indisponivel.");return;}
        const auto records=ServerDiscovery::Read(stream);
        const auto lookup=[&](const char* key){const auto it=records.find(key);return it==records.end()?std::string{}:it->second;};
        const auto url=lookup("last_server_url");
        std::string endpoint=ServerDiscovery::Endpoint(url);
        if(endpoint.empty())endpoint=ServerDiscovery::Endpoint(lookup("last_server"));
        if(!endpoint.empty())g_Variables.ServerIp=endpoint.substr(endpoint.find("://")+3);
        std::string token=ServerDiscovery::Token(url);
        const std::string identity=url+"|"+endpoint;
        if(identity!=serverIdentity) {
            std::lock_guard<std::mutex> lock(namesMutex);
            networkMap.clear();
            discoveredVips.clear();   // nova cidade — reseta lista de VIPs
            serverIdentity=identity;
        }
        if(endpoint.empty() && token.empty()){SetStatus("Conexao atual nao encontrada nos dados do FiveM.");return;}
        if(!endpoint.empty()) {
            if(Publish(Players(Request(endpoint+"/players.json"))))return;
            if(token.empty())token=ServerDiscovery::Token(Request(endpoint).url);
        }
        if(!token.empty() && Publish(Players(Request("https://servers-frontend.fivem.net/api/servers/single/"+token))))return;
        {
            std::lock_guard<std::mutex> lock(namesMutex);
            networkMap.clear();
        }
        SetStatus("Servidor nao disponibilizou os nomes. Exibindo IDs dos jogadores.");
    }
public:
    SDK::Game::NetworkInfo Lookup(int id) {
        const auto nickname=MenuBridge::Connection().Nickname(id);
        std::lock_guard<std::mutex> lock(namesMutex);
        const auto it=networkMap.find(id);
        auto info=it==networkMap.end()?SDK::Game::NetworkInfo{}:it->second;
        if(!nickname.empty())info.UserName=nickname;
        // Spoof local player data if enabled
        if (g_Config.ServerSpoof->Enabled && Core::SDK::Pointers::pLocalPlayer) {
            const int localId = Core::SDK::Pointers::pLocalPlayer->GetID();
            if (id == localId) {
                if (g_Config.ServerSpoof->VipName[0] != '\0')
                    info.VipName = g_Config.ServerSpoof->VipName;
                if (g_Config.ServerSpoof->PlayTime > 0)
                    info.PlayTime = g_Config.ServerSpoof->PlayTime;
                if (g_Config.ServerSpoof->JobName[0] != '\0')
                    info.JobName = g_Config.ServerSpoof->JobName;
                if (g_Config.ServerSpoof->RemoveNovato)
                    info.VipName = info.VipName.empty() ? g_Config.ServerSpoof->VipName : info.VipName;
            }
        }
        return info;
    }
    std::string Status() {std::lock_guard<std::mutex> lock(namesMutex);return status;}
    // Retorna todos os VIPs únicos vistos neste servidor (thread-safe)
    std::vector<std::string> GetDiscoveredVips() {
        std::lock_guard<std::mutex> lock(namesMutex);
        return std::vector<std::string>(discoveredVips.begin(), discoveredVips.end());
    }
    // Retorna chaves brutas do 1º player do último fetch (para debug)
    std::string GetDebugKeys() {
        std::lock_guard<std::mutex> lock(namesMutex);
        return debugRawKeys;
    }
    void Update() {
        while(true) {
            try{Refresh();}
            catch(const std::exception&){SetStatus("Falha na consulta dos nomes. Nova tentativa automatica.");}
            std::this_thread::sleep_for(std::chrono::seconds(10));
        }
    }
};
inline cUpdateNames g_UpdateNames;
}}
