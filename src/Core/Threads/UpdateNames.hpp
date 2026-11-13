#pragma once
#include <Includes/Includes.hpp>
#include <Core/SDK/SDK.hpp>
#include <Bridge/Client.hpp>
#include <Core/SDK/ServerDiscovery.hpp>
#include <fstream>
#include <filesystem>
#include <mutex>

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
    std::string serverIdentity;
    std::string status="Aguardando consulta dos nomes.";
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
        if (data.is_array()) return data;
        if (data.is_object() && data.contains("Data") && data["Data"].is_object() &&
            data["Data"].contains("players") && data["Data"]["players"].is_array()) return data["Data"]["players"];
        return nullptr;
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
            // Recent FXServer versions anonymize public payloads as id=0/name=Player.
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
            next[id]=std::move(info);
        }
        if(next.empty())return false;
        std::lock_guard<std::mutex> lock(namesMutex);
        networkMap=std::move(next);
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
            networkMap.clear();serverIdentity=identity;
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
        return info;
    }
    std::string Status() {std::lock_guard<std::mutex> lock(namesMutex);return status;}
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
