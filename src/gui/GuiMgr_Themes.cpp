#include "gui/GuiMgr.hpp"			// Clase de gestión de UI
#include <system/SystemMgr.hpp>

#if defined IMGUILIB || defined IMGUILIB_VERSION

    #include <imgui.h>
    #include "files/JsonMgr.hpp"

    // Se puede evitar poner "ImGui::" para simplificar
    using namespace ImGui;

	// Temas --------------------------------------------------------------------------------
    
    void GuiMgr::apply_theme() {
        if (theme_selected_ == "AdobeInspired")
            Style_AdobeInspired();
        else if (theme_selected_ == "AyuDark")
            Style_AyuDark();
        else if (theme_selected_ == "Confy")
            Style_Confy();
        else if (theme_selected_ == "DarkCyan")
            Style_DarkCyan();
        else if (theme_selected_ == "DefaultDark")
            Style_DefaultDark();
        else if (theme_selected_ == "DefaultLight")
            Style_DefaultLight();
        else if (theme_selected_ == "Everforest")
            Style_Everforest();
        else if (theme_selected_ == "FutureDark")
            Style_FutureDark();
        else if (theme_selected_ == "Gold")
            Style_Gold();
        else if (theme_selected_ == "HazyDark")
            Style_HazyDark();
        else if (theme_selected_ == "KazamsCherry")
            Style_KazamsCherry();
        else if (theme_selected_ == "LightOrange")
            Style_LightOrange();
        else if (theme_selected_ == "QuickMinimalLook")
            Style_QuickMinimalLook();
        else if (theme_selected_ == "Modern")
            Style_Modern();
        else if (theme_selected_ == "Microfrost")
            Style_Microfrost();
        else if (theme_selected_ == "Moonlight")
            Style_Moonlight();
        else if (theme_selected_ == "SonicRiders")
            Style_SonicRiders();
        else if (theme_selected_ == "VisualStudio")
            Style_VisualStudio();
        // Tema por defecto si no coincide
        else {
            SYS_WARN("GuiMgr", "Theme '" + theme_selected_ + "' not found. Using default.");
            Style_DefaultLight();
        }
    }

    void GuiMgr::saveConfig() {

        // #TODO
        /* Pendiente de gestionar o la configuración del módulo o del propio GuiMgr */

        if (!config_) return;
        
        // Toma la configuración con la que se ha inicializado el módulo
        json* cfg = static_cast<json*>(config_);
        JsonMgr& jsonMgr = JsonMgr::instance();
        jsonMgr.get_or_set(cfg, "theme_selected", theme_selected_);
    }

    void GuiMgr::Style_AdobeInspired() {

        StyleColorsDark();
        titlebar_dark_mode(true);

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

    void GuiMgr::Style_AyuDark() {

        StyleColorsDark();
        titlebar_dark_mode(true);

        style_->Alpha                  = 1.0000f;
        style_->DisabledAlpha          = 0.6000f;
        style_->WindowPadding          = ImVec2(8.0000f, 8.0000f);
        style_->WindowRounding         = 5.0000f;
        style_->WindowBorderSize       = 1.0000f;
        style_->WindowMinSize          = ImVec2(32.0000f, 32.0000f);
        style_->WindowTitleAlign       = ImVec2(0.0000f, 0.5000f);
        style_->WindowMenuButtonPosition = ImGuiDir_Left;
        style_->ChildRounding          = 0.0000f;
        style_->ChildBorderSize        = 1.0000f;
        style_->PopupRounding          = 0.0000f;
        style_->PopupBorderSize        = 1.0000f;
        style_->FramePadding           = ImVec2(4.0000f, 3.0000f);
        style_->FrameRounding          = 5.0000f;
        style_->FrameBorderSize        = 0.0000f;
        style_->ItemSpacing            = ImVec2(8.0000f, 4.0000f);
        style_->ItemInnerSpacing       = ImVec2(4.0000f, 4.0000f);
        style_->CellPadding            = ImVec2(4.0000f, 2.0000f);
        style_->IndentSpacing          = 20.0000f;
        style_->ColumnsMinSpacing      = 6.0000f;
        style_->ScrollbarSize          = 12.9000f;
        style_->ScrollbarRounding      = 9.0000f;
        style_->GrabMinSize            = 8.0000f;
        style_->GrabRounding           = 5.0000f;
        style_->TabRounding            = 4.0000f;
        style_->TabBorderSize          = 1.0000f;
        style_->ColorButtonPosition    = ImGuiDir_Right;
        style_->ButtonTextAlign        = ImVec2(0.5000f, 0.5000f);
        style_->SelectableTextAlign    = ImVec2(0.0000f, 0.0000f);

        style_->Colors[ImGuiCol_Text]                  = ImVec4(0.9020f, 0.7059f, 0.3137f, 1.0000f);
        style_->Colors[ImGuiCol_TextDisabled]          = ImVec4(0.9020f, 0.7059f, 0.3137f, 0.5020f);
        style_->Colors[ImGuiCol_WindowBg]              = ImVec4(0.0392f, 0.0549f, 0.0784f, 1.0000f);
        style_->Colors[ImGuiCol_ChildBg]               = ImVec4(0.0000f, 0.0000f, 0.0000f, 0.0000f);
        style_->Colors[ImGuiCol_PopupBg]               = ImVec4(0.0784f, 0.0784f, 0.0784f, 0.9400f);
        style_->Colors[ImGuiCol_Border]                = ImVec4(0.4275f, 0.4275f, 0.4980f, 0.5000f);
        style_->Colors[ImGuiCol_BorderShadow]          = ImVec4(0.0000f, 0.0000f, 0.0000f, 0.0000f);
        style_->Colors[ImGuiCol_FrameBg]               = ImVec4(0.0745f, 0.0902f, 0.1294f, 1.0000f);
        style_->Colors[ImGuiCol_FrameBgHovered]        = ImVec4(0.2510f, 0.2588f, 0.2784f, 1.0000f);
        style_->Colors[ImGuiCol_FrameBgActive]         = ImVec4(0.2510f, 0.2588f, 0.2784f, 1.0000f);
        style_->Colors[ImGuiCol_TitleBg]               = ImVec4(0.0000f, 0.0000f, 0.0000f, 0.5020f);
        style_->Colors[ImGuiCol_TitleBgActive]         = ImVec4(0.0588f, 0.0745f, 0.1020f, 1.0000f);
        style_->Colors[ImGuiCol_TitleBgCollapsed]      = ImVec4(0.0000f, 0.0000f, 0.0000f, 0.5020f);
        style_->Colors[ImGuiCol_MenuBarBg]             = ImVec4(0.0431f, 0.0549f, 0.0784f, 1.0000f);
        style_->Colors[ImGuiCol_ScrollbarBg]           = ImVec4(0.0196f, 0.0196f, 0.0196f, 0.5300f);
        style_->Colors[ImGuiCol_ScrollbarGrab]         = ImVec4(0.3098f, 0.3098f, 0.3098f, 1.0000f);
        style_->Colors[ImGuiCol_ScrollbarGrabHovered]  = ImVec4(0.4078f, 0.4078f, 0.4078f, 1.0000f);
        style_->Colors[ImGuiCol_ScrollbarGrabActive]   = ImVec4(0.5098f, 0.5098f, 0.5098f, 1.0000f);
        style_->Colors[ImGuiCol_CheckMark]             = ImVec4(0.2471f, 0.6980f, 1.0000f, 1.0000f);
        style_->Colors[ImGuiCol_SliderGrab]            = ImVec4(0.9020f, 0.7059f, 0.3137f, 1.0000f);
        style_->Colors[ImGuiCol_SliderGrabActive]      = ImVec4(1.0000f, 0.5608f, 0.2510f, 1.0000f);
        style_->Colors[ImGuiCol_Button]                = ImVec4(0.2510f, 0.2588f, 0.2784f, 1.0000f);
        style_->Colors[ImGuiCol_ButtonHovered]         = ImVec4(0.3098f, 0.3176f, 0.3373f, 1.0000f);
        style_->Colors[ImGuiCol_ButtonActive]          = ImVec4(0.2510f, 0.2588f, 0.2784f, 1.0000f);
        style_->Colors[ImGuiCol_Header]                = ImVec4(0.2510f, 0.2588f, 0.2784f, 1.0000f);
        style_->Colors[ImGuiCol_HeaderHovered]         = ImVec4(0.3098f, 0.3176f, 0.3373f, 1.0000f);
        style_->Colors[ImGuiCol_HeaderActive]          = ImVec4(0.3098f, 0.3176f, 0.3373f, 1.0000f);
        style_->Colors[ImGuiCol_Separator]             = ImVec4(0.2510f, 0.2588f, 0.2784f, 1.0000f);
        style_->Colors[ImGuiCol_SeparatorHovered]      = ImVec4(0.3098f, 0.3176f, 0.3373f, 1.0000f);
        style_->Colors[ImGuiCol_SeparatorActive]       = ImVec4(0.2510f, 0.2588f, 0.2784f, 1.0000f);
        style_->Colors[ImGuiCol_ResizeGrip]            = ImVec4(0.2471f, 0.6980f, 1.0000f, 1.0000f);
        style_->Colors[ImGuiCol_ResizeGripHovered]     = ImVec4(0.1333f, 0.4118f, 0.5490f, 1.0000f);
        style_->Colors[ImGuiCol_ResizeGripActive]      = ImVec4(0.1333f, 0.4118f, 0.5490f, 1.0000f);
        style_->Colors[ImGuiCol_Tab]                   = ImVec4(0.0745f, 0.0902f, 0.1294f, 1.0000f);
        style_->Colors[ImGuiCol_TabHovered]            = ImVec4(0.2510f, 0.2588f, 0.2784f, 1.0000f);
        style_->Colors[ImGuiCol_TabActive]             = ImVec4(0.2510f, 0.2588f, 0.2784f, 1.0000f);
        style_->Colors[ImGuiCol_TabUnfocused]          = ImVec4(0.0667f, 0.1020f, 0.1451f, 0.9724f);
        style_->Colors[ImGuiCol_TabUnfocusedActive]    = ImVec4(0.1333f, 0.2588f, 0.4235f, 1.0000f);
        style_->Colors[ImGuiCol_PlotLines]             = ImVec4(0.6078f, 0.6078f, 0.6078f, 1.0000f);
        style_->Colors[ImGuiCol_PlotLinesHovered]      = ImVec4(0.2471f, 0.6980f, 1.0000f, 1.0000f);
        style_->Colors[ImGuiCol_PlotHistogram]         = ImVec4(0.2471f, 0.6980f, 1.0000f, 1.0000f);
        style_->Colors[ImGuiCol_PlotHistogramHovered]  = ImVec4(0.1333f, 0.4118f, 0.5490f, 1.0000f);
        style_->Colors[ImGuiCol_TableHeaderBg]         = ImVec4(0.2510f, 0.2588f, 0.2784f, 1.0000f);
        style_->Colors[ImGuiCol_TableBorderStrong]     = ImVec4(0.2510f, 0.2588f, 0.2784f, 1.0000f);
        style_->Colors[ImGuiCol_TableBorderLight]      = ImVec4(0.0392f, 0.0549f, 0.0784f, 1.0000f);
        style_->Colors[ImGuiCol_TableRowBg]            = ImVec4(0.0392f, 0.0549f, 0.0784f, 1.0000f);
        style_->Colors[ImGuiCol_TableRowBgAlt]         = ImVec4(0.0667f, 0.1098f, 0.1608f, 1.0000f);
        style_->Colors[ImGuiCol_TextSelectedBg]        = ImVec4(0.3098f, 0.3098f, 0.3490f, 1.0000f);
        style_->Colors[ImGuiCol_DragDropTarget]        = ImVec4(0.2471f, 0.6980f, 1.0000f, 1.0000f);
        style_->Colors[ImGuiCol_NavHighlight]          = ImVec4(0.9765f, 0.2588f, 0.2588f, 1.0000f);
        style_->Colors[ImGuiCol_NavWindowingHighlight] = ImVec4(1.0000f, 1.0000f, 1.0000f, 0.7000f);
        style_->Colors[ImGuiCol_NavWindowingDimBg]     = ImVec4(0.8000f, 0.8000f, 0.8000f, 0.2000f);
        style_->Colors[ImGuiCol_ModalWindowDimBg]      = ImVec4(0.8000f, 0.8000f, 0.8000f, 0.3500f);
    
        SYS_INFO("GuiMgr", "'Ayu Dark' theme applied.");
    
    }

    void GuiMgr::Style_Confy() {

        StyleColorsDark();
        titlebar_dark_mode(true);

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

    void GuiMgr::Style_DarkCyan() {
        
        StyleColorsDark();
        titlebar_dark_mode(true);
        
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
        style_->Colors[ImGuiCol_ButtonHovered] = ImVec4(0.9000f, 0.9000f, 0.9000f, 1.0000f);    // <- Modificado
        style_->Colors[ImGuiCol_ButtonActive]  = ImVec4(0.8000f, 0.8000f, 0.8000f, 1.0000f);    // <- Modificado
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
        titlebar_dark_mode(true);
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
        titlebar_dark_mode(false);
        SYS_INFO("GuiMgr", "'DefaultLight' theme applied.");
    }
    
    void GuiMgr::Style_Everforest() {

        StyleColorsDark();
        titlebar_dark_mode(true);
        
        style_->Alpha                  = 1.0f;
        style_->DisabledAlpha          = 0.6f;
        style_->WindowPadding          = ImVec2(6.0f, 3.0f);
        style_->WindowRounding         = 6.0f;
        style_->WindowBorderSize       = 1.0f;
        style_->WindowMinSize          = ImVec2(32.0f, 32.0f);
        style_->WindowTitleAlign       = ImVec2(0.5f, 0.5f);
        style_->WindowMenuButtonPosition = ImGuiDir_Left;
        style_->ChildRounding          = 0.0f;
        style_->ChildBorderSize        = 1.0f;
        style_->PopupRounding          = 0.0f;
        style_->PopupBorderSize        = 1.0f;
        style_->FramePadding           = ImVec2(5.0f, 1.0f);
        style_->FrameRounding          = 3.0f;
        style_->FrameBorderSize        = 1.0f;
        style_->ItemSpacing            = ImVec2(8.0f, 4.0f);
        style_->ItemInnerSpacing       = ImVec2(4.0f, 4.0f);
        style_->CellPadding            = ImVec2(4.0f, 2.0f);
        style_->IndentSpacing          = 21.0f;
        style_->ColumnsMinSpacing      = 6.0f;
        style_->ScrollbarSize          = 13.0f;
        style_->ScrollbarRounding      = 16.0f;
        style_->GrabMinSize            = 20.0f;
        style_->GrabRounding           = 2.0f;
        style_->TabRounding            = 4.0f;
        style_->TabBorderSize          = 1.0f;
        style_->ColorButtonPosition    = ImGuiDir_Right;
        style_->ButtonTextAlign        = ImVec2(0.5f, 0.5f);
        style_->SelectableTextAlign    = ImVec2(0.0f, 0.0f);

        style_->Colors[ImGuiCol_Text]                  = ImVec4(0.8745f, 0.8706f, 0.8392f, 1.0000f);
        style_->Colors[ImGuiCol_TextDisabled]          = ImVec4(0.5843f, 0.5725f, 0.5216f, 1.0000f);
        style_->Colors[ImGuiCol_WindowBg]              = ImVec4(0.2353f, 0.2196f, 0.2118f, 1.0000f);
        style_->Colors[ImGuiCol_ChildBg]               = ImVec4(0.2353f, 0.2196f, 0.2118f, 1.0000f);
        style_->Colors[ImGuiCol_PopupBg]               = ImVec4(0.2353f, 0.2196f, 0.2118f, 1.0000f);
        style_->Colors[ImGuiCol_Border]                = ImVec4(0.3137f, 0.2863f, 0.2706f, 1.0000f);
        style_->Colors[ImGuiCol_BorderShadow]          = ImVec4(0.2353f, 0.2196f, 0.2118f, 0.0000f);
        style_->Colors[ImGuiCol_FrameBg]               = ImVec4(0.3137f, 0.2863f, 0.2706f, 1.0000f);
        style_->Colors[ImGuiCol_FrameBgHovered]        = ImVec4(0.4000f, 0.3608f, 0.3294f, 1.0000f);
        style_->Colors[ImGuiCol_FrameBgActive]         = ImVec4(0.4863f, 0.4353f, 0.3922f, 1.0000f);
        style_->Colors[ImGuiCol_TitleBg]               = ImVec4(0.2353f, 0.2196f, 0.2118f, 1.0000f);
        style_->Colors[ImGuiCol_TitleBgActive]         = ImVec4(0.3137f, 0.2863f, 0.2706f, 1.0000f);
        style_->Colors[ImGuiCol_TitleBgCollapsed]      = ImVec4(0.2353f, 0.2196f, 0.2118f, 1.0000f);
        style_->Colors[ImGuiCol_MenuBarBg]             = ImVec4(0.3137f, 0.2863f, 0.2706f, 1.0000f);
        style_->Colors[ImGuiCol_ScrollbarBg]           = ImVec4(0.2353f, 0.2196f, 0.2118f, 1.0000f);
        style_->Colors[ImGuiCol_ScrollbarGrab]         = ImVec4(0.4863f, 0.4353f, 0.3922f, 1.0000f);
        style_->Colors[ImGuiCol_ScrollbarGrabHovered]  = ImVec4(0.4000f, 0.3608f, 0.3294f, 1.0000f);
        style_->Colors[ImGuiCol_ScrollbarGrabActive]   = ImVec4(0.3137f, 0.2863f, 0.2706f, 1.0000f);
        style_->Colors[ImGuiCol_CheckMark]             = ImVec4(0.5961f, 0.5922f, 0.1020f, 1.0000f);
        style_->Colors[ImGuiCol_SliderGrab]            = ImVec4(0.5961f, 0.5922f, 0.1020f, 1.0000f);
        style_->Colors[ImGuiCol_SliderGrabActive]      = ImVec4(0.7412f, 0.7176f, 0.4196f, 1.0000f);
        style_->Colors[ImGuiCol_Button]                = ImVec4(0.4000f, 0.3608f, 0.3294f, 1.0000f);
        style_->Colors[ImGuiCol_ButtonHovered]         = ImVec4(0.4863f, 0.4353f, 0.3922f, 1.0000f);
        style_->Colors[ImGuiCol_ButtonActive]          = ImVec4(0.7412f, 0.7176f, 0.4196f, 1.0000f);
        style_->Colors[ImGuiCol_Header]                = ImVec4(0.4000f, 0.3608f, 0.3294f, 1.0000f);
        style_->Colors[ImGuiCol_HeaderHovered]         = ImVec4(0.4863f, 0.4353f, 0.3922f, 1.0000f);
        style_->Colors[ImGuiCol_HeaderActive]          = ImVec4(0.7412f, 0.7176f, 0.4196f, 1.0000f);
        style_->Colors[ImGuiCol_Separator]             = ImVec4(0.7412f, 0.7176f, 0.4196f, 1.0000f);
        style_->Colors[ImGuiCol_SeparatorHovered]      = ImVec4(0.4863f, 0.4353f, 0.3922f, 1.0000f);
        style_->Colors[ImGuiCol_SeparatorActive]       = ImVec4(0.7412f, 0.7176f, 0.4196f, 1.0000f);
        style_->Colors[ImGuiCol_ResizeGrip]            = ImVec4(0.4000f, 0.3608f, 0.3294f, 1.0000f);
        style_->Colors[ImGuiCol_ResizeGripHovered]     = ImVec4(0.4863f, 0.4353f, 0.3922f, 1.0000f);
        style_->Colors[ImGuiCol_ResizeGripActive]      = ImVec4(0.7412f, 0.7176f, 0.4196f, 1.0000f);
        style_->Colors[ImGuiCol_Tab]                   = ImVec4(0.3137f, 0.2863f, 0.2706f, 1.0000f);
        style_->Colors[ImGuiCol_TabHovered]            = ImVec4(0.4000f, 0.3608f, 0.3294f, 1.0000f);
        style_->Colors[ImGuiCol_TabActive]             = ImVec4(0.4863f, 0.4353f, 0.3922f, 1.0000f);
        style_->Colors[ImGuiCol_TabUnfocused]          = ImVec4(0.2353f, 0.2196f, 0.2118f, 0.9725f);
        style_->Colors[ImGuiCol_TabUnfocusedActive]    = ImVec4(0.3137f, 0.2863f, 0.2706f, 1.0000f);
        style_->Colors[ImGuiCol_PlotLines]             = ImVec4(0.7412f, 0.7176f, 0.4196f, 1.0000f);
        style_->Colors[ImGuiCol_PlotLinesHovered]      = ImVec4(0.8392f, 0.7490f, 0.4000f, 1.0000f);
        style_->Colors[ImGuiCol_PlotHistogram]         = ImVec4(0.7412f, 0.7176f, 0.4196f, 1.0000f);
        style_->Colors[ImGuiCol_PlotHistogramHovered]  = ImVec4(0.8392f, 0.7490f, 0.4000f, 1.0000f);
        style_->Colors[ImGuiCol_TableHeaderBg]         = ImVec4(0.8392f, 0.7490f, 0.4000f, 0.6094f);
        style_->Colors[ImGuiCol_TableBorderStrong]     = ImVec4(0.3098f, 0.3098f, 0.3490f, 1.0000f);
        style_->Colors[ImGuiCol_TableBorderLight]      = ImVec4(0.2275f, 0.2275f, 0.2471f, 1.0000f);
        style_->Colors[ImGuiCol_TableRowBg]            = ImVec4(0.0000f, 0.0000f, 0.0000f, 0.0000f);
        style_->Colors[ImGuiCol_TableRowBgAlt]         = ImVec4(1.0000f, 1.0000f, 1.0000f, 0.0600f);
        style_->Colors[ImGuiCol_TextSelectedBg]        = ImVec4(0.8392f, 0.7490f, 0.4000f, 0.4314f);
        style_->Colors[ImGuiCol_DragDropTarget]        = ImVec4(0.8392f, 0.7490f, 0.4000f, 0.9020f);
        style_->Colors[ImGuiCol_NavHighlight]          = ImVec4(0.2353f, 0.2196f, 0.2118f, 1.0000f);
        style_->Colors[ImGuiCol_NavWindowingHighlight] = ImVec4(1.0000f, 1.0000f, 1.0000f, 0.7000f);
        style_->Colors[ImGuiCol_NavWindowingDimBg]     = ImVec4(0.8000f, 0.8000f, 0.8000f, 0.2000f);
        style_->Colors[ImGuiCol_ModalWindowDimBg]      = ImVec4(0.8000f, 0.8000f, 0.8000f, 0.3500f);
        
        SYS_INFO("GuiMgr", "'Everforest' theme applied.");
    }

    void GuiMgr::Style_FutureDark() {

        StyleColorsDark();
        titlebar_dark_mode(true);

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

    void GuiMgr::Style_Gold() {

        StyleColorsDark();
        titlebar_dark_mode(true);

        style_->Alpha                  = 1.0f;
        style_->DisabledAlpha          = 0.6f;
        style_->WindowPadding          = ImVec2(8.0f, 8.0f);
        style_->WindowRounding         = 4.0f;
        style_->WindowBorderSize       = 1.0f;
        style_->WindowMinSize          = ImVec2(32.0f, 32.0f);
        style_->WindowTitleAlign       = ImVec2(1.0f, 0.5f);
        style_->WindowMenuButtonPosition = ImGuiDir_Right;
        style_->ChildRounding          = 0.0f;
        style_->ChildBorderSize        = 1.0f;
        style_->PopupRounding          = 4.0f;
        style_->PopupBorderSize        = 1.0f;
        style_->FramePadding           = ImVec2(4.0f, 2.0f);
        style_->FrameRounding          = 4.0f;
        style_->FrameBorderSize        = 0.0f;
        style_->ItemSpacing            = ImVec2(10.0f, 2.0f);
        style_->ItemInnerSpacing       = ImVec2(4.0f, 4.0f);
        style_->CellPadding            = ImVec2(4.0f, 2.0f);
        style_->IndentSpacing          = 12.0f;
        style_->ColumnsMinSpacing      = 6.0f;
        style_->ScrollbarSize          = 10.0f;
        style_->ScrollbarRounding      = 6.0f;
        style_->GrabMinSize            = 10.0f;
        style_->GrabRounding           = 4.0f;
        style_->TabRounding            = 4.0f;
        style_->TabBorderSize          = 0.0f;
        style_->ColorButtonPosition    = ImGuiDir_Right;
        style_->ButtonTextAlign        = ImVec2(0.5f, 0.5f);
        style_->SelectableTextAlign    = ImVec2(0.0f, 0.0f);

        style_->Colors[ImGuiCol_Text]                  = ImVec4(0.9176f, 0.9176f, 0.9176f, 1.0000f);
        style_->Colors[ImGuiCol_TextDisabled]          = ImVec4(0.4392f, 0.4392f, 0.4392f, 1.0000f);
        style_->Colors[ImGuiCol_WindowBg]              = ImVec4(0.0588f, 0.0588f, 0.0588f, 1.0000f);
        style_->Colors[ImGuiCol_ChildBg]               = ImVec4(0.0000f, 0.0000f, 0.0000f, 0.0000f);
        style_->Colors[ImGuiCol_PopupBg]               = ImVec4(0.0784f, 0.0784f, 0.0784f, 0.9400f);
        style_->Colors[ImGuiCol_Border]                = ImVec4(0.5098f, 0.3569f, 0.1490f, 1.0000f);
        style_->Colors[ImGuiCol_BorderShadow]          = ImVec4(0.0000f, 0.0000f, 0.0000f, 0.0000f);
        style_->Colors[ImGuiCol_FrameBg]               = ImVec4(0.1098f, 0.1098f, 0.1098f, 1.0000f);
        style_->Colors[ImGuiCol_FrameBgHovered]        = ImVec4(0.5098f, 0.3569f, 0.1490f, 1.0000f);
        style_->Colors[ImGuiCol_FrameBgActive]         = ImVec4(0.7765f, 0.5490f, 0.2078f, 1.0000f);
        style_->Colors[ImGuiCol_TitleBg]               = ImVec4(0.5098f, 0.3569f, 0.1490f, 1.0000f);
        style_->Colors[ImGuiCol_TitleBgActive]         = ImVec4(0.9098f, 0.6392f, 0.1294f, 1.0000f);
        style_->Colors[ImGuiCol_TitleBgCollapsed]      = ImVec4(0.0000f, 0.0000f, 0.0000f, 0.5100f);
        style_->Colors[ImGuiCol_MenuBarBg]             = ImVec4(0.1098f, 0.1098f, 0.1098f, 1.0000f);
        style_->Colors[ImGuiCol_ScrollbarBg]           = ImVec4(0.0588f, 0.0588f, 0.0588f, 0.5300f);
        style_->Colors[ImGuiCol_ScrollbarGrab]         = ImVec4(0.2078f, 0.2078f, 0.2078f, 1.0000f);
        style_->Colors[ImGuiCol_ScrollbarGrabHovered]  = ImVec4(0.4667f, 0.4667f, 0.4667f, 1.0000f);
        style_->Colors[ImGuiCol_ScrollbarGrabActive]   = ImVec4(0.8078f, 0.8275f, 0.8078f, 1.0000f);
        style_->Colors[ImGuiCol_CheckMark]             = ImVec4(0.7765f, 0.5490f, 0.2078f, 1.0000f);
        style_->Colors[ImGuiCol_SliderGrab]            = ImVec4(0.9098f, 0.6392f, 0.1294f, 1.0000f);
        style_->Colors[ImGuiCol_SliderGrabActive]      = ImVec4(0.9098f, 0.6392f, 0.1294f, 1.0000f);
        style_->Colors[ImGuiCol_Button]                = ImVec4(0.5098f, 0.3569f, 0.1490f, 1.0000f);
        style_->Colors[ImGuiCol_ButtonHovered]         = ImVec4(0.9098f, 0.6392f, 0.1294f, 1.0000f);
        style_->Colors[ImGuiCol_ButtonActive]          = ImVec4(0.7765f, 0.5490f, 0.2078f, 1.0000f);
        style_->Colors[ImGuiCol_Header]                = ImVec4(0.5098f, 0.3569f, 0.1490f, 1.0000f);
        style_->Colors[ImGuiCol_HeaderHovered]         = ImVec4(0.9098f, 0.6392f, 0.1294f, 1.0000f);
        style_->Colors[ImGuiCol_HeaderActive]          = ImVec4(0.9294f, 0.6471f, 0.1373f, 1.0000f);
        style_->Colors[ImGuiCol_Separator]             = ImVec4(0.2078f, 0.2078f, 0.2078f, 1.0000f);
        style_->Colors[ImGuiCol_SeparatorHovered]      = ImVec4(0.9098f, 0.6392f, 0.1294f, 1.0000f);
        style_->Colors[ImGuiCol_SeparatorActive]       = ImVec4(0.7765f, 0.5490f, 0.2078f, 1.0000f);
        style_->Colors[ImGuiCol_ResizeGrip]            = ImVec4(0.2078f, 0.2078f, 0.2078f, 1.0000f);
        style_->Colors[ImGuiCol_ResizeGripHovered]     = ImVec4(0.9098f, 0.6392f, 0.1294f, 1.0000f);
        style_->Colors[ImGuiCol_ResizeGripActive]      = ImVec4(0.7765f, 0.5490f, 0.2078f, 1.0000f);
        style_->Colors[ImGuiCol_Tab]                   = ImVec4(0.5098f, 0.3569f, 0.1490f, 1.0000f);
        style_->Colors[ImGuiCol_TabHovered]            = ImVec4(0.9098f, 0.6392f, 0.1294f, 1.0000f);
        style_->Colors[ImGuiCol_TabActive]             = ImVec4(0.7765f, 0.5490f, 0.2078f, 1.0000f);
        style_->Colors[ImGuiCol_TabUnfocused]          = ImVec4(0.0667f, 0.0980f, 0.1490f, 0.9700f);
        style_->Colors[ImGuiCol_TabUnfocusedActive]    = ImVec4(0.1373f, 0.2588f, 0.4196f, 1.0000f);
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
      
        SYS_INFO("GuiMgr", "'Gold' theme applied.");
    
    }

    void GuiMgr::Style_HazyDark() {

        StyleColorsDark();
        titlebar_dark_mode(true);

        style_->Alpha                  = 1.0000f;
        style_->DisabledAlpha          = 0.6000f;
        style_->WindowPadding          = ImVec2(5.5000f, 8.3000f);
        style_->WindowRounding         = 4.5000f;
        style_->WindowBorderSize       = 1.0000f;
        style_->WindowMinSize          = ImVec2(32.0000f, 32.0000f);
        style_->WindowTitleAlign       = ImVec2(0.0000f, 0.5000f);
        style_->WindowMenuButtonPosition = ImGuiDir_Left;
        style_->ChildRounding          = 3.2000f;
        style_->ChildBorderSize        = 1.0000f;
        style_->PopupRounding          = 2.7000f;
        style_->PopupBorderSize        = 1.0000f;
        style_->FramePadding           = ImVec2(4.0000f, 3.0000f);
        style_->FrameRounding          = 2.4000f;
        style_->FrameBorderSize        = 0.0000f;
        style_->ItemSpacing            = ImVec2(8.0000f, 4.0000f);
        style_->ItemInnerSpacing       = ImVec2(4.0000f, 4.0000f);
        style_->CellPadding            = ImVec2(4.0000f, 2.0000f);
        style_->IndentSpacing          = 21.0000f;
        style_->ColumnsMinSpacing      = 6.0000f;
        style_->ScrollbarSize          = 14.0000f;
        style_->ScrollbarRounding      = 9.0000f;
        style_->GrabMinSize            = 10.0000f;
        style_->GrabRounding           = 3.2000f;
        style_->TabRounding            = 3.5000f;
        style_->TabBorderSize          = 1.0000f;
        style_->ColorButtonPosition    = ImGuiDir_Right;
        style_->ButtonTextAlign        = ImVec2(0.5000f, 0.5000f);
        style_->SelectableTextAlign    = ImVec2(0.0000f, 0.0000f);

        style_->Colors[ImGuiCol_Text]                  = ImVec4(1.0000f, 1.0000f, 1.0000f, 1.0000f);
        style_->Colors[ImGuiCol_TextDisabled]          = ImVec4(0.4980f, 0.4980f, 0.4980f, 1.0000f);
        style_->Colors[ImGuiCol_WindowBg]              = ImVec4(0.0588f, 0.0588f, 0.0588f, 0.9400f);
        style_->Colors[ImGuiCol_ChildBg]               = ImVec4(0.0000f, 0.0000f, 0.0000f, 0.0000f);
        style_->Colors[ImGuiCol_PopupBg]               = ImVec4(0.0784f, 0.0784f, 0.0784f, 0.9400f);
        style_->Colors[ImGuiCol_Border]                = ImVec4(0.4275f, 0.4275f, 0.4980f, 0.5000f);
        style_->Colors[ImGuiCol_BorderShadow]          = ImVec4(0.0000f, 0.0000f, 0.0000f, 0.0000f);
        style_->Colors[ImGuiCol_FrameBg]               = ImVec4(0.1373f, 0.1725f, 0.2275f, 0.5400f);
        style_->Colors[ImGuiCol_FrameBgHovered]        = ImVec4(0.2118f, 0.2549f, 0.3020f, 0.4000f);
        style_->Colors[ImGuiCol_FrameBgActive]         = ImVec4(0.0431f, 0.0471f, 0.0471f, 0.6700f);
        style_->Colors[ImGuiCol_TitleBg]               = ImVec4(0.0392f, 0.0392f, 0.0392f, 1.0000f);
        style_->Colors[ImGuiCol_TitleBgActive]         = ImVec4(0.0784f, 0.0824f, 0.0902f, 1.0000f);
        style_->Colors[ImGuiCol_TitleBgCollapsed]      = ImVec4(0.0000f, 0.0000f, 0.0000f, 0.5100f);
        style_->Colors[ImGuiCol_MenuBarBg]             = ImVec4(0.1373f, 0.1373f, 0.1373f, 1.0000f);
        style_->Colors[ImGuiCol_ScrollbarBg]           = ImVec4(0.0196f, 0.0196f, 0.0196f, 0.5300f);
        style_->Colors[ImGuiCol_ScrollbarGrab]         = ImVec4(0.3098f, 0.3098f, 0.3098f, 1.0000f);
        style_->Colors[ImGuiCol_ScrollbarGrabHovered]  = ImVec4(0.4078f, 0.4078f, 0.4078f, 1.0000f);
        style_->Colors[ImGuiCol_ScrollbarGrabActive]   = ImVec4(0.5098f, 0.5098f, 0.5098f, 1.0000f);
        style_->Colors[ImGuiCol_CheckMark]             = ImVec4(0.7176f, 0.7843f, 0.8431f, 1.0000f);
        style_->Colors[ImGuiCol_SliderGrab]            = ImVec4(0.4784f, 0.5255f, 0.5725f, 1.0000f);
        style_->Colors[ImGuiCol_SliderGrabActive]      = ImVec4(0.2902f, 0.3176f, 0.3529f, 1.0000f);
        style_->Colors[ImGuiCol_Button]                = ImVec4(0.1490f, 0.1608f, 0.1765f, 0.4000f);
        style_->Colors[ImGuiCol_ButtonHovered]         = ImVec4(0.1373f, 0.1451f, 0.1569f, 1.0000f);
        style_->Colors[ImGuiCol_ButtonActive]          = ImVec4(0.0784f, 0.0863f, 0.0902f, 1.0000f);
        style_->Colors[ImGuiCol_Header]                = ImVec4(0.1961f, 0.2157f, 0.2392f, 0.3100f);
        style_->Colors[ImGuiCol_HeaderHovered]         = ImVec4(0.1647f, 0.1765f, 0.1922f, 0.8000f);
        style_->Colors[ImGuiCol_HeaderActive]          = ImVec4(0.0745f, 0.0824f, 0.0902f, 1.0000f);
        style_->Colors[ImGuiCol_Separator]             = ImVec4(0.4275f, 0.4275f, 0.4980f, 0.5000f);
        style_->Colors[ImGuiCol_SeparatorHovered]      = ImVec4(0.2392f, 0.3255f, 0.4235f, 0.7800f);
        style_->Colors[ImGuiCol_SeparatorActive]       = ImVec4(0.2745f, 0.3804f, 0.4980f, 1.0000f);
        style_->Colors[ImGuiCol_ResizeGrip]            = ImVec4(0.2902f, 0.3294f, 0.3765f, 0.2000f);
        style_->Colors[ImGuiCol_ResizeGripHovered]     = ImVec4(0.2392f, 0.2980f, 0.3686f, 0.6700f);
        style_->Colors[ImGuiCol_ResizeGripActive]      = ImVec4(0.1647f, 0.1765f, 0.1882f, 0.9500f);
        style_->Colors[ImGuiCol_Tab]                   = ImVec4(0.1176f, 0.1255f, 0.1333f, 0.8620f);
        style_->Colors[ImGuiCol_TabHovered]            = ImVec4(0.3294f, 0.4078f, 0.5020f, 0.8000f);
        style_->Colors[ImGuiCol_TabActive]             = ImVec4(0.2431f, 0.2471f, 0.2549f, 1.0000f);
        style_->Colors[ImGuiCol_TabUnfocused]          = ImVec4(0.0667f, 0.1020f, 0.1451f, 0.9724f);
        style_->Colors[ImGuiCol_TabUnfocusedActive]    = ImVec4(0.1333f, 0.2588f, 0.4235f, 1.0000f);
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
   
        SYS_INFO("GuiMgr", "'Hazy Dark' theme applied.");
    }

    void GuiMgr::Style_KazamsCherry () {

        StyleColorsDark();
        titlebar_dark_mode(true);

        style_->Alpha                  = 1.0000f;
        style_->DisabledAlpha          = 0.6000f;
        style_->WindowPadding          = ImVec2(6.0000f, 3.0000f);
        style_->WindowRounding         = 0.0000f;
        style_->WindowBorderSize       = 1.0000f;
        style_->WindowMinSize          = ImVec2(32.0000f, 32.0000f);
        style_->WindowTitleAlign       = ImVec2(0.5000f, 0.5000f);
        style_->WindowMenuButtonPosition = ImGuiDir_Left;
        style_->ChildRounding          = 0.0000f;
        style_->ChildBorderSize        = 1.0000f;
        style_->PopupRounding          = 0.0000f;
        style_->PopupBorderSize        = 1.0000f;
        style_->FramePadding           = ImVec2(5.0000f, 5.0000f);
        style_->FrameRounding          = 1.0000f;
        style_->FrameBorderSize        = 0.0000f;
        style_->ItemSpacing            = ImVec2(7.0000f, 1.0000f);
        style_->ItemInnerSpacing       = ImVec2(1.0000f, 1.0000f);
        style_->CellPadding            = ImVec2(4.0000f, 4.0000f);
        style_->IndentSpacing          = 6.0000f;
        style_->ColumnsMinSpacing      = 6.0000f;
        style_->ScrollbarSize          = 13.0000f;
        style_->ScrollbarRounding      = 16.0000f;
        style_->GrabMinSize            = 20.0000f;
        style_->GrabRounding           = 2.0000f;
        style_->TabRounding            = 2.0000f;
        style_->TabBorderSize          = 0.0000f;
        style_->ColorButtonPosition    = ImGuiDir_Right;
        style_->ButtonTextAlign        = ImVec2(0.5000f, 0.5000f);
        style_->SelectableTextAlign    = ImVec2(0.0000f, 0.0000f);

        style_->Colors[ImGuiCol_Text]                  = ImVec4(0.8588f, 0.9294f, 0.8863f, 0.8800f);
        style_->Colors[ImGuiCol_TextDisabled]          = ImVec4(0.8588f, 0.9294f, 0.8863f, 0.2800f);
        style_->Colors[ImGuiCol_WindowBg]              = ImVec4(0.1294f, 0.1373f, 0.1686f, 1.0000f);
        style_->Colors[ImGuiCol_ChildBg]               = ImVec4(0.0000f, 0.0000f, 0.0000f, 0.0000f);
        style_->Colors[ImGuiCol_PopupBg]               = ImVec4(0.2000f, 0.2196f, 0.2667f, 0.9000f);
        style_->Colors[ImGuiCol_Border]                = ImVec4(0.5373f, 0.4784f, 0.2549f, 0.1620f);
        style_->Colors[ImGuiCol_BorderShadow]          = ImVec4(0.0000f, 0.0000f, 0.0000f, 0.0000f);
        style_->Colors[ImGuiCol_FrameBg]               = ImVec4(0.2000f, 0.2196f, 0.2667f, 1.0000f);
        style_->Colors[ImGuiCol_FrameBgHovered]        = ImVec4(0.4549f, 0.1961f, 0.2980f, 0.7800f);
        style_->Colors[ImGuiCol_FrameBgActive]         = ImVec4(0.4549f, 0.1961f, 0.2980f, 1.0000f);
        style_->Colors[ImGuiCol_TitleBg]               = ImVec4(0.2314f, 0.2000f, 0.2706f, 1.0000f);
        style_->Colors[ImGuiCol_TitleBgActive]         = ImVec4(0.5020f, 0.0745f, 0.2549f, 1.0000f);
        style_->Colors[ImGuiCol_TitleBgCollapsed]      = ImVec4(0.2000f, 0.2196f, 0.2667f, 0.7500f);
        style_->Colors[ImGuiCol_MenuBarBg]             = ImVec4(0.2000f, 0.2196f, 0.2667f, 0.4700f);
        style_->Colors[ImGuiCol_ScrollbarBg]           = ImVec4(0.2000f, 0.2196f, 0.2667f, 1.0000f);
        style_->Colors[ImGuiCol_ScrollbarGrab]         = ImVec4(0.0863f, 0.1490f, 0.1569f, 1.0000f);
        style_->Colors[ImGuiCol_ScrollbarGrabHovered]  = ImVec4(0.4549f, 0.1961f, 0.2980f, 0.7800f);
        style_->Colors[ImGuiCol_ScrollbarGrabActive]   = ImVec4(0.4549f, 0.1961f, 0.2980f, 1.0000f);
        style_->Colors[ImGuiCol_CheckMark]             = ImVec4(0.7098f, 0.2196f, 0.2667f, 1.0000f);
        style_->Colors[ImGuiCol_SliderGrab]            = ImVec4(0.4667f, 0.7686f, 0.8275f, 0.1400f);
        style_->Colors[ImGuiCol_SliderGrabActive]      = ImVec4(0.7098f, 0.2196f, 0.2667f, 1.0000f);
        style_->Colors[ImGuiCol_Button]                = ImVec4(0.4667f, 0.7686f, 0.8275f, 0.1400f);
        style_->Colors[ImGuiCol_ButtonHovered]         = ImVec4(0.4549f, 0.1961f, 0.2980f, 0.8600f);
        style_->Colors[ImGuiCol_ButtonActive]          = ImVec4(0.4549f, 0.1961f, 0.2980f, 1.0000f);
        style_->Colors[ImGuiCol_Header]                = ImVec4(0.4549f, 0.1961f, 0.2980f, 0.7600f);
        style_->Colors[ImGuiCol_HeaderHovered]         = ImVec4(0.4549f, 0.1961f, 0.2980f, 0.8600f);
        style_->Colors[ImGuiCol_HeaderActive]          = ImVec4(0.5020f, 0.0745f, 0.2549f, 1.0000f);
        style_->Colors[ImGuiCol_Separator]             = ImVec4(0.4275f, 0.4275f, 0.4980f, 0.5000f);
        style_->Colors[ImGuiCol_SeparatorHovered]      = ImVec4(0.5176f, 0.2196f, 0.3373f, 0.8841f);
        style_->Colors[ImGuiCol_SeparatorActive]       = ImVec4(0.4549f, 0.1961f, 0.2980f, 1.0000f);
        style_->Colors[ImGuiCol_ResizeGrip]            = ImVec4(0.4667f, 0.7686f, 0.8275f, 0.0400f);
        style_->Colors[ImGuiCol_ResizeGripHovered]     = ImVec4(0.4549f, 0.1961f, 0.2980f, 0.7800f);
        style_->Colors[ImGuiCol_ResizeGripActive]      = ImVec4(0.4549f, 0.1961f, 0.2980f, 1.0000f);
        style_->Colors[ImGuiCol_Tab]                   = ImVec4(0.7098f, 0.2196f, 0.2667f, 0.8283f);
        style_->Colors[ImGuiCol_TabHovered]            = ImVec4(0.7098f, 0.2196f, 0.2667f, 1.0000f);
        style_->Colors[ImGuiCol_TabActive]             = ImVec4(0.7098f, 0.2196f, 0.2667f, 1.0000f);
        style_->Colors[ImGuiCol_TabUnfocused]          = ImVec4(0.0667f, 0.1020f, 0.1451f, 0.9724f);
        style_->Colors[ImGuiCol_TabUnfocusedActive]    = ImVec4(0.4549f, 0.1961f, 0.2980f, 1.0000f);
        style_->Colors[ImGuiCol_PlotLines]             = ImVec4(0.8588f, 0.9294f, 0.8863f, 0.6300f);
        style_->Colors[ImGuiCol_PlotLinesHovered]      = ImVec4(0.4549f, 0.1961f, 0.2980f, 1.0000f);
        style_->Colors[ImGuiCol_PlotHistogram]         = ImVec4(0.8588f, 0.9294f, 0.8863f, 0.6300f);
        style_->Colors[ImGuiCol_PlotHistogramHovered]  = ImVec4(0.4549f, 0.1961f, 0.2980f, 1.0000f);
        style_->Colors[ImGuiCol_TableHeaderBg]         = ImVec4(0.1882f, 0.1882f, 0.2000f, 1.0000f);
        style_->Colors[ImGuiCol_TableBorderStrong]     = ImVec4(0.3098f, 0.3098f, 0.3490f, 1.0000f);
        style_->Colors[ImGuiCol_TableBorderLight]      = ImVec4(0.2275f, 0.2275f, 0.2471f, 1.0000f);
        style_->Colors[ImGuiCol_TableRowBg]            = ImVec4(0.0000f, 0.0000f, 0.0000f, 0.0000f);
        style_->Colors[ImGuiCol_TableRowBgAlt]         = ImVec4(1.0000f, 1.0000f, 1.0000f, 0.0600f);
        style_->Colors[ImGuiCol_TextSelectedBg]        = ImVec4(0.4549f, 0.1961f, 0.2980f, 0.4300f);
        style_->Colors[ImGuiCol_DragDropTarget]        = ImVec4(1.0000f, 1.0000f, 0.0000f, 0.9000f);
        style_->Colors[ImGuiCol_NavHighlight]          = ImVec4(0.2588f, 0.5882f, 0.9765f, 1.0000f);
        style_->Colors[ImGuiCol_NavWindowingHighlight] = ImVec4(1.0000f, 1.0000f, 1.0000f, 0.7000f);
        style_->Colors[ImGuiCol_NavWindowingDimBg]     = ImVec4(0.8000f, 0.8000f, 0.8000f, 0.2000f);
        style_->Colors[ImGuiCol_ModalWindowDimBg]      = ImVec4(0.8000f, 0.8000f, 0.8000f, 0.3500f);
    
        SYS_INFO("GuiMgr", "'Kazam's Cherry' theme applied.");

    }

    void GuiMgr::Style_LightOrange() {

        StyleColorsLight();
        titlebar_dark_mode(false);

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
        style_->Colors[ImGuiCol_ButtonActive]   = ImVec4(0.8554f, 0.5585f, 0.2226f, 1.0000f);   // <- Modificado
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

    void GuiMgr::Style_QuickMinimalLook() {

        StyleColorsDark();
        titlebar_dark_mode(true);

        style_->Alpha                  = 1.0000f;
        style_->DisabledAlpha          = 0.3000f;
        style_->WindowPadding          = ImVec2(6.5000f, 2.7000f);
        style_->WindowRounding         = 0.0000f;
        style_->WindowBorderSize       = 1.0000f;
        style_->WindowMinSize          = ImVec2(20.0000f, 32.0000f);
        style_->WindowTitleAlign       = ImVec2(0.0000f, 0.6000f);
        style_->WindowMenuButtonPosition = ImGuiDir_None;
        style_->ChildRounding          = 0.0000f;
        style_->ChildBorderSize        = 1.0000f;
        style_->PopupRounding          = 10.1000f;
        style_->PopupBorderSize        = 1.0000f;
        style_->FramePadding           = ImVec2(20.0000f, 3.5000f);
        style_->FrameRounding          = 0.0000f;
        style_->FrameBorderSize        = 0.0000f;
        style_->ItemSpacing            = ImVec2(4.4000f, 4.0000f);
        style_->ItemInnerSpacing       = ImVec2(4.6000f, 3.6000f);
        style_->CellPadding            = ImVec2(3.1000f, 6.3000f);
        style_->IndentSpacing          = 4.4000f;
        style_->ColumnsMinSpacing      = 5.4000f;
        style_->ScrollbarSize          = 8.8000f;
        style_->ScrollbarRounding      = 9.0000f;
        style_->GrabMinSize            = 9.4000f;
        style_->GrabRounding           = 0.0000f;
        style_->TabRounding            = 0.0000f;
        style_->TabBorderSize          = 0.0000f;
        style_->ColorButtonPosition    = ImGuiDir_Right;
        style_->ButtonTextAlign        = ImVec2(0.5000f, 0.5000f);
        style_->SelectableTextAlign    = ImVec2(0.0000f, 0.0000f);

        style_->Colors[ImGuiCol_Text]                  = ImVec4(1.0000f, 1.0000f, 1.0000f, 1.0000f);
        style_->Colors[ImGuiCol_TextDisabled]          = ImVec4(0.4980f, 0.4980f, 0.4980f, 1.0000f);
        style_->Colors[ImGuiCol_WindowBg]              = ImVec4(0.0510f, 0.0353f, 0.0392f, 1.0000f);
        style_->Colors[ImGuiCol_ChildBg]               = ImVec4(0.0000f, 0.0000f, 0.0000f, 0.0000f);
        style_->Colors[ImGuiCol_PopupBg]               = ImVec4(0.0784f, 0.0784f, 0.0784f, 0.9400f);
        style_->Colors[ImGuiCol_Border]                = ImVec4(0.1020f, 0.1020f, 0.1020f, 0.5000f);
        style_->Colors[ImGuiCol_BorderShadow]          = ImVec4(0.0000f, 0.0000f, 0.0000f, 0.0000f);
        style_->Colors[ImGuiCol_FrameBg]               = ImVec4(0.1608f, 0.1490f, 0.1922f, 1.0000f);
        style_->Colors[ImGuiCol_FrameBgHovered]        = ImVec4(0.2784f, 0.2510f, 0.3373f, 1.0000f);
        style_->Colors[ImGuiCol_FrameBgActive]         = ImVec4(0.2784f, 0.2510f, 0.3373f, 1.0000f);
        style_->Colors[ImGuiCol_TitleBg]               = ImVec4(0.2784f, 0.2510f, 0.3373f, 1.0000f);
        style_->Colors[ImGuiCol_TitleBgActive]         = ImVec4(0.2784f, 0.2510f, 0.3373f, 1.0000f);
        style_->Colors[ImGuiCol_TitleBgCollapsed]      = ImVec4(0.0000f, 0.0000f, 0.0000f, 0.5100f);
        style_->Colors[ImGuiCol_MenuBarBg]             = ImVec4(0.1373f, 0.1373f, 0.1373f, 1.0000f);
        style_->Colors[ImGuiCol_ScrollbarBg]           = ImVec4(0.0196f, 0.0196f, 0.0196f, 0.5300f);
        style_->Colors[ImGuiCol_ScrollbarGrab]         = ImVec4(0.3098f, 0.3098f, 0.3098f, 1.0000f);
        style_->Colors[ImGuiCol_ScrollbarGrabHovered]  = ImVec4(0.4078f, 0.4078f, 0.4078f, 1.0000f);
        style_->Colors[ImGuiCol_ScrollbarGrabActive]   = ImVec4(0.5098f, 0.5098f, 0.5098f, 1.0000f);
        style_->Colors[ImGuiCol_CheckMark]             = ImVec4(0.5451f, 0.4667f, 0.7176f, 1.0000f);
        style_->Colors[ImGuiCol_SliderGrab]            = ImVec4(0.2784f, 0.2510f, 0.3373f, 1.0000f);
        style_->Colors[ImGuiCol_SliderGrabActive]      = ImVec4(0.2784f, 0.2510f, 0.3373f, 1.0000f);
        style_->Colors[ImGuiCol_Button]                = ImVec4(0.2784f, 0.2510f, 0.3373f, 1.0000f);
        style_->Colors[ImGuiCol_ButtonHovered]         = ImVec4(0.3451f, 0.2941f, 0.4588f, 1.0000f);
        style_->Colors[ImGuiCol_ButtonActive]          = ImVec4(0.3137f, 0.2588f, 0.4275f, 1.0000f);
        style_->Colors[ImGuiCol_Header]                = ImVec4(0.3176f, 0.2784f, 0.4078f, 1.0000f);
        style_->Colors[ImGuiCol_HeaderHovered]         = ImVec4(0.4157f, 0.3647f, 0.5294f, 1.0000f);
        style_->Colors[ImGuiCol_HeaderActive]          = ImVec4(0.4039f, 0.3529f, 0.5098f, 1.0000f);
        style_->Colors[ImGuiCol_Separator]             = ImVec4(0.4275f, 0.4275f, 0.4980f, 0.5000f);
        style_->Colors[ImGuiCol_SeparatorHovered]      = ImVec4(0.2784f, 0.2510f, 0.3373f, 1.0000f);
        style_->Colors[ImGuiCol_SeparatorActive]       = ImVec4(0.2784f, 0.2510f, 0.3373f, 1.0000f);
        style_->Colors[ImGuiCol_ResizeGrip]            = ImVec4(0.2784f, 0.2510f, 0.3373f, 1.0000f);
        style_->Colors[ImGuiCol_ResizeGripHovered]     = ImVec4(0.2784f, 0.2510f, 0.3373f, 1.0000f);
        style_->Colors[ImGuiCol_ResizeGripActive]      = ImVec4(0.2784f, 0.2510f, 0.3373f, 1.0000f);
        style_->Colors[ImGuiCol_Tab]                   = ImVec4(0.2784f, 0.2510f, 0.3373f, 1.0000f);
        style_->Colors[ImGuiCol_TabHovered]            = ImVec4(0.3255f, 0.2863f, 0.4157f, 1.0000f);
        style_->Colors[ImGuiCol_TabActive]             = ImVec4(0.4000f, 0.3490f, 0.5059f, 1.0000f);
        style_->Colors[ImGuiCol_TabUnfocused]          = ImVec4(0.2784f, 0.2510f, 0.3373f, 1.0000f);
        style_->Colors[ImGuiCol_TabUnfocusedActive]    = ImVec4(0.2784f, 0.2510f, 0.3373f, 1.0000f);
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
        style_->Colors[ImGuiCol_NavHighlight]          = ImVec4(0.2784f, 0.2510f, 0.3373f, 1.0000f);
        style_->Colors[ImGuiCol_NavWindowingHighlight] = ImVec4(1.0000f, 1.0000f, 1.0000f, 0.7000f);
        style_->Colors[ImGuiCol_NavWindowingDimBg]     = ImVec4(0.8000f, 0.8000f, 0.8000f, 0.2000f);
        style_->Colors[ImGuiCol_ModalWindowDimBg]      = ImVec4(0.8000f, 0.8000f, 0.8000f, 0.3500f);
    
        SYS_INFO("GuiMgr", "'Quick Minimal Look' theme applied.");
    }

    void GuiMgr::Style_Modern() {

        StyleColorsDark();
        titlebar_dark_mode(true);
        
        style_->Alpha                  = 1.0000f;
        style_->DisabledAlpha          = 0.3000f;
        style_->WindowPadding          = ImVec2(10.1000f, 10.1000f);
        style_->WindowRounding         = 10.3000f;
        style_->WindowBorderSize       = 1.0000f;
        style_->WindowMinSize          = ImVec2(20.0000f, 32.0000f);
        style_->WindowTitleAlign       = ImVec2(0.5000f, 0.5000f);
        style_->WindowMenuButtonPosition = ImGuiDir_Right;
        style_->ChildRounding          = 8.2000f;
        style_->ChildBorderSize        = 1.0000f;
        style_->PopupRounding          = 10.7000f;
        style_->PopupBorderSize        = 1.0000f;
        style_->FramePadding           = ImVec2(20.0000f, 1.5000f);
        style_->FrameRounding          = 4.8000f;
        style_->FrameBorderSize        = 0.0000f;
        style_->ItemSpacing            = ImVec2(9.7000f, 5.3000f);
        style_->ItemInnerSpacing       = ImVec2(5.4000f, 9.3000f);
        style_->CellPadding            = ImVec2(7.9000f, 2.0000f);
        style_->IndentSpacing          = 10.7000f;
        style_->ColumnsMinSpacing      = 6.0000f;
        style_->ScrollbarSize          = 12.1000f;
        style_->ScrollbarRounding      = 20.0000f;
        style_->GrabMinSize            = 10.0000f;
        style_->GrabRounding           = 4.6000f;
        style_->TabRounding            = 4.0000f;
        style_->TabBorderSize          = 0.0000f;
        style_->ColorButtonPosition    = ImGuiDir_Right;
        style_->ButtonTextAlign        = ImVec2(0.5000f, 0.5000f);
        style_->SelectableTextAlign    = ImVec2(0.0000f, 0.0000f);

        style_->Colors[ImGuiCol_Text]                  = ImVec4(1.0000f, 1.0000f, 1.0000f, 1.0000f);
        style_->Colors[ImGuiCol_TextDisabled]          = ImVec4(1.0000f, 1.0000f, 1.0000f, 0.3991f);
        style_->Colors[ImGuiCol_WindowBg]              = ImVec4(0.0392f, 0.0392f, 0.0392f, 0.9400f);
        style_->Colors[ImGuiCol_ChildBg]               = ImVec4(0.0000f, 0.0000f, 0.0000f, 0.0000f);
        style_->Colors[ImGuiCol_PopupBg]               = ImVec4(0.0510f, 0.0510f, 0.0510f, 0.9400f);
        style_->Colors[ImGuiCol_Border]                = ImVec4(0.4275f, 0.4275f, 0.4980f, 0.5000f);
        style_->Colors[ImGuiCol_BorderShadow]          = ImVec4(0.0000f, 0.0000f, 0.0000f, 0.0000f);
        style_->Colors[ImGuiCol_FrameBg]               = ImVec4(0.0000f, 0.0000f, 0.0000f, 0.4206f);
        style_->Colors[ImGuiCol_FrameBgHovered]        = ImVec4(0.1412f, 0.1412f, 0.1412f, 0.4000f);
        style_->Colors[ImGuiCol_FrameBgActive]         = ImVec4(0.2314f, 0.2314f, 0.2314f, 0.8627f);
        style_->Colors[ImGuiCol_TitleBg]               = ImVec4(0.0000f, 0.0000f, 0.0000f, 1.0000f);
        style_->Colors[ImGuiCol_TitleBgActive]         = ImVec4(0.0941f, 0.0941f, 0.0941f, 1.0000f);
        style_->Colors[ImGuiCol_TitleBgCollapsed]      = ImVec4(0.0000f, 0.0000f, 0.0000f, 0.2918f);
        style_->Colors[ImGuiCol_MenuBarBg]             = ImVec4(0.1373f, 0.1373f, 0.1373f, 1.0000f);
        style_->Colors[ImGuiCol_ScrollbarBg]           = ImVec4(0.0196f, 0.0196f, 0.0196f, 0.5300f);
        style_->Colors[ImGuiCol_ScrollbarGrab]         = ImVec4(0.3098f, 0.3098f, 0.3098f, 1.0000f);
        style_->Colors[ImGuiCol_ScrollbarGrabHovered]  = ImVec4(0.4078f, 0.4078f, 0.4078f, 1.0000f);
        style_->Colors[ImGuiCol_ScrollbarGrabActive]   = ImVec4(0.5098f, 0.5098f, 0.5098f, 1.0000f);
        style_->Colors[ImGuiCol_CheckMark]             = ImVec4(0.9804f, 0.2588f, 0.2588f, 1.0000f);
        style_->Colors[ImGuiCol_SliderGrab]            = ImVec4(1.0000f, 1.0000f, 1.0000f, 1.0000f);
        style_->Colors[ImGuiCol_SliderGrabActive]      = ImVec4(0.2588f, 0.5882f, 0.9804f, 1.0000f);
        style_->Colors[ImGuiCol_Button]                = ImVec4(0.0000f, 0.0000f, 0.0000f, 0.5794f);
        style_->Colors[ImGuiCol_ButtonHovered]         = ImVec4(0.0980f, 0.0980f, 0.0980f, 1.0000f);
        style_->Colors[ImGuiCol_ButtonActive]          = ImVec4(1.0000f, 0.2314f, 0.2314f, 1.0000f);
        style_->Colors[ImGuiCol_Header]                = ImVec4(0.0000f, 0.0000f, 0.0000f, 0.4549f);
        style_->Colors[ImGuiCol_HeaderHovered]         = ImVec4(0.1804f, 0.1804f, 0.1804f, 0.8000f);
        style_->Colors[ImGuiCol_HeaderActive]          = ImVec4(0.9765f, 0.2588f, 0.2588f, 1.0000f);
        style_->Colors[ImGuiCol_Separator]             = ImVec4(0.0000f, 0.0000f, 0.0000f, 0.5000f);
        style_->Colors[ImGuiCol_SeparatorHovered]      = ImVec4(0.0980f, 0.4000f, 0.7490f, 0.7800f);
        style_->Colors[ImGuiCol_SeparatorActive]       = ImVec4(0.0980f, 0.4000f, 0.7490f, 1.0000f);
        style_->Colors[ImGuiCol_ResizeGrip]            = ImVec4(0.2588f, 0.5882f, 0.9765f, 0.2000f);
        style_->Colors[ImGuiCol_ResizeGripHovered]     = ImVec4(0.2588f, 0.5882f, 0.9765f, 0.6700f);
        style_->Colors[ImGuiCol_ResizeGripActive]      = ImVec4(0.2588f, 0.5882f, 0.9765f, 0.9500f);
        style_->Colors[ImGuiCol_Tab]                   = ImVec4(0.1059f, 0.1059f, 0.1059f, 1.0000f);
        style_->Colors[ImGuiCol_TabHovered]            = ImVec4(1.0000f, 0.3647f, 0.6745f, 0.8000f);
        style_->Colors[ImGuiCol_TabActive]             = ImVec4(1.0000f, 0.2235f, 0.2235f, 1.0000f);
        style_->Colors[ImGuiCol_TabUnfocused]          = ImVec4(0.1098f, 0.1686f, 0.2392f, 0.9724f);
        style_->Colors[ImGuiCol_TabUnfocusedActive]    = ImVec4(0.1333f, 0.2588f, 0.4235f, 1.0000f);
        style_->Colors[ImGuiCol_PlotLines]             = ImVec4(1.0000f, 1.0000f, 1.0000f, 1.0000f);
        style_->Colors[ImGuiCol_PlotLinesHovered]      = ImVec4(1.0000f, 0.4275f, 0.3490f, 1.0000f);
        style_->Colors[ImGuiCol_PlotHistogram]         = ImVec4(1.0000f, 0.2157f, 0.2157f, 1.0000f);
        style_->Colors[ImGuiCol_PlotHistogramHovered]  = ImVec4(1.0000f, 0.2157f, 0.6980f, 1.0000f);
        style_->Colors[ImGuiCol_TableHeaderBg]         = ImVec4(1.0000f, 0.2353f, 0.2353f, 1.0000f);
        style_->Colors[ImGuiCol_TableBorderStrong]     = ImVec4(1.0000f, 0.3176f, 0.3176f, 1.0000f);
        style_->Colors[ImGuiCol_TableBorderLight]      = ImVec4(1.0000f, 0.5647f, 0.5647f, 0.3691f);
        style_->Colors[ImGuiCol_TableRowBg]            = ImVec4(0.7255f, 0.3373f, 1.0000f, 0.0000f);
        style_->Colors[ImGuiCol_TableRowBgAlt]         = ImVec4(1.0000f, 0.2745f, 0.2745f, 0.1116f);
        style_->Colors[ImGuiCol_TextSelectedBg]        = ImVec4(0.9765f, 0.2588f, 0.2588f, 1.0000f);
        style_->Colors[ImGuiCol_DragDropTarget]        = ImVec4(1.0000f, 1.0000f, 0.0000f, 0.9000f);
        style_->Colors[ImGuiCol_NavHighlight]          = ImVec4(0.0000f, 0.0000f, 0.0000f, 0.6438f);
        style_->Colors[ImGuiCol_NavWindowingHighlight] = ImVec4(1.0000f, 1.0000f, 1.0000f, 0.4678f);
        style_->Colors[ImGuiCol_NavWindowingDimBg]     = ImVec4(0.0000f, 0.0000f, 0.0000f, 0.7339f);
        style_->Colors[ImGuiCol_ModalWindowDimBg]      = ImVec4(0.0000f, 0.0000f, 0.0000f, 0.7983f);
    
        SYS_INFO("GuiMgr", "'Modern' theme applied.");
    }

    void GuiMgr::Style_Microfrost() {

        StyleColorsLight();
        titlebar_dark_mode(false);

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

    void GuiMgr::Style_Moonlight() {

        StyleColorsDark();
        titlebar_dark_mode(true);

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
        style_->Colors[ImGuiCol_ButtonHovered] = ImVec4(0.9000f, 0.9000f, 0.9000f, 1.0000f);    // <- Modificado
        style_->Colors[ImGuiCol_ButtonActive]  = ImVec4(0.8000f, 0.8000f, 0.8000f, 1.0000f);    // <- Modificado
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

    void GuiMgr::Style_SonicRiders() {

        StyleColorsDark();
        titlebar_dark_mode(true);

        style_->Alpha                  = 1.0000f;
        style_->DisabledAlpha          = 0.6000f;
        style_->WindowPadding          = ImVec2(8.0000f, 8.0000f);
        style_->WindowRounding         = 0.0000f;
        style_->WindowBorderSize       = 0.0000f;
        style_->WindowMinSize          = ImVec2(32.0000f, 32.0000f);
        style_->WindowTitleAlign       = ImVec2(0.0000f, 0.5000f);
        style_->WindowMenuButtonPosition = ImGuiDir_Left;
        style_->ChildRounding          = 0.0000f;
        style_->ChildBorderSize        = 1.0000f;
        style_->PopupRounding          = 0.0000f;
        style_->PopupBorderSize        = 0.0000f;
        style_->FramePadding           = ImVec2(4.0000f, 3.0000f);
        style_->FrameRounding          = 4.0000f;
        style_->FrameBorderSize        = 0.0000f;
        style_->ItemSpacing            = ImVec2(8.0000f, 4.0000f);
        style_->ItemInnerSpacing       = ImVec2(4.0000f, 4.0000f);
        style_->CellPadding            = ImVec2(4.0000f, 2.0000f);
        style_->IndentSpacing          = 21.0000f;
        style_->ColumnsMinSpacing      = 6.0000f;
        style_->ScrollbarSize          = 14.0000f;
        style_->ScrollbarRounding      = 9.0000f;
        style_->GrabMinSize            = 10.0000f;
        style_->GrabRounding           = 4.0000f;
        style_->TabRounding            = 4.0000f;
        style_->TabBorderSize          = 0.0000f;
        style_->ColorButtonPosition    = ImGuiDir_Right;
        style_->ButtonTextAlign        = ImVec2(0.5000f, 0.5000f);
        style_->SelectableTextAlign    = ImVec2(0.0000f, 0.0000f);

        style_->Colors[ImGuiCol_Text]                  = ImVec4(1.0000f, 1.0000f, 1.0000f, 1.0000f);
        style_->Colors[ImGuiCol_TextDisabled]          = ImVec4(0.7294f, 0.7490f, 0.7373f, 1.0000f);
        style_->Colors[ImGuiCol_WindowBg]              = ImVec4(0.0863f, 0.0863f, 0.0863f, 0.9400f);
        style_->Colors[ImGuiCol_ChildBg]               = ImVec4(0.0000f, 0.0000f, 0.0000f, 0.0000f);
        style_->Colors[ImGuiCol_PopupBg]               = ImVec4(0.0784f, 0.0784f, 0.0784f, 0.9400f);
        style_->Colors[ImGuiCol_Border]                = ImVec4(0.2000f, 0.2000f, 0.2000f, 0.5000f);
        style_->Colors[ImGuiCol_BorderShadow]          = ImVec4(0.0000f, 0.0000f, 0.0000f, 0.0000f);
        style_->Colors[ImGuiCol_FrameBg]               = ImVec4(0.7098f, 0.3882f, 0.3882f, 0.5400f);
        style_->Colors[ImGuiCol_FrameBgHovered]        = ImVec4(0.8392f, 0.6588f, 0.6588f, 0.4000f);
        style_->Colors[ImGuiCol_FrameBgActive]         = ImVec4(0.8392f, 0.6588f, 0.6588f, 0.6700f);
        style_->Colors[ImGuiCol_TitleBg]               = ImVec4(0.4667f, 0.2196f, 0.2196f, 0.6700f);
        style_->Colors[ImGuiCol_TitleBgActive]         = ImVec4(0.4667f, 0.2196f, 0.2196f, 1.0000f);
        style_->Colors[ImGuiCol_TitleBgCollapsed]      = ImVec4(0.4667f, 0.2196f, 0.2196f, 0.6700f);
        style_->Colors[ImGuiCol_MenuBarBg]             = ImVec4(0.3373f, 0.1569f, 0.1569f, 1.0000f);
        style_->Colors[ImGuiCol_ScrollbarBg]           = ImVec4(0.0196f, 0.0196f, 0.0196f, 0.5300f);
        style_->Colors[ImGuiCol_ScrollbarGrab]         = ImVec4(0.3098f, 0.3098f, 0.3098f, 1.0000f);
        style_->Colors[ImGuiCol_ScrollbarGrabHovered]  = ImVec4(0.4078f, 0.4078f, 0.4078f, 1.0000f);
        style_->Colors[ImGuiCol_ScrollbarGrabActive]   = ImVec4(0.5098f, 0.5098f, 0.5098f, 1.0000f);
        style_->Colors[ImGuiCol_CheckMark]             = ImVec4(1.0000f, 1.0000f, 1.0000f, 1.0000f);
        style_->Colors[ImGuiCol_SliderGrab]            = ImVec4(0.7098f, 0.3882f, 0.3882f, 1.0000f);
        style_->Colors[ImGuiCol_SliderGrabActive]      = ImVec4(0.8392f, 0.6588f, 0.6588f, 1.0000f);
        style_->Colors[ImGuiCol_Button]                = ImVec4(0.4667f, 0.2196f, 0.2196f, 0.6500f);
        style_->Colors[ImGuiCol_ButtonHovered]         = ImVec4(0.7098f, 0.3882f, 0.3882f, 0.6500f);
        style_->Colors[ImGuiCol_ButtonActive]          = ImVec4(0.2000f, 0.2000f, 0.2000f, 0.5000f);
        style_->Colors[ImGuiCol_Header]                = ImVec4(0.7098f, 0.3882f, 0.3882f, 0.5400f);
        style_->Colors[ImGuiCol_HeaderHovered]         = ImVec4(0.8392f, 0.6588f, 0.6588f, 0.6500f);
        style_->Colors[ImGuiCol_HeaderActive]          = ImVec4(0.8392f, 0.6588f, 0.6588f, 0.0000f);
        style_->Colors[ImGuiCol_Separator]             = ImVec4(0.4275f, 0.4275f, 0.4980f, 0.5000f);
        style_->Colors[ImGuiCol_SeparatorHovered]      = ImVec4(0.7098f, 0.3882f, 0.3882f, 0.5400f);
        style_->Colors[ImGuiCol_SeparatorActive]       = ImVec4(0.7098f, 0.3882f, 0.3882f, 0.5400f);
        style_->Colors[ImGuiCol_ResizeGrip]            = ImVec4(0.7098f, 0.3882f, 0.3882f, 0.5400f);
        style_->Colors[ImGuiCol_ResizeGripHovered]     = ImVec4(0.8392f, 0.6588f, 0.6588f, 0.6600f);
        style_->Colors[ImGuiCol_ResizeGripActive]      = ImVec4(0.8392f, 0.6588f, 0.6588f, 0.6600f);
        style_->Colors[ImGuiCol_Tab]                   = ImVec4(0.7098f, 0.3882f, 0.3882f, 0.5400f);
        style_->Colors[ImGuiCol_TabHovered]            = ImVec4(0.8392f, 0.6588f, 0.6588f, 0.6600f);
        style_->Colors[ImGuiCol_TabActive]             = ImVec4(0.8392f, 0.6588f, 0.6588f, 0.6600f);
        style_->Colors[ImGuiCol_TabUnfocused]          = ImVec4(0.0667f, 0.0980f, 0.1490f, 0.9700f);
        style_->Colors[ImGuiCol_TabUnfocusedActive]    = ImVec4(0.1373f, 0.2588f, 0.4196f, 1.0000f);
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
        style_->Colors[ImGuiCol_NavHighlight]          = ImVec4(0.4078f, 0.4078f, 0.4078f, 1.0000f);
        style_->Colors[ImGuiCol_NavWindowingHighlight] = ImVec4(1.0000f, 1.0000f, 1.0000f, 0.7000f);
        style_->Colors[ImGuiCol_NavWindowingDimBg]     = ImVec4(0.8000f, 0.8000f, 0.8000f, 0.2000f);
        style_->Colors[ImGuiCol_ModalWindowDimBg]      = ImVec4(0.8000f, 0.8000f, 0.8000f, 0.3500f);
    
        SYS_INFO("GuiMgr", "'Sonic Riders' theme applied.");

    }

    void GuiMgr::Style_VisualStudio() {

        StyleColorsDark();
        titlebar_dark_mode(true);

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
        

#else
// ============================================================
//  (Stubs)
// ============================================================

// Temas --------------------------------------------------------------------------------
    void GuiMgr::apply_theme()               { return; }
    void GuiMgr::saveConfig()               { return; }
    void GuiMgr::Style_AdobeInspired()      { return; }
    void GuiMgr::Style_AyuDark()            { return; }
    void GuiMgr::Style_Confy()              { return; }
    void GuiMgr::Style_DarkCyan()           { return; }
    void GuiMgr::Style_DefaultDark()        { return; }
    void GuiMgr::Style_DefaultLight()       { return; }
    void GuiMgr::Style_Everforest()         { return; }
    void GuiMgr::Style_FutureDark()         { return; }
    void GuiMgr::Style_Gold()               { return; }
    void GuiMgr::Style_HazyDark()           { return; }
    void GuiMgr::Style_KazamsCherry()       { return; }
    void GuiMgr::Style_LightOrange()        { return; }
    void GuiMgr::Style_QuickMinimalLook()   { return; }
    void GuiMgr::Style_Modern()             { return; }
    void GuiMgr::Style_Microfrost()         { return; }
    void GuiMgr::Style_Moonlight()          { return; }
    void GuiMgr::Style_SonicRiders()        { return; }
    void GuiMgr::Style_VisualStudio()       { return; }

#endif
