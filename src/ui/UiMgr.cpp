
/// ------------------------------- INFORMACIÓN -------------------------------------
//	# Configuraciones de ventana
//	- Buscar en imgui.h -> ImGuiWindowFlags_
//  - Añadir en una variable: ImGuiWindowFlags window_flags = *********************;
//	- Añadir a la ventana (en Begin("", nullptr, window_flags); )
// ---------------------------------------------------------------------------------

#include "ui/UiMgr.h"			// Clase de gestión de UI
#include <stb_image.h>          // Implementación para soporte de imágenes.
#include "imgui-knobs.h"		// Soporte de knobs
#include "imspinner.h"			// Soporte de spinners de carga
#include <iostream>

// GLFW / OpenGL
#include <GLFW/glfw3.h>
#include <backends/imgui_impl_glfw.h>
#include <backends/imgui_impl_opengl3.h>
#ifdef _WIN32
    #include <GLFW/glfw3native.h>
#endif

// Imgui
#include <imgui.h>
#include <implot.h>
#include <imgui_internal.h>

// Sistema
#ifdef _WIN32
    #include <windows.h>
    #include <dwmapi.h>
#else
	#include <filesystem>
	namespace fs = std::filesystem;
#endif
#include "resources.h"  // icono
#include "ttf_archive-medium.h"

// Se puede evitar poner "ImGui::" para simplificar
using namespace ImGui;

// General ------------------------------------------------------------------------------

UiMgr::UiMgr(IAppControl* controller) : ctrl_(controller) {
	if (ctrl_==nullptr)
		std::cerr << "[UiMgr]	Cannot handle any controller." << std::endl;
}

UiMgr::~UiMgr() {
	cerrar();
}

void UiMgr::setController(IAppControl* controller){
	ctrl_ = controller;
}

bool UiMgr::init() {

	std::cout << "[UiMgr]	Initializating UI..." << std::endl;

	if (ctrl_!=nullptr)
		std::cout << "[UiMgr]	Linked with IAppController" << std::endl;
	else
		std::cerr << "[UiMgr]	ERROR No controller linked." << std::endl;

    if (!glfwInit()) {
		std::cerr << "[UiMgr]	ERROR glfwInit error." << std::endl;
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
        std::cerr << "[UiMgr]	ERROR glfwCreateWindow error." << std::endl;
        return false;
    }
    glfwMakeContextCurrent(window_);
    glfwSwapInterval(1);

    // Inicializa ImGui
    IMGUI_CHECKVERSION();
    CreateContext();
	
	// Obtener puntero a style e io para poder modificarlo
	style_ = &GetStyle();
    io_ = &GetIO();
	
	// No usar archivo .ini de imgui
    io_->IniFilename = NULL;  

    // Cargar fuente predeterminada (desde resources, el .h)
	ImFontConfig font_cfg;
	font_cfg.FontDataOwnedByAtlas = false; // no liberar memoria de fuente al salir (crash)
	io_->Fonts->AddFontFromMemoryTTF(
		(void*)archivo_medium_ttf, 
		(int)archivo_medium_ttf_len, 
		fontSize_, 
		&font_cfg, 
		io_->Fonts->GetGlyphRangesDefault()
	);

    // Configuración de estilo por defecto
	Style_Microfrost();

    // Propiedades de ventana de windows
    #ifdef _WIN32
        HINSTANCE hInstance = GetModuleHandle(NULL);
        HWND hwnd = glfwGetWin32Window(window_);

        // Cargar el icono de la ventana utilizando WinAPI
        HICON hIcon = LoadIcon(hInstance, MAKEINTRESOURCE(IDI_ICON1));
        SendMessage(hwnd, WM_SETICON, ICON_BIG, (LPARAM)hIcon);
	#else // Alternativa Linux. Necesita el icon.png en imageres
		int width, height, channels;
		fs::path rutaIcono = fs::absolute("imageres") / "icon.png";
		unsigned char* pixels = stbi_load(rutaIcono.string().c_str(), &width, &height, &channels, 4);
		if (pixels) {
			GLFWimage icon_image;
			icon_image.width = width;
			icon_image.height = height;
			icon_image.pixels = pixels;
			glfwSetWindowIcon(window_, 1, &icon_image);
			stbi_image_free(pixels);
			std::cout << "[UiMgr]   Linux window icon loaded from file." << std::endl;
		} else {
			std::cerr << "[UiMgr]   WARN: Could not load icon.png for Linux window." << std::endl;
		}
	#endif

	// Carga de imágenes
	loadImages();

    ImGui_ImplGlfw_InitForOpenGL(window_, true);
    ImGui_ImplOpenGL3_Init("#version 130");     // Versión de OpenGL

	// marcar que está inicializado
	running_ = true;
    return true;
}

