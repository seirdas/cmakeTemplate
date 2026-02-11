
/// ------------------------------- INFORMACIÓN -------------------------------------
//	# Configuraciones de ventana
//	- Buscar en imgui.h -> ImGuiWindowFlags_
//  - Añadir en una variable: ImGuiWindowFlags window_flags = *********************;
//	- Añadir a la ventana (en Begin("", nullptr, window_flags); )
// ---------------------------------------------------------------------------------

#include "winMgr.h"
#include <iostream>

// de la clase imgui de dialogo
#include <GLFW/glfw3.h>
#include <imgui.h>
#include <imgui_internal.h>
#include <backends/imgui_impl_glfw.h>
#include <backends/imgui_impl_opengl3.h>
#ifdef _WIN32
    #define GLFW_EXPOSE_NATIVE_WIN32
    #include <GLFW/glfw3native.h>
    #include <windows.h>
    #include <dwmapi.h>
#endif
#include "resources.h"  // icono

using namespace ImGui;

// General ------------------------------------------------------------------------------

// Public methods

bool WinMgr::init() {
    if (!glfwInit()) return false;

    // 1. Configuración de la ventana GLFW
    glfwWindowHint(GLFW_TRANSPARENT_FRAMEBUFFER, GLFW_TRUE); // Fondo transparente
    glfwWindowHint(GLFW_DECORATED, GLFW_TRUE);               // Bordes y barra de título
    glfwWindowHint(GLFW_RESIZABLE, GLFW_TRUE);               // Redimensionable

    // Creación de ventana
    window = glfwCreateWindow(sizeX, sizeY, AppName.c_str(), NULL, NULL);
    if(!window) {
        glfwTerminate();
        std::cerr << "Error al crear la ventana GLFW" << std::endl;
        return false;
    }
    glfwMakeContextCurrent(window);
    glfwSwapInterval(1);

    // Inicializa ImGui
    IMGUI_CHECKVERSION();
    ImGui::CreateContext();
    ImGuiIO& io = ImGui::GetIO(); (void)io; // No usar
    io.IniFilename = NULL;  // No usar archivo .ini de imgui

    // Cargar fuente personalizada
    io.Fonts->AddFontFromFileTTF(customFont.c_str(), fontSize, NULL, io.Fonts->GetGlyphRangesDefault());

    // Configuración de estilo
    ImGui::StyleColorsDark();
	Style_VisualStudio();

    // Tomar la ventana (para modificar cosas)
    #ifdef _WIN32
        HINSTANCE hInstance = GetModuleHandle(NULL);
        HWND hwnd = glfwGetWin32Window(window);

        // Cargar el icono de la ventana utilizando WinAPI
        HICON hIcon = LoadIcon(hInstance, MAKEINTRESOURCE(IDI_ICON1));
        SendMessage(hwnd, WM_SETICON, ICON_BIG, (LPARAM)hIcon);

        // Cambiar color de la barra de titulo (2)
        BOOL useDarkMode = TRUE;
        DwmSetWindowAttribute(hwnd, 20, &useDarkMode, sizeof(useDarkMode));
    #endif

    ImGui_ImplGlfw_InitForOpenGL(window, true);
    ImGui_ImplOpenGL3_Init("#version 130");     // Versión de OpenGL
    return true;
}

bool WinMgr::isRunning() const
{
    return window && !glfwWindowShouldClose(window);
}

void WinMgr::cerrar() {
    ImGui_ImplOpenGL3_Shutdown();
    ImGui_ImplGlfw_Shutdown();
    ImGui::DestroyContext();

    if (window)
    {
        glfwDestroyWindow(window);
        window = nullptr;
    }

    glfwTerminate();
}


// Private methods

void WinMgr::initCuadro(){
    glfwPollEvents();
    
    ImGui_ImplOpenGL3_NewFrame();
    ImGui_ImplGlfw_NewFrame();
    ImGui::NewFrame();
}

void WinMgr::endCuadro(){
    // Renderiza
    ImGui::Render();
    glClearColor(clearColor[0], clearColor[1], clearColor[2], clearColor[3]);
    glClear(GL_COLOR_BUFFER_BIT);
    ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());

    // Intercambia los buffers
    glfwSwapBuffers(window);
}



// Bucle principal ----------------------------------------------------------------------

// public methods

