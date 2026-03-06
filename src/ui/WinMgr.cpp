
/// ------------------------------- INFORMACIÓN -------------------------------------
//	# Configuraciones de ventana
//	- Buscar en imgui.h -> ImGuiWindowFlags_
//  - Añadir en una variable: ImGuiWindowFlags window_flags = *********************;
//	- Añadir a la ventana (en Begin("", nullptr, window_flags); )
// ---------------------------------------------------------------------------------

#include "ui/WinMgr.h"			// Clase de gestión de UI
#include <stb_image.h>          // Implementación para soporte de imágenes.
#include <iostream>

// de la clase imgui de dialogo
#include <GLFW/glfw3.h>
#include <implot.h>
#include <imgui_internal.h>
#include <backends/imgui_impl_glfw.h>
#include <backends/imgui_impl_opengl3.h>
#include "resources.h"  // icono
#ifdef _WIN32
    #include <GLFW/glfw3native.h>
    #include <windows.h>
    #include <dwmapi.h>
#endif

using namespace ImGui;

// General ------------------------------------------------------------------------------

WinMgr::WinMgr(IAppControl* controller) : ctrl_(controller) {
	if (ctrl_==nullptr)
		std::cerr << "[WinMgr]	Cannot handle any controller." << std::endl;
}

WinMgr::~WinMgr() {
	cerrar();
}

void WinMgr::setController(IAppControl* controller){
	ctrl_ = controller;
}

bool WinMgr::init() {

	std::cout << "[WinMgr]	Initializating UI..." << std::endl;

	if (ctrl_!=nullptr)
		std::cout << "[WinMgr]	Linked with IAppController" << std::endl;
	else
		std::cerr << "[WinMgr]	ERROR No controller linked." << std::endl;

    if (!glfwInit()) {
		std::cerr << "[WinMgr]	ERROR glfwInit error." << std::endl;
		return false;
	}

    // Configuración de la ventana GLFW
    glfwWindowHint(GLFW_TRANSPARENT_FRAMEBUFFER, GLFW_TRUE); // Fondo transparente
    glfwWindowHint(GLFW_DECORATED, GLFW_TRUE);               // Bordes y barra de título
    glfwWindowHint(GLFW_RESIZABLE, GLFW_TRUE);               // Redimensionable

    // Creación de ventana
    window_ = glfwCreateWindow(sizeX_, sizeY_, AppName_.c_str(), NULL, NULL);
    if(!window_) {
        glfwTerminate();
        std::cerr << "[WinMgr]	EEROR glfwCreateWindow error." << std::endl;
        return false;
    }
    glfwMakeContextCurrent(window_);
    glfwSwapInterval(1);

    // Inicializa ImGui
    IMGUI_CHECKVERSION();
    ImGui::CreateContext();
	
	style_ = &GetStyle();
    io_ = &GetIO(); (void)io_; 
	
    // Cargar fuente personalizada
    io_->IniFilename = NULL;  // No usar archivo .ini de imgui
    io_->Fonts->AddFontFromFileTTF(customFont_.c_str(), (float)fontSize_, NULL, io_->Fonts->GetGlyphRangesDefault());

    // Configuración de estilo
	StyleColorsLight();
	Style_Microfost();

    // Propiedades de ventana de windows
    #ifdef _WIN32
        HINSTANCE hInstance = GetModuleHandle(NULL);
        HWND hwnd = glfwGetWin32Window(window_);

        // Cargar el icono de la ventana utilizando WinAPI
        HICON hIcon = LoadIcon(hInstance, MAKEINTRESOURCE(IDI_ICON1));
        SendMessage(hwnd, WM_SETICON, ICON_BIG, (LPARAM)hIcon);

        // Cambiar color de la barra de titulo (2)
        BOOL useDarkMode = FALSE;
        DwmSetWindowAttribute(hwnd, 20, &useDarkMode, sizeof(useDarkMode));
    #endif

	// Carga de imágenes
	loadImages();

    ImGui_ImplGlfw_InitForOpenGL(window_, true);
    ImGui_ImplOpenGL3_Init("#version 130");     // Versión de OpenGL
    return true;
}