void UiMgr::run() {

	while (isRunning())
		BuclePrincipal();		// <-- Se queda aqui hasta cerrar
	
	cerrar();
}

bool UiMgr::isRunning() const {
    return window_ && !glfwWindowShouldClose(window_);
}

void UiMgr::cerrar() {
	// No intentar cerrar de nuevo (excepción)
	if(!running_) 
		return;

	std::cout << "[UiMgr]	Closing UI..." << std::endl;

    ImGui_ImplOpenGL3_Shutdown();
    ImGui_ImplGlfw_Shutdown();
    DestroyContext();
    if (window_)
    {
        glfwDestroyWindow(window_);
        window_ = nullptr;
    }
    glfwTerminate();

	// Liberar recursos de imágenes cargadas
	unloadImages();

	std::cout << "[UiMgr]	UI closed." << std::endl;
	running_ = false;
}


void UiMgr::initCuadro() {
    glfwPollEvents();
    
    ImGui_ImplOpenGL3_NewFrame();
    ImGui_ImplGlfw_NewFrame();
    NewFrame();
}

void UiMgr::endCuadro() {
    // Renderiza
    Render();
    glClear(GL_COLOR_BUFFER_BIT);
    ImGui_ImplOpenGL3_RenderDrawData(GetDrawData());
    glfwSwapBuffers(window_);
}

void UiMgr::captureKeys() {

	// Modo debug de detección de teclas
	if (captureKeys_)
		for (int key = ImGuiKey_NamedKey_BEGIN; key < ImGuiKey_NamedKey_END; key++)
			if (IsKeyPressed((ImGuiKey)key))
				printf("Key pressed: %d\n", key);

	// Aumentar el tamaño de la fuente Ctrl+"+"
	if (io_->KeyCtrl && (IsKeyPressed((ImGuiKey)605) || IsKeyPressed((ImGuiKey)626)) )
		updateDensity(1);
	
	// Reducir el tamaño de la fuente Ctrl+"-"
	if (io_->KeyCtrl && (IsKeyPressed(ImGuiKey_Minus) || IsKeyPressed((ImGuiKey)625)) )
		updateDensity(-1);

}

