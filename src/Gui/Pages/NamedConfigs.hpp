#pragma once
#include <filesystem>
#include <shellapi.h>
#include <fstream>
#include <iterator>
#include <cctype>

namespace NamedConfigs {
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
        ImGui::TextUnformatted("Named configs");
        ImGui::InputText("Name", name, sizeof(name));
        const auto directory = Directory();
        if (ImGui::Button("Save named")) {
            if (!ValidName(name) || directory.empty()) status = "Use letters, digits, - or _.";
            else {
                const auto encoded = Core::g_Config.EncodeCurrentConfig(name);
                if (encoded.empty()) status = "Encoding failed.";
                else {
                    const auto path = directory / (std::string(name) + ".wcfg");
                    const auto temporary = directory / (std::string(name) + ".tmp");
                    std::ofstream file(temporary, std::ios::binary | std::ios::trunc);
                    file << encoded; file.close();
                    if (file.fail()) status = "Save failed.";
                    else if (!MoveFileExA(temporary.string().c_str(), path.string().c_str(), MOVEFILE_REPLACE_EXISTING | MOVEFILE_WRITE_THROUGH))
                        status = "Could not finish save.";
                    else status = "Saved.";
                }
            }
        }
        ImGui::SameLine();
        if (ImGui::Button("Load named")) {
            if (!ValidName(name) || directory.empty()) status = "Invalid name or folder.";
            else {
                std::ifstream file(directory / (std::string(name) + ".wcfg"), std::ios::binary);
                if (!file) status = "Config not found.";
                else {
                    const std::string encoded((std::istreambuf_iterator<char>(file)), std::istreambuf_iterator<char>());
                    status = Core::g_Config.LoadCfg(name, encoded);
                }
            }
        }
        if (ImGui::Button("Delete named")) {
            if (!ValidName(name) || directory.empty()) status = "Invalid name or folder.";
            else {
                std::error_code error;
                const bool removed = std::filesystem::remove(directory / (std::string(name) + ".wcfg"), error);
                status = !error && removed ? "Deleted." : "Could not delete config.";
            }
        }
        ImGui::SameLine();
        if (ImGui::Button("Open folder") && !directory.empty())
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