void WinMgr::run() {

	while (isRunning())
		BuclePrincipal();		// <-- Se queda aqui hasta cerrar
	
	std::cout << "[WinMgr]	Closing window..." << std::endl;
	cerrar();
}

bool WinMgr::isRunning() const {
    return window_ && !glfwWindowShouldClose(window_);
}

void WinMgr::cerrar() {
	// No intentar cerrar de nuevo (excepción)
	if(cerrado_) 
		return;

    ImGui_ImplOpenGL3_Shutdown();
    ImGui_ImplGlfw_Shutdown();
    ImGui::DestroyContext();
    if (window_)
    {
        glfwDestroyWindow(window_);
        window_ = nullptr;
    }
    glfwTerminate();

	// Liberar recursos de imágenes cargadas
	unloadImages();
	
	cerrado_ = true;
}


void WinMgr::initCuadro() {
    glfwPollEvents();
    
    ImGui_ImplOpenGL3_NewFrame();
    ImGui_ImplGlfw_NewFrame();
    ImGui::NewFrame();
}

void WinMgr::endCuadro() {
    // Renderiza
    ImGui::Render();
    glClear(GL_COLOR_BUFFER_BIT);
    ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());
    glfwSwapBuffers(window_);
}

void WinMgr::captureKeys() {

	// Modo debug de detección de teclas
	if (captureKeys_)
		for (int key = ImGuiKey_NamedKey_BEGIN; key < ImGuiKey_NamedKey_END; key++)
			if (ImGui::IsKeyPressed((ImGuiKey)key))
				printf("Key pressed: %d\n", key);

	// Aumentar el tamaño de la fuente Ctrl+"+"
	if (io_->KeyCtrl && (IsKeyPressed((ImGuiKey)605) || IsKeyPressed((ImGuiKey)626)) )
		updateDensity(1);
	
	// Reducir el tamaño de la fuente Ctrl+"-"
	if (io_->KeyCtrl && (IsKeyPressed(ImGuiKey_Minus) || IsKeyPressed((ImGuiKey)625)) )
		updateDensity(-1);

}

void WinMgr::BuclePrincipal() {
    initCuadro();
	captureKeys();

    //ShowDemoWindow();     // Ventana de demostración
	//ShowMetricsWindow();  // Ventana de métricas

	// vvvvvvvvv Contenido de la ventana vvvvvvvvv

	// BARRA DE MENÚ SUPERIOR
	crearMainMenuBar();

	// Forzar bordes cuadrados y eliminar paddings innecesarios para el frame principal
    PushStyleVar(ImGuiStyleVar_WindowRounding, 0.0f);
    PushStyleVar(ImGuiStyleVar_ChildRounding, 0.0f);
    PushStyleVar(ImGuiStyleVar_WindowBorderSize, 0.0f);
    PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(10, 10)); 

	SetNextWindowPos(ImVec2(0,MainMenuBar_Height_));
	SetNextWindowSize(ImVec2(io_->DisplaySize.x, io_->DisplaySize.y - MainMenuBar_Height_));
    
	ImGuiWindowFlags window_flags = ImGuiWindowFlags_NoTitleBar |
                                    ImGuiWindowFlags_NoCollapse |
                                    ImGuiWindowFlags_NoResize |
                                    ImGuiWindowFlags_NoScrollbar |
                                    ImGuiWindowFlags_NoMove |
                                    ImGuiWindowFlags_NoBringToFrontOnFocus |
                                    ImGuiWindowFlags_NoNav;

	// Ventana que cubre todo el frame
	Begin("Ventana que cubre todo el frame", nullptr, window_flags);
		ventanaPrincipal();
    End();
    PopStyleVar(4);

	
	// ^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^

    endCuadro();
}

void WinMgr::crearMainMenuBar() {
	if (ImGui::BeginMainMenuBar()) {
		if (ImGui::BeginMenu("File")) {
			if (ImGui::MenuItem("New", "Ctrl+N")) { /* Acción */ }
			if (ImGui::MenuItem("Open", "Ctrl+O")) { /* Acción */ }
			ImGui::Separator();
			if (ImGui::MenuItem("Exit")) { glfwSetWindowShouldClose(window_, true); }
			ImGui::EndMenu();
		}
		if (ImGui::BeginMenu("Edit")) {
			if (ImGui::MenuItem("Undo", "Ctrl+Z")) {}
			if (ImGui::MenuItem("Redo", "Ctrl+Y")) {}
			ImGui::EndMenu();
		}
		if (ImGui::BeginMenu("Settings")) {
			if (ImGui::MenuItem("Connections...")) { /* Abrir un popup de ajustes */ }
			ImGui::EndMenu();
		}
		
		// Guardamos el alto de la barra para ajustar la ventana de abajo
		ImGui::EndMainMenuBar();
	}

	MainMenuBar_Height_ = ImGui::GetFrameHeight();
}

void WinMgr::ventanaPrincipal() {
	// Variables estáticas para guardar las alturas (proporciones iniciales)
	static float heightRightTop = 0.20f; // 20% para F3
	
	float totalHeight = ImGui::GetContentRegionAvail().y;
	float sizeX__Izq = ImGui::GetContentRegionAvail().x * 0.2f;

	// COLUMNA IZQUIERDA
	BeginGroup();
	{
		// Panel F1 (Arriba Izquierda)
		BeginChild("##F1", ImVec2(sizeX__Izq, totalHeight), true);
		Text("F1 (70%% inicial)");

		std::string txt_mode = (ctrl_->getMode()) ? "ONLINE" : "OFFLINE";
		if (Button(txt_mode.c_str())){
			ctrl_->setMode(!ctrl_->getMode());
		}

		EndChild();
	}
	EndGroup();

	SameLine(); // Pegamos la siguiente columna

	// COLUMNA DERECHA
	BeginGroup();
	{
		// Panel F3 (Arriba Derecha)
		BeginChild("##F3", ImVec2(0, totalHeight * heightRightTop), true);
		Text("F3 (20%% inicial)");
		Image(images_["cat.png"].tex, ImVec2(200,100));
		EndChild();

		// Splitter Horizontal Derecho
		Button("##h_splitter_r", ImVec2(-FLT_MIN, 4.0f));
		if (ImGui::IsItemActive())
			heightRightTop += ImGui::GetIO().MouseDelta.y / totalHeight;

		// Panel F4 (Abajo Derecha)
		BeginChild("##F4", ImVec2(0, 0), false);

			unsigned int boxSize= (GetContentRegionAvail().y+GetContentRegionAvail().x)/2 * 0.4;

			BeginChild("##Module",ImVec2(boxSize,boxSize*.5), true);
				Text("Module");

				if ( ImageButton("##BTN_play", images_["play.png"].tex, ImVec2(20,20)) ) {
					std::cout << "Button pressed" << std::endl;
				}
				SameLine();
				if ( ImageButton("##BTN_pause", images_["pause.png"].tex, ImVec2(20,20)) ) {
					std::cout << "Button pressed" << std::endl;
				}
				SameLine();
				if ( ImageButton("##BTN_stop", images_["stop.png"].tex, ImVec2(20,20)) ) {
					std::cout << "Button pressed" << std::endl;
				}
				SliderInt("Volume",&sl_volume,0,100,"%d");
				SliderFloat("Pitch",&sl_pitch,-2,2,"x%.2f");
			EndChild();

			SameLine();

			BeginChild("##Module2",ImVec2(boxSize,boxSize*.5), true);
				Text("Module");
				SliderInt("Volume",&sl_volume,0,100,"%d");
				SliderFloat("Pitch",&sl_pitch,-2,2,"x%.2f");
			EndChild();

		EndChild();
	}
	EndGroup();
}

void WinMgr::loadImages() {
	std::cout << "[WinMgr]	Loading images..." << std::endl;

	// Añadir aquí las imágenes que se desean cargar
	addTextureFromFile("cat.png");
	addTextureFromFile("play.png");
	addTextureFromFile("stop.png");
	addTextureFromFile("pause.png");

}

void WinMgr::unloadImages() {
	std::cout << "[WinMgr]	Unloading images..." << std::endl;

	GLuint glTex;

	// Descargar todas las imágenes guardadas
	for (auto & img : images_) {
        if (img.second.tex != 0) {
            std::cout << "[WinMgr] Deleting texture: " << img.first << std::endl;
            
            // Convertimos el uintptr_t de vuelta a GLuint para OpenGL
            glTex = (GLuint)img.second.tex;
            glDeleteTextures(1, &glTex);
            
            img.second.tex = 0; // Limpiar para evitar usos accidentales
        }
    }
}