void WinMgr::BuclePrincipal() {
    initCuadro();
	// Referencia a la ventana GLFW
    ImGuiIO& io = ImGui::GetIO();

    ShowDemoWindow();     // Ventana de demostración
	//ShowMetricsWindow();  // Ventana de métricas

	// vvvvvvvvv Contenido de la ventana vvvvvvvvv

	// 1. BARRA DE MENÚ SUPERIOR
	crearMainMenuBar();

	// 1. Forzar bordes cuadrados y eliminar paddings innecesarios para el frame principal
    ImGui::PushStyleVar(ImGuiStyleVar_WindowRounding, 0.0f);
    ImGui::PushStyleVar(ImGuiStyleVar_ChildRounding, 0.0f);
    ImGui::PushStyleVar(ImGuiStyleVar_WindowBorderSize, 0.0f);
    
	ImGuiWindowFlags window_flags = ImGuiWindowFlags_NoTitleBar |
                                    ImGuiWindowFlags_NoCollapse |
                                    ImGuiWindowFlags_NoResize |
                                    ImGuiWindowFlags_NoScrollbar |
                                    ImGuiWindowFlags_NoMove |
                                    ImGuiWindowFlags_NoBringToFrontOnFocus |
                                    ImGuiWindowFlags_NoNav;

        // Ventana que cubre todo el frame
		SetNextWindowPos(ImVec2(0,MainMenuBar_Height));
		SetNextWindowSize(ImVec2(io.DisplaySize.x, io.DisplaySize.y - MainMenuBar_Height));
        ImGui::Begin("Ventana que cubre todo el frame", nullptr, window_flags);


// --- LÓGICA DE PANELES INDEPENDIENTES ---
    
    // Variables estáticas para guardar las alturas (proporciones iniciales)
    static float heightLeftTop = 0.70f;  // 70% para F1
    static float heightRightTop = 0.20f; // 20% para F3
    
	float sizeSplitterVertical = 4.0f;
    float totalHeight = ImGui::GetContentRegionAvail().y;
    //float widthHalf = ImGui::GetContentRegionAvail().x * 0.5f;
    float sizeX_Izq = ImGui::GetContentRegionAvail().x * 0.2f;
    //float sizeX_Der = ImGui::GetContentRegionAvail().x - sizeX_Izq - sizeSplitterVertical; // Restamos el espacio del splitter vertical

    // COLUMNA IZQUIERDA
    ImGui::BeginGroup();
    {
        // Panel F1 (Arriba Izquierda)
        BeginChild("F1", ImVec2(sizeX_Izq, totalHeight * heightLeftTop), true);
        Text("F1 (70%% inicial)");
		Button("hola");
        EndChild();

        // Splitter Horizontal Izquierdo
        Button("##h_splitter_l", ImVec2(sizeX_Izq, 4.0f));
        if (IsItemActive())
            heightLeftTop += GetIO().MouseDelta.y / totalHeight;

        // Panel F2 (Abajo Izquierda)
        ImGui::BeginChild("F2", ImVec2(sizeX_Izq, 0), true); // Altura 0 para que rellene el resto
        ImGui::Text("F2 (30%% inicial)");
        ImGui::EndChild();
    }
    ImGui::EndGroup();

    ImGui::SameLine(); // Pegamos la siguiente columna
	Button("##v_splitter", ImVec2(sizeSplitterVertical, -FLT_MIN)); // Splitter Vertical
	if (ImGui::IsItemActive())
	{
	    heightLeftTop += ImGui::GetIO().MouseDelta.y / totalHeight;
	    heightRightTop += ImGui::GetIO().MouseDelta.y / totalHeight;
	}
	SameLine();

    // COLUMNA DERECHA
    ImGui::BeginGroup();
    {
        // Panel F3 (Arriba Derecha)
        ImGui::BeginChild("F3", ImVec2(0, totalHeight * heightRightTop), true);
        ImGui::Text("F3 (20%% inicial)");
        ImGui::EndChild();

        // Splitter Horizontal Derecho
        ImGui::Button("##h_splitter_r", ImVec2(-FLT_MIN, 4.0f));
        if (ImGui::IsItemActive())
            heightRightTop += ImGui::GetIO().MouseDelta.y / totalHeight;

        // Panel F4 (Abajo Derecha)
        ImGui::BeginChild("F4", ImVec2(0, 0), true);
        ImGui::Text("F4 (80%% inicial)");
        ImGui::EndChild();
    }
    ImGui::EndGroup();

    ImGui::End();
    ImGui::PopStyleVar(3);

	
	// ^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^

    endCuadro();
}

// private methods

void WinMgr::crearMainMenuBar() {
	if (ImGui::BeginMainMenuBar()) {
		if (ImGui::BeginMenu("File")) {
			if (ImGui::MenuItem("New", "Ctrl+N")) { /* Acción */ }
			if (ImGui::MenuItem("Open", "Ctrl+O")) { /* Acción */ }
			ImGui::Separator();
			if (ImGui::MenuItem("Exit")) { glfwSetWindowShouldClose(window, true); }
			ImGui::EndMenu();
		}
		if (ImGui::BeginMenu("Edit")) {
			if (ImGui::MenuItem("Undo", "Ctrl+Z")) {}
			if (ImGui::MenuItem("Redo", "Ctrl+Y")) {}
			ImGui::EndMenu();
		}
		if (ImGui::BeginMenu("Ajustes")) {
			if (ImGui::MenuItem("Configurar conexión...")) { /* Abrir un popup de ajustes */ }
			ImGui::EndMenu();
		}
		
		// Guardamos el alto de la barra para ajustar la ventana de abajo
		ImGui::EndMainMenuBar();
	}

	MainMenuBar_Height = ImGui::GetFrameHeightWithSpacing();
}


// Temas --------------------------------------------------------------------------------

