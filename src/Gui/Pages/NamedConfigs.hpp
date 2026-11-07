#pragma once
#include <filesystem>
#include <shellapi.h>
#include <fstream>
#include <iterator>
#include <cctype>

namespace NamedConfigs {
    inline std::string PortugueseStatus(const std::string &result) {
        if(result=="Config Exported to Clipboard.") return "Configuração copiada.";
        if(result=="Failed to save config.") return "Falha ao salvar configuração.";
        if(result=="Config loaded with success.") return "Configuração carregada.";
        if(result=="Error Loading Config!") return "Erro ao carregar configuração.";
        return result;
    }

    inline std::filesystem::path Directory() {
        char root[MAX_PATH] = {};
        DWORD size = GetEnvironmentVariableA("LOCALAPPDATA", root, MAX_PATH);
        if (!size || size >= MAX_PATH) return {};
        std::filesystem::path path = std::filesystem::path(root) / "Wexize" / "Configs";
        std::error_code error;
        std::filesystem::create_directories(path, error);
        return error ? std::filesystem::path{} : path;
    }
    inline bool ValidName(const std::string& name) {
        if (name.empty() || name.size() > 64) return false;
        for (unsigned char c : name)
            if (!std::isalnum(c) && c != '-' && c != '_') return false;
        return true;
    }
    inline void Render() {
        static char name[65] = "default";
        static std::string status;
        ImGui::Separator();
        ImGui::TextUnformatted("Configurações salvas");
        ImGui::InputText("Nome", name, sizeof(name));
        const auto directory = Directory();
        if (ImGui::Button("Salvar")) {
            if (!ValidName(name) || directory.empty()) status = "Use letras, números, - ou _.";
            else {
                const auto encoded = Core::g_Config.EncodeCurrentConfig(name);
                if (encoded.empty()) status = "Falha ao codificar.";
                else {
                    const auto path = directory / (std::string(name) + ".wcfg");
                    const auto temporary = directory / (std::string(name) + ".tmp");
                    std::ofstream file(temporary, std::ios::binary | std::ios::trunc);
                    file << encoded; file.close();
                    if (file.fail()) status = "Falha ao salvar.";
                    else if (!MoveFileExA(temporary.string().c_str(), path.string().c_str(), MOVEFILE_REPLACE_EXISTING | MOVEFILE_WRITE_THROUGH))
                        status = "Não foi possível concluir.";
                    else status = "Salvo.";
                }
            }
        }
        ImGui::SameLine();
        if (ImGui::Button("Carregar")) {
            if (!ValidName(name) || directory.empty()) status = "Nome ou pasta inválidos.";
            else {
                std::ifstream file(directory / (std::string(name) + ".wcfg"), std::ios::binary);
                if (!file) status = "Configuração não encontrada.";
                else {
                    const std::string encoded((std::istreambuf_iterator<char>(file)), std::istreambuf_iterator<char>());
                    status = PortugueseStatus(Core::g_Config.LoadCfg(name, encoded));
                }
            }
        }
        if (ImGui::Button("Excluir")) {
            if (!ValidName(name) || directory.empty()) status = "Nome ou pasta inválidos.";
            else {
                std::error_code error;
                const bool removed = std::filesystem::remove(directory / (std::string(name) + ".wcfg"), error);
                status = !error && removed ? "Excluído." : "Não foi possível excluir.";
            }
        }
        ImGui::SameLine();
        if (ImGui::Button("Abrir pasta") && !directory.empty())
            ShellExecuteA(nullptr, "open", directory.string().c_str(), nullptr, nullptr, SW_SHOWNORMAL);
        if (!status.empty()) ImGui::TextWrapped("%s", status.c_str());
        ImGui::BeginChild("SavedConfigsList", ImVec2(0, 90), true);
        if (!directory.empty()) {
            std::error_code error;
            std::filesystem::directory_iterator it(directory, error), end;
            for (; !error && it != end; it.increment(error)) {
                if (it->path().extension() != ".wcfg") continue;
                const auto entry = it->path().stem().string();
                if (!ValidName(entry)) continue;
                if (ImGui::Selectable(entry.c_str(), entry == name))
                    strcpy_s(name, entry.c_str());
            }
        }
        ImGui::EndChild();
    }
}
