#pragma once
#include "imgui.h"

inline void SetupKemTheme() {
    ImGuiStyle& style = ImGui::GetStyle();
    ImVec4* colors = style.Colors;

    // Fondo general
    colors[ImGuiCol_WindowBg]         = ImVec4(0.043f, 0.059f, 0.055f, 1.00f); // #0B0F0E
    colors[ImGuiCol_ChildBg]          = ImVec4(0.071f, 0.094f, 0.086f, 1.00f); // #121816
    colors[ImGuiCol_PopupBg]          = ImVec4(0.071f, 0.094f, 0.086f, 0.94f);
    colors[ImGuiCol_Border]           = ImVec4(0.165f, 0.227f, 0.180f, 1.00f); // #2A3A2E
    colors[ImGuiCol_FrameBg]          = ImVec4(0.071f, 0.094f, 0.086f, 1.00f);
    colors[ImGuiCol_FrameBgHovered]   = ImVec4(0.090f, 0.122f, 0.106f, 1.00f); // #171F1B
    colors[ImGuiCol_FrameBgActive]    = ImVec4(0.090f, 0.122f, 0.106f, 1.00f);

    // Botones
    colors[ImGuiCol_Button]           = ImVec4(0.165f, 0.227f, 0.180f, 1.00f);
    colors[ImGuiCol_ButtonHovered]    = ImVec4(0.220f, 0.290f, 0.235f, 1.00f);
    colors[ImGuiCol_ButtonActive]     = ImVec4(0.098f, 0.154f, 0.125f, 1.00f);

    // Acento verde #98CA3F
    colors[ImGuiCol_CheckMark]        = ImVec4(0.596f, 0.792f, 0.247f, 1.00f);
    colors[ImGuiCol_SliderGrab]       = ImVec4(0.596f, 0.792f, 0.247f, 1.00f);
    colors[ImGuiCol_SliderGrabActive] = ImVec4(0.663f, 0.878f, 0.298f, 1.00f); // #A9E04C
    colors[ImGuiCol_Tab]              = ImVec4(0.071f, 0.094f, 0.086f, 1.00f);
    colors[ImGuiCol_TabHovered]       = ImVec4(0.596f, 0.792f, 0.247f, 0.80f);
    colors[ImGuiCol_TabActive]        = ImVec4(0.596f, 0.792f, 0.247f, 1.00f);
    colors[ImGuiCol_TitleBgActive]    = ImVec4(0.043f, 0.059f, 0.055f, 1.00f);

    // Texto
    colors[ImGuiCol_Text]             = ImVec4(0.937f, 0.953f, 0.949f, 1.00f); // #EFF3F2
    colors[ImGuiCol_TextDisabled]     = ImVec4(0.561f, 0.620f, 0.608f, 1.00f); // #8F9E9B

    // Scrollbar
    colors[ImGuiCol_ScrollbarBg]      = ImVec4(0.043f, 0.059f, 0.055f, 0.53f);
    colors[ImGuiCol_ScrollbarGrab]    = ImVec4(0.165f, 0.227f, 0.180f, 1.00f);
    colors[ImGuiCol_ScrollbarGrabHovered] = ImVec4(0.220f, 0.290f, 0.235f, 1.00f);
    colors[ImGuiCol_ScrollbarGrabActive]  = ImVec4(0.596f, 0.792f, 0.247f, 0.80f);

    // Header
    colors[ImGuiCol_Header]           = ImVec4(0.165f, 0.227f, 0.180f, 0.55f);
    colors[ImGuiCol_HeaderHovered]    = ImVec4(0.596f, 0.792f, 0.247f, 0.45f);
    colors[ImGuiCol_HeaderActive]     = ImVec4(0.596f, 0.792f, 0.247f, 0.55f);

    colors[ImGuiCol_DockingPreview]   = ImVec4(0.596f, 0.792f, 0.247f, 0.5f);
    colors[ImGuiCol_DockingEmptyBg]   = ImVec4(0.043f, 0.059f, 0.055f, 1.0f);
    colors[ImGuiCol_Tab]              = ImVec4(0.071f, 0.094f, 0.086f, 1.0f);
    colors[ImGuiCol_TabHovered]       = ImVec4(0.596f, 0.792f, 0.247f, 0.80f);
    colors[ImGuiCol_TabActive]        = ImVec4(0.596f, 0.792f, 0.247f, 1.0f);

    // Redondeo
    style.WindowRounding    = 12.0f;
    style.FrameRounding     = 8.0f;
    style.ChildRounding     = 10.0f;
    style.GrabRounding      = 8.0f;
    style.TabRounding       = 8.0f;
    style.ScrollbarRounding = 8.0f;
    style.WindowBorderSize  = 1.0f;
    style.FrameBorderSize   = 1.0f;
    style.TabBorderSize     = 1.0f;
}