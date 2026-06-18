#include "gui/GuiMgr.hpp"			// Clase de gestión de UI
#include <system/SystemMgr.hpp>

#if defined IMGUI || defined IMGUI_VERSION

    #include <imgui.h>

    // Se puede evitar poner "ImGui::" para simplificar
    using namespace ImGui;

	// Temas --------------------------------------------------------------------------------

    
    void GuiMgr::Style_DefaultDark() {

        style_->Alpha                       = 1.0000f;
        style_->DisabledAlpha               = 0.1000f;
        style_->WindowPadding               = ImVec2(8.0000f, 8.0000f);
        style_->WindowRounding              = 10.0000f;
        style_->WindowBorderSize            = 0.0000f;
        style_->WindowMinSize               = ImVec2(30.0000f, 30.0000f);
        style_->WindowTitleAlign            = ImVec2(0.5000f, 0.5000f);
        style_->WindowMenuButtonPosition    = ImGuiDir_Right;
        style_->ChildRounding               = 5.0000f;
        style_->ChildBorderSize             = 1.0000f;
        style_->PopupRounding               = 10.0000f;
        style_->PopupBorderSize             = 0.0000f;
        style_->FramePadding                = ImVec2(5.0000f, 3.5000f);
        style_->FrameRounding               = 5.0000f;
        style_->FrameBorderSize             = 0.0000f;
        style_->ItemSpacing                 = ImVec2(5.0000f, 4.0000f);
        style_->ItemInnerSpacing            = ImVec2(5.0000f, 5.0000f);
        style_->CellPadding                 = ImVec2(4.0000f, 2.0000f);
        style_->IndentSpacing               = 5.0000f;
        style_->ColumnsMinSpacing           = 5.0000f;
        style_->ScrollbarSize               = 15.0000f;
        style_->ScrollbarRounding           = 9.0000f;
        style_->GrabMinSize                 = 15.0000f;
        style_->GrabRounding                = 5.0000f;
        style_->TabRounding                 = 5.0000f;
        style_->TabBorderSize               = 0.0000f;
        style_->ColorButtonPosition         = ImGuiDir_Right;
        style_->ButtonTextAlign             = ImVec2(0.5000f, 0.5000f);
        style_->SelectableTextAlign         = ImVec2(0.0000f, 0.0000f);
        
        StyleColorsDark();
        titleBarDarkMode(true);
        SYS_INFO("GuiMgr", "'DefaultDark' theme applied.");
    }

    void GuiMgr::Style_DefaultLight() {

        style_->Alpha                       = 1.0000f;
        style_->DisabledAlpha               = 0.1000f;
        style_->WindowPadding               = ImVec2(8.0000f, 8.0000f);
        style_->WindowRounding              = 10.0000f;
        style_->WindowBorderSize            = 0.0000f;
        style_->WindowMinSize               = ImVec2(30.0000f, 30.0000f);
        style_->WindowTitleAlign            = ImVec2(0.5000f, 0.5000f);
        style_->WindowMenuButtonPosition    = ImGuiDir_Right;
        style_->ChildRounding               = 5.0000f;
        style_->ChildBorderSize             = 1.0000f;
        style_->PopupRounding               = 10.0000f;
        style_->PopupBorderSize             = 0.0000f;
        style_->FramePadding                = ImVec2(5.0000f, 3.5000f);
        style_->FrameRounding               = 5.0000f;
        style_->FrameBorderSize             = 0.0000f;
        style_->ItemSpacing                 = ImVec2(5.0000f, 4.0000f);
        style_->ItemInnerSpacing            = ImVec2(5.0000f, 5.0000f);
        style_->CellPadding                 = ImVec2(4.0000f, 2.0000f);
        style_->IndentSpacing               = 5.0000f;
        style_->ColumnsMinSpacing           = 5.0000f;
        style_->ScrollbarSize               = 15.0000f;
        style_->ScrollbarRounding           = 9.0000f;
        style_->GrabMinSize                 = 15.0000f;
        style_->GrabRounding                = 5.0000f;
        style_->TabRounding                 = 5.0000f;
        style_->TabBorderSize               = 0.0000f;
        style_->ColorButtonPosition         = ImGuiDir_Right;
        style_->ButtonTextAlign             = ImVec2(0.5000f, 0.5000f);
        style_->SelectableTextAlign         = ImVec2(0.0000f, 0.0000f);
        
        StyleColorsLight();
        titleBarDarkMode(false);
        SYS_INFO("GuiMgr", "'DefaultLight' theme applied.");
    }

    void GuiMgr::Style_Confy() {

        StyleColorsDark();
        titleBarDarkMode(true);

        style_->Alpha                       = 1.0000f;
        style_->DisabledAlpha               = 0.1000f;
        style_->WindowPadding               = ImVec2(8.0000f, 8.0000f);
        style_->WindowRounding              = 10.0000f;
        style_->WindowBorderSize            = 0.0000f;
        style_->WindowMinSize               = ImVec2(30.0000f, 30.0000f);
        style_->WindowTitleAlign            = ImVec2(0.5000f, 0.5000f);
        style_->WindowMenuButtonPosition    = ImGuiDir_Right;
        style_->ChildRounding               = 5.0000f;
        style_->ChildBorderSize             = 1.0000f;
        style_->PopupRounding               = 10.0000f;
        style_->PopupBorderSize             = 0.0000f;
        style_->FramePadding                = ImVec2(5.0000f, 3.5000f);
        style_->FrameRounding               = 5.0000f;
        style_->FrameBorderSize             = 0.0000f;
        style_->ItemSpacing                 = ImVec2(5.0000f, 4.0000f);
        style_->ItemInnerSpacing            = ImVec2(5.0000f, 5.0000f);
        style_->CellPadding                 = ImVec2(4.0000f, 2.0000f);
        style_->IndentSpacing               = 5.0000f;
        style_->ColumnsMinSpacing           = 5.0000f;
        style_->ScrollbarSize               = 15.0000f;
        style_->ScrollbarRounding           = 9.0000f;
        style_->GrabMinSize                 = 15.0000f;
        style_->GrabRounding                = 5.0000f;
        style_->TabRounding                 = 5.0000f;
        style_->TabBorderSize               = 0.0000f;
        style_->ColorButtonPosition         = ImGuiDir_Right;
        style_->ButtonTextAlign             = ImVec2(0.5000f, 0.5000f);
        style_->SelectableTextAlign         = ImVec2(0.0000f, 0.0000f);

        style_->Colors[ImGuiCol_Text]                     = ImVec4(1.0000f, 1.0000f, 1.0000f, 1.0000f);
        style_->Colors[ImGuiCol_TextDisabled]             = ImVec4(1.0000f, 1.0000f, 1.0000f, 0.3605f);
        style_->Colors[ImGuiCol_WindowBg]                 = ImVec4(0.0980f, 0.0980f, 0.0980f, 1.0000f);
        style_->Colors[ImGuiCol_ChildBg]                  = ImVec4(1.0000f, 0.0000f, 0.0000f, 0.0000f);
        style_->Colors[ImGuiCol_PopupBg]                  = ImVec4(0.0980f, 0.0980f, 0.0980f, 1.0000f);
        style_->Colors[ImGuiCol_Border]                   = ImVec4(0.4235f, 0.3804f, 0.5725f, 0.5494f);
        style_->Colors[ImGuiCol_BorderShadow]             = ImVec4(0.0000f, 0.0000f, 0.0000f, 0.0000f);
        style_->Colors[ImGuiCol_FrameBg]                  = ImVec4(0.1569f, 0.1569f, 0.1569f, 1.0000f);
        style_->Colors[ImGuiCol_FrameBgHovered]           = ImVec4(0.3804f, 0.4235f, 0.5725f, 0.5490f);
        style_->Colors[ImGuiCol_FrameBgActive]            = ImVec4(0.6196f, 0.5765f, 0.7686f, 0.5490f);
        style_->Colors[ImGuiCol_TitleBg]                  = ImVec4(0.0980f, 0.0980f, 0.0980f, 1.0000f);
        style_->Colors[ImGuiCol_TitleBgActive]            = ImVec4(0.0980f, 0.0980f, 0.0980f, 1.0000f);
        style_->Colors[ImGuiCol_TitleBgCollapsed]         = ImVec4(0.2588f, 0.2588f, 0.2588f, 0.0000f);
        style_->Colors[ImGuiCol_MenuBarBg]                = ImVec4(0.0000f, 0.0000f, 0.0000f, 0.0000f);
        style_->Colors[ImGuiCol_ScrollbarBg]              = ImVec4(0.1569f, 0.1569f, 0.1569f, 0.0000f);
        style_->Colors[ImGuiCol_ScrollbarGrab]            = ImVec4(0.1569f, 0.1569f, 0.1569f, 1.0000f);
        style_->Colors[ImGuiCol_ScrollbarGrabHovered]     = ImVec4(0.2353f, 0.2353f, 0.2353f, 1.0000f);
        style_->Colors[ImGuiCol_ScrollbarGrabActive]      = ImVec4(0.2941f, 0.2941f, 0.2941f, 1.0000f);
        style_->Colors[ImGuiCol_CheckMark]                = ImVec4(0.2941f, 0.2941f, 0.2941f, 1.0000f);
        style_->Colors[ImGuiCol_SliderGrab]               = ImVec4(0.6196f, 0.5765f, 0.7686f, 0.5490f);
        style_->Colors[ImGuiCol_SliderGrabActive]         = ImVec4(0.8157f, 0.7725f, 0.9647f, 0.5490f);
        style_->Colors[ImGuiCol_Button]                   = ImVec4(0.6196f, 0.5765f, 0.7686f, 0.5490f);
        style_->Colors[ImGuiCol_ButtonHovered]            = ImVec4(0.7373f, 0.6941f, 0.8863f, 0.5490f);
        style_->Colors[ImGuiCol_ButtonActive]             = ImVec4(0.8157f, 0.7725f, 0.9647f, 0.5490f);
        style_->Colors[ImGuiCol_Header]                   = ImVec4(0.6196f, 0.5765f, 0.7686f, 0.5490f);
        style_->Colors[ImGuiCol_HeaderHovered]            = ImVec4(0.7373f, 0.6941f, 0.8863f, 0.5490f);
        style_->Colors[ImGuiCol_HeaderActive]             = ImVec4(0.8157f, 0.7725f, 0.9647f, 0.5490f);
        style_->Colors[ImGuiCol_Separator]                = ImVec4(0.6196f, 0.5765f, 0.7686f, 0.5490f);
        style_->Colors[ImGuiCol_SeparatorHovered]         = ImVec4(0.7373f, 0.6941f, 0.8863f, 0.5490f);
        style_->Colors[ImGuiCol_SeparatorActive]          = ImVec4(0.8157f, 0.7725f, 0.9647f, 0.5490f);
        style_->Colors[ImGuiCol_ResizeGrip]               = ImVec4(0.6196f, 0.5765f, 0.7686f, 0.5490f);
        style_->Colors[ImGuiCol_ResizeGripHovered]        = ImVec4(0.7373f, 0.6941f, 0.8863f, 0.5490f);
        style_->Colors[ImGuiCol_ResizeGripActive]         = ImVec4(0.8157f, 0.7725f, 0.9647f, 0.5490f);
        style_->Colors[ImGuiCol_Tab]                      = ImVec4(0.6196f, 0.5765f, 0.7686f, 0.5490f);
        style_->Colors[ImGuiCol_TabHovered]               = ImVec4(0.7373f, 0.6941f, 0.8863f, 0.5490f);
        style_->Colors[ImGuiCol_TabActive]                = ImVec4(0.8157f, 0.7725f, 0.9647f, 0.5490f);
        style_->Colors[ImGuiCol_TabUnfocused]             = ImVec4(0.0000f, 0.4510f, 1.0000f, 0.0000f);
        style_->Colors[ImGuiCol_TabUnfocusedActive]       = ImVec4(0.1333f, 0.2588f, 0.4235f, 0.0000f);
        style_->Colors[ImGuiCol_PlotLines]                = ImVec4(0.2941f, 0.2941f, 0.2941f, 1.0000f);
        style_->Colors[ImGuiCol_PlotLinesHovered]         = ImVec4(0.7373f, 0.6941f, 0.8863f, 0.5490f);
        style_->Colors[ImGuiCol_PlotHistogram]            = ImVec4(0.6196f, 0.5765f, 0.7686f, 0.5490f);
        style_->Colors[ImGuiCol_PlotHistogramHovered]     = ImVec4(0.7373f, 0.6941f, 0.8863f, 0.5490f);
        style_->Colors[ImGuiCol_TableHeaderBg]            = ImVec4(0.1882f, 0.1882f, 0.2000f, 1.0000f);
        style_->Colors[ImGuiCol_TableBorderStrong]        = ImVec4(0.4235f, 0.3804f, 0.5725f, 0.5490f);
        style_->Colors[ImGuiCol_TableBorderLight]         = ImVec4(0.4235f, 0.3804f, 0.5725f, 0.2918f);
        style_->Colors[ImGuiCol_TableRowBg]               = ImVec4(0.0000f, 0.0000f, 0.0000f, 0.0000f);
        style_->Colors[ImGuiCol_TableRowBgAlt]            = ImVec4(1.0000f, 1.0000f, 1.0000f, 0.0343f);
        style_->Colors[ImGuiCol_TextSelectedBg]           = ImVec4(0.7373f, 0.6941f, 0.8863f, 0.5490f);
        style_->Colors[ImGuiCol_DragDropTarget]           = ImVec4(1.0000f, 1.0000f, 0.0000f, 0.9000f);
        style_->Colors[ImGuiCol_NavHighlight]             = ImVec4(0.0000f, 0.0000f, 0.0000f, 1.0000f);
        style_->Colors[ImGuiCol_NavWindowingHighlight]    = ImVec4(1.0000f, 1.0000f, 1.0000f, 0.7000f);
        style_->Colors[ImGuiCol_NavWindowingDimBg]        = ImVec4(0.8000f, 0.8000f, 0.8000f, 0.2000f);
        style_->Colors[ImGuiCol_ModalWindowDimBg]         = ImVec4(0.8000f, 0.8000f, 0.8000f, 0.3500f);
        
        SYS_INFO("GuiMgr", "'Confy' theme applied.");
    }

    void GuiMgr::Style_FutureDark() {

        StyleColorsDark();
        titleBarDarkMode(true);

        style_->Alpha                       = 1.0000f;
        style_->DisabledAlpha               = 1.0000f;
        style_->WindowPadding               = ImVec2(12.0000f, 12.0000f);
        style_->WindowRounding              = 0.0000f;
        style_->WindowBorderSize            = 0.0000f;
        style_->WindowMinSize               = ImVec2(20.0000f, 20.0000f);
        style_->WindowTitleAlign            = ImVec2(0.5000f, 0.5000f);
        style_->WindowMenuButtonPosition    = ImGuiDir_None;
        style_->ChildRounding               = 0.0000f;
        style_->ChildBorderSize             = 1.0000f;
        style_->PopupRounding               = 0.0000f;
        style_->PopupBorderSize             = 1.0000f;
        style_->FramePadding                = ImVec2(6.0000f, 6.0000f);
        style_->FrameRounding               = 0.0000f;
        style_->FrameBorderSize             = 0.0000f;
        style_->ItemSpacing                 = ImVec2(12.0000f, 6.0000f);
        style_->ItemInnerSpacing            = ImVec2(6.0000f, 3.0000f);
        style_->CellPadding                 = ImVec2(12.0000f, 6.0000f);
        style_->IndentSpacing               = 20.0000f;
        style_->ColumnsMinSpacing           = 6.0000f;
        style_->ScrollbarSize               = 12.0000f;
        style_->ScrollbarRounding           = 0.0000f;
        style_->GrabMinSize                 = 12.0000f;
        style_->GrabRounding                = 0.0000f;
        style_->TabRounding                 = 0.0000f;
        style_->TabBorderSize               = 0.0000f;
        style_->ColorButtonPosition         = ImGuiDir_Right;
        style_->ButtonTextAlign             = ImVec2(0.5000f, 0.5000f);
        style_->SelectableTextAlign         = ImVec2(0.0000f, 0.0000f);

        style_->Colors[ImGuiCol_Text]                     = ImVec4(1.0000f, 1.0000f, 1.0000f, 1.0000f);
        style_->Colors[ImGuiCol_TextDisabled]             = ImVec4(0.2745f, 0.3176f, 0.4510f, 1.0000f);
        style_->Colors[ImGuiCol_WindowBg]                 = ImVec4(0.0784f, 0.0863f, 0.1020f, 1.0000f);
        style_->Colors[ImGuiCol_ChildBg]                  = ImVec4(0.0784f, 0.0863f, 0.1020f, 1.0000f);
        style_->Colors[ImGuiCol_PopupBg]                  = ImVec4(0.0784f, 0.0863f, 0.1020f, 1.0000f);
        style_->Colors[ImGuiCol_Border]                   = ImVec4(0.1569f, 0.1686f, 0.1922f, 1.0000f);
        style_->Colors[ImGuiCol_BorderShadow]             = ImVec4(0.0784f, 0.0863f, 0.1020f, 1.0000f);
        style_->Colors[ImGuiCol_FrameBg]                  = ImVec4(0.1176f, 0.1333f, 0.1490f, 1.0000f);
        style_->Colors[ImGuiCol_FrameBgHovered]           = ImVec4(0.1569f, 0.1686f, 0.1922f, 1.0000f);
        style_->Colors[ImGuiCol_FrameBgActive]            = ImVec4(0.2353f, 0.2157f, 0.5961f, 1.0000f);
        style_->Colors[ImGuiCol_TitleBg]                  = ImVec4(0.0471f, 0.0549f, 0.0706f, 1.0000f);
        style_->Colors[ImGuiCol_TitleBgActive]            = ImVec4(0.0471f, 0.0549f, 0.0706f, 1.0000f);
        style_->Colors[ImGuiCol_TitleBgCollapsed]         = ImVec4(0.0784f, 0.0863f, 0.1020f, 1.0000f);
        style_->Colors[ImGuiCol_MenuBarBg]                = ImVec4(0.0980f, 0.1059f, 0.1216f, 1.0000f);
        style_->Colors[ImGuiCol_ScrollbarBg]              = ImVec4(0.0471f, 0.0549f, 0.0706f, 1.0000f);
        style_->Colors[ImGuiCol_ScrollbarGrab]            = ImVec4(0.1176f, 0.1333f, 0.1490f, 1.0000f);
        style_->Colors[ImGuiCol_ScrollbarGrabHovered]     = ImVec4(0.1569f, 0.1686f, 0.1922f, 1.0000f);
        style_->Colors[ImGuiCol_ScrollbarGrabActive]      = ImVec4(0.1176f, 0.1333f, 0.1490f, 1.0000f);
        style_->Colors[ImGuiCol_CheckMark]                = ImVec4(0.4980f, 0.5137f, 1.0000f, 1.0000f);
        style_->Colors[ImGuiCol_SliderGrab]               = ImVec4(0.4980f, 0.5137f, 1.0000f, 1.0000f);
        style_->Colors[ImGuiCol_SliderGrabActive]         = ImVec4(0.5373f, 0.5529f, 1.0000f, 1.0000f);
        style_->Colors[ImGuiCol_Button]                   = ImVec4(0.1176f, 0.1333f, 0.1490f, 1.0000f);
        style_->Colors[ImGuiCol_ButtonHovered]            = ImVec4(0.1961f, 0.1765f, 0.5451f, 1.0000f);
        style_->Colors[ImGuiCol_ButtonActive]             = ImVec4(0.2353f, 0.2157f, 0.5961f, 1.0000f);
        style_->Colors[ImGuiCol_Header]                   = ImVec4(0.1176f, 0.1333f, 0.1490f, 1.0000f);
        style_->Colors[ImGuiCol_HeaderHovered]            = ImVec4(0.1961f, 0.1765f, 0.5451f, 1.0000f);
        style_->Colors[ImGuiCol_HeaderActive]             = ImVec4(0.2353f, 0.2157f, 0.5961f, 1.0000f);
        style_->Colors[ImGuiCol_Separator]                = ImVec4(0.1569f, 0.1843f, 0.2510f, 1.0000f);
        style_->Colors[ImGuiCol_SeparatorHovered]         = ImVec4(0.1569f, 0.1843f, 0.2510f, 1.0000f);
        style_->Colors[ImGuiCol_SeparatorActive]          = ImVec4(0.1569f, 0.1843f, 0.2510f, 1.0000f);
        style_->Colors[ImGuiCol_ResizeGrip]               = ImVec4(0.1176f, 0.1333f, 0.1490f, 1.0000f);
        style_->Colors[ImGuiCol_ResizeGripHovered]        = ImVec4(0.1961f, 0.1765f, 0.5451f, 1.0000f);
        style_->Colors[ImGuiCol_ResizeGripActive]         = ImVec4(0.2353f, 0.2157f, 0.5961f, 1.0000f);
        style_->Colors[ImGuiCol_Tab]                      = ImVec4(0.0471f, 0.0549f, 0.0706f, 1.0000f);
        style_->Colors[ImGuiCol_TabHovered]               = ImVec4(0.1176f, 0.1333f, 0.1490f, 1.0000f);
        style_->Colors[ImGuiCol_TabActive]                = ImVec4(0.0980f, 0.1059f, 0.1216f, 1.0000f);
        style_->Colors[ImGuiCol_TabUnfocused]             = ImVec4(0.0471f, 0.0549f, 0.0706f, 1.0000f);
        style_->Colors[ImGuiCol_TabUnfocusedActive]       = ImVec4(0.0784f, 0.0863f, 0.1020f, 1.0000f);
        style_->Colors[ImGuiCol_PlotLines]                = ImVec4(0.5216f, 0.6000f, 0.7020f, 1.0000f);
        style_->Colors[ImGuiCol_PlotLinesHovered]         = ImVec4(0.0392f, 0.9804f, 0.9804f, 1.0000f);
        style_->Colors[ImGuiCol_PlotHistogram]            = ImVec4(1.0000f, 0.2902f, 0.5961f, 1.0000f);
        style_->Colors[ImGuiCol_PlotHistogramHovered]     = ImVec4(0.9961f, 0.4745f, 0.6980f, 1.0000f);
        style_->Colors[ImGuiCol_TableHeaderBg]            = ImVec4(0.0471f, 0.0549f, 0.0706f, 1.0000f);
        style_->Colors[ImGuiCol_TableBorderStrong]        = ImVec4(0.0471f, 0.0549f, 0.0706f, 1.0000f);
        style_->Colors[ImGuiCol_TableBorderLight]         = ImVec4(0.0000f, 0.0000f, 0.0000f, 1.0000f);
        style_->Colors[ImGuiCol_TableRowBg]               = ImVec4(0.1176f, 0.1333f, 0.1490f, 1.0000f);
        style_->Colors[ImGuiCol_TableRowBgAlt]            = ImVec4(0.0980f, 0.1059f, 0.1216f, 1.0000f);
        style_->Colors[ImGuiCol_TextSelectedBg]           = ImVec4(0.2353f, 0.2157f, 0.5961f, 1.0000f);
        style_->Colors[ImGuiCol_DragDropTarget]           = ImVec4(0.4980f, 0.5137f, 1.0000f, 1.0000f);
        style_->Colors[ImGuiCol_NavHighlight]             = ImVec4(0.4980f, 0.5137f, 1.0000f, 1.0000f);
        style_->Colors[ImGuiCol_NavWindowingHighlight]    = ImVec4(0.4980f, 0.5137f, 1.0000f, 1.0000f);
        style_->Colors[ImGuiCol_NavWindowingDimBg]        = ImVec4(0.1961f, 0.1765f, 0.5451f, 0.50196f);
        style_->Colors[ImGuiCol_ModalWindowDimBg]         = ImVec4(0.1961f, 0.1765f, 0.5451f, 0.50196f);
        
        SYS_INFO("GuiMgr", "'FutureDark' theme applied.");
    }

    void GuiMgr::Style_Moonlight() {
        
        StyleColorsDark();
        titleBarDarkMode(true);

        style_->Alpha                       = 1.0000f;
        style_->DisabledAlpha               = 1.0000f;
        style_->WindowPadding               = ImVec2(12.0000f, 12.0000f);
        style_->WindowRounding              = 11.5000f;
        style_->WindowBorderSize            = 0.0000f;
        style_->WindowMinSize               = ImVec2(20.0000f, 20.0000f);
        style_->WindowTitleAlign            = ImVec2(0.5000f, 0.5000f);
        style_->WindowMenuButtonPosition    = ImGuiDir_Right;
        style_->ChildRounding               = 0.0000f;
        style_->ChildBorderSize             = 1.0000f;
        style_->PopupRounding               = 0.0000f;
        style_->PopupBorderSize             = 1.0000f;
        style_->FramePadding                = ImVec2(20.0000f, 3.4000f);
        style_->FrameRounding               = 11.9000f;
        style_->FrameBorderSize             = 0.0000f;
        style_->ItemSpacing                 = ImVec2(4.3000f, 5.5000f);
        style_->ItemInnerSpacing            = ImVec2(7.1000f, 1.8000f);
        style_->CellPadding                 = ImVec2(12.1000f, 9.2000f);
        style_->IndentSpacing               = 0.0000f;
        style_->ColumnsMinSpacing           = 4.9000f;
        style_->ScrollbarSize               = 11.6000f;
        style_->ScrollbarRounding           = 15.9000f;
        style_->GrabMinSize                 = 3.7000f;
        style_->GrabRounding                = 20.0000f;
        style_->TabRounding                 = 0.0000f;
        style_->TabBorderSize               = 0.0000f;
        style_->ColorButtonPosition         = ImGuiDir_Right;
        style_->ButtonTextAlign             = ImVec2(0.5000f, 0.5000f);
        style_->SelectableTextAlign         = ImVec2(0.0000f, 0.0000f);

        style_->Colors[ImGuiCol_Text]                     = ImVec4(1.0000f, 1.0000f, 1.0000f, 1.0000f);
        style_->Colors[ImGuiCol_TextDisabled]             = ImVec4(0.2745f, 0.3176f, 0.4510f, 1.0000f);
        style_->Colors[ImGuiCol_WindowBg]                 = ImVec4(0.0784f, 0.0863f, 0.1020f, 1.0000f);
        style_->Colors[ImGuiCol_ChildBg]                  = ImVec4(0.0941f, 0.1020f, 0.1176f, 1.0000f);
        style_->Colors[ImGuiCol_PopupBg]                  = ImVec4(0.0784f, 0.0863f, 0.1020f, 1.0000f);
        style_->Colors[ImGuiCol_Border]                   = ImVec4(0.1569f, 0.1686f, 0.1922f, 1.0000f);
        style_->Colors[ImGuiCol_BorderShadow]             = ImVec4(0.0784f, 0.0863f, 0.1020f, 1.0000f);
        style_->Colors[ImGuiCol_FrameBg]                  = ImVec4(0.1137f, 0.1255f, 0.1529f, 1.0000f);
        style_->Colors[ImGuiCol_FrameBgHovered]           = ImVec4(0.1569f, 0.1686f, 0.1922f, 1.0000f);
        style_->Colors[ImGuiCol_FrameBgActive]            = ImVec4(0.1569f, 0.1686f, 0.1922f, 1.0000f);
        style_->Colors[ImGuiCol_TitleBg]                  = ImVec4(0.0471f, 0.0549f, 0.0706f, 1.0000f);
        style_->Colors[ImGuiCol_TitleBgActive]            = ImVec4(0.0471f, 0.0549f, 0.0706f, 1.0000f);
        style_->Colors[ImGuiCol_TitleBgCollapsed]         = ImVec4(0.0784f, 0.0863f, 0.1020f, 1.0000f);
        style_->Colors[ImGuiCol_MenuBarBg]                = ImVec4(0.0980f, 0.1059f, 0.1216f, 1.0000f);
        style_->Colors[ImGuiCol_ScrollbarBg]              = ImVec4(0.0471f, 0.0549f, 0.0706f, 1.0000f);
        style_->Colors[ImGuiCol_ScrollbarGrab]            = ImVec4(0.1176f, 0.1333f, 0.1490f, 1.0000f);
        style_->Colors[ImGuiCol_ScrollbarGrabHovered]     = ImVec4(0.1569f, 0.1686f, 0.1922f, 1.0000f);
        style_->Colors[ImGuiCol_ScrollbarGrabActive]      = ImVec4(0.1176f, 0.1333f, 0.1490f, 1.0000f);
        style_->Colors[ImGuiCol_CheckMark]                = ImVec4(0.9725f, 1.0000f, 0.4980f, 1.0000f);
        style_->Colors[ImGuiCol_SliderGrab]               = ImVec4(0.9725f, 1.0000f, 0.4980f, 1.0000f);
        style_->Colors[ImGuiCol_SliderGrabActive]         = ImVec4(1.0000f, 0.7961f, 0.4980f, 1.0000f);
        style_->Colors[ImGuiCol_Button]                   = ImVec4(0.1176f, 0.1333f, 0.1490f, 1.0000f);
        style_->Colors[ImGuiCol_ButtonHovered]            = ImVec4(0.1804f, 0.1882f, 0.1961f, 1.0000f);
        style_->Colors[ImGuiCol_ButtonActive]             = ImVec4(0.1529f, 0.1529f, 0.1529f, 1.0000f);
        style_->Colors[ImGuiCol_Header]                   = ImVec4(0.1412f, 0.1647f, 0.2078f, 1.0000f);
        style_->Colors[ImGuiCol_HeaderHovered]            = ImVec4(0.1059f, 0.1059f, 0.1059f, 1.0000f);
        style_->Colors[ImGuiCol_HeaderActive]             = ImVec4(0.0784f, 0.0863f, 0.1020f, 1.0000f);
        style_->Colors[ImGuiCol_Separator]                = ImVec4(0.1294f, 0.1490f, 0.1922f, 1.0000f);
        style_->Colors[ImGuiCol_SeparatorHovered]         = ImVec4(0.1569f, 0.1843f, 0.2510f, 1.0000f);
        style_->Colors[ImGuiCol_SeparatorActive]          = ImVec4(0.1569f, 0.1843f, 0.2510f, 1.0000f);
        style_->Colors[ImGuiCol_ResizeGrip]               = ImVec4(0.1451f, 0.1451f, 0.1451f, 1.0000f);
        style_->Colors[ImGuiCol_ResizeGripHovered]        = ImVec4(0.9725f, 1.0000f, 0.4980f, 1.0000f);
        style_->Colors[ImGuiCol_ResizeGripActive]         = ImVec4(1.0000f, 1.0000f, 1.0000f, 1.0000f);
        style_->Colors[ImGuiCol_Tab]                      = ImVec4(0.0784f, 0.0863f, 0.1020f, 1.0000f);
        style_->Colors[ImGuiCol_TabHovered]               = ImVec4(0.1176f, 0.1333f, 0.1490f, 1.0000f);
        style_->Colors[ImGuiCol_TabActive]                = ImVec4(0.1176f, 0.1333f, 0.1490f, 1.0000f);
        style_->Colors[ImGuiCol_TabUnfocused]             = ImVec4(0.0784f, 0.0863f, 0.1020f, 1.0000f);
        style_->Colors[ImGuiCol_TabUnfocusedActive]       = ImVec4(0.1255f, 0.2745f, 0.5725f, 1.0000f);
        style_->Colors[ImGuiCol_PlotLines]                = ImVec4(0.5216f, 0.6000f, 0.7020f, 1.0000f);
        style_->Colors[ImGuiCol_PlotLinesHovered]         = ImVec4(0.0392f, 0.9804f, 0.9804f, 1.0000f);
        style_->Colors[ImGuiCol_PlotHistogram]            = ImVec4(0.8824f, 0.7961f, 0.5608f, 1.0000f);
        style_->Colors[ImGuiCol_PlotHistogramHovered]     = ImVec4(0.9569f, 0.9569f, 0.9569f, 1.0000f);
        style_->Colors[ImGuiCol_TableHeaderBg]            = ImVec4(0.0471f, 0.0549f, 0.0706f, 1.0000f);
        style_->Colors[ImGuiCol_TableBorderStrong]        = ImVec4(0.0471f, 0.0549f, 0.0706f, 1.0000f);
        style_->Colors[ImGuiCol_TableBorderLight]         = ImVec4(0.0000f, 0.0000f, 0.0000f, 1.0000f);
        style_->Colors[ImGuiCol_TableRowBg]               = ImVec4(0.1176f, 0.1333f, 0.1490f, 1.0000f);
        style_->Colors[ImGuiCol_TableRowBgAlt]            = ImVec4(0.0980f, 0.1059f, 0.1216f, 1.0000f);
        style_->Colors[ImGuiCol_TextSelectedBg]           = ImVec4(0.9373f, 0.9373f, 0.9373f, 1.0000f);
        style_->Colors[ImGuiCol_DragDropTarget]           = ImVec4(0.4980f, 0.5137f, 1.0000f, 1.0000f);
        style_->Colors[ImGuiCol_NavHighlight]             = ImVec4(0.2667f, 0.2902f, 1.0000f, 1.0000f);
        style_->Colors[ImGuiCol_NavWindowingHighlight]    = ImVec4(0.4980f, 0.5137f, 1.0000f, 1.0000f);
        style_->Colors[ImGuiCol_NavWindowingDimBg]        = ImVec4(0.1961f, 0.1765f, 0.5451f, 0.50196f);
        style_->Colors[ImGuiCol_ModalWindowDimBg]         = ImVec4(0.1961f, 0.1765f, 0.5451f, 0.50196f);

        SYS_INFO("GuiMgr", "'Moonlight' theme applied.");
    }

    void GuiMgr::Style_VisualStudio() {

        StyleColorsDark();
        titleBarDarkMode(true);

        style_->Alpha                           = 1.0000f;
        style_->DisabledAlpha                   = 0.6000f;
        style_->WindowPadding                   = ImVec2(8.0000f, 8.0000f);
        style_->WindowRounding                  = 4.0000f;
        style_->WindowBorderSize                = 0.0000f;
        style_->WindowMinSize                   = ImVec2(32.0000f, 32.0000f);
        style_->WindowTitleAlign                = ImVec2(0.0000f, 0.5000f);
        style_->WindowMenuButtonPosition        = ImGuiDir_Left;
        style_->ChildRounding                   = 0.0000f;
        style_->ChildBorderSize                 = 1.0000f;
        style_->PopupRounding                   = 4.0000f;
        style_->PopupBorderSize                 = 1.0000f;
        style_->FramePadding                    = ImVec2(4.0000f, 3.0000f);
        style_->FrameRounding                   = 2.5000f;
        style_->FrameBorderSize                 = 0.0000f;
        style_->ItemSpacing                     = ImVec2(8.0000f, 4.0000f);
        style_->ItemInnerSpacing                = ImVec2(4.0000f, 4.0000f);
        style_->CellPadding                     = ImVec2(4.0000f, 2.0000f);
        style_->IndentSpacing                   = 21.0000f;
        style_->ColumnsMinSpacing               = 6.0000f;
        style_->ScrollbarSize                   = 11.0000f;
        style_->ScrollbarRounding               = 2.5000f;
        style_->GrabMinSize                     = 10.0000f;
        style_->GrabRounding                    = 2.0000f;
        style_->TabRounding                     = 3.5000f;
        style_->TabBorderSize                   = 0.0000f;
        style_->ColorButtonPosition             = ImGuiDir_Right;
        style_->ButtonTextAlign                 = ImVec2(0.5000f, 0.5000f);
        style_->SelectableTextAlign             = ImVec2(0.0000f, 0.0000f);

        style_->Colors[ImGuiCol_Text]                   = ImVec4(1.0000f, 1.0000f, 1.0000f, 1.0000f);
        style_->Colors[ImGuiCol_TextDisabled]           = ImVec4(0.5922f, 0.5922f, 0.5922f, 1.0000f);
        style_->Colors[ImGuiCol_WindowBg]               = ImVec4(0.1451f, 0.1451f, 0.1490f, 1.0000f);
        style_->Colors[ImGuiCol_ChildBg]                = ImVec4(0.1451f, 0.1451f, 0.1490f, 1.0000f);
        style_->Colors[ImGuiCol_PopupBg]                = ImVec4(0.1451f, 0.1451f, 0.1490f, 1.0000f);
        style_->Colors[ImGuiCol_Border]                 = ImVec4(0.3059f, 0.3059f, 0.3059f, 1.0000f);
        style_->Colors[ImGuiCol_BorderShadow]           = ImVec4(0.3059f, 0.3059f, 0.3059f, 1.0000f);
        style_->Colors[ImGuiCol_FrameBg]                = ImVec4(0.2000f, 0.2000f, 0.2157f, 1.0000f);
        style_->Colors[ImGuiCol_FrameBgHovered]         = ImVec4(0.1137f, 0.5922f, 0.9255f, 1.0000f);
        style_->Colors[ImGuiCol_FrameBgActive]          = ImVec4(0.0000f, 0.4667f, 0.7843f, 1.0000f);
        style_->Colors[ImGuiCol_TitleBg]                = ImVec4(0.1451f, 0.1451f, 0.1490f, 1.0000f);
        style_->Colors[ImGuiCol_TitleBgActive]          = ImVec4(0.1451f, 0.1451f, 0.1490f, 1.0000f);
        style_->Colors[ImGuiCol_TitleBgCollapsed]       = ImVec4(0.1451f, 0.1451f, 0.1490f, 1.0000f);
        style_->Colors[ImGuiCol_MenuBarBg]              = ImVec4(0.2000f, 0.2000f, 0.2157f, 1.0000f);
        style_->Colors[ImGuiCol_ScrollbarBg]            = ImVec4(0.2000f, 0.2000f, 0.2157f, 1.0000f);
        style_->Colors[ImGuiCol_ScrollbarGrab]          = ImVec4(0.3216f, 0.3216f, 0.3333f, 1.0000f);
        style_->Colors[ImGuiCol_ScrollbarGrabHovered]   = ImVec4(0.3529f, 0.3529f, 0.3725f, 1.0000f);
        style_->Colors[ImGuiCol_ScrollbarGrabActive]    = ImVec4(0.3529f, 0.3529f, 0.3725f, 1.0000f);
        style_->Colors[ImGuiCol_CheckMark]              = ImVec4(0.0000f, 0.4667f, 0.7843f, 1.0000f);
        style_->Colors[ImGuiCol_SliderGrab]             = ImVec4(0.1137f, 0.5922f, 0.9255f, 1.0000f);
        style_->Colors[ImGuiCol_SliderGrabActive]       = ImVec4(0.0000f, 0.4667f, 0.7843f, 1.0000f);
        style_->Colors[ImGuiCol_Button]                 = ImVec4(0.2000f, 0.2000f, 0.2157f, 1.0000f);
        style_->Colors[ImGuiCol_ButtonHovered]          = ImVec4(0.1137f, 0.5922f, 0.9255f, 1.0000f);
        style_->Colors[ImGuiCol_ButtonActive]           = ImVec4(0.1137f, 0.5922f, 0.9255f, 1.0000f);
        style_->Colors[ImGuiCol_Header]                 = ImVec4(0.2000f, 0.2000f, 0.2157f, 1.0000f);
        style_->Colors[ImGuiCol_HeaderHovered]          = ImVec4(0.1137f, 0.5922f, 0.9255f, 1.0000f);
        style_->Colors[ImGuiCol_HeaderActive]           = ImVec4(0.0000f, 0.4667f, 0.7843f, 1.0000f);
        style_->Colors[ImGuiCol_Separator]              = ImVec4(0.3059f, 0.3059f, 0.3059f, 1.0000f);
        style_->Colors[ImGuiCol_SeparatorHovered]       = ImVec4(0.3059f, 0.3059f, 0.3059f, 1.0000f);
        style_->Colors[ImGuiCol_SeparatorActive]        = ImVec4(0.3059f, 0.3059f, 0.3059f, 1.0000f);
        style_->Colors[ImGuiCol_ResizeGrip]             = ImVec4(0.1451f, 0.1451f, 0.1490f, 1.0000f);
        style_->Colors[ImGuiCol_ResizeGripHovered]      = ImVec4(0.2000f, 0.2000f, 0.2157f, 1.0000f);
        style_->Colors[ImGuiCol_ResizeGripActive]       = ImVec4(0.3216f, 0.3216f, 0.3333f, 1.0000f);
        style_->Colors[ImGuiCol_Tab]                    = ImVec4(0.1451f, 0.1451f, 0.1490f, 1.0000f);
        style_->Colors[ImGuiCol_TabHovered]             = ImVec4(0.1137f, 0.5922f, 0.9255f, 1.0000f);
        style_->Colors[ImGuiCol_TabActive]              = ImVec4(0.0000f, 0.4667f, 0.7843f, 1.0000f);
        style_->Colors[ImGuiCol_TabUnfocused]           = ImVec4(0.1451f, 0.1451f, 0.1490f, 1.0000f);
        style_->Colors[ImGuiCol_TabUnfocusedActive]     = ImVec4(0.0000f, 0.4667f, 0.7843f, 1.0000f);
        style_->Colors[ImGuiCol_PlotLines]              = ImVec4(0.0000f, 0.4667f, 0.7843f, 1.0000f);
        style_->Colors[ImGuiCol_PlotLinesHovered]       = ImVec4(0.1137f, 0.5922f, 0.9255f, 1.0000f);
        style_->Colors[ImGuiCol_PlotHistogram]          = ImVec4(0.0000f, 0.4667f, 0.7843f, 1.0000f);
        style_->Colors[ImGuiCol_PlotHistogramHovered]   = ImVec4(0.1137f, 0.5922f, 0.9255f, 1.0000f);
        style_->Colors[ImGuiCol_TableHeaderBg]          = ImVec4(0.1882f, 0.1882f, 0.2000f, 1.0000f);
        style_->Colors[ImGuiCol_TableBorderStrong]      = ImVec4(0.3098f, 0.3098f, 0.3490f, 1.0000f);
        style_->Colors[ImGuiCol_TableBorderLight]       = ImVec4(0.2275f, 0.2275f, 0.2471f, 1.0000f);
        style_->Colors[ImGuiCol_TableRowBg]             = ImVec4(0.0000f, 0.0000f, 0.0000f, 0.0000f);
        style_->Colors[ImGuiCol_TableRowBgAlt]          = ImVec4(1.0000f, 1.0000f, 1.0000f, 0.0600f);
        style_->Colors[ImGuiCol_TextSelectedBg]         = ImVec4(0.0000f, 0.4667f, 0.7843f, 1.0000f);
        style_->Colors[ImGuiCol_DragDropTarget]         = ImVec4(0.1451f, 0.1451f, 0.1490f, 1.0000f);
        style_->Colors[ImGuiCol_NavHighlight]           = ImVec4(0.1451f, 0.1451f, 0.1490f, 1.0000f);
        style_->Colors[ImGuiCol_NavWindowingHighlight]  = ImVec4(1.0000f, 1.0000f, 1.0000f, 0.7000f);
        style_->Colors[ImGuiCol_NavWindowingDimBg]      = ImVec4(0.8000f, 0.8000f, 0.8000f, 0.2000f);
        style_->Colors[ImGuiCol_ModalWindowDimBg]       = ImVec4(0.1451f, 0.1451f, 0.1490f, 1.0000f);

        SYS_INFO("GuiMgr", "'Visual Studio' theme applied.");
    }

    void GuiMgr::Style_Microfrost() {

        StyleColorsLight();
        titleBarDarkMode(false);

        style_->Alpha                       = 1.0f;
        style_->DisabledAlpha               = 0.6f;
        style_->WindowPadding               = ImVec2(4.0f, 6.0f);
        style_->WindowRounding              = 0.0f;
        style_->WindowBorderSize            = 0.0f;
        style_->WindowMinSize               = ImVec2(32.0f, 32.0f);
        style_->WindowTitleAlign            = ImVec2(0.0f, 0.5f);
        style_->WindowMenuButtonPosition    = ImGuiDir_Left;
        style_->ChildRounding               = 0.0f;
        style_->ChildBorderSize             = 1.0f;
        style_->PopupRounding               = 0.0f;
        style_->PopupBorderSize             = 1.0f;
        style_->FramePadding                = ImVec2(8.0f, 6.0f);
        style_->FrameRounding               = 0.0f;
        style_->FrameBorderSize             = 1.0f;
        style_->ItemSpacing                 = ImVec2(8.0f, 6.0f);
        style_->ItemInnerSpacing            = ImVec2(8.0f, 6.0f);
        style_->CellPadding                 = ImVec2(4.0f, 2.0f);
        style_->IndentSpacing               = 20.0f;
        style_->ColumnsMinSpacing           = 6.0f;
        style_->ScrollbarSize               = 20.0f;
        style_->ScrollbarRounding           = 0.0f;
        style_->GrabMinSize                 = 5.0f;
        style_->GrabRounding                = 0.0f;
        style_->TabRounding                 = 4.0f;
        style_->TabBorderSize               = 0.0f;
        style_->ColorButtonPosition         = ImGuiDir_Right;
        style_->ButtonTextAlign             = ImVec2(0.5f, 0.5f);
        style_->SelectableTextAlign         = ImVec2(0.0f, 0.0f);

        style_->Colors[ImGuiCol_Text]                   = ImVec4(0.0980f, 0.0980f, 0.0980f, 1.0000f);
        style_->Colors[ImGuiCol_TextDisabled]           = ImVec4(0.4980f, 0.4980f, 0.4980f, 1.0000f);
        style_->Colors[ImGuiCol_WindowBg]               = ImVec4(0.9490f, 0.9490f, 0.9490f, 1.0000f);
        style_->Colors[ImGuiCol_ChildBg]                = ImVec4(0.9490f, 0.9490f, 0.9490f, 1.0000f);
        style_->Colors[ImGuiCol_PopupBg]                = ImVec4(1.0000f, 1.0000f, 1.0000f, 1.0000f);
        style_->Colors[ImGuiCol_Border]                 = ImVec4(0.6000f, 0.6000f, 0.6000f, 1.0000f);
        style_->Colors[ImGuiCol_BorderShadow]           = ImVec4(0.0000f, 0.0000f, 0.0000f, 0.0000f);
        style_->Colors[ImGuiCol_FrameBg]                = ImVec4(1.0000f, 1.0000f, 1.0000f, 1.0000f);
        style_->Colors[ImGuiCol_FrameBgHovered]         = ImVec4(0.0000f, 0.4667f, 0.8392f, 0.2000f);
        style_->Colors[ImGuiCol_FrameBgActive]          = ImVec4(0.0000f, 0.4667f, 0.8392f, 1.0000f);
        style_->Colors[ImGuiCol_TitleBg]                = ImVec4(0.0392f, 0.0392f, 0.0392f, 1.0000f);
        style_->Colors[ImGuiCol_TitleBgActive]          = ImVec4(0.1569f, 0.2863f, 0.4784f, 1.0000f);
        style_->Colors[ImGuiCol_TitleBgCollapsed]       = ImVec4(0.0000f, 0.0000f, 0.0000f, 0.5100f);
        style_->Colors[ImGuiCol_MenuBarBg]              = ImVec4(0.8588f, 0.8588f, 0.8588f, 1.0000f);
        style_->Colors[ImGuiCol_ScrollbarBg]            = ImVec4(0.8588f, 0.8588f, 0.8588f, 1.0000f);
        style_->Colors[ImGuiCol_ScrollbarGrab]          = ImVec4(0.6863f, 0.6863f, 0.6863f, 1.0000f);
        style_->Colors[ImGuiCol_ScrollbarGrabHovered]   = ImVec4(0.0000f, 0.0000f, 0.0000f, 0.2000f);
        style_->Colors[ImGuiCol_ScrollbarGrabActive]    = ImVec4(0.0000f, 0.0000f, 0.0000f, 0.5000f);
        style_->Colors[ImGuiCol_CheckMark]              = ImVec4(0.0980f, 0.0980f, 0.0980f, 1.0000f);
        style_->Colors[ImGuiCol_SliderGrab]             = ImVec4(0.6863f, 0.6863f, 0.6863f, 1.0000f);
        style_->Colors[ImGuiCol_SliderGrabActive]       = ImVec4(0.0000f, 0.0000f, 0.0000f, 0.5000f);
        style_->Colors[ImGuiCol_Button]                 = ImVec4(0.8588f, 0.8588f, 0.8588f, 1.0000f);
        style_->Colors[ImGuiCol_ButtonHovered]          = ImVec4(0.0000f, 0.4667f, 0.8392f, 0.2000f);
        style_->Colors[ImGuiCol_ButtonActive]           = ImVec4(0.0000f, 0.4667f, 0.8392f, 1.0000f);
        style_->Colors[ImGuiCol_Header]                 = ImVec4(0.8588f, 0.8588f, 0.8588f, 1.0000f);
        style_->Colors[ImGuiCol_HeaderHovered]          = ImVec4(0.0000f, 0.4667f, 0.8392f, 0.2000f);
        style_->Colors[ImGuiCol_HeaderActive]           = ImVec4(0.0000f, 0.4667f, 0.8392f, 1.0000f);
        style_->Colors[ImGuiCol_Separator]              = ImVec4(0.4275f, 0.4275f, 0.4980f, 0.5000f);
        style_->Colors[ImGuiCol_SeparatorHovered]       = ImVec4(0.0980f, 0.4000f, 0.7490f, 0.7800f);
        style_->Colors[ImGuiCol_SeparatorActive]        = ImVec4(0.0980f, 0.4000f, 0.7490f, 1.0000f);
        style_->Colors[ImGuiCol_ResizeGrip]             = ImVec4(0.2588f, 0.5882f, 0.9765f, 0.2000f);
        style_->Colors[ImGuiCol_ResizeGripHovered]      = ImVec4(0.2588f, 0.5882f, 0.9765f, 0.6700f);
        style_->Colors[ImGuiCol_ResizeGripActive]       = ImVec4(0.2588f, 0.5882f, 0.9765f, 0.9500f);
        style_->Colors[ImGuiCol_Tab]                    = ImVec4(0.1765f, 0.3490f, 0.5765f, 0.8620f);
        style_->Colors[ImGuiCol_TabHovered]             = ImVec4(0.2588f, 0.5882f, 0.9765f, 0.8000f);
        style_->Colors[ImGuiCol_TabActive]              = ImVec4(0.1961f, 0.4078f, 0.6784f, 1.0000f);
        style_->Colors[ImGuiCol_TabUnfocused]           = ImVec4(0.0667f, 0.1020f, 0.1451f, 0.9724f);
        style_->Colors[ImGuiCol_TabUnfocusedActive]     = ImVec4(0.1333f, 0.2588f, 0.4235f, 1.0000f);
        style_->Colors[ImGuiCol_PlotLines]              = ImVec4(0.6078f, 0.6078f, 0.6078f, 1.0000f);
        style_->Colors[ImGuiCol_PlotLinesHovered]       = ImVec4(1.0000f, 0.4275f, 0.3490f, 1.0000f);
        style_->Colors[ImGuiCol_PlotHistogram]          = ImVec4(0.8980f, 0.6980f, 0.0000f, 1.0000f);
        style_->Colors[ImGuiCol_PlotHistogramHovered]   = ImVec4(1.0000f, 0.6000f, 0.0000f, 1.0000f);
        style_->Colors[ImGuiCol_TableHeaderBg]          = ImVec4(0.8882f, 0.8882f, 0.8882f, 1.0000f);	// modificado
        style_->Colors[ImGuiCol_TableBorderStrong]      = ImVec4(0.0980f, 0.0980f, 0.0980f, 1.0000f);	// modificado
        style_->Colors[ImGuiCol_TableBorderLight]       = ImVec4(0.8275f, 0.8275f, 0.8471f, 1.0000f);	// modificado
        style_->Colors[ImGuiCol_TableRowBg]             = ImVec4(0.0000f, 0.0000f, 0.0000f, 0.0000f);
        style_->Colors[ImGuiCol_TableRowBgAlt]          = ImVec4(1.0000f, 1.0000f, 1.0000f, 0.0600f);
        style_->Colors[ImGuiCol_TextSelectedBg]         = ImVec4(0.2588f, 0.5882f, 0.9765f, 0.3500f);
        style_->Colors[ImGuiCol_DragDropTarget]         = ImVec4(1.0000f, 1.0000f, 0.0000f, 0.9000f);
        style_->Colors[ImGuiCol_NavHighlight]           = ImVec4(0.2588f, 0.5882f, 0.9765f, 1.0000f);
        style_->Colors[ImGuiCol_NavWindowingHighlight]  = ImVec4(1.0000f, 1.0000f, 1.0000f, 0.7000f);
        style_->Colors[ImGuiCol_NavWindowingDimBg]      = ImVec4(0.8000f, 0.8000f, 0.8000f, 0.2000f);
        style_->Colors[ImGuiCol_ModalWindowDimBg]       = ImVec4(0.8000f, 0.8000f, 0.8000f, 0.3500f);

        SYS_INFO("GuiMgr", "'Microfrost' theme applied.");
    }

    void GuiMgr::Style_AdobeInspired() {

        StyleColorsDark();
        titleBarDarkMode(true);

        style_->Alpha = 1.0f;
        style_->DisabledAlpha = 0.6f;
        style_->WindowPadding = ImVec2(8.0f, 8.0f);
        style_->WindowRounding = 4.0f;
        style_->WindowBorderSize = 1.0f;
        style_->WindowMinSize = ImVec2(32.0f, 32.0f);
        style_->WindowTitleAlign = ImVec2(0.0f, 0.5f);
        style_->WindowMenuButtonPosition = ImGuiDir_None;
        style_->ChildRounding = 4.0f;
        style_->ChildBorderSize = 1.0f;
        style_->PopupRounding = 4.0f;
        style_->PopupBorderSize = 1.0f;
        style_->FramePadding = ImVec2(4.0f, 3.0f);
        style_->FrameRounding = 4.0f;
        style_->FrameBorderSize = 1.0f;
        style_->ItemSpacing = ImVec2(8.0f, 4.0f);
        style_->ItemInnerSpacing = ImVec2(4.0f, 4.0f);
        style_->CellPadding = ImVec2(4.0f, 2.0f);
        style_->IndentSpacing = 21.0f;
        style_->ColumnsMinSpacing = 6.0f;
        style_->ScrollbarSize = 14.0f;
        style_->ScrollbarRounding = 4.0f;
        style_->GrabMinSize = 10.0f;
        style_->GrabRounding = 20.0f;
        style_->TabRounding = 4.0f;
        style_->TabBorderSize = 1.0f;
        style_->ColorButtonPosition = ImGuiDir_Right;
        style_->ButtonTextAlign = ImVec2(0.5f, 0.5f);
        style_->SelectableTextAlign = ImVec2(0.0f, 0.0f);
        
        style_->Colors[ImGuiCol_Text]                  = ImVec4(1.0000f, 1.0000f, 1.0000f, 1.0000f);
        style_->Colors[ImGuiCol_TextDisabled]          = ImVec4(0.4980f, 0.4980f, 0.4980f, 1.0000f);
        style_->Colors[ImGuiCol_WindowBg]              = ImVec4(0.1137f, 0.1137f, 0.1137f, 1.0000f);
        style_->Colors[ImGuiCol_ChildBg]               = ImVec4(0.0000f, 0.0000f, 0.0000f, 0.0000f);
        style_->Colors[ImGuiCol_PopupBg]               = ImVec4(0.0784f, 0.0784f, 0.0784f, 0.9400f);
        style_->Colors[ImGuiCol_Border]                = ImVec4(1.0000f, 1.0000f, 1.0000f, 0.1631f);
        style_->Colors[ImGuiCol_BorderShadow]          = ImVec4(0.0000f, 0.0000f, 0.0000f, 0.0000f);
        style_->Colors[ImGuiCol_FrameBg]               = ImVec4(0.0863f, 0.0863f, 0.0863f, 1.0000f);
        style_->Colors[ImGuiCol_FrameBgHovered]        = ImVec4(0.1529f, 0.1529f, 0.1529f, 1.0000f);
        style_->Colors[ImGuiCol_FrameBgActive]         = ImVec4(0.1882f, 0.1882f, 0.1882f, 1.0000f);
        style_->Colors[ImGuiCol_TitleBg]               = ImVec4(0.1137f, 0.1137f, 0.1137f, 1.0000f);
        style_->Colors[ImGuiCol_TitleBgActive]         = ImVec4(0.1059f, 0.1059f, 0.1059f, 1.0000f);
        style_->Colors[ImGuiCol_TitleBgCollapsed]      = ImVec4(0.0000f, 0.0000f, 0.0000f, 0.5100f);
        style_->Colors[ImGuiCol_MenuBarBg]             = ImVec4(0.1137f, 0.1137f, 0.1137f, 1.0000f);
        style_->Colors[ImGuiCol_ScrollbarBg]           = ImVec4(0.0196f, 0.0196f, 0.0196f, 0.5300f);
        style_->Colors[ImGuiCol_ScrollbarGrab]         = ImVec4(0.3098f, 0.3098f, 0.3098f, 1.0000f);
        style_->Colors[ImGuiCol_ScrollbarGrabHovered]  = ImVec4(0.4078f, 0.4078f, 0.4078f, 1.0000f);
        style_->Colors[ImGuiCol_ScrollbarGrabActive]   = ImVec4(0.5098f, 0.5098f, 0.5098f, 1.0000f);
        style_->Colors[ImGuiCol_CheckMark]             = ImVec4(1.0000f, 1.0000f, 1.0000f, 1.0000f);
        style_->Colors[ImGuiCol_SliderGrab]            = ImVec4(0.8784f, 0.8784f, 0.8784f, 1.0000f);
        style_->Colors[ImGuiCol_SliderGrabActive]      = ImVec4(0.9804f, 0.9804f, 0.9804f, 1.0000f);
        style_->Colors[ImGuiCol_Button]                = ImVec4(0.1490f, 0.1490f, 0.1490f, 1.0000f);
        style_->Colors[ImGuiCol_ButtonHovered]         = ImVec4(0.2471f, 0.2471f, 0.2471f, 1.0000f);
        style_->Colors[ImGuiCol_ButtonActive]          = ImVec4(0.3294f, 0.3294f, 0.3294f, 1.0000f);
        style_->Colors[ImGuiCol_Header]                = ImVec4(0.9765f, 0.9765f, 0.9765f, 0.3098f);
        style_->Colors[ImGuiCol_HeaderHovered]         = ImVec4(0.9765f, 0.9765f, 0.9765f, 0.8000f);
        style_->Colors[ImGuiCol_HeaderActive]          = ImVec4(0.9765f, 0.9765f, 0.9765f, 1.0000f);
        style_->Colors[ImGuiCol_Separator]             = ImVec4(0.4275f, 0.4275f, 0.4980f, 0.5000f);
        style_->Colors[ImGuiCol_SeparatorHovered]      = ImVec4(0.7490f, 0.7490f, 0.7490f, 0.7804f);
        style_->Colors[ImGuiCol_SeparatorActive]       = ImVec4(0.7490f, 0.7490f, 0.7490f, 1.0000f);
        style_->Colors[ImGuiCol_ResizeGrip]            = ImVec4(0.9765f, 0.9765f, 0.9765f, 0.2000f);
        style_->Colors[ImGuiCol_ResizeGripHovered]     = ImVec4(0.9373f, 0.9373f, 0.9373f, 0.6706f);
        style_->Colors[ImGuiCol_ResizeGripActive]      = ImVec4(0.9765f, 0.9765f, 0.9765f, 0.9490f);
        style_->Colors[ImGuiCol_Tab]                   = ImVec4(0.2235f, 0.2235f, 0.2235f, 0.8627f);
        style_->Colors[ImGuiCol_TabHovered]            = ImVec4(0.3216f, 0.3216f, 0.3216f, 0.8000f);
        style_->Colors[ImGuiCol_TabActive]             = ImVec4(0.2745f, 0.2745f, 0.2745f, 1.0000f);
        style_->Colors[ImGuiCol_TabUnfocused]          = ImVec4(0.1451f, 0.1451f, 0.1451f, 0.9725f);
        style_->Colors[ImGuiCol_TabUnfocusedActive]    = ImVec4(0.4235f, 0.4235f, 0.4235f, 1.0000f);
        style_->Colors[ImGuiCol_PlotLines]             = ImVec4(0.6078f, 0.6078f, 0.6078f, 1.0000f);
        style_->Colors[ImGuiCol_PlotLinesHovered]      = ImVec4(1.0000f, 0.4275f, 0.3490f, 1.0000f);
        style_->Colors[ImGuiCol_PlotHistogram]         = ImVec4(0.8980f, 0.6980f, 0.0000f, 1.0000f);
        style_->Colors[ImGuiCol_PlotHistogramHovered]  = ImVec4(1.0000f, 0.6000f, 0.0000f, 1.0000f);
        style_->Colors[ImGuiCol_TableHeaderBg]         = ImVec4(0.1882f, 0.1882f, 0.2000f, 1.0000f);
        style_->Colors[ImGuiCol_TableBorderStrong]     = ImVec4(0.3098f, 0.3098f, 0.3490f, 1.0000f);
        style_->Colors[ImGuiCol_TableBorderLight]      = ImVec4(0.2275f, 0.2275f, 0.2471f, 1.0000f);
        style_->Colors[ImGuiCol_TableRowBg]            = ImVec4(0.0000f, 0.0000f, 0.0000f, 0.0000f);
        style_->Colors[ImGuiCol_TableRowBgAlt]         = ImVec4(1.0000f, 1.0000f, 1.0000f, 0.0600f);
        style_->Colors[ImGuiCol_TextSelectedBg]        = ImVec4(0.2588f, 0.5882f, 0.9765f, 0.3500f);
        style_->Colors[ImGuiCol_DragDropTarget]        = ImVec4(1.0000f, 1.0000f, 0.0000f, 0.9000f);
        style_->Colors[ImGuiCol_NavHighlight]          = ImVec4(0.2588f, 0.5882f, 0.9765f, 1.0000f);
        style_->Colors[ImGuiCol_NavWindowingHighlight] = ImVec4(1.0000f, 1.0000f, 1.0000f, 0.7000f);
        style_->Colors[ImGuiCol_NavWindowingDimBg]     = ImVec4(0.8000f, 0.8000f, 0.8000f, 0.2000f);
        style_->Colors[ImGuiCol_ModalWindowDimBg]      = ImVec4(0.8000f, 0.8000f, 0.8000f, 0.3500f);

        SYS_INFO("GuiMgr", "'AdobeInspired' theme applied.");
    }


    void GuiMgr::Style_DarkCyan() {
        
        StyleColorsDark();
        titleBarDarkMode(true);
        
        style_->Alpha                    = 1.0000f;
        style_->DisabledAlpha            = 1.0000f;
        style_->WindowPadding            = ImVec2(20.0000f, 20.0000f);
        style_->WindowRounding           = 11.5000f;
        style_->WindowBorderSize         = 0.0000f;
        style_->WindowMinSize            = ImVec2(20.0000f, 20.0000f);
        style_->WindowTitleAlign         = ImVec2(0.5000f, 0.5000f);
        style_->WindowMenuButtonPosition = ImGuiDir_None;
        style_->ChildRounding            = 20.0000f;
        style_->ChildBorderSize          = 1.0000f;
        style_->PopupRounding            = 17.4000f;
        style_->PopupBorderSize          = 1.0000f;
        style_->FramePadding             = ImVec2(20.0000f, 3.4000f);
        style_->FrameRounding            = 11.9000f;
        style_->FrameBorderSize          = 0.0000f;
        style_->ItemSpacing              = ImVec2(8.9000f, 13.4000f);
        style_->ItemInnerSpacing         = ImVec2(7.1000f, 1.8000f);
        style_->CellPadding              = ImVec2(12.1000f, 9.2000f);
        style_->IndentSpacing            = 0.0000f;
        style_->ColumnsMinSpacing        = 8.7000f;
        style_->ScrollbarSize            = 11.6000f;
        style_->ScrollbarRounding        = 15.9000f;
        style_->GrabMinSize              = 3.7000f;
        style_->GrabRounding             = 20.0000f;
        style_->TabRounding              = 9.8000f;
        style_->TabBorderSize            = 0.0000f;
        style_->ColorButtonPosition      = ImGuiDir_Right;
        style_->ButtonTextAlign          = ImVec2(0.5000f, 0.5000f);
        style_->SelectableTextAlign      = ImVec2(0.0000f, 0.0000f);

        style_->Colors[ImGuiCol_Text]                  = ImVec4(1.0000f, 1.0000f, 1.0000f, 1.0000f);
        style_->Colors[ImGuiCol_TextDisabled]          = ImVec4(0.2745f, 0.3176f, 0.4510f, 1.0000f);
        style_->Colors[ImGuiCol_WindowBg]              = ImVec4(0.0784f, 0.0863f, 0.1020f, 1.0000f);
        style_->Colors[ImGuiCol_ChildBg]               = ImVec4(0.0941f, 0.1020f, 0.1176f, 1.0000f);
        style_->Colors[ImGuiCol_PopupBg]               = ImVec4(0.0784f, 0.0863f, 0.1020f, 1.0000f);
        style_->Colors[ImGuiCol_Border]                = ImVec4(0.1569f, 0.1686f, 0.1922f, 1.0000f);
        style_->Colors[ImGuiCol_BorderShadow]          = ImVec4(0.0784f, 0.0863f, 0.1020f, 1.0000f);
        style_->Colors[ImGuiCol_FrameBg]               = ImVec4(0.1137f, 0.1255f, 0.1529f, 1.0000f);
        style_->Colors[ImGuiCol_FrameBgHovered]        = ImVec4(0.1569f, 0.1686f, 0.1922f, 1.0000f);
        style_->Colors[ImGuiCol_FrameBgActive]         = ImVec4(0.1569f, 0.1686f, 0.1922f, 1.0000f);
        style_->Colors[ImGuiCol_TitleBg]               = ImVec4(0.0471f, 0.0549f, 0.0706f, 1.0000f);
        style_->Colors[ImGuiCol_TitleBgActive]         = ImVec4(0.0471f, 0.0549f, 0.0706f, 1.0000f);
        style_->Colors[ImGuiCol_TitleBgCollapsed]      = ImVec4(0.0784f, 0.0863f, 0.1020f, 1.0000f);
        style_->Colors[ImGuiCol_MenuBarBg]             = ImVec4(0.0980f, 0.1059f, 0.1216f, 1.0000f);
        style_->Colors[ImGuiCol_ScrollbarBg]           = ImVec4(0.0471f, 0.0549f, 0.0706f, 1.0000f);
        style_->Colors[ImGuiCol_ScrollbarGrab]         = ImVec4(0.1176f, 0.1333f, 0.1490f, 1.0000f);
        style_->Colors[ImGuiCol_ScrollbarGrabHovered]  = ImVec4(0.1569f, 0.1686f, 0.1922f, 1.0000f);
        style_->Colors[ImGuiCol_ScrollbarGrabActive]   = ImVec4(0.1176f, 0.1333f, 0.1490f, 1.0000f);
        style_->Colors[ImGuiCol_CheckMark]             = ImVec4(0.0314f, 0.9490f, 0.8431f, 1.0000f);
        style_->Colors[ImGuiCol_SliderGrab]            = ImVec4(0.0314f, 0.9490f, 0.8431f, 1.0000f);
        style_->Colors[ImGuiCol_SliderGrabActive]      = ImVec4(0.6000f, 0.9647f, 0.0314f, 1.0000f);
        style_->Colors[ImGuiCol_Button]                = ImVec4(0.1176f, 0.1333f, 0.1490f, 1.0000f);
        style_->Colors[ImGuiCol_ButtonHovered]         = ImVec4(0.1804f, 0.1882f, 0.1961f, 1.0000f);
        style_->Colors[ImGuiCol_ButtonActive]          = ImVec4(0.1529f, 0.1529f, 0.1529f, 1.0000f);
        style_->Colors[ImGuiCol_Header]                = ImVec4(0.1412f, 0.1647f, 0.2078f, 1.0000f);
        style_->Colors[ImGuiCol_HeaderHovered]         = ImVec4(0.1059f, 0.1059f, 0.1059f, 1.0000f);
        style_->Colors[ImGuiCol_HeaderActive]          = ImVec4(0.0784f, 0.0863f, 0.1020f, 1.0000f);
        style_->Colors[ImGuiCol_Separator]             = ImVec4(0.1294f, 0.1490f, 0.1922f, 1.0000f);
        style_->Colors[ImGuiCol_SeparatorHovered]      = ImVec4(0.1569f, 0.1843f, 0.2510f, 1.0000f);
        style_->Colors[ImGuiCol_SeparatorActive]       = ImVec4(0.1569f, 0.1843f, 0.2510f, 1.0000f);
        style_->Colors[ImGuiCol_ResizeGrip]            = ImVec4(0.1451f, 0.1451f, 0.1451f, 1.0000f);
        style_->Colors[ImGuiCol_ResizeGripHovered]     = ImVec4(0.0314f, 0.9490f, 0.8431f, 1.0000f);
        style_->Colors[ImGuiCol_ResizeGripActive]      = ImVec4(1.0000f, 1.0000f, 1.0000f, 1.0000f);
        style_->Colors[ImGuiCol_Tab]                   = ImVec4(0.0784f, 0.0863f, 0.1020f, 1.0000f);
        style_->Colors[ImGuiCol_TabHovered]            = ImVec4(0.1176f, 0.1333f, 0.1490f, 1.0000f);
        style_->Colors[ImGuiCol_TabActive]             = ImVec4(0.1176f, 0.1333f, 0.1490f, 1.0000f);
        style_->Colors[ImGuiCol_TabUnfocused]          = ImVec4(0.0784f, 0.0863f, 0.1020f, 1.0000f);
        style_->Colors[ImGuiCol_TabUnfocusedActive]    = ImVec4(0.1255f, 0.2745f, 0.5725f, 1.0000f);
        style_->Colors[ImGuiCol_PlotLines]             = ImVec4(0.5216f, 0.6000f, 0.7020f, 1.0000f);
        style_->Colors[ImGuiCol_PlotLinesHovered]      = ImVec4(0.0392f, 0.9804f, 0.9804f, 1.0000f);
        style_->Colors[ImGuiCol_PlotHistogram]         = ImVec4(0.0314f, 0.9490f, 0.8431f, 1.0000f);
        style_->Colors[ImGuiCol_PlotHistogramHovered]  = ImVec4(0.1569f, 0.1843f, 0.2510f, 1.0000f);
        style_->Colors[ImGuiCol_TableHeaderBg]         = ImVec4(0.0471f, 0.0549f, 0.0706f, 1.0000f);
        style_->Colors[ImGuiCol_TableBorderStrong]     = ImVec4(0.0471f, 0.0549f, 0.0706f, 1.0000f);
        style_->Colors[ImGuiCol_TableBorderLight]      = ImVec4(0.0000f, 0.0000f, 0.0000f, 1.0000f);
        style_->Colors[ImGuiCol_TableRowBg]            = ImVec4(0.1176f, 0.1333f, 0.1490f, 1.0000f);
        style_->Colors[ImGuiCol_TableRowBgAlt]         = ImVec4(0.0980f, 0.1059f, 0.1216f, 1.0000f);
        style_->Colors[ImGuiCol_TextSelectedBg]        = ImVec4(0.9373f, 0.9373f, 0.9373f, 1.0000f);
        style_->Colors[ImGuiCol_DragDropTarget]        = ImVec4(0.4980f, 0.5137f, 1.0000f, 1.0000f);
        style_->Colors[ImGuiCol_NavHighlight]          = ImVec4(0.2667f, 0.2902f, 1.0000f, 1.0000f);
        style_->Colors[ImGuiCol_NavWindowingHighlight] = ImVec4(0.4980f, 0.5137f, 1.0000f, 1.0000f);
        style_->Colors[ImGuiCol_NavWindowingDimBg]     = ImVec4(0.1961f, 0.1765f, 0.5451f, 0.5020f);
        style_->Colors[ImGuiCol_ModalWindowDimBg]      = ImVec4(0.1961f, 0.1765f, 0.5451f, 0.5020f);

        SYS_INFO("GuiMgr", "'DarkCyan' theme applied.");
    }

    void GuiMgr::Style_LightOrange() {

        StyleColorsLight();
        titleBarDarkMode(false);

        style_->Alpha                    = 1.0000f;
        style_->DisabledAlpha            = 1.0000f;
        style_->WindowPadding            = ImVec2(20.0000f, 20.0000f);
        style_->WindowRounding           = 11.5000f;
        style_->WindowBorderSize         = 0.0000f;
        style_->WindowMinSize            = ImVec2(20.0000f, 20.0000f);
        style_->WindowTitleAlign         = ImVec2(0.5000f, 0.5000f);
        style_->WindowMenuButtonPosition = ImGuiDir_None;
        style_->ChildRounding            = 20.0000f;
        style_->ChildBorderSize          = 1.0000f;
        style_->PopupRounding            = 17.4000f;
        style_->PopupBorderSize          = 1.0000f;
        style_->FramePadding             = ImVec2(20.0000f, 3.4000f);
        style_->FrameRounding            = 11.9000f;
        style_->FrameBorderSize          = 0.0000f;
        style_->ItemSpacing              = ImVec2(8.9000f, 13.4000f);
        style_->ItemInnerSpacing         = ImVec2(7.1000f, 1.8000f);
        style_->CellPadding              = ImVec2(12.1000f, 9.2000f);
        style_->IndentSpacing            = 0.0000f;
        style_->ColumnsMinSpacing        = 8.7000f;
        style_->ScrollbarSize            = 11.6000f;
        style_->ScrollbarRounding        = 15.9000f;
        style_->GrabMinSize              = 3.7000f;
        style_->GrabRounding             = 20.0000f;
        style_->TabRounding              = 9.8000f;
        style_->TabBorderSize            = 0.0000f;
        style_->ColorButtonPosition      = ImGuiDir_Right;
        style_->ButtonTextAlign          = ImVec2(0.5000f, 0.5000f);
        style_->SelectableTextAlign      = ImVec2(0.0000f, 0.0000f);

        style_->Colors[ImGuiCol_Text]                  = ImVec4(0.0000f, 0.0000f, 0.0000f, 1.0000f);
        style_->Colors[ImGuiCol_TextDisabled]          = ImVec4(0.7255f, 0.6824f, 0.5490f, 1.0000f);
        style_->Colors[ImGuiCol_WindowBg]              = ImVec4(0.9216f, 0.9137f, 0.8980f, 1.0000f);
        style_->Colors[ImGuiCol_ChildBg]               = ImVec4(0.9059f, 0.8980f, 0.8824f, 1.0000f);
        style_->Colors[ImGuiCol_PopupBg]               = ImVec4(0.9216f, 0.9137f, 0.8980f, 1.0000f);
        style_->Colors[ImGuiCol_Border]                = ImVec4(0.8431f, 0.8314f, 0.8078f, 1.0000f);
        style_->Colors[ImGuiCol_BorderShadow]          = ImVec4(0.9216f, 0.9137f, 0.8980f, 1.0000f);
        style_->Colors[ImGuiCol_FrameBg]               = ImVec4(0.8863f, 0.8745f, 0.8471f, 1.0000f);
        style_->Colors[ImGuiCol_FrameBgHovered]        = ImVec4(0.8431f, 0.8314f, 0.8078f, 1.0000f);
        style_->Colors[ImGuiCol_FrameBgActive]         = ImVec4(0.8431f, 0.8314f, 0.8078f, 1.0000f);
        style_->Colors[ImGuiCol_TitleBg]               = ImVec4(0.9529f, 0.9451f, 0.9294f, 1.0000f);
        style_->Colors[ImGuiCol_TitleBgActive]         = ImVec4(0.9529f, 0.9451f, 0.9294f, 1.0000f);
        style_->Colors[ImGuiCol_TitleBgCollapsed]      = ImVec4(0.9216f, 0.9137f, 0.8980f, 1.0000f);
        style_->Colors[ImGuiCol_MenuBarBg]             = ImVec4(0.9020f, 0.8941f, 0.8784f, 1.0000f);
        style_->Colors[ImGuiCol_ScrollbarBg]           = ImVec4(0.9529f, 0.9451f, 0.9294f, 1.0000f);
        style_->Colors[ImGuiCol_ScrollbarGrab]         = ImVec4(0.8824f, 0.8667f, 0.8510f, 1.0000f);
        style_->Colors[ImGuiCol_ScrollbarGrabHovered]  = ImVec4(0.8431f, 0.8314f, 0.8078f, 1.0000f);
        style_->Colors[ImGuiCol_ScrollbarGrabActive]   = ImVec4(0.8824f, 0.8667f, 0.8510f, 1.0000f);
        style_->Colors[ImGuiCol_CheckMark]             = ImVec4(0.9686f, 0.0510f, 0.1569f, 1.0000f);
        style_->Colors[ImGuiCol_SliderGrab]            = ImVec4(0.9647f, 0.8000f, 0.0275f, 1.0000f);
        style_->Colors[ImGuiCol_SliderGrabActive]      = ImVec4(0.9686f, 0.5882f, 0.0353f, 1.0000f);
        style_->Colors[ImGuiCol_Button]                = ImVec4(0.8824f, 0.8667f, 0.8510f, 1.0000f);
        style_->Colors[ImGuiCol_ButtonHovered]         = ImVec4(0.8196f, 0.8118f, 0.8039f, 1.0000f);
        style_->Colors[ImGuiCol_ButtonActive]          = ImVec4(0.8471f, 0.8471f, 0.8471f, 1.0000f);
        style_->Colors[ImGuiCol_Header]                = ImVec4(0.8588f, 0.8353f, 0.7922f, 1.0000f);
        style_->Colors[ImGuiCol_HeaderHovered]         = ImVec4(0.8941f, 0.8941f, 0.8941f, 1.0000f);
        style_->Colors[ImGuiCol_HeaderActive]          = ImVec4(0.9216f, 0.9137f, 0.8980f, 1.0000f);
        style_->Colors[ImGuiCol_Separator]             = ImVec4(0.8706f, 0.8510f, 0.8078f, 1.0000f);
        style_->Colors[ImGuiCol_SeparatorHovered]      = ImVec4(0.8431f, 0.8157f, 0.7490f, 1.0000f);
        style_->Colors[ImGuiCol_SeparatorActive]       = ImVec4(0.8431f, 0.8157f, 0.7490f, 1.0000f);
        style_->Colors[ImGuiCol_ResizeGrip]            = ImVec4(0.8549f, 0.8549f, 0.8549f, 1.0000f);
        style_->Colors[ImGuiCol_ResizeGripHovered]     = ImVec4(0.9686f, 0.0510f, 0.1569f, 1.0000f);
        style_->Colors[ImGuiCol_ResizeGripActive]      = ImVec4(0.0000f, 0.0000f, 0.0000f, 1.0000f);
        style_->Colors[ImGuiCol_Tab]                   = ImVec4(0.9216f, 0.9137f, 0.8980f, 1.0000f);
        style_->Colors[ImGuiCol_TabHovered]            = ImVec4(0.8824f, 0.8667f, 0.8510f, 1.0000f);
        style_->Colors[ImGuiCol_TabActive]             = ImVec4(0.8824f, 0.8667f, 0.8510f, 1.0000f);
        style_->Colors[ImGuiCol_TabUnfocused]          = ImVec4(0.9216f, 0.9137f, 0.8980f, 1.0000f);
        style_->Colors[ImGuiCol_TabUnfocusedActive]    = ImVec4(0.8745f, 0.7255f, 0.4275f, 1.0000f);
        style_->Colors[ImGuiCol_PlotLines]             = ImVec4(0.4784f, 0.4000f, 0.2980f, 1.0000f);
        style_->Colors[ImGuiCol_PlotLinesHovered]      = ImVec4(0.9608f, 0.0196f, 0.1176f, 1.0000f);
        style_->Colors[ImGuiCol_PlotHistogram]         = ImVec4(0.9059f, 0.6627f, 0.3098f, 1.0000f);
        style_->Colors[ImGuiCol_PlotHistogramHovered]  = ImVec4(0.6392f, 0.3961f, 0.0431f, 1.0000f);
        style_->Colors[ImGuiCol_TableHeaderBg]         = ImVec4(0.9529f, 0.9451f, 0.9294f, 1.0000f);
        style_->Colors[ImGuiCol_TableBorderStrong]     = ImVec4(0.9529f, 0.9451f, 0.9294f, 1.0000f);
        style_->Colors[ImGuiCol_TableBorderLight]      = ImVec4(1.0000f, 1.0000f, 1.0000f, 1.0000f);
        style_->Colors[ImGuiCol_TableRowBg]            = ImVec4(0.8824f, 0.8667f, 0.8510f, 1.0000f);
        style_->Colors[ImGuiCol_TableRowBgAlt]         = ImVec4(0.9020f, 0.8941f, 0.8784f, 1.0000f);
        style_->Colors[ImGuiCol_TextSelectedBg]        = ImVec4(0.0627f, 0.0627f, 0.0627f, 1.0000f);
        style_->Colors[ImGuiCol_DragDropTarget]        = ImVec4(0.5020f, 0.4863f, 0.0000f, 1.0000f);
        style_->Colors[ImGuiCol_NavHighlight]          = ImVec4(0.7333f, 0.7098f, 0.0000f, 1.0000f);
        style_->Colors[ImGuiCol_NavWindowingHighlight] = ImVec4(0.5020f, 0.4863f, 0.0000f, 1.0000f);
        style_->Colors[ImGuiCol_NavWindowingDimBg]     = ImVec4(0.8039f, 0.8235f, 0.4549f, 0.5020f);
        style_->Colors[ImGuiCol_ModalWindowDimBg]      = ImVec4(0.8039f, 0.8235f, 0.4549f, 0.5020f);

        SYS_INFO("GuiMgr", "'LightOrange' theme applied.");
    }


#else
// ============================================================
//  (Stubs)
// ============================================================

	// Temas --------------------------------------------------------------------------------
    void GuiMgr::Style_Confy();
    void GuiMgr::Style_FutureDark();
    void GuiMgr::Style_Moonlight();
    void GuiMgr::Style_VisualStudio();
    void GuiMgr::Style_Microfrost();
    void GuiMgr::Style_AdobeInspired();

#endif