///@brief Comfy style by Giuseppe from ImThemes
void WinMgr::Style_Confy(){
	ImGuiStyle& style = ImGui::GetStyle();
	
	style.Alpha = 1.0f;
	style.DisabledAlpha = 0.1f;
	style.WindowPadding = ImVec2(8.0f, 8.0f);
	style.WindowRounding = 10.0f;
	style.WindowBorderSize = 0.0f;
	style.WindowMinSize = ImVec2(30.0f, 30.0f);
	style.WindowTitleAlign = ImVec2(0.5f, 0.5f);
	style.WindowMenuButtonPosition = ImGuiDir_Right;
	style.ChildRounding = 5.0f;
	style.ChildBorderSize = 1.0f;
	style.PopupRounding = 10.0f;
	style.PopupBorderSize = 0.0f;
	style.FramePadding = ImVec2(5.0f, 3.5f);
	style.FrameRounding = 5.0f;
	style.FrameBorderSize = 0.0f;
	style.ItemSpacing = ImVec2(5.0f, 4.0f);
	style.ItemInnerSpacing = ImVec2(5.0f, 5.0f);
	style.CellPadding = ImVec2(4.0f, 2.0f);
	style.IndentSpacing = 5.0f;
	style.ColumnsMinSpacing = 5.0f;
	style.ScrollbarSize = 15.0f;
	style.ScrollbarRounding = 9.0f;
	style.GrabMinSize = 15.0f;
	style.GrabRounding = 5.0f;
	style.TabRounding = 5.0f;
	style.TabBorderSize = 0.0f;
	style.ColorButtonPosition = ImGuiDir_Right;
	style.ButtonTextAlign = ImVec2(0.5f, 0.5f);
	style.SelectableTextAlign = ImVec2(0.0f, 0.0f);
	
	style.Colors[ImGuiCol_Text] = ImVec4(1.0f, 1.0f, 1.0f, 1.0f);
	style.Colors[ImGuiCol_TextDisabled] = ImVec4(1.0f, 1.0f, 1.0f, 0.360515f);
	style.Colors[ImGuiCol_WindowBg] = ImVec4(0.09803922f, 0.09803922f, 0.09803922f, 1.0f);
	style.Colors[ImGuiCol_ChildBg] = ImVec4(1.0f, 0.0f, 0.0f, 0.0f);
	style.Colors[ImGuiCol_PopupBg] = ImVec4(0.09803922f, 0.09803922f, 0.09803922f, 1.0f);
	style.Colors[ImGuiCol_Border] = ImVec4(0.42352942f, 0.38039216f, 0.57254905f, 0.5493562f);
	style.Colors[ImGuiCol_BorderShadow] = ImVec4(0.0f, 0.0f, 0.0f, 0.0f);
	style.Colors[ImGuiCol_FrameBg] = ImVec4(0.15686275f, 0.15686275f, 0.15686275f, 1.0f);
	style.Colors[ImGuiCol_FrameBgHovered] = ImVec4(0.38039216f, 0.42352942f, 0.57254905f, 0.54901963f);
	style.Colors[ImGuiCol_FrameBgActive] = ImVec4(0.61960787f, 0.5764706f, 0.76862746f, 0.54901963f);
	style.Colors[ImGuiCol_TitleBg] = ImVec4(0.09803922f, 0.09803922f, 0.09803922f, 1.0f);
	style.Colors[ImGuiCol_TitleBgActive] = ImVec4(0.09803922f, 0.09803922f, 0.09803922f, 1.0f);
	style.Colors[ImGuiCol_TitleBgCollapsed] = ImVec4(0.25882354f, 0.25882354f, 0.25882354f, 0.0f);
	style.Colors[ImGuiCol_MenuBarBg] = ImVec4(0.0f, 0.0f, 0.0f, 0.0f);
	style.Colors[ImGuiCol_ScrollbarBg] = ImVec4(0.15686275f, 0.15686275f, 0.15686275f, 0.0f);
	style.Colors[ImGuiCol_ScrollbarGrab] = ImVec4(0.15686275f, 0.15686275f, 0.15686275f, 1.0f);
	style.Colors[ImGuiCol_ScrollbarGrabHovered] = ImVec4(0.23529412f, 0.23529412f, 0.23529412f, 1.0f);
	style.Colors[ImGuiCol_ScrollbarGrabActive] = ImVec4(0.29411766f, 0.29411766f, 0.29411766f, 1.0f);
	style.Colors[ImGuiCol_CheckMark] = ImVec4(0.29411766f, 0.29411766f, 0.29411766f, 1.0f);
	style.Colors[ImGuiCol_SliderGrab] = ImVec4(0.61960787f, 0.5764706f, 0.76862746f, 0.54901963f);
	style.Colors[ImGuiCol_SliderGrabActive] = ImVec4(0.8156863f, 0.77254903f, 0.9647059f, 0.54901963f);
	style.Colors[ImGuiCol_Button] = ImVec4(0.61960787f, 0.5764706f, 0.76862746f, 0.54901963f);
	style.Colors[ImGuiCol_ButtonHovered] = ImVec4(0.7372549f, 0.69411767f, 0.8862745f, 0.54901963f);
	style.Colors[ImGuiCol_ButtonActive] = ImVec4(0.8156863f, 0.77254903f, 0.9647059f, 0.54901963f);
	style.Colors[ImGuiCol_Header] = ImVec4(0.61960787f, 0.5764706f, 0.76862746f, 0.54901963f);
	style.Colors[ImGuiCol_HeaderHovered] = ImVec4(0.7372549f, 0.69411767f, 0.8862745f, 0.54901963f);
	style.Colors[ImGuiCol_HeaderActive] = ImVec4(0.8156863f, 0.77254903f, 0.9647059f, 0.54901963f);
	style.Colors[ImGuiCol_Separator] = ImVec4(0.61960787f, 0.5764706f, 0.76862746f, 0.54901963f);
	style.Colors[ImGuiCol_SeparatorHovered] = ImVec4(0.7372549f, 0.69411767f, 0.8862745f, 0.54901963f);
	style.Colors[ImGuiCol_SeparatorActive] = ImVec4(0.8156863f, 0.77254903f, 0.9647059f, 0.54901963f);
	style.Colors[ImGuiCol_ResizeGrip] = ImVec4(0.61960787f, 0.5764706f, 0.76862746f, 0.54901963f);
	style.Colors[ImGuiCol_ResizeGripHovered] = ImVec4(0.7372549f, 0.69411767f, 0.8862745f, 0.54901963f);
	style.Colors[ImGuiCol_ResizeGripActive] = ImVec4(0.8156863f, 0.77254903f, 0.9647059f, 0.54901963f);
	style.Colors[ImGuiCol_Tab] = ImVec4(0.61960787f, 0.5764706f, 0.76862746f, 0.54901963f);
	style.Colors[ImGuiCol_TabHovered] = ImVec4(0.7372549f, 0.69411767f, 0.8862745f, 0.54901963f);
	style.Colors[ImGuiCol_TabActive] = ImVec4(0.8156863f, 0.77254903f, 0.9647059f, 0.54901963f);
	style.Colors[ImGuiCol_TabUnfocused] = ImVec4(0.0f, 0.4509804f, 1.0f, 0.0f);
	style.Colors[ImGuiCol_TabUnfocusedActive] = ImVec4(0.13333334f, 0.25882354f, 0.42352942f, 0.0f);
	style.Colors[ImGuiCol_PlotLines] = ImVec4(0.29411766f, 0.29411766f, 0.29411766f, 1.0f);
	style.Colors[ImGuiCol_PlotLinesHovered] = ImVec4(0.7372549f, 0.69411767f, 0.8862745f, 0.54901963f);
	style.Colors[ImGuiCol_PlotHistogram] = ImVec4(0.61960787f, 0.5764706f, 0.76862746f, 0.54901963f);
	style.Colors[ImGuiCol_PlotHistogramHovered] = ImVec4(0.7372549f, 0.69411767f, 0.8862745f, 0.54901963f);
	style.Colors[ImGuiCol_TableHeaderBg] = ImVec4(0.1882353f, 0.1882353f, 0.2f, 1.0f);
	style.Colors[ImGuiCol_TableBorderStrong] = ImVec4(0.42352942f, 0.38039216f, 0.57254905f, 0.54901963f);
	style.Colors[ImGuiCol_TableBorderLight] = ImVec4(0.42352942f, 0.38039216f, 0.57254905f, 0.2918455f);
	style.Colors[ImGuiCol_TableRowBg] = ImVec4(0.0f, 0.0f, 0.0f, 0.0f);
	style.Colors[ImGuiCol_TableRowBgAlt] = ImVec4(1.0f, 1.0f, 1.0f, 0.03433478f);
	style.Colors[ImGuiCol_TextSelectedBg] = ImVec4(0.7372549f, 0.69411767f, 0.8862745f, 0.54901963f);
	style.Colors[ImGuiCol_DragDropTarget] = ImVec4(1.0f, 1.0f, 0.0f, 0.9f);
	style.Colors[ImGuiCol_NavHighlight] = ImVec4(0.0f, 0.0f, 0.0f, 1.0f);
	style.Colors[ImGuiCol_NavWindowingHighlight] = ImVec4(1.0f, 1.0f, 1.0f, 0.7f);
	style.Colors[ImGuiCol_NavWindowingDimBg] = ImVec4(0.8f, 0.8f, 0.8f, 0.2f);
	style.Colors[ImGuiCol_ModalWindowDimBg] = ImVec4(0.8f, 0.8f, 0.8f, 0.35f);
}

/// @brief Future Dark style by rewrking from ImThemes
void WinMgr::Style_FutureDark(){
	ImGuiStyle& style = ImGui::GetStyle();
	
	style.Alpha = 1.0f;
	style.DisabledAlpha = 1.0f;
	style.WindowPadding = ImVec2(12.0f, 12.0f);
	style.WindowRounding = 0.0f;
	style.WindowBorderSize = 0.0f;
	style.WindowMinSize = ImVec2(20.0f, 20.0f);
	style.WindowTitleAlign = ImVec2(0.5f, 0.5f);
	style.WindowMenuButtonPosition = ImGuiDir_None;
	style.ChildRounding = 0.0f;
	style.ChildBorderSize = 1.0f;
	style.PopupRounding = 0.0f;
	style.PopupBorderSize = 1.0f;
	style.FramePadding = ImVec2(6.0f, 6.0f);
	style.FrameRounding = 0.0f;
	style.FrameBorderSize = 0.0f;
	style.ItemSpacing = ImVec2(12.0f, 6.0f);
	style.ItemInnerSpacing = ImVec2(6.0f, 3.0f);
	style.CellPadding = ImVec2(12.0f, 6.0f);
	style.IndentSpacing = 20.0f;
	style.ColumnsMinSpacing = 6.0f;
	style.ScrollbarSize = 12.0f;
	style.ScrollbarRounding = 0.0f;
	style.GrabMinSize = 12.0f;
	style.GrabRounding = 0.0f;
	style.TabRounding = 0.0f;
	style.TabBorderSize = 0.0f;
	style.ColorButtonPosition = ImGuiDir_Right;
	style.ButtonTextAlign = ImVec2(0.5f, 0.5f);
	style.SelectableTextAlign = ImVec2(0.0f, 0.0f);
	
	style.Colors[ImGuiCol_Text] = ImVec4(1.0f, 1.0f, 1.0f, 1.0f);
	style.Colors[ImGuiCol_TextDisabled] = ImVec4(0.27450982f, 0.31764707f, 0.4509804f, 1.0f);
	style.Colors[ImGuiCol_WindowBg] = ImVec4(0.078431375f, 0.08627451f, 0.101960786f, 1.0f);
	style.Colors[ImGuiCol_ChildBg] = ImVec4(0.078431375f, 0.08627451f, 0.101960786f, 1.0f);
	style.Colors[ImGuiCol_PopupBg] = ImVec4(0.078431375f, 0.08627451f, 0.101960786f, 1.0f);
	style.Colors[ImGuiCol_Border] = ImVec4(0.15686275f, 0.16862746f, 0.19215687f, 1.0f);
	style.Colors[ImGuiCol_BorderShadow] = ImVec4(0.078431375f, 0.08627451f, 0.101960786f, 1.0f);
	style.Colors[ImGuiCol_FrameBg] = ImVec4(0.11764706f, 0.13333334f, 0.14901961f, 1.0f);
	style.Colors[ImGuiCol_FrameBgHovered] = ImVec4(0.15686275f, 0.16862746f, 0.19215687f, 1.0f);
	style.Colors[ImGuiCol_FrameBgActive] = ImVec4(0.23529412f, 0.21568628f, 0.59607846f, 1.0f);
	style.Colors[ImGuiCol_TitleBg] = ImVec4(0.047058824f, 0.05490196f, 0.07058824f, 1.0f);
	style.Colors[ImGuiCol_TitleBgActive] = ImVec4(0.047058824f, 0.05490196f, 0.07058824f, 1.0f);
	style.Colors[ImGuiCol_TitleBgCollapsed] = ImVec4(0.078431375f, 0.08627451f, 0.101960786f, 1.0f);
	style.Colors[ImGuiCol_MenuBarBg] = ImVec4(0.09803922f, 0.105882354f, 0.12156863f, 1.0f);
	style.Colors[ImGuiCol_ScrollbarBg] = ImVec4(0.047058824f, 0.05490196f, 0.07058824f, 1.0f);
	style.Colors[ImGuiCol_ScrollbarGrab] = ImVec4(0.11764706f, 0.13333334f, 0.14901961f, 1.0f);
	style.Colors[ImGuiCol_ScrollbarGrabHovered] = ImVec4(0.15686275f, 0.16862746f, 0.19215687f, 1.0f);
	style.Colors[ImGuiCol_ScrollbarGrabActive] = ImVec4(0.11764706f, 0.13333334f, 0.14901961f, 1.0f);
	style.Colors[ImGuiCol_CheckMark] = ImVec4(0.49803922f, 0.5137255f, 1.0f, 1.0f);
	style.Colors[ImGuiCol_SliderGrab] = ImVec4(0.49803922f, 0.5137255f, 1.0f, 1.0f);
	style.Colors[ImGuiCol_SliderGrabActive] = ImVec4(0.5372549f, 0.5529412f, 1.0f, 1.0f);
	style.Colors[ImGuiCol_Button] = ImVec4(0.11764706f, 0.13333334f, 0.14901961f, 1.0f);
	style.Colors[ImGuiCol_ButtonHovered] = ImVec4(0.19607843f, 0.1764706f, 0.54509807f, 1.0f);
	style.Colors[ImGuiCol_ButtonActive] = ImVec4(0.23529412f, 0.21568628f, 0.59607846f, 1.0f);
	style.Colors[ImGuiCol_Header] = ImVec4(0.11764706f, 0.13333334f, 0.14901961f, 1.0f);
	style.Colors[ImGuiCol_HeaderHovered] = ImVec4(0.19607843f, 0.1764706f, 0.54509807f, 1.0f);
	style.Colors[ImGuiCol_HeaderActive] = ImVec4(0.23529412f, 0.21568628f, 0.59607846f, 1.0f);
	style.Colors[ImGuiCol_Separator] = ImVec4(0.15686275f, 0.18431373f, 0.2509804f, 1.0f);
	style.Colors[ImGuiCol_SeparatorHovered] = ImVec4(0.15686275f, 0.18431373f, 0.2509804f, 1.0f);
	style.Colors[ImGuiCol_SeparatorActive] = ImVec4(0.15686275f, 0.18431373f, 0.2509804f, 1.0f);
	style.Colors[ImGuiCol_ResizeGrip] = ImVec4(0.11764706f, 0.13333334f, 0.14901961f, 1.0f);
	style.Colors[ImGuiCol_ResizeGripHovered] = ImVec4(0.19607843f, 0.1764706f, 0.54509807f, 1.0f);
	style.Colors[ImGuiCol_ResizeGripActive] = ImVec4(0.23529412f, 0.21568628f, 0.59607846f, 1.0f);
	style.Colors[ImGuiCol_Tab] = ImVec4(0.047058824f, 0.05490196f, 0.07058824f, 1.0f);
	style.Colors[ImGuiCol_TabHovered] = ImVec4(0.11764706f, 0.13333334f, 0.14901961f, 1.0f);
	style.Colors[ImGuiCol_TabActive] = ImVec4(0.09803922f, 0.105882354f, 0.12156863f, 1.0f);
	style.Colors[ImGuiCol_TabUnfocused] = ImVec4(0.047058824f, 0.05490196f, 0.07058824f, 1.0f);
	style.Colors[ImGuiCol_TabUnfocusedActive] = ImVec4(0.078431375f, 0.08627451f, 0.101960786f, 1.0f);
	style.Colors[ImGuiCol_PlotLines] = ImVec4(0.52156866f, 0.6f, 0.7019608f, 1.0f);
	style.Colors[ImGuiCol_PlotLinesHovered] = ImVec4(0.039215688f, 0.98039216f, 0.98039216f, 1.0f);
	style.Colors[ImGuiCol_PlotHistogram] = ImVec4(1.0f, 0.2901961f, 0.59607846f, 1.0f);
	style.Colors[ImGuiCol_PlotHistogramHovered] = ImVec4(0.99607843f, 0.4745098f, 0.69803923f, 1.0f);
	style.Colors[ImGuiCol_TableHeaderBg] = ImVec4(0.047058824f, 0.05490196f, 0.07058824f, 1.0f);
	style.Colors[ImGuiCol_TableBorderStrong] = ImVec4(0.047058824f, 0.05490196f, 0.07058824f, 1.0f);
	style.Colors[ImGuiCol_TableBorderLight] = ImVec4(0.0f, 0.0f, 0.0f, 1.0f);
	style.Colors[ImGuiCol_TableRowBg] = ImVec4(0.11764706f, 0.13333334f, 0.14901961f, 1.0f);
	style.Colors[ImGuiCol_TableRowBgAlt] = ImVec4(0.09803922f, 0.105882354f, 0.12156863f, 1.0f);
	style.Colors[ImGuiCol_TextSelectedBg] = ImVec4(0.23529412f, 0.21568628f, 0.59607846f, 1.0f);
	style.Colors[ImGuiCol_DragDropTarget] = ImVec4(0.49803922f, 0.5137255f, 1.0f, 1.0f);
	style.Colors[ImGuiCol_NavHighlight] = ImVec4(0.49803922f, 0.5137255f, 1.0f, 1.0f);
	style.Colors[ImGuiCol_NavWindowingHighlight] = ImVec4(0.49803922f, 0.5137255f, 1.0f, 1.0f);
	style.Colors[ImGuiCol_NavWindowingDimBg] = ImVec4(0.19607843f, 0.1764706f, 0.54509807f, 0.5019608f);
	style.Colors[ImGuiCol_ModalWindowDimBg] = ImVec4(0.19607843f, 0.1764706f, 0.54509807f, 0.5019608f);
}

