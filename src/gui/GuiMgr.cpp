
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

#include "gui/GuiMgr.hpp"			// Clase de gestión de UI
#include "system/SystemMgr.hpp"
#include "app/IAppControl.hpp"      // Interfaz de comunicación entre miembros de la aplicación

#include "tts/TTSData.hpp"

#if defined IMGUILIB || defined IMGUILIB_VERSION

	#include <imgui.h>				// ImGui external lib
	#include <imgui_internal.h>		// ImGui external lib
	#include <implot.h>				// ImPlot external lib
	#include "imgui-knobs.h"		// Soporte de knobs
	#include "imspinner.h"			// Soporte de spinners de carga
	#include "imspinner_dots.h"		// Spinners de puntos (SpinnerSwingDots, etc.)
	#include "imspinner_bars.h"		// Spinners de barras (SpinnerBarChartRainbow, etc.)
	#include <GLFW/glfw3.h>						// Gestor de ventanas GLFW
	#include <backends/imgui_impl_glfw.h>		// Gestor de ventanas GLFW
	#include <backends/imgui_impl_opengl3.h>	// Gestor GLFW/OpenGL
	#ifdef _WIN32
		#include <GLFW/glfw3native.h>
	#endif
	#if defined STB || defined STB_VERSION
		#include <stb_image.h>          // Implementación para soporte de imágenes.
	#endif

	#include "files/JsonMgr.hpp"
	

	// Sistema
	#ifdef _WIN32
		#include <windows.h>
		#include <dwmapi.h>
	#else
		#include <filesystem>
		namespace fs = std::filesystem;
	#endif
	#include "resources.h"  // icono
	#include "fonts.h"		// Fuentes generadas en resources/

	#include <cstring>


	// Se puede evitar poner "ImGui::" para simplificar
	using namespace ImGui;

	// Implementación de miembros de declaraciones forward
    struct GuiMgr::Impl {
        TTSCoreData         datosTTS;
    };


	// General ------------------------------------------------------------------------------

	GuiMgr::GuiMgr(IAppControl* controller) : 
        pimpl_(std::make_unique<Impl>()),
		config_(nullptr),
		style_(nullptr),
		io_(nullptr),
		captureKeys_(false),
		AppName_("Demo"),
		windowSizeX_(1280),
		windowSizeY_(720),
		windowPosX_(0),
		windowPosY_(0),
		fullscreen_(false),
		theme_selected_("DefaultLight"),
		fontSize_(16),
		deviceRefreshInterval_(5),
		window_(nullptr),
		ctrl_(controller),
		running_(false)
	{
		
		// Avisa si se ha inicializado sin interfaz de comunicación para saber de otros módulos
		if (!ctrl_)
			SYS_WARN("GuiMgr","Cannot handle any controller.");

		// Avisa si no tiene soporte STB para imágenes
		#ifndef STB
			SYS_WARN("GuiMgr","STB Image library has not been implemented.");
		#endif

	}

	GuiMgr::~GuiMgr() {
		close();
	}

	void GuiMgr::setController(IAppControl* controller){
		ctrl_ = controller;
	}

	// Ejecución ----------------------------------------------------------------------------

	bool GuiMgr::init(void* config) {

		SYS_INFO("GuiMgr", "Initializating UI...");

		// Validar y asignar valores de variables miembro a partir de la config pasada (json)
        if (config) {
            loadConfig(config);
			config_ = config;
		}
        else  // Puede llegar aquí cuando se hace reload()
            SYS_WARN("GuiMgr","Cannot load config. Using default values.");

		// Identificación de posibles errores de inicialización
		glfwSetErrorCallback([](int error, const char* description) {
			SYS_ERROR("GuiMgr", "GLFW Error (" + std::to_string(error) + "): " + std::string(description));
		});

		// Check si está vinculado con la App
		if (ctrl_!=nullptr)
			SYS_INFO("GuiMgr", "Linked with IAppController");
		else
			SYS_ERROR("GuiMgr","No controller linked.");

		// Inicializar GLFW
		if (!glfwInit()) {
			SYS_ERROR("GuiMgr","glfwInit error.");
			return false;
		}

		// Configuración de la ventana GLFW
		glfwWindowHint(GLFW_TRANSPARENT_FRAMEBUFFER, GLFW_TRUE); // Fondo transparente
		glfwWindowHint(GLFW_DECORATED, GLFW_TRUE);               // Bordes y barra de título
		glfwWindowHint(GLFW_RESIZABLE, GLFW_TRUE);               // Redimensionable

		// Creación de ventana
		window_ = glfwCreateWindow(windowSizeX_, windowSizeY_, AppName_.c_str(), NULL, NULL);
		if(!window_) {
			glfwTerminate();
			SYS_ERROR("GuiMgr","glfwCreateWindow error.");
			return false;
		}
		glfwMakeContextCurrent(window_);
		glfwSwapInterval(1);

		// Inicializa ImGui
		IMGUI_CHECKVERSION();
		CreateContext();

		// Inicializa ImPlot
		ImPlot::CreateContext();
		
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

		// Configuración de estilo por defecto, según config
		ApplyTheme();

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
				SYS_INFO("GuiMgr", "Linux window icon loaded from file.");
			} else {
				SYS_WARN("GuiMgr","Could not load icon.png for Linux window.");
			}
		#endif

		ImGui_ImplGlfw_InitForOpenGL(window_, true);
		ImGui_ImplOpenGL3_Init("#version 130");     // Versión de OpenGL

		// marcar que está inicializado
		initialized_ = true;
		return true;
	}

	bool GuiMgr::isInitialized() const{
        return initialized_;
    }

	void GuiMgr::loadConfig(void* config) {
        if (!config)
			return;

        SYS_INFO("GuiMgr","Reading config node...");

        // Se considera que la configuración se pasa como json
        json* cfg = static_cast<json*>(config);
        JsonMgr& jsonMgr = JsonMgr::instance();
        
        jsonMgr.get_or_set(cfg, "AppName", 		AppName_);
        jsonMgr.get_or_set(cfg, "windowSizeX", 	windowSizeX_);
        jsonMgr.get_or_set(cfg, "windowSizeY", 	windowSizeY_);
        jsonMgr.get_or_set(cfg, "windowPosX", 	windowPosX_);
        jsonMgr.get_or_set(cfg, "windowPosY", 	windowPosY_);
        jsonMgr.get_or_set(cfg, "fullscreen", 	fullscreen_);
        jsonMgr.get_or_set(cfg, "fontSize", 	fontSize_);
        jsonMgr.get_or_set(cfg, "deviceRefreshInterval", deviceRefreshInterval_);
        jsonMgr.get_or_set(cfg, "theme_selected", theme_selected_);

	    SYS_INFO("GuiMgr","Config node read OK");
    }

	void GuiMgr::run() {

		running_ = true;

		while (isRunning())
			BuclePrincipal();		// <-- Se queda aqui hasta cerrar
		
		close();
	}

	bool GuiMgr::close() {
		// No intentar cerrar de nuevo (excepción)
		if(!running_) 
			return true;

		SYS_INFO("GuiMgr", "Closing UI...");

		ImGui_ImplOpenGL3_Shutdown();
		ImGui_ImplGlfw_Shutdown();
		ImPlot::DestroyContext();
		DestroyContext();
		if (window_)
		{
			glfwDestroyWindow(window_);
			window_ = nullptr;
		}
		glfwTerminate();

		// Liberar recursos de imágenes cargadas
		unloadImages();

		SYS_INFO("GuiMgr", "UI closed.");
		running_ = false;

		return true;
	}

	bool GuiMgr::isRunning() const {
		return window_ && !glfwWindowShouldClose(window_);
	}
	
	// Captura de teclas --------------------------------------------------------------------

	void GuiMgr::captureKeys() {

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

		// Alternar modo pantalla completa F11 / Alt+Enter
		if ( IsKeyPressed((ImGuiKey)582) || (io_->KeyAlt && IsKeyPressed((ImGuiKey)525))  )
			setFullscreenMode(!fullscreen_);

	}


	// Bucle principal -----------------------------------------------------------------------

	void GuiMgr::initCuadro() {
		glfwPollEvents();
		
		ImGui_ImplOpenGL3_NewFrame();
		ImGui_ImplGlfw_NewFrame();
		NewFrame();
	}

	void GuiMgr::endCuadro() {
		// Renderiza
		Render();
		glClear(GL_COLOR_BUFFER_BIT);
		ImGui_ImplOpenGL3_RenderDrawData(GetDrawData());
		glfwSwapBuffers(window_);
	}

	void GuiMgr::BuclePrincipal() {
		initCuadro();
		captureKeys();

		//Meto un timer para que refresque todo el rato los devices
		static float device_refresh_timer = 0.0f;
    	device_refresh_timer += io_->DeltaTime;
		if (device_refresh_timer >= deviceRefreshInterval_)
		{
			ctrl_->refreshAudioDevices();
			device_refresh_timer = 0.0f;
		}

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


	// Elementos de interfaz ----------------------------------------------------------------

	void GuiMgr::crearMainMenuBar() {
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

	void GuiMgr::ventanaPrincipal() {
		// Variables estáticas (solo se crean una vez) para guardar datos
		static float heightRightTop = 0.5f; 
		static float totalHeight = GetContentRegionAvail().y;
		static float sizeX__Izq = GetContentRegionAvail().x * 0.3f;
		static std::string TTS_text;
		
		// Coge datos de módulos
		pimpl_->datosTTS = ctrl_->getTTSData();
	
		
		// COLUMNA IZQUIERDA
		BeginGroup();
		{
			// Panel F1 (Arriba Izquierda)
			BeginChild("##F1", ImVec2(sizeX__Izq, GetContentRegionAvail().y), ImGuiChildFlags_AutoResizeX | ImGuiChildFlags_AutoResizeY | ImGuiChildFlags_FrameStyle);

			// Mostrar carga de TTS
			if (pimpl_->datosTTS.init_percent < 100) {
				ImSpinner::SpinnerPulsar("Pulsar",  6, 2, ImColor(.5f,.5f,.5f));
				SameLine();
				TTS_text = "Loading TTS voice models: ";
				TTS_text += std::to_string(pimpl_->datosTTS.num_loaded_models) + "/" + std::to_string(pimpl_->datosTTS.num_available_models);
			} else TTS_text = "TTS voice models loaded.";
			
			Text("%s", TTS_text.c_str());
			ProgressBar(pimpl_->datosTTS.init_percent/100.0f, ImVec2(0.0f, 0.0f));
	
			// Botón de modo
			if (Button(       (ctrl_->isOnlineMode()) ? "ONLINE" : "OFFLINE"        ) ){
				ctrl_->setOnlineMode(!ctrl_->isOnlineMode());
			}

			// Test de temas (Combobox from imgui demo)

			Text("Tema:"); SameLine();
			static const char* theme_items[] = {
				"Adobe Inspired",
				"Ayu Dark",
				"Confy",
				"Dark Cyan",
				"Default Dark",
				"Default Light",
				"Everforest",
				"FutureDark",
				"Gold",
				"Hazy Dark",
				"Kazam's Cherry",
				"Light Orange",
				"Quick Minimal Look",
				"Modern",
				"Microfrost",
				"Moonlight",
				"Sonic Riders",
				"VisualStudio"
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
							case 0: Style_AdobeInspired(); 		break;
							case 1: Style_AyuDark(); 			break;
							case 2: Style_Confy(); 				break;
							case 3: Style_DarkCyan(); 			break;
							case 4: Style_DefaultDark(); 		break;
							case 5: Style_DefaultLight();		break;
							case 6: Style_Everforest(); 		break;
							case 7: Style_FutureDark(); 		break;
							case 8: Style_Gold(); 				break;
							case 9: Style_HazyDark(); 			break;
							case 10: Style_KazamsCherry(); 		break;
							case 11: Style_LightOrange(); 		break;
							case 12: Style_QuickMinimalLook(); 	break;
							case 13: Style_Modern(); 			break;
							case 14: Style_Microfrost(); 		break;
							case 15: Style_Moonlight(); 		break;
							case 16: Style_SonicRiders(); 		break;
							case 17: Style_VisualStudio(); 		break;
						
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
	
		columnaDerecha();
	
	}
	
	void GuiMgr::columnaDerecha() {
		//  COLUMNA DERECHA (Layout principal)
		BeginGroup(); 
		{
			static ImGuiTabBarFlags tab_bar_flags = ImGuiTabBarFlags_None;
			if (BeginTabBar("MyTabBar", tab_bar_flags)) {

				if (BeginTabItem("Playground")) {

					Text("Espacio preparado para duplicar o probar funciones");

					EndTabItem();
				}

				if (BeginTabItem("Demo1")) {
					// Test imagen
					if (BeginTable("demo1tab", 3))
					{
						// No sale nada antes de estos dos
						TableNextRow();
						TableNextColumn();
						
						Text("Test Imagen:");
						TableNextColumn();
						Text("Test imagen fallida");
						TableNextColumn();
						Text("Test imagen precargada");
						TableNextRow();
						TableNextColumn();
						Image(getImage("imageres/cat.png"), ImVec2(200,100));
						TableNextColumn();
						Image(getImage("imageres/nonexist?"), ImVec2(200,100));
						TableNextColumn();
						Image(getImage("imageres/cat.png"), ImVec2(100,200));

						EndTable();
					}
					
					// Test spinners
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
					std::string current_model = (!pimpl_->datosTTS.loaded_models.empty()) ? pimpl_->datosTTS.loaded_models[selected_idx] : "";
	
					static std::string proc_text;

					/* De momento esto no se usa, comento porque sino logea todo el rato cannot process text*/
					// proc_text = ctrl_->getTTSProcessingText(current_model);
					// bool is_busy = !proc_text.empty();
					

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
						if (ctrl_->isOnlineMode()) {
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
						const char* preview_value = (pimpl_->datosTTS.loaded_models.empty()) ? "" : pimpl_->datosTTS.loaded_models[selected_idx].c_str();
						if (BeginCombo("##cb_model", preview_value)) {
							for (int n = 0; n < static_cast<int>(pimpl_->datosTTS.loaded_models.size()); ++n) {
								if (Selectable(pimpl_->datosTTS.loaded_models[n].c_str(), selected_idx == n)) selected_idx = n;
							}
							EndCombo();
						}

						Spacing(); Spacing();

						if (Button("Generate Wav", ImVec2(-FLT_MIN, 40))) { 
							// Usamos el buffer manual que el usuario ha escrito
							ctrl_->TTSgenerate(current_model, manual_buffer, current_model);
						}
						if (Button("Play (default playback)", ImVec2(-FLT_MIN, 40))) { 
							ctrl_->TTSPlay(current_model, manual_buffer, current_model);
						}
						if (is_online) EndDisabled(); // Cerramos el bloque de deshabilitado

					EndChild();



					EndTabItem();
				}
				
				if (BeginTabItem("audio")) {

					// Pide al controlador la lista de micrófonos disponibles en este momento
					std::vector<std::string> entradas = ctrl_->getAvailableInputDevices();

					// Lista con los nombres de los dispositivos que el usuario ha activado
					static std::vector<std::string> dispositivos_activos;
					dispositivos_activos = ctrl_->getManagedCaptures();

					// Flag para abrir/cerrar la ventana flotante del selector
					static bool show_device_selector = false;

					// --- Muestra los dispositivos activos ---
					Text("Dispositivos activos:");
					if (dispositivos_activos.empty()) {
						TextDisabled("  (ninguno)");   // Si no hay ninguno, muestra texto gris
					} else {

						short i = 0;
						for (std::string const& captureName : dispositivos_activos) {
							PushID(i);

							// Si el dispositivo se ha desconectado, mostrar aviso en rojo
							if (!ctrl_->isInputDeviceValid(captureName)) {
								TextColored(ImVec4(1, 0, 0, 1), " %s - DESCONECTADO", captureName.c_str());
								SameLine();
								if (SmallButton("x")) 
									ctrl_->removeInputDevice(captureName);
								
								
							} else {
								
								Text("%s", captureName.c_str());
								SameLine();
								if (SmallButton("Grabar"))
									ctrl_->StartRecording(captureName);
								SameLine();
								if (SmallButton("Parar"))
									ctrl_->StopRecording(captureName);
								SameLine();
								if (SmallButton("x"))
									ctrl_->removeInputDevice(captureName);
								
	
								SameLine();
	
								std::string cadena = std::to_string(ctrl_->getInputBufferSize(captureName));
								Text(cadena.c_str());

								std::string cadena_rec = std::to_string(ctrl_->getInputRecBufferSize(captureName));
								Text(cadena_rec.c_str());

								SameLine();

								// Medidor VU - barra vertical RMS
								float LevelVal = ctrl_->getInputPeakLevel(captureName);

								ImVec4 barColor;
								if      (LevelVal < 60.f) 	barColor = ImVec4(0.18f, 0.80f, 0.18f, 1.0f); // verde
								else if (LevelVal < 85.f) 	barColor = ImVec4(1.00f, 0.75f, 0.00f, 1.0f); // amarillo
								else						barColor = ImVec4(0.90f, 0.15f, 0.15f, 1.0f); // rojo

								if (ImPlot::BeginPlot("##vu", ImVec2(30, 70),
									ImPlotFlags_NoLegend | ImPlotFlags_NoMouseText |
									ImPlotFlags_NoMenus  | ImPlotFlags_NoTitle     | ImPlotFlags_NoFrame))
								{
									ImPlot::SetupAxes(nullptr, nullptr,
										ImPlotAxisFlags_NoDecorations,
										ImPlotAxisFlags_NoDecorations);
									ImPlot::SetupAxisLimits(ImAxis_X1, 0.5, 1.5, ImGuiCond_Always);
									ImPlot::SetupAxisLimits(ImAxis_Y1, 0.0, 100.0, ImGuiCond_Always);
									ImPlot::SetNextFillStyle(barColor);
									ImPlot::PlotBars("##bar", &LevelVal, 1, 0.9, 1.0);
									ImPlot::EndPlot();
								}
							}


							i++;
							PopID();
						}
					}

					// Botón para abrir el selector de dispositivos disponibles
					if (Button("Selecciona dispositivo disponible"))
						show_device_selector = true;

					SameLine();
					// Botón para refrescar la lista de dispositivos del sistema
					if (Button("Actualizar"))
						ctrl_->refreshAudioDevices();

					// --- Ventana flotante: selector de dispositivos disponibles ---
					if (show_device_selector) {
						SetNextWindowSize(ImVec2(400, 300), ImGuiCond_FirstUseEver);    // Tamaño inicial de la ventana
						if (Begin("Dispositivos de entrada", &show_device_selector)) {  // &show_device_selector: la X de la ventana la cierra
							if (entradas.empty()) {
								TextDisabled("No hay dispositivos disponibles.");
							} else {
								for (int n = 0; n < static_cast<int>(entradas.size()); ++n) {
									if (Selectable(entradas[n].c_str())) {
										ctrl_->addInputDevice("DEFAULT", entradas[n]);         // Inicializa el dispositivo en SoundMgr
										show_device_selector = false;               // Cierra el popup
									}
								}
							}
						}
						End();
					}

					EndTabItem();
				}
	
				EndTabBar();
			}

		}
		EndGroup();       // grupo principal

	}


	// Carga de imágenes --------------------------------------------------------------------

	uintptr_t GuiMgr::getImage(std::string path) {
		// Si la imagen no está precacheada, se añade
		if (images_.find(path) == images_.end())
			addTextureFromFile(path);

		return images_[path].tex;
	}

	void GuiMgr::unloadImages() {
		SYS_INFO("GuiMgr", "Unloading images...");

		GLuint glTex;

		// Descargar todas las imágenes guardadas
		for (auto & img : images_) {
			if (img.second.tex != 0) {
				SYS_INFO("GuiMgr", "Unloading cached texture: " + img.first);
				
				// Convertimos el uintptr_t de vuelta a GLuint para OpenGL
				glTex = (GLuint)img.second.tex;
				glDeleteTextures(1, &glTex);
				
				img.second.tex = 0; // Limpiar para evitar usos accidentales
			}
		}
		images_.clear();	// Por si se vuelve a usar
	}

	void GuiMgr::addTextureFromFile(std::string filename) {

		SYS_INFO("GuiMgr","Saving '" + filename + "' image texture to cache...");

		imageData img_data;		// Variable temporal para almacenar datos de la imagen cargada
		img_data.tex 	  = 0;	// Textura por defecto

		// Generar textura rosa/blanca por defecto si no se ha generado aún
		if (defaultTexture_ == 0) generateDefaultTexture();

		#if defined STB || defined STB_VERSION

		// Carga de archivo de imagen
		unsigned char* data = stbi_load(filename.c_str(), &img_data.x, &img_data.y, &img_data.channels, 4);
		
		// Generar tex: Desde imagen o textura por defecto
		if (data)
		{
			// Generar la textura desde imagen
			GLuint texture;
			glGenTextures(1, &texture);
			glBindTexture(GL_TEXTURE_2D, texture);
			glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA,	img_data.x, img_data.y ,0 , GL_RGBA, GL_UNSIGNED_BYTE, data);
			glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
			glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
			img_data.tex = (uintptr_t)texture;				// Guardar la textura generada
			stbi_image_free(data);	// Liberar memoria de datos de imagen temportales
			
			SYS_INFO("GuiMgr", "Loaded image " + filename);
		} else {
			img_data.tex = defaultTexture_;
			SYS_WARN("GuiMgr","Error loading image: " + filename);
		}

		#else		// STB no está implementado, meter textura por defecto
			img_data.tex = defaultTexture_;
			SYS_WARN("GuiMgr","Error loading image: " + filename);
		#endif

		// Guardar la imagen en el mapa de imágenes
		images_[filename] = img_data;
		
		return;
	};

	void GuiMgr::generateDefaultTexture() {
		SYS_INFO("GuiMgr", "Generating default texture image...");

		const int width = 64;
		const int height = 64;
		const int checkSize = 32; // Tamaño de cada cuadro del mosaico
		
		// 4 bytes por píxel (RGBA)
		std::vector<unsigned char> data(width * height * 4);

		for (int y = 0; y < height; ++y) {
			for (int x = 0; x < width; ++x) {
				int index = (y * width + x) * 4;
				
				// Lógica del mosaico: (x / tamaño) + (y / tamaño) es par o impar
				bool isPink = ((x / checkSize) + (y / checkSize)) % 2 == 0;

				if (isPink) {
					// Rosa (Magenta: R=255, G=0, B=255)
					data[index + 0] = 255; data[index + 1] = 0; data[index + 2] = 255; data[index + 3] = 255;
				} else {
					// Negro (R=0, G=0, B=0) o Blanco (R=255, G=255, B=255)
					data[index + 0] = 0;   data[index + 1] = 0; data[index + 2] = 0;   data[index + 3] = 255;
				}
			}
		}

		GLuint texture;
		glGenTextures(1, &texture);
		glBindTexture(GL_TEXTURE_2D, texture);
		
		// Usamos GL_NEAREST para que los bordes del mosaico se vean nítidos y "retro"
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
		
		glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, width, height, 0, GL_RGBA, GL_UNSIGNED_BYTE, data.data());

		defaultTexture_ = (uintptr_t)texture;
		return;
	}


	// Aspecto ------------------------------------------------------------------------------

	bool GuiMgr::setFullscreenMode(bool enabled) {
		GLFWmonitor* monitor = glfwGetPrimaryMonitor();
		if (monitor == nullptr) {
			SYS_WARN("GuiMgr", "Primary monitor not detected. Cannot set fullscreen mode");
			return false;
		}

		const GLFWvidmode* mode = glfwGetVideoMode(monitor);
		if (mode == nullptr) {
			SYS_WARN("GuiMgr", "Monitor mode not detected. Cannot set fullscreen mode");
			return false;
		}

		if (enabled) {
			// Guardamos las dimensiones actuales antes de cambiar
			glfwGetWindowPos(window_, (int*)&windowPosX_, (int*)&windowPosY_);
			glfwGetWindowSize(window_, (int*)&windowSizeX_, (int*)&windowSizeY_);

			// Cambiar a pantalla completa (Full Screen)
			glfwSetWindowMonitor(window_, monitor, 0, 0, mode->width, mode->height, mode->refreshRate);
		} else {
			// Volver a modo ventana usando las dimensiones guardadas
			glfwSetWindowMonitor(window_, NULL, windowPosX_, windowPosY_, windowSizeX_, windowSizeY_, 0);
		}

		fullscreen_ = enabled;
		return true;
	}

	void GuiMgr::updateDensity(int delta) {

		int new_size = (int)style_->FontSizeBase + delta;

		if (new_size > (int)MAX_FONT_SIZE_) {
			SYS_WARN("GuiMgr","Font size bigger than max allowed.");
			return;
		}
		if (new_size < (int)MIN_FONT_SIZE_) {
			SYS_WARN("GuiMgr","Font size smaller than min allowed.");
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

		SYS_INFO("GuiMgr", "Density adjusted to font size: " + std::to_string(style_->FontSizeBase));
	};

	void GuiMgr::titleBarDarkMode(bool useDarkMode) {
		#ifdef _WIN32
			std::string st_darkmode=(useDarkMode ? "Dark" : "Light");
			BOOL useDarkMode_ = useDarkMode ? TRUE : FALSE;
			DwmSetWindowAttribute(glfwGetWin32Window(window_), 20, &useDarkMode_, sizeof(useDarkMode_));
			SYS_INFO("GuiMgr",  st_darkmode + " window title set");
		#endif
	}

#else
// ============================================================
//  (Stubs)
// ============================================================

    GuiMgr::GuiMgr(IAppControl*)	{
		SYS_WARN("GuiMgr", "ImGUI Graphics library has not been implemented.");
	}

    GuiMgr::~GuiMgr()									{ }
    void GuiMgr::setController(IAppControl*)	{ return; }
    
	// Ejecución ----------------------------------------------------------------------------
    bool GuiMgr::init()				{ return false;}
    void GuiMgr::run()				{ return; }
    bool GuiMgr::isRunning() const 	{ return false; }
    void GuiMgr::close()			{ return; }

	// Bucle principal ----------------------------------------------------------------------
    void GuiMgr::initCuadro()		{ return; }
    void GuiMgr::endCuadro()		{ return; }
    void GuiMgr::captureKeys()		{ return; }
    void GuiMgr::BuclePrincipal()	{ return; }

	// Elementos de interfaz ----------------------------------------------------------------
    void GuiMgr::crearMainMenuBar()		{ return; }
    void GuiMgr::ventanaPrincipal()		{ return; }

	// Carga de imágenes --------------------------------------------------------------------
    uintptr_t GuiMgr::getImage(std::string)					{ return 0; }
    void GuiMgr::unloadImages()								{ return; }
    void GuiMgr::addTextureFromFile(std::string)			{ return; }
    void GuiMgr::generateDefaultTexture()					{ return; }
    
	// Aspecto y temas ----------------------------------------------------------------------
    void updateDensity(int)						{ return; }
    void titleBarDarkMode(bool)					{ return; }

#endif