void WinMgr::addTextureFromFile(std::string filename) {
	imageData img_data;		// Variable temporal para almacenar datos de la imagen cargada
	int channels;			// Canales de imagen

	// Carga de archivo de imagen
	unsigned char* data = stbi_load(filename.c_str(), &img_data.x, &img_data.y, &channels, 4);
	if (!data)
	{
		std::cerr << "[WinMgr]	ERROR loading image: " << filename << std::endl;
		return;
	}

	// Textura desde imagen
	GLuint texture;
	glGenTextures(1, &texture);
	glBindTexture(GL_TEXTURE_2D, texture);
	glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA,	img_data.x, img_data.y ,0 , GL_RGBA, GL_UNSIGNED_BYTE, data);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);

	// Guardar la textura generada
	img_data.tex = (uintptr_t)texture;

	// Liberar memoria de datos de imagen temporales
	stbi_image_free(data);

	// Guardar la imagen en el mapa de imágenes
	images_[filename] = img_data;
	images_[filename].name = filename.c_str();
	
	std::cout << "[WinMgr]	Loaded image " << filename << std::endl;
    return;
};

void WinMgr::updateDensity(int delta) {

    int new_size = (int)style_->FontSizeBase + delta;

    if (new_size > (int)MAX_FONT_SIZE_) {
        std::cerr << "[WinMgr] WARN font size bigger than max allowed." << std::endl;
        return;
    }
    if (new_size < (int)MIN_FONT_SIZE_) {
        std::cerr << "[WinMgr] WARN font size smaller than min allowed." << std::endl;
        return;
    }

    // Guardar factor de escala relativo al tamaño anterior
    float prev = style_->FontSizeBase;
    float next = (float)new_size;
    float scale = (prev > 0.0f) ? (next / prev) : 1.0f;

    // Aplicar nuevo tamaño de fuente
    style_->FontSizeBase = next;
    style_->_NextFrameFontSizeBase = style_->FontSizeBase;

    // Escalar paddings y espacios que afectan a botones
    style_->FramePadding = ImVec2(style_->FramePadding.x * scale, style_->FramePadding.y * scale);
    style_->ItemSpacing = ImVec2(style_->ItemSpacing.x * scale, style_->ItemSpacing.y * scale);
    style_->ItemInnerSpacing = ImVec2(style_->ItemInnerSpacing.x * scale, style_->ItemInnerSpacing.y * scale);
    style_->WindowPadding = ImVec2(style_->WindowPadding.x * scale, style_->WindowPadding.y * scale);
    style_->FrameRounding = style_->FrameRounding * scale;
    style_->GrabRounding = style_->GrabRounding * scale;

    std::cout << "[WinMgr] Density adjusted to font size: " << style_->FontSizeBase << std::endl;
};

// Temas --------------------------------------------------------------------------------

