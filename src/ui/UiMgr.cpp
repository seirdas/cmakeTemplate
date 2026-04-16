
/// ------------------------------- INFORMACIÓN -------------------------------------
//	# Configuraciones de ventana
//	- Buscar en imgui.h -> ImGuiWindowFlags_
//  - Añadir en una variable: ImGuiWindowFlags window_flags = *********************;
//	- Añadir a la ventana (en Begin("", nullptr, window_flags); )
//
//	DEMO DE ELEMENTOS Y CÓDIGO:
// 		https://pthom.github.io/imgui_explorer/
//		https://traineq.org/imgui_bundle_explorer/
// ---------------------------------------------------------------------------------

#include "ui/UiMgr.h"			// Clase de gestión de UI
#include <cstring>
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
#include <system/sys.hpp>

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

	SYS_INFO("UiMgr", "Initializating UI...");

	if (ctrl_!=nullptr)
		SYS_INFO("UiMgr", "Linked with IAppController");
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
			SYS_INFO("UiMgr", "Linux window icon loaded from file.");
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

	SYS_INFO("UiMgr", "Closing UI...");

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

	SYS_INFO("UiMgr", "UI closed.");
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
	// Variables estáticas (solo se crean una vez) para guardar datos
	static float heightRightTop = 0.5f; 
	static float totalHeight = GetContentRegionAvail().y;
	static float sizeX__Izq = GetContentRegionAvail().x * 0.3f;
	static std::string TTS_text;
	
	// Coge datos de módulos
	static TTSData tts;
	tts = ctrl_->getTTSData();
	
	// COLUMNA IZQUIERDA
	BeginGroup();
	{
		// Panel F1 (Arriba Izquierda)
		BeginChild("##F1", ImVec2(sizeX__Izq, GetContentRegionAvail().y), ImGuiChildFlags_AutoResizeX | ImGuiChildFlags_AutoResizeY | ImGuiChildFlags_FrameStyle);

		// Mostrar carga de TTS
		if (tts.init_percent < 100) {
			ImSpinner::SpinnerPulsar("Pulsar",  6, 2, ImColor(.5f,.5f,.5f));
			SameLine();
			TTS_text = "Loading TTS voice models: ";
			TTS_text += std::to_string(tts.num_loaded_models) + "/" + std::to_string(tts.num_available_models);
		} else TTS_text = "TTS voice models loaded.";
		
		Text("%s", TTS_text.c_str());
		ProgressBar(tts.init_percent/100.0f, ImVec2(0.0f, 0.0f));

		// Botón de modo
		if (Button(       (ctrl_->isOnlineMode()) ? "ONLINE" : "OFFLINE"        ) ){
			ctrl_->setOnlineMode(!ctrl_->isOnlineMode());
		}

		// Test de temas (Combobox from imgui demo)

		Text("Tema:"); SameLine();
		static const char* theme_items[] = { 
			"Confy", 
			"FutureDark", 
			"Moonlight",
			"VisualStudio",
			"Microfost"
		};
        static int item_selected_idx;
        if (BeginCombo("##cbth", theme_items[item_selected_idx]))
        {
            // Recorremos todas las opciones
			for (int n = 0; n < static_cast<int>(std::size(theme_items)); ++n) {
				const bool is_selected = (item_selected_idx == n);

				// **SELECTABLE**: se ejecuta *una sola vez* cuando el usuario
				// hace click (o pulsa Enter) sobre la opción.
				if (Selectable(theme_items[n], is_selected)) {
					// ----> CAMBIO DE SELECCIÓN <----
					item_selected_idx = n;                     // actualizar índice

					// ----> ACCIÓN A EJECUTAR ----
					// Por ejemplo:
					switch(n){
						case 0: Style_Confy(); 			break;
						case 1: Style_FutureDark(); 	break;
						case 2: Style_Moonlight(); 		break;
						case 3: Style_VisualStudio(); 	break;
						case 4: Style_Microfrost(); 	break;
					}
				}

				// Mantener el foco visual en el elemento activo
				if (is_selected)
					SetItemDefaultFocus();
			}
            EndCombo();
        }

		EndChild();
	}
	EndGroup();

	SameLine(); // Pegamos la siguiente columna

	
	//  Layout principal (columna derecha)
	BeginGroup(); 
	{
		static ImGuiTabBarFlags tab_bar_flags = ImGuiTabBarFlags_None;
		if (BeginTabBar("MyTabBar", tab_bar_flags)) {

			if (BeginTabItem("Demo1")) {
				// Test imagen
				Image(images_["imageres/cat.png"].tex, ImVec2(200, 100));
	
				// Test spinners
				SameLine();
				ImSpinner::SpinnerAng8(           "Ang",     16, 2);	SameLine(0.0, -1.0);
				ImSpinner::SpinnerPulsar(         "Pulsar",  16, 2);	SameLine(0.0, -1.0);
				ImSpinner::SpinnerClock(          "Clock",   16, 2);	SameLine(0.0, -1.0);
				ImSpinner::SpinnerAtom(           "atom",    16, 2);	SameLine(0.0, -1.0);
				ImSpinner::SpinnerSwingDots(      "wheel",   16, 6);	SameLine(0.0, -1.0);
				ImSpinner::SpinnerFadeDots(		  "dots",	 16, 2, ImColor(.5f,.5f,.5f));		
				SameLine(0.0, -1.0);
				ImSpinner::SpinnerRainbowMix(     "Rmix",    16, 2, ImColor(1.0f,1.0f,1.0f),4);	
				SameLine(0.0, -1.0);
				ImSpinner::SpinnerDotsToBar(      "tobar",   16, 2, ImColor(1.0f,1.0f,1.0f),4);	
				SameLine(0.0, -1.0);
				ImSpinner::SpinnerBarChartRainbow("rainbow", 16, 4, ImColor(1.0f,1.0f,1.0f),4);

				EndTabItem();
			}

			if (BeginTabItem("knobs")) { 
				// Valores de ejemplo
				static float val1 = 0;
				static float val2 = 0;
				static float val3 = 0;
				static float val4 = 0;
				static int 	 val5 = 1;
				static float val6 = 1;
				static float val7 = 500.0f;

				// Test knobs
				ImGuiKnobs::Knob("Gain", &val1, -6.0f, 6.0f, 0.1f, "%.1fdB", ImGuiKnobVariant_Tick, 0,ImGuiKnobFlags_ValueTooltip | ImGuiKnobFlags_NoTitle);
				SameLine();
				ImGuiKnobs::Knob("Mix", &val2, -1.0f, 1.0f, 0.1f, "%.1f", ImGuiKnobVariant_Stepped);
				SameLine();
				ImGuiKnobs::Knob("Pitch", &val3, -6.0f, 6.0f, 0.1f, "%.1f", ImGuiKnobVariant_WiperOnly, 0, ImGuiKnobFlags_ValueTooltip | ImGuiKnobFlags_NoTitle | ImGuiKnobFlags_NoInput);
				SameLine();
				ImGuiKnobs::Knob("Dry", &val4, -6.0f, 6.0f, 0.1f, "%.1f", ImGuiKnobVariant_Stepped, 0, 0, 10, 1.570796f, 3.141592f);
				SameLine();
				ImGuiKnobs::KnobInt("Wet", &val5, 1, 10, 0.1f, "%i", ImGuiKnobVariant_Stepped, 0, 0, 10);
				SameLine();
				ImGuiKnobs::Knob("Vertical", &val6, 0.f, 10.f, 0.1f, "%.1f", ImGuiKnobVariant_Space, 0, ImGuiKnobFlags_DragVertical);
				SameLine();
				ImGuiKnobs::Knob("Logarithmic", &val7, 20, 20000, 20.0f, "%.1f", ImGuiKnobVariant_WiperOnly, 0, ImGuiKnobFlags_Logarithmic | ImGuiKnobFlags_AlwaysClamp);
				
				// Double click to reset
				if (ImGui::IsItemActive() && ImGui::IsMouseDoubleClicked(0)) {
					val1 = 0; val2 = 0; val3 = 0; val4 = 0;	val5 = 0; val6 = 0;	val7 = 0;
				}

				EndTabItem();
			}

			// COMUNICACIONES (TABLA CON KNOBS)
			if (BeginTabItem("Communications")) {
				// Definimos los nombres de las columnas. 
				const char* col_names[] = { "TX/RX", "Piloto", "Copiloto", "3Hombre", "IOS OnBoard", "IOS Offboard 1", "IOS OffBoard 2" };
				
				// Calculamos el número de columnas dinámicamente
				const int num_col = (int)(sizeof(col_names) / sizeof(col_names[0]));
				const int num_rows = num_col-1; // Puedes cambiar esto por una constante o variable de tu lógica

				// Matriz de valores para los Knobs: [Filas][Columnas]
				static float k_vals[num_rows][num_col];

				//style_->TableAngledHeadersAngle = 35.0f;

				if (BeginTable("KnobTable", num_col, ImGuiTableFlags_ScrollY | ImGuiTableFlags_Borders, ImVec2(0, 0))) {
					
					// Primera columna (Nombres de canales)
					TableSetupColumn(col_names[0], ImGuiTableColumnFlags_WidthFixed, 100.0f);
					
					// Resto de columnas (Angled Headers)
					for (int n = 1; n < num_col; n++) {
						TableSetupColumn(col_names[n], ImGuiTableColumnFlags_AngledHeader | ImGuiTableColumnFlags_WidthFixed, 55.0f);
					}

					TableAngledHeadersRow();
					TableHeadersRow();

					for (int row = 0; row < num_rows; row++) {
						PushID(row);
						TableNextRow(ImGuiTableRowFlags_None, 65.0f);
						
						// Columna 0: Etiqueta del canal
						TableSetColumnIndex(0);
						AlignTextToFramePadding();
						Text("%s", col_names[row+1]);

						// Columnas 1 a N: Knobs
						for (int col = 1; col < num_col; col++) {
							if (TableSetColumnIndex(col)) {
								PushID(col);
								
								float c_width = GetColumnWidth();
								// Centramos el knob (40.0f es su diámetro)
								SetCursorPosX(GetCursorPosX() + (c_width - 40.0f) * 0.5f);								
								ImGuiKnobs::Knob("##vol", &k_vals[row][col], 0.0f, 100.0f, 1.0f, "%.1f", ImGuiKnobVariant_WiperOnly, 40.0f, ImGuiKnobFlags_ValueTooltip | ImGuiKnobFlags_NoTitle);
								
								PopID();
							}
						}
						PopID();
					}
					EndTable();
				}
				EndTabItem();
			}

			if (BeginTabItem("TTS")) {
				// --- CONFIGURACIÓN PREVIA ---
				static int selected_idx = 0;
				std::string current_model = (!tts.loaded_models.empty()) ? tts.loaded_models[selected_idx] : "";
				std::string proc_text = ctrl_->getTTSProcessingText(current_model);
				bool is_busy = !proc_text.empty();

				static char manual_buffer[2048] = ""; 

				// --- LAYOUT ---

				Text("TTS Content:");

				// Definimos un alto para que ambos lados midan lo mismo
				float content_height = GetTextLineHeight() * 12;

				// COLUMNA IZQUIERDA: InputText
				// Usamos un Child o simplemente calculamos el ancho para dejar espacio a la derecha
				static float right_panel_width = 250.0f; // Ancho fijo para el panel de controles
				static float input_width = GetContentRegionAvail().x - right_panel_width - GetStyle().ItemSpacing.x;

				BeginGroup(); // Agrupamos el input para que Sameline funcione con el bloque siguiente

					ImGuiInputTextFlags flags = ImGuiInputTextFlags_AllowTabInput | ImGuiInputTextFlags_WordWrap;
					if (ctrl_->isOnlineMode() || is_busy) {
						flags |= ImGuiInputTextFlags_ReadOnly;
						PushStyleColor(ImGuiCol_Text, GetStyle().Colors[ImGuiCol_TextDisabled]);
						InputTextMultiline("##ttstext_v", const_cast<char*>(proc_text.c_str()), proc_text.size(), 
							ImVec2(input_width, content_height), ImGuiInputTextFlags_ReadOnly | ImGuiInputTextFlags_WordWrap);
						PopStyleColor();
					} else {
						InputTextMultiline("##ttstext_e", manual_buffer, IM_ARRAYSIZE(manual_buffer), 
							ImVec2(input_width, content_height), flags);
					}
				EndGroup();

				SameLine();

				// COLUMNA DERECHA: Combo y Botón
				BeginChild("ControlsPanel", ImVec2(right_panel_width, content_height), false);
					
					Text("Voice Model:");

					// Desactiva visualmente y bloquea interacción
					bool is_online = ctrl_->isOnlineMode();
					if (is_online) BeginDisabled(); // Si es online, todo lo que sigue se deshabilita

					SetNextItemWidth(-FLT_MIN); // Que ocupe todo el ancho del child
					const char* preview_value = (tts.loaded_models.empty()) ? "" : tts.loaded_models[selected_idx].c_str();
					if (BeginCombo("##cb_model", preview_value)) {
						for (int n = 0; n < static_cast<int>(tts.loaded_models.size()); ++n) {
							if (Selectable(tts.loaded_models[n].c_str(), selected_idx == n)) selected_idx = n;
						}
						EndCombo();
					}

					Spacing(); Spacing();

					if (Button("Generate Audio", ImVec2(-FLT_MIN, 40))) { 
						// Usamos el buffer manual que el usuario ha escrito
						ctrl_->TTSgenerate(current_model, manual_buffer, current_model);
					}
					if (is_online) EndDisabled(); // Cerramos el bloque de deshabilitado

					if (is_busy) 
						ImSpinner::SpinnerDotsToBar(      "tobar",   16, 2, ImColor(1.0f,1.0f,1.0f),4);

				EndChild();



				EndTabItem();
			}
			EndTabBar();
		}

	}
	EndGroup();       // grupo principal

}