void UiMgr::BuclePrincipal() {
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

void UiMgr::crearMainMenuBar() {
	if (BeginMainMenuBar()) {
		if (BeginMenu("File")) {
			if (MenuItem("New", "Ctrl+N")) { /* Acción */ }
			if (MenuItem("Open", "Ctrl+O")) { /* Acción */ }
			Separator();
			if (MenuItem("Exit")) { glfwSetWindowShouldClose(window_, true); }
			ImGui::EndMenu();
		}
		if (BeginMenu("Edit")) {
			if (MenuItem("Undo", "Ctrl+Z")) {}
			if (MenuItem("Redo", "Ctrl+Y")) {}
			ImGui::EndMenu();
		}
		if (BeginMenu("Settings")) {
			if (MenuItem("Connections...")) { /* Abrir un popup de ajustes */ }
			ImGui::EndMenu();
		}
		
		// Guardamos el alto de la barra para ajustar la ventana de abajo
		EndMainMenuBar();
	}

	MainMenuBar_Height_ = GetFrameHeight();
}

void UiMgr::ventanaPrincipal() {
	/* CUIDADO: Estas variables se están creando en cada frame (a 60fps). Es más conveniente guardarlas instanciadas en la clase.*/ 
	// Variables estáticas para guardar las alturas (proporciones iniciales)
	static float heightRightTop = 0.5f; 
	float totalHeight = GetContentRegionAvail().y;
	float sizeX__Izq = GetContentRegionAvail().x * 0.3f;

	short TTS_percent;
	std::string TTS_text;
	
	// COLUMNA IZQUIERDA
	BeginGroup();
	{
		// Panel F1 (Arriba Izquierda)
		BeginChild("##F1", ImVec2(sizeX__Izq, totalHeight), true);

		// Mostrar carga de TTS
		TTS_percent = ctrl_->getTTSInitPercent();
		if (TTS_percent < 100) {
			ImSpinner::SpinnerPulsar("Pulsar",  6, 2, ImColor(.5f,.5f,.5f));
			SameLine();
			TTS_text = "Loading TTS voice models: ";
			TTS_text += std::to_string(ctrl_->getLoadedNumTTSModels()) + "/" + std::to_string(ctrl_->getAvailableNumTTSModels());
		} else TTS_text = "TTS voice models loaded.";
		
		Text(TTS_text.c_str());
		ImGui::ProgressBar(TTS_percent/100.0f, ImVec2(0.0f, 0.0f));

		// Botón de modo
		if (Button(       (ctrl_->isOnlineMode()) ? "ONLINE" : "OFFLINE"        ) ){
			ctrl_->setOnlineMode(!ctrl_->isOnlineMode());
		}

		// Test de temas

		if (Button("darkmode window")) 
			titleBarDarkMode(true);
		
		if (Button("lightmode window"))
			titleBarDarkMode(false);

		if (Button("Theme: Confy"))
			Style_Confy();

		if (Button("Theme: FutureDark"))
			Style_FutureDark();

		if (Button("Theme: Moonlight"))
			Style_Moonlight();

		if (Button("Theme: Visual Studio"))
			Style_VisualStudio();

		if (Button("Theme: Microfrost"))
			Style_Microfrost();

		EndChild();
	}
	EndGroup();

	SameLine(); // Pegamos la siguiente columna

	//  Layout principal (columna derecha)
	BeginGroup(); {
		// ---------- Panel F3 (arriba derecha) ----------
		BeginChild("##F3", ImVec2(0, totalHeight * heightRightTop), true);
		Text("Demo panel");
		// Test imagen
		Image(images_["imageres/cat.png"].tex, ImVec2(200, 100));

		// Test spinners
		SameLine();
		ImSpinner::SpinnerFadeDots(		  "dots",	 16, 2, ImColor(.5f,.5f,.5f));		SameLine(0.0, -1.0);
		ImSpinner::SpinnerRainbowMix(     "Rmix",    16, 2, ImColor(1.0f,1.0f,1.0f),4);	SameLine(0.0, -1.0);
		ImSpinner::SpinnerAng8(           "Ang",     16, 2);							SameLine(0.0, -1.0);
		ImSpinner::SpinnerPulsar(         "Pulsar",  16, 2);							SameLine(0.0, -1.0);
		ImSpinner::SpinnerClock(          "Clock",   16, 2);							SameLine(0.0, -1.0);
		ImSpinner::SpinnerAtom(           "atom",    16, 2);							SameLine(0.0, -1.0);
		ImSpinner::SpinnerSwingDots(      "wheel",   16, 6);							SameLine(0.0, -1.0);
		ImSpinner::SpinnerDotsToBar(      "tobar",   16, 2, ImColor(1.0f,1.0f,1.0f),4);	SameLine(0.0, -1.0);
		ImSpinner::SpinnerBarChartRainbow("rainbow", 16, 4, ImColor(1.0f,1.0f,1.0f),4);

		// Test knobs
		static float val1 = 0;
		if (ImGuiKnobs::Knob("Gain", &val1, -6.0f, 6.0f, 0.1f, "%.1fdB", ImGuiKnobVariant_Tick)) {
			// value was changed
		}

		SameLine();

		static float val2 = 0;
		if (ImGuiKnobs::Knob("Mix", &val2, -1.0f, 1.0f, 0.1f, "%.1f", ImGuiKnobVariant_Stepped)) {
			// value was changed
		}

		// Double click to reset
		if (ImGui::IsItemActive() && ImGui::IsMouseDoubleClicked(0)) {
			val2 = 0;
		}


		ImGui::SameLine();

		static float val3 = 0;

		// Custom colors
		ImGui::PushStyleColor(ImGuiCol_ButtonActive, ImVec4(255.f, 0, 0, 0.7f));
		ImGui::PushStyleColor(ImGuiCol_ButtonHovered, ImVec4(255.f, 0, 0, 1));
		ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(0, 255.f, 0, 1));
		// Push/PopStyleColor() for each colors used (namely ImGuiCol_ButtonActive and ImGuiCol_ButtonHovered for primary and ImGuiCol_Framebg for Track)
		if (ImGuiKnobs::Knob("Pitch", &val3, -6.0f, 6.0f, 0.1f, "%.1f", ImGuiKnobVariant_WiperOnly)) {
			// value was changed
		}

		ImGui::PopStyleColor(3);


		ImGui::SameLine();

		// Custom min/max angle
		static float val4 = 0;
		if (ImGuiKnobs::Knob("Dry", &val4, -6.0f, 6.0f, 0.1f, "%.1f", ImGuiKnobVariant_Stepped, 0, 0, 10, 1.570796f, 3.141592f)) {
			// value was changed
		}

		ImGui::SameLine();

		// Int value
		static int val5 = 1;
		if (ImGuiKnobs::KnobInt("Wet", &val5, 1, 10, 0.1f, "%i", ImGuiKnobVariant_Stepped, 0, 0, 10)) {
			// value was changed
		}

		ImGui::SameLine();

		// Vertical drag only
		static float val6 = 1;
		if (ImGuiKnobs::Knob("Vertical", &val6, 0.f, 10.f, 0.1f, "%.1f", ImGuiKnobVariant_Space, 0, ImGuiKnobFlags_DragVertical)) {
			// value was changed
		}

		ImGui::SameLine();

		static float val7 = 500.0f;
		if (ImGuiKnobs::Knob("Logarithmic", &val7, 20, 20000, 20.0f, "%.1f", ImGuiKnobVariant_WiperOnly, 0, ImGuiKnobFlags_Logarithmic | ImGuiKnobFlags_AlwaysClamp)) {
			// value was changed
		}




		EndChild();

		// ---------- Splitter horizontal derecho ----------
		Button("##h_splitter_r", ImVec2(-FLT_MIN, 4.0f));
		if (IsItemActive())
			heightRightTop += GetIO().MouseDelta.y / totalHeight;

		// ---------- Panel F4 (abajo derecha) ----------
		BeginChild("##F4", ImVec2(0, 0), false); {

			//  Cada AudioPlaybackModule (APM) → un bloque colapsable
			if (CollapsingHeader("ADF (running)", false))	{

				ImGuiChildFlags child_flags = ImGuiChildFlags_AlwaysAutoResize |
											ImGuiChildFlags_AutoResizeY |
											ImGuiChildFlags_Borders;

				// ------------------------------------------------
				//  Bucle ficticio: aquí iterarías sobre los tonos
				// ------------------------------------------------
				// for (int i = 0; i < apmCount; ++i) { … }
				// -----------------------------------------------------------------
				//  Ejemplo con 3 tonos (Tone1, Tone3, Tone4) – sustituye por tu bucle
				// -----------------------------------------------------------------

				// ---------- Tono 1 ----------
				if (BeginChild("Tone1Child", ImVec2(0, 0), child_flags))
				{
					Text("Tone1");

					float duration_seconds = 3600.0f; // ← obtener con SoundMgr::getDuration()

					//  Slider de posición + tiempo formateado
					int total = static_cast<int>(sl_position);
					int h = total / 3600;
					int m = (total % 3600) / 60;
					int s = total % 60;
					char buf[16];
					if (h > 0)
						sprintf(buf, "%d:%02d:%02d", h, m, s);
					else
						sprintf(buf, "%02d:%02d", m, s);

					BeginDisabled(ctrl_->isOnlineMode());
					{
						// --- barra de reproducción compacta ---
						PushStyleVar(ImGuiStyleVar_FramePadding, ImVec2(0, 1));
						PushStyleVar(ImGuiStyleVar_GrabMinSize, 12.0f);
						PushStyleVar(ImGuiStyleVar_GrabRounding, 24.0f);
						PushItemWidth(GetContentRegionAvail().x * 0.8f);

						SliderFloat("##Position", &sl_position, 0.0f, duration_seconds, "");

						PopItemWidth();
						PopStyleVar(3);

						SameLine();
						TextUnformatted(buf);

						// --- botones de control ---
						if (ImageButton("##btn_play", images_["imageres/play.png"].tex, ImVec2(20, 20)))
							std::cout << "Button play pressed\n";
						SameLine();
						if (ImageButton("##btn_pause", images_["imageres/pause.png"].tex, ImVec2(20, 20)))
							std::cout << "Button pause pressed\n";
						SameLine();
						if (ImageButton("##btn_stop", images_["imageres/stop.png"].tex, ImVec2(20, 20)))
							std::cout << "Button stop pressed\n";
						SameLine();
						if (ImageButton("##btn_repeat", images_["imageres/repeat.png"].tex, ImVec2(20, 20)))
							std::cout << "Button repeat pressed\n";

						// --- volumen y pitch ---
						SameLine();
						PushItemWidth(GetContentRegionAvail().x * 0.3f);
						SliderInt("Volume", &sl_volume, 0, 100, "%d");
						PopItemWidth();
						SameLine();
						PushItemWidth(GetContentRegionAvail().x * 0.3f);
						SliderFloat("Pitch", &sl_pitch, -2.0f, 2.0f, "x%.2f");
						PopItemWidth();
					}
					EndDisabled();

					EndChild();
				}
				
				// ---------- Tono 2 ----------
				if (BeginChild("Tone2Child", ImVec2(0, 0), child_flags))
				{
					Text("Tone2");

					float duration_seconds = 3600.0f; // ← obtener con SoundMgr::getDuration()

					//  Slider de posición + tiempo formateado
					int total = static_cast<int>(sl_position);
					int h = total / 3600;
					int m = (total % 3600) / 60;
					int s = total % 60;
					char buf[16];
					if (h > 0)
						sprintf(buf, "%d:%02d:%02d", h, m, s);
					else
						sprintf(buf, "%02d:%02d", m, s);

					BeginDisabled(ctrl_->isOnlineMode());
					{
						// --- barra de reproducción compacta ---
						PushStyleVar(ImGuiStyleVar_FramePadding, ImVec2(0, 1));
						PushStyleVar(ImGuiStyleVar_GrabMinSize, 12.0f);
						PushStyleVar(ImGuiStyleVar_GrabRounding, 24.0f);
						PushItemWidth(GetContentRegionAvail().x * 0.8f);

						SliderFloat("##Position", &sl_position, 0.0f, duration_seconds, "");

						PopItemWidth();
						PopStyleVar(3);

						SameLine();
						TextUnformatted(buf);

						// --- botones de control ---
						if (ImageButton("##btn_play", images_["imageres/play.png"].tex, ImVec2(20, 20)))
							std::cout << "Button play pressed\n";
						SameLine();
						if (ImageButton("##btn_pause", images_["imageres/pause.png"].tex, ImVec2(20, 20)))
							std::cout << "Button pause pressed\n";
						SameLine();
						if (ImageButton("##btn_stop", images_["imageres/stop.png"].tex, ImVec2(20, 20)))
							std::cout << "Button stop pressed\n";
						SameLine();
						if (ImageButton("##btn_repeat", images_["imageres/repeat.png"].tex, ImVec2(20, 20)))
							std::cout << "Button repeat pressed\n";

						// --- volumen y pitch ---
						SameLine();
						PushItemWidth(GetContentRegionAvail().x * 0.3f);
						SliderInt("Volume", &sl_volume, 0, 100, "%d");
						PopItemWidth();
						SameLine();
						PushItemWidth(GetContentRegionAvail().x * 0.3f);
						SliderFloat("Pitch", &sl_pitch, -2.0f, 2.0f, "x%.2f");
						PopItemWidth();
					}
					EndDisabled();

					EndChild();
				}

				// ---------- Tono 3 ----------
				if (BeginChild("Tone3Child", ImVec2(0, 0), child_flags))
				{
					Text("Tone3");
					Text("#TODO");
					EndChild();
				}

				// ---------- Tono 4 ----------
				if (BeginChild("Tone4Child", ImVec2(0, 0), child_flags))
				{
					Text("Tone4");
					Text("#TODO");
					EndChild();
				}
			}

			// ----------------------------------------------------
			//  Otros bloques colapsables
			// ----------------------------------------------------
			if (CollapsingHeader("ADF2", false)) { Text("#TODO"); }
			if (CollapsingHeader("NAV", false))  { Text("#TODO"); }
			if (CollapsingHeader("TACAN", false)) { Text("#TODO"); }
			if (CollapsingHeader("LOL", false)) { Text("#TODO"); }
		}
		EndChild();   // ##F4
	}
	EndGroup();       // grupo principal

}