void WinMgr::Style_Confy() {
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

void WinMgr::Style_FutureDark() {
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

void WinMgr::Style_Moonlight() {
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

void WinMgr::Style_VisualStudio() {
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

void WinMgr::Style_Microfost() {

	// Microsoft style by usernameiwantedwasalreadytaken from ImThemes
	ImGuiStyle& style = ImGui::GetStyle();
	
	style.Alpha = 1.0f;
	style.DisabledAlpha = 0.6f;
	style.WindowPadding = ImVec2(4.0f, 6.0f);
	style.WindowRounding = 0.0f;
	style.WindowBorderSize = 0.0f;
	style.WindowMinSize = ImVec2(32.0f, 32.0f);
	style.WindowTitleAlign = ImVec2(0.0f, 0.5f);
	style.WindowMenuButtonPosition = ImGuiDir_Left;
	style.ChildRounding = 0.0f;
	style.ChildBorderSize = 1.0f;
	style.PopupRounding = 0.0f;
	style.PopupBorderSize = 1.0f;
	style.FramePadding = ImVec2(8.0f, 6.0f);
	style.FrameRounding = 0.0f;
	style.FrameBorderSize = 1.0f;
	style.ItemSpacing = ImVec2(8.0f, 6.0f);
	style.ItemInnerSpacing = ImVec2(8.0f, 6.0f);
	style.CellPadding = ImVec2(4.0f, 2.0f);
	style.IndentSpacing = 20.0f;
	style.ColumnsMinSpacing = 6.0f;
	style.ScrollbarSize = 20.0f;
	style.ScrollbarRounding = 0.0f;
	style.GrabMinSize = 5.0f;
	style.GrabRounding = 0.0f;
	style.TabRounding = 4.0f;
	style.TabBorderSize = 0.0f;
	style.ColorButtonPosition = ImGuiDir_Right;
	style.ButtonTextAlign = ImVec2(0.5f, 0.5f);
	style.SelectableTextAlign = ImVec2(0.0f, 0.0f);
	
	style.Colors[ImGuiCol_Text] = ImVec4(0.09803922f, 0.09803922f, 0.09803922f, 1.0f);
	style.Colors[ImGuiCol_TextDisabled] = ImVec4(0.49803922f, 0.49803922f, 0.49803922f, 1.0f);
	style.Colors[ImGuiCol_WindowBg] = ImVec4(0.9490196f, 0.9490196f, 0.9490196f, 1.0f);
	style.Colors[ImGuiCol_ChildBg] = ImVec4(0.9490196f, 0.9490196f, 0.9490196f, 1.0f);
	style.Colors[ImGuiCol_PopupBg] = ImVec4(1.0f, 1.0f, 1.0f, 1.0f);
	style.Colors[ImGuiCol_Border] = ImVec4(0.6f, 0.6f, 0.6f, 1.0f);
	style.Colors[ImGuiCol_BorderShadow] = ImVec4(0.0f, 0.0f, 0.0f, 0.0f);
	style.Colors[ImGuiCol_FrameBg] = ImVec4(1.0f, 1.0f, 1.0f, 1.0f);
	style.Colors[ImGuiCol_FrameBgHovered] = ImVec4(0.0f, 0.46666667f, 0.8392157f, 0.2f);
	style.Colors[ImGuiCol_FrameBgActive] = ImVec4(0.0f, 0.46666667f, 0.8392157f, 1.0f);
	style.Colors[ImGuiCol_TitleBg] = ImVec4(0.039215688f, 0.039215688f, 0.039215688f, 1.0f);
	style.Colors[ImGuiCol_TitleBgActive] = ImVec4(0.15686275f, 0.28627452f, 0.47843137f, 1.0f);
	style.Colors[ImGuiCol_TitleBgCollapsed] = ImVec4(0.0f, 0.0f, 0.0f, 0.51f);
	style.Colors[ImGuiCol_MenuBarBg] = ImVec4(0.85882354f, 0.85882354f, 0.85882354f, 1.0f);
	style.Colors[ImGuiCol_ScrollbarBg] = ImVec4(0.85882354f, 0.85882354f, 0.85882354f, 1.0f);
	style.Colors[ImGuiCol_ScrollbarGrab] = ImVec4(0.6862745f, 0.6862745f, 0.6862745f, 1.0f);
	style.Colors[ImGuiCol_ScrollbarGrabHovered] = ImVec4(0.0f, 0.0f, 0.0f, 0.2f);
	style.Colors[ImGuiCol_ScrollbarGrabActive] = ImVec4(0.0f, 0.0f, 0.0f, 0.5f);
	style.Colors[ImGuiCol_CheckMark] = ImVec4(0.09803922f, 0.09803922f, 0.09803922f, 1.0f);
	style.Colors[ImGuiCol_SliderGrab] = ImVec4(0.6862745f, 0.6862745f, 0.6862745f, 1.0f);
	style.Colors[ImGuiCol_SliderGrabActive] = ImVec4(0.0f, 0.0f, 0.0f, 0.5f);
	style.Colors[ImGuiCol_Button] = ImVec4(0.85882354f, 0.85882354f, 0.85882354f, 1.0f);
	style.Colors[ImGuiCol_ButtonHovered] = ImVec4(0.0f, 0.46666667f, 0.8392157f, 0.2f);
	style.Colors[ImGuiCol_ButtonActive] = ImVec4(0.0f, 0.46666667f, 0.8392157f, 1.0f);
	style.Colors[ImGuiCol_Header] = ImVec4(0.85882354f, 0.85882354f, 0.85882354f, 1.0f);
	style.Colors[ImGuiCol_HeaderHovered] = ImVec4(0.0f, 0.46666667f, 0.8392157f, 0.2f);
	style.Colors[ImGuiCol_HeaderActive] = ImVec4(0.0f, 0.46666667f, 0.8392157f, 1.0f);
	style.Colors[ImGuiCol_Separator] = ImVec4(0.42745098f, 0.42745098f, 0.49803922f, 0.5f);
	style.Colors[ImGuiCol_SeparatorHovered] = ImVec4(0.09803922f, 0.4f, 0.7490196f, 0.78f);
	style.Colors[ImGuiCol_SeparatorActive] = ImVec4(0.09803922f, 0.4f, 0.7490196f, 1.0f);
	style.Colors[ImGuiCol_ResizeGrip] = ImVec4(0.25882354f, 0.5882353f, 0.9764706f, 0.2f);
	style.Colors[ImGuiCol_ResizeGripHovered] = ImVec4(0.25882354f, 0.5882353f, 0.9764706f, 0.67f);
	style.Colors[ImGuiCol_ResizeGripActive] = ImVec4(0.25882354f, 0.5882353f, 0.9764706f, 0.95f);
	style.Colors[ImGuiCol_Tab] = ImVec4(0.1764706f, 0.34901962f, 0.5764706f, 0.862f);
	style.Colors[ImGuiCol_TabHovered] = ImVec4(0.25882354f, 0.5882353f, 0.9764706f, 0.8f);
	style.Colors[ImGuiCol_TabActive] = ImVec4(0.19607843f, 0.40784314f, 0.6784314f, 1.0f);
	style.Colors[ImGuiCol_TabUnfocused] = ImVec4(0.06666667f, 0.101960786f, 0.14509805f, 0.9724f);
	style.Colors[ImGuiCol_TabUnfocusedActive] = ImVec4(0.13333334f, 0.25882354f, 0.42352942f, 1.0f);
	style.Colors[ImGuiCol_PlotLines] = ImVec4(0.60784316f, 0.60784316f, 0.60784316f, 1.0f);
	style.Colors[ImGuiCol_PlotLinesHovered] = ImVec4(1.0f, 0.42745098f, 0.34901962f, 1.0f);
	style.Colors[ImGuiCol_PlotHistogram] = ImVec4(0.8980392f, 0.69803923f, 0.0f, 1.0f);
	style.Colors[ImGuiCol_PlotHistogramHovered] = ImVec4(1.0f, 0.6f, 0.0f, 1.0f);
	style.Colors[ImGuiCol_TableHeaderBg] = ImVec4(0.1882353f, 0.1882353f, 0.2f, 1.0f);
	style.Colors[ImGuiCol_TableBorderStrong] = ImVec4(0.30980393f, 0.30980393f, 0.34901962f, 1.0f);
	style.Colors[ImGuiCol_TableBorderLight] = ImVec4(0.22745098f, 0.22745098f, 0.24705882f, 1.0f);
	style.Colors[ImGuiCol_TableRowBg] = ImVec4(0.0f, 0.0f, 0.0f, 0.0f);
	style.Colors[ImGuiCol_TableRowBgAlt] = ImVec4(1.0f, 1.0f, 1.0f, 0.06f);
	style.Colors[ImGuiCol_TextSelectedBg] = ImVec4(0.25882354f, 0.5882353f, 0.9764706f, 0.35f);
	style.Colors[ImGuiCol_DragDropTarget] = ImVec4(1.0f, 1.0f, 0.0f, 0.9f);
	style.Colors[ImGuiCol_NavHighlight] = ImVec4(0.25882354f, 0.5882353f, 0.9764706f, 1.0f);
	style.Colors[ImGuiCol_NavWindowingHighlight] = ImVec4(1.0f, 1.0f, 1.0f, 0.7f);
	style.Colors[ImGuiCol_NavWindowingDimBg] = ImVec4(0.8f, 0.8f, 0.8f, 0.2f);
	style.Colors[ImGuiCol_ModalWindowDimBg] = ImVec4(0.8f, 0.8f, 0.8f, 0.35f);
}
