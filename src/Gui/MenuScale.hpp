#pragma once
#include <Includes/ImGui/Files/imgui_internal.h>
#include <algorithm>
#include <cfloat>
#include <Gui/MenuScaleLogic.hpp>

namespace MenuScale {
inline float factor = 1.f, previous = 1.f;
inline ImVec2 physicalSize;
inline ImDrawList* background = nullptr;
inline bool backgroundClip = false;

inline void PrepareFrame() {
    auto& io = ImGui::GetIO();
    physicalSize = io.DisplaySize;
    bool leftDown=io.MouseDown[ImGuiMouseButton_Left];
    for(const auto& event:GImGui->InputEventsQueue)
        if(event.Type==ImGuiInputEventType_MouseButton&&event.MouseButton.Button==ImGuiMouseButton_Left)
            leftDown=event.MouseButton.Down;
    const bool dragging=leftDown&&GImGui->ActiveId!=0;
    const ImVec2 base=g_MenuInfo.IsLogged?ImVec2(920.f,600.f):ImVec2(500.f,350.f);
    factor=MenuScaleLogic::Resolve(g_Config.General->MenuScale,previous,physicalSize.x,physicalSize.y,base.x,base.y,dragging);
    auto convert = [](ImVec2& point, float multiplier) {
        if (point.x > -FLT_MAX * 0.5f && point.y > -FLT_MAX * 0.5f) {
            point.x *= multiplier; point.y *= multiplier;
        }
    };
    // Cached positions already use last frame's logical coordinates.
    const float ratio = MenuScaleLogic::CoordinateRatio(previous,factor);
    if(ratio!=1.f){
        if(ImGuiWindow* menu=ImGui::FindWindowByName(" "))
            ImGui::SetWindowPos(menu,ImVec2(menu->Pos.x*ratio,menu->Pos.y*ratio),ImGuiCond_Always);
    }
    convert(io.MousePos, ratio);
    convert(io.MousePosPrev, ratio);
    for (auto& point : io.MouseClickedPos) convert(point, ratio);
    convert(GImGui->MouseLastValidPos, ratio);
    // Win32 queues physical mouse positions. Convert once before NewFrame consumes them.
    for (auto& event : GImGui->InputEventsQueue) {
        if (event.Type == ImGuiInputEventType_MousePos &&
            event.MousePos.PosX > -FLT_MAX * 0.5f && event.MousePos.PosY > -FLT_MAX * 0.5f) {
            event.MousePos.PosX /= factor; event.MousePos.PosY /= factor;
        }
    }
    io.DisplaySize = ImVec2(physicalSize.x / factor, physicalSize.y / factor);
    previous = factor;
}
inline void BeginFrame() {
    background = ImGui::GetBackgroundDrawList();
    // ESP, crosshairs and FOV circles retain physical screen coordinates and size.
    background->PushClipRect(ImVec2(0,0), physicalSize, false);
    backgroundClip = true;
}
inline void EndFrame() {
    if (backgroundClip) background->PopClipRect();
    backgroundClip = false;
}
inline void PrepareDrawData(ImDrawData* data) {
    if (!data) { ImGui::GetIO().DisplaySize = physicalSize; return; }
    const ImVec2 origin = data->DisplayPos;
    for (int i = 0; i < data->CmdListsCount; ++i) {
        ImDrawList* list = data->CmdLists[i];
        if (list == background) continue;
        for (auto& vertex : list->VtxBuffer) {
            vertex.pos.x = origin.x + (vertex.pos.x - origin.x) * factor;
            vertex.pos.y = origin.y + (vertex.pos.y - origin.y) * factor;
        }
        for (auto& command : list->CmdBuffer) {
            command.ClipRect.x = origin.x + (command.ClipRect.x - origin.x) * factor;
            command.ClipRect.z = origin.x + (command.ClipRect.z - origin.x) * factor;
            command.ClipRect.y = origin.y + (command.ClipRect.y - origin.y) * factor;
            command.ClipRect.w = origin.y + (command.ClipRect.w - origin.y) * factor;
        }
    }
    data->DisplaySize = physicalSize;
    ImGui::GetIO().DisplaySize = physicalSize;
}
}