void UiMgr::loadImages() {
	std::cout << "[UiMgr]	Loading images..." << std::endl;

	// Añadir aquí las imágenes que se desean cargar
	addTextureFromFile("imageres/cat.png");
	addTextureFromFile("imageres/play.png");
	addTextureFromFile("imageres/stop.png");
	addTextureFromFile("imageres/pause.png");
	addTextureFromFile("imageres/repeat.png");

}

void UiMgr::unloadImages() {
	std::cout << "[UiMgr]	Unloading images..." << std::endl;

	GLuint glTex;

	// Descargar todas las imágenes guardadas
	for (auto & img : images_) {
        if (img.second.tex != 0) {
            std::cout << "[UiMgr] Unloading cached texture: " << img.first << std::endl;
            
            // Convertimos el uintptr_t de vuelta a GLuint para OpenGL
            glTex = (GLuint)img.second.tex;
            glDeleteTextures(1, &glTex);
            
            img.second.tex = 0; // Limpiar para evitar usos accidentales
        }
    }
}

void UiMgr::addTextureFromFile(std::string filename) {
	imageData img_data;		// Variable temporal para almacenar datos de la imagen cargada
	int channels;			// Canales de imagen

	// Carga de archivo de imagen
	unsigned char* data = stbi_load(filename.c_str(), &img_data.x, &img_data.y, &channels, 4);
	if (!data)
	{
		std::cerr << "[UiMgr]	ERROR loading image: " << filename << std::endl;
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
	
	std::cout << "[UiMgr]	Loaded image " << filename << std::endl;
    return;
};

void UiMgr::updateDensity(int delta) {

    int new_size = (int)style_->FontSizeBase + delta;

    if (new_size > (int)MAX_FONT_SIZE_) {
        std::cerr << "[UiMgr] WARN font size bigger than max allowed." << std::endl;
        return;
    }
    if (new_size < (int)MIN_FONT_SIZE_) {
        std::cerr << "[UiMgr] WARN font size smaller than min allowed." << std::endl;
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

    std::cout << "[UiMgr] Density adjusted to font size: " << style_->FontSizeBase << std::endl;
};

// Temas --------------------------------------------------------------------------------

void UiMgr::titleBarDarkMode(bool useDarkMode) {
	#ifdef _WIN32
		BOOL useDarkMode_ = useDarkMode ? TRUE : FALSE;
		DwmSetWindowAttribute(glfwGetWin32Window(window_), 20, &useDarkMode_, sizeof(useDarkMode_));
		std::cout << "[UiMgr] " << (useDarkMode ? "Dark" : "Light") << " window title set" << std::endl;
	#endif
}

void UiMgr::Style_Confy() {
	
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

}

void UiMgr::Style_FutureDark() {
	
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

}

void UiMgr::Style_Moonlight() {
	
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

}

void UiMgr::Style_VisualStudio() {
	
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

}

void UiMgr::Style_Microfrost() {
	
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
}