/// @brief Moonlight style by rewrking from ImThemes
void WinMgr::Style_Moonlight(){
	ImGuiStyle& style = ImGui::GetStyle();
	
	style.Alpha = 1.0f;
	style.DisabledAlpha = 1.0f;
	style.WindowPadding = ImVec2(12.0f, 12.0f);
	style.WindowRounding = 11.5f;
	style.WindowBorderSize = 0.0f;
	style.WindowMinSize = ImVec2(20.0f, 20.0f);
	style.WindowTitleAlign = ImVec2(0.5f, 0.5f);
	style.WindowMenuButtonPosition = ImGuiDir_Right;
	style.ChildRounding = 0.0f;
	style.ChildBorderSize = 1.0f;
	style.PopupRounding = 0.0f;
	style.PopupBorderSize = 1.0f;
	style.FramePadding = ImVec2(20.0f, 3.4f);
	style.FrameRounding = 11.9f;
	style.FrameBorderSize = 0.0f;
	style.ItemSpacing = ImVec2(4.3f, 5.5f);
	style.ItemInnerSpacing = ImVec2(7.1f, 1.8f);
	style.CellPadding = ImVec2(12.1f, 9.2f);
	style.IndentSpacing = 0.0f;
	style.ColumnsMinSpacing = 4.9f;
	style.ScrollbarSize = 11.6f;
	style.ScrollbarRounding = 15.9f;
	style.GrabMinSize = 3.7f;
	style.GrabRounding = 20.0f;
	style.TabRounding = 0.0f;
	style.TabBorderSize = 0.0f;
	style.ColorButtonPosition = ImGuiDir_Right;
	style.ButtonTextAlign = ImVec2(0.5f, 0.5f);
	style.SelectableTextAlign = ImVec2(0.0f, 0.0f);
	
	style.Colors[ImGuiCol_Text] = ImVec4(1.0f, 1.0f, 1.0f, 1.0f);
	style.Colors[ImGuiCol_TextDisabled] = ImVec4(0.27450982f, 0.31764707f, 0.4509804f, 1.0f);
	style.Colors[ImGuiCol_WindowBg] = ImVec4(0.078431375f, 0.08627451f, 0.101960786f, 1.0f);
	style.Colors[ImGuiCol_ChildBg] = ImVec4(0.09411765f, 0.101960786f, 0.11764706f, 1.0f);
	style.Colors[ImGuiCol_PopupBg] = ImVec4(0.078431375f, 0.08627451f, 0.101960786f, 1.0f);
	style.Colors[ImGuiCol_Border] = ImVec4(0.15686275f, 0.16862746f, 0.19215687f, 1.0f);
	style.Colors[ImGuiCol_BorderShadow] = ImVec4(0.078431375f, 0.08627451f, 0.101960786f, 1.0f);
	style.Colors[ImGuiCol_FrameBg] = ImVec4(0.11372549f, 0.1254902f, 0.15294118f, 1.0f);
	style.Colors[ImGuiCol_FrameBgHovered] = ImVec4(0.15686275f, 0.16862746f, 0.19215687f, 1.0f);
	style.Colors[ImGuiCol_FrameBgActive] = ImVec4(0.15686275f, 0.16862746f, 0.19215687f, 1.0f);
	style.Colors[ImGuiCol_TitleBg] = ImVec4(0.047058824f, 0.05490196f, 0.07058824f, 1.0f);
	style.Colors[ImGuiCol_TitleBgActive] = ImVec4(0.047058824f, 0.05490196f, 0.07058824f, 1.0f);
	style.Colors[ImGuiCol_TitleBgCollapsed] = ImVec4(0.078431375f, 0.08627451f, 0.101960786f, 1.0f);
	style.Colors[ImGuiCol_MenuBarBg] = ImVec4(0.09803922f, 0.105882354f, 0.12156863f, 1.0f);
	style.Colors[ImGuiCol_ScrollbarBg] = ImVec4(0.047058824f, 0.05490196f, 0.07058824f, 1.0f);
	style.Colors[ImGuiCol_ScrollbarGrab] = ImVec4(0.11764706f, 0.13333334f, 0.14901961f, 1.0f);
	style.Colors[ImGuiCol_ScrollbarGrabHovered] = ImVec4(0.15686275f, 0.16862746f, 0.19215687f, 1.0f);
	style.Colors[ImGuiCol_ScrollbarGrabActive] = ImVec4(0.11764706f, 0.13333334f, 0.14901961f, 1.0f);
	style.Colors[ImGuiCol_CheckMark] = ImVec4(0.972549f, 1.0f, 0.49803922f, 1.0f);
	style.Colors[ImGuiCol_SliderGrab] = ImVec4(0.972549f, 1.0f, 0.49803922f, 1.0f);
	style.Colors[ImGuiCol_SliderGrabActive] = ImVec4(1.0f, 0.79607844f, 0.49803922f, 1.0f);
	style.Colors[ImGuiCol_Button] = ImVec4(0.11764706f, 0.13333334f, 0.14901961f, 1.0f);
	style.Colors[ImGuiCol_ButtonHovered] = ImVec4(0.18039216f, 0.1882353f, 0.19607843f, 1.0f);
	style.Colors[ImGuiCol_ButtonActive] = ImVec4(0.15294118f, 0.15294118f, 0.15294118f, 1.0f);
	style.Colors[ImGuiCol_Header] = ImVec4(0.14117648f, 0.16470589f, 0.20784314f, 1.0f);
	style.Colors[ImGuiCol_HeaderHovered] = ImVec4(0.105882354f, 0.105882354f, 0.105882354f, 1.0f);
	style.Colors[ImGuiCol_HeaderActive] = ImVec4(0.078431375f, 0.08627451f, 0.101960786f, 1.0f);
	style.Colors[ImGuiCol_Separator] = ImVec4(0.12941177f, 0.14901961f, 0.19215687f, 1.0f);
	style.Colors[ImGuiCol_SeparatorHovered] = ImVec4(0.15686275f, 0.18431373f, 0.2509804f, 1.0f);
	style.Colors[ImGuiCol_SeparatorActive] = ImVec4(0.15686275f, 0.18431373f, 0.2509804f, 1.0f);
	style.Colors[ImGuiCol_ResizeGrip] = ImVec4(0.14509805f, 0.14509805f, 0.14509805f, 1.0f);
	style.Colors[ImGuiCol_ResizeGripHovered] = ImVec4(0.972549f, 1.0f, 0.49803922f, 1.0f);
	style.Colors[ImGuiCol_ResizeGripActive] = ImVec4(1.0f, 1.0f, 1.0f, 1.0f);
	style.Colors[ImGuiCol_Tab] = ImVec4(0.078431375f, 0.08627451f, 0.101960786f, 1.0f);
	style.Colors[ImGuiCol_TabHovered] = ImVec4(0.11764706f, 0.13333334f, 0.14901961f, 1.0f);
	style.Colors[ImGuiCol_TabActive] = ImVec4(0.11764706f, 0.13333334f, 0.14901961f, 1.0f);
	style.Colors[ImGuiCol_TabUnfocused] = ImVec4(0.078431375f, 0.08627451f, 0.101960786f, 1.0f);
	style.Colors[ImGuiCol_TabUnfocusedActive] = ImVec4(0.1254902f, 0.27450982f, 0.57254905f, 1.0f);
	style.Colors[ImGuiCol_PlotLines] = ImVec4(0.52156866f, 0.6f, 0.7019608f, 1.0f);
	style.Colors[ImGuiCol_PlotLinesHovered] = ImVec4(0.039215688f, 0.98039216f, 0.98039216f, 1.0f);
	style.Colors[ImGuiCol_PlotHistogram] = ImVec4(0.88235295f, 0.79607844f, 0.56078434f, 1.0f);
	style.Colors[ImGuiCol_PlotHistogramHovered] = ImVec4(0.95686275f, 0.95686275f, 0.95686275f, 1.0f);
	style.Colors[ImGuiCol_TableHeaderBg] = ImVec4(0.047058824f, 0.05490196f, 0.07058824f, 1.0f);
	style.Colors[ImGuiCol_TableBorderStrong] = ImVec4(0.047058824f, 0.05490196f, 0.07058824f, 1.0f);
	style.Colors[ImGuiCol_TableBorderLight] = ImVec4(0.0f, 0.0f, 0.0f, 1.0f);
	style.Colors[ImGuiCol_TableRowBg] = ImVec4(0.11764706f, 0.13333334f, 0.14901961f, 1.0f);
	style.Colors[ImGuiCol_TableRowBgAlt] = ImVec4(0.09803922f, 0.105882354f, 0.12156863f, 1.0f);
	style.Colors[ImGuiCol_TextSelectedBg] = ImVec4(0.9372549f, 0.9372549f, 0.9372549f, 1.0f);
	style.Colors[ImGuiCol_DragDropTarget] = ImVec4(0.49803922f, 0.5137255f, 1.0f, 1.0f);
	style.Colors[ImGuiCol_NavHighlight] = ImVec4(0.26666668f, 0.2901961f, 1.0f, 1.0f);
	style.Colors[ImGuiCol_NavWindowingHighlight] = ImVec4(0.49803922f, 0.5137255f, 1.0f, 1.0f);
	style.Colors[ImGuiCol_NavWindowingDimBg] = ImVec4(0.19607843f, 0.1764706f, 0.54509807f, 0.5019608f);
	style.Colors[ImGuiCol_ModalWindowDimBg] = ImVec4(0.19607843f, 0.1764706f, 0.54509807f, 0.5019608f);
}