void UiMgr::loadImages() {
	SYS_INFO("UiMgr", "Loading images...");

	// Añadir aquí las imágenes que se desean cargar
	addTextureFromFile("imageres/cat.png");
	addTextureFromFile("imageres/play.png");
	addTextureFromFile("imageres/stop.png");
	addTextureFromFile("imageres/pause.png");
	addTextureFromFile("imageres/repeat.png");

}

void UiMgr::unloadImages() {
	SYS_INFO("UiMgr", "Unloading images...");

	GLuint glTex;

	// Descargar todas las imágenes guardadas
	for (auto & img : images_) {
        if (img.second.tex != 0) {
            SYS_INFO("UiMgr", "Unloading cached texture: " + img.first);
            
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
	
	SYS_INFO("UiMgr", "Loaded image " + filename);
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

    SYS_INFO("UiMgr", "Density adjusted to font size: " + std::to_string(style_->FontSizeBase));
};

// Temas --------------------------------------------------------------------------------

void UiMgr::titleBarDarkMode(bool useDarkMode) {
	#ifdef _WIN32
		std::string st_darkmode=(useDarkMode ? "Dark" : "Light");
		BOOL useDarkMode_ = useDarkMode ? TRUE : FALSE;
		DwmSetWindowAttribute(glfwGetWin32Window(window_), 20, &useDarkMode_, sizeof(useDarkMode_));
		SYS_INFO("UiMgr",  st_darkmode + " window title set");
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

	SYS_INFO("UiMgr", "'Confy' theme applied.");
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

	SYS_INFO("UiMgr", "'FutureDark' theme applied.");
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

	SYS_INFO("UiMgr", "'Moonlight' theme applied.");
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

	SYS_INFO("UiMgr", "'Visual Studio' theme applied.");
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

	SYS_INFO("UiMgr", "'Microfrost' theme applied.");
}