///@brief Rounded Visual Studio style by RedNicStone from ImThemes
void WinMgr::Style_VisualStudio()
{
	ImGuiStyle& style = ImGui::GetStyle();
	
	style.Alpha = 1.0f;
	style.DisabledAlpha = 0.6f;
	style.WindowPadding = ImVec2(8.0f, 8.0f);
	style.WindowRounding = 4.0f;
	style.WindowBorderSize = 0.0f;
	style.WindowMinSize = ImVec2(32.0f, 32.0f);
	style.WindowTitleAlign = ImVec2(0.0f, 0.5f);
	style.WindowMenuButtonPosition = ImGuiDir_Left;
	style.ChildRounding = 0.0f;
	style.ChildBorderSize = 1.0f;
	style.PopupRounding = 4.0f;
	style.PopupBorderSize = 1.0f;
	style.FramePadding = ImVec2(4.0f, 3.0f);
	style.FrameRounding = 2.5f;
	style.FrameBorderSize = 0.0f;
	style.ItemSpacing = ImVec2(8.0f, 4.0f);
	style.ItemInnerSpacing = ImVec2(4.0f, 4.0f);
	style.CellPadding = ImVec2(4.0f, 2.0f);
	style.IndentSpacing = 21.0f;
	style.ColumnsMinSpacing = 6.0f;
	style.ScrollbarSize = 11.0f;
	style.ScrollbarRounding = 2.5f;
	style.GrabMinSize = 10.0f;
	style.GrabRounding = 2.0f;
	style.TabRounding = 3.5f;
	style.TabBorderSize = 0.0f;
	style.ColorButtonPosition = ImGuiDir_Right;
	style.ButtonTextAlign = ImVec2(0.5f, 0.5f);
	style.SelectableTextAlign = ImVec2(0.0f, 0.0f);
	
	style.Colors[ImGuiCol_Text] = ImVec4(1.0f, 1.0f, 1.0f, 1.0f);
	style.Colors[ImGuiCol_TextDisabled] = ImVec4(0.5921569f, 0.5921569f, 0.5921569f, 1.0f);
	style.Colors[ImGuiCol_WindowBg] = ImVec4(0.14509805f, 0.14509805f, 0.14901961f, 1.0f);
	style.Colors[ImGuiCol_ChildBg] = ImVec4(0.14509805f, 0.14509805f, 0.14901961f, 1.0f);
	style.Colors[ImGuiCol_PopupBg] = ImVec4(0.14509805f, 0.14509805f, 0.14901961f, 1.0f);
	style.Colors[ImGuiCol_Border] = ImVec4(0.30588236f, 0.30588236f, 0.30588236f, 1.0f);
	style.Colors[ImGuiCol_BorderShadow] = ImVec4(0.30588236f, 0.30588236f, 0.30588236f, 1.0f);
	style.Colors[ImGuiCol_FrameBg] = ImVec4(0.2f, 0.2f, 0.21568628f, 1.0f);
	style.Colors[ImGuiCol_FrameBgHovered] = ImVec4(0.11372549f, 0.5921569f, 0.9254902f, 1.0f);
	style.Colors[ImGuiCol_FrameBgActive] = ImVec4(0.0f, 0.46666667f, 0.78431374f, 1.0f);
	style.Colors[ImGuiCol_TitleBg] = ImVec4(0.14509805f, 0.14509805f, 0.14901961f, 1.0f);
	style.Colors[ImGuiCol_TitleBgActive] = ImVec4(0.14509805f, 0.14509805f, 0.14901961f, 1.0f);
	style.Colors[ImGuiCol_TitleBgCollapsed] = ImVec4(0.14509805f, 0.14509805f, 0.14901961f, 1.0f);
	style.Colors[ImGuiCol_MenuBarBg] = ImVec4(0.2f, 0.2f, 0.21568628f, 1.0f);
	style.Colors[ImGuiCol_ScrollbarBg] = ImVec4(0.2f, 0.2f, 0.21568628f, 1.0f);
	style.Colors[ImGuiCol_ScrollbarGrab] = ImVec4(0.32156864f, 0.32156864f, 0.33333334f, 1.0f);
	style.Colors[ImGuiCol_ScrollbarGrabHovered] = ImVec4(0.3529412f, 0.3529412f, 0.37254903f, 1.0f);
	style.Colors[ImGuiCol_ScrollbarGrabActive] = ImVec4(0.3529412f, 0.3529412f, 0.37254903f, 1.0f);
	style.Colors[ImGuiCol_CheckMark] = ImVec4(0.0f, 0.46666667f, 0.78431374f, 1.0f);
	style.Colors[ImGuiCol_SliderGrab] = ImVec4(0.11372549f, 0.5921569f, 0.9254902f, 1.0f);
	style.Colors[ImGuiCol_SliderGrabActive] = ImVec4(0.0f, 0.46666667f, 0.78431374f, 1.0f);
	style.Colors[ImGuiCol_Button] = ImVec4(0.2f, 0.2f, 0.21568628f, 1.0f);
	style.Colors[ImGuiCol_ButtonHovered] = ImVec4(0.11372549f, 0.5921569f, 0.9254902f, 1.0f);
	style.Colors[ImGuiCol_ButtonActive] = ImVec4(0.11372549f, 0.5921569f, 0.9254902f, 1.0f);
	style.Colors[ImGuiCol_Header] = ImVec4(0.2f, 0.2f, 0.21568628f, 1.0f);
	style.Colors[ImGuiCol_HeaderHovered] = ImVec4(0.11372549f, 0.5921569f, 0.9254902f, 1.0f);
	style.Colors[ImGuiCol_HeaderActive] = ImVec4(0.0f, 0.46666667f, 0.78431374f, 1.0f);
	style.Colors[ImGuiCol_Separator] = ImVec4(0.30588236f, 0.30588236f, 0.30588236f, 1.0f);
	style.Colors[ImGuiCol_SeparatorHovered] = ImVec4(0.30588236f, 0.30588236f, 0.30588236f, 1.0f);
	style.Colors[ImGuiCol_SeparatorActive] = ImVec4(0.30588236f, 0.30588236f, 0.30588236f, 1.0f);
	style.Colors[ImGuiCol_ResizeGrip] = ImVec4(0.14509805f, 0.14509805f, 0.14901961f, 1.0f);
	style.Colors[ImGuiCol_ResizeGripHovered] = ImVec4(0.2f, 0.2f, 0.21568628f, 1.0f);
	style.Colors[ImGuiCol_ResizeGripActive] = ImVec4(0.32156864f, 0.32156864f, 0.33333334f, 1.0f);
	style.Colors[ImGuiCol_Tab] = ImVec4(0.14509805f, 0.14509805f, 0.14901961f, 1.0f);
	style.Colors[ImGuiCol_TabHovered] = ImVec4(0.11372549f, 0.5921569f, 0.9254902f, 1.0f);
	style.Colors[ImGuiCol_TabActive] = ImVec4(0.0f, 0.46666667f, 0.78431374f, 1.0f);
	style.Colors[ImGuiCol_TabUnfocused] = ImVec4(0.14509805f, 0.14509805f, 0.14901961f, 1.0f);
	style.Colors[ImGuiCol_TabUnfocusedActive] = ImVec4(0.0f, 0.46666667f, 0.78431374f, 1.0f);
	style.Colors[ImGuiCol_PlotLines] = ImVec4(0.0f, 0.46666667f, 0.78431374f, 1.0f);
	style.Colors[ImGuiCol_PlotLinesHovered] = ImVec4(0.11372549f, 0.5921569f, 0.9254902f, 1.0f);
	style.Colors[ImGuiCol_PlotHistogram] = ImVec4(0.0f, 0.46666667f, 0.78431374f, 1.0f);
	style.Colors[ImGuiCol_PlotHistogramHovered] = ImVec4(0.11372549f, 0.5921569f, 0.9254902f, 1.0f);
	style.Colors[ImGuiCol_TableHeaderBg] = ImVec4(0.1882353f, 0.1882353f, 0.2f, 1.0f);
	style.Colors[ImGuiCol_TableBorderStrong] = ImVec4(0.30980393f, 0.30980393f, 0.34901962f, 1.0f);
	style.Colors[ImGuiCol_TableBorderLight] = ImVec4(0.22745098f, 0.22745098f, 0.24705882f, 1.0f);
	style.Colors[ImGuiCol_TableRowBg] = ImVec4(0.0f, 0.0f, 0.0f, 0.0f);
	style.Colors[ImGuiCol_TableRowBgAlt] = ImVec4(1.0f, 1.0f, 1.0f, 0.06f);
	style.Colors[ImGuiCol_TextSelectedBg] = ImVec4(0.0f, 0.46666667f, 0.78431374f, 1.0f);
	style.Colors[ImGuiCol_DragDropTarget] = ImVec4(0.14509805f, 0.14509805f, 0.14901961f, 1.0f);
	style.Colors[ImGuiCol_NavHighlight] = ImVec4(0.14509805f, 0.14509805f, 0.14901961f, 1.0f);
	style.Colors[ImGuiCol_NavWindowingHighlight] = ImVec4(1.0f, 1.0f, 1.0f, 0.7f);
	style.Colors[ImGuiCol_NavWindowingDimBg] = ImVec4(0.8f, 0.8f, 0.8f, 0.2f);
	style.Colors[ImGuiCol_ModalWindowDimBg] = ImVec4(0.14509805f, 0.14509805f, 0.14901961f, 1.0f);
}