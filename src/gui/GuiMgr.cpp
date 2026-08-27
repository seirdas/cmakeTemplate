
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
#include "sound/AudioCaptureModule.hpp"
#include "system/SystemMgr.hpp"
#include "app/IAppControl.hpp"      // Interfaz de comunicación entre miembros de la aplicación


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
	#include "sound/SoundMgr.hpp"
	#include "sound/TTSCore.hpp"
	#include "sound/AudioPlaybackModule.hpp"
	#include "sound/PlayerAudio.hpp"
	#include "sound/PlayerMorse.hpp"
	#include "sound/PlayerTTS.hpp"
	

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


	// General ------------------------------------------------------------------------------

	GuiMgr::GuiMgr() : 
		config_(nullptr),
		ctrl_(nullptr),
		running_(false),
		initialized_(false),
		window_(nullptr),
		style_(nullptr),
		io_(nullptr),
		captureKeys_(false),
		AppName_("app"),
		windowSizeX_(1280),
		windowSizeY_(720),
		windowPosX_(0),
		windowPosY_(0),
		fullscreen_(false),
		theme_selected_("DefaultLight"),
		transparent_bk_(false),
		fontSize_(16),
		deviceRefreshInterval_(5),
		soundsData_({})
	{
		// Avisa si no tiene soporte STB para imágenes
		#ifndef STB
			SYS_WARN("GuiMgr","STB Image library has not been implemented.");
		#endif
	}

	GuiMgr::~GuiMgr() {
		close();
	}

	bool GuiMgr::setController(IAppControl* controller){
		ctrl_ = controller;
		return static_cast<bool>(ctrl_);
	}


	// Ejecución ----------------------------------------------------------------------------

	bool GuiMgr::init(void* config) {

		SYS_INFO("GuiMgr", "Initializing UI...");

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
		glfwWindowHint(GLFW_TRANSPARENT_FRAMEBUFFER, (transparent_bk_) ? GLFW_TRUE : GLFW_FALSE); // Fondo transparente
		glfwWindowHint(GLFW_DECORATED, GLFW_TRUE);               // Bordes y barra de título
		glfwWindowHint(GLFW_RESIZABLE, GLFW_TRUE);               // Redimensionable

		// Creación de ventana
		window_ = glfwCreateWindow(
			static_cast<int>(windowSizeX_), 
			static_cast<int>(windowSizeY_), 
			AppName_.c_str(), 
			NULL, 
			NULL
		);
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
		apply_theme();

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

        // Se considera que la configuración se pasa como json
        json* cfg = static_cast<json*>(config);
        JsonMgr& jsonMgr = JsonMgr::instance();
        
        jsonMgr.get_or_set(cfg, "app_name_window",	AppName_);
        jsonMgr.get_or_set(cfg, "window_size_x",	windowSizeX_);
        jsonMgr.get_or_set(cfg, "window_size_y", 	windowSizeY_);
        jsonMgr.get_or_set(cfg, "window_pos_x", 	windowPosX_);
        jsonMgr.get_or_set(cfg, "window_pos_y", 	windowPosY_);
        jsonMgr.get_or_set(cfg, "fullscreen", 		fullscreen_);
        jsonMgr.get_or_set(cfg, "font_size", 		fontSize_);
        jsonMgr.get_or_set(cfg, "device_refresh_interval",	deviceRefreshInterval_);
        jsonMgr.get_or_set(cfg, "theme_selected", 		  	theme_selected_);
        jsonMgr.get_or_set(cfg, "transparent_background", 	transparent_bk_);

    }

	bool GuiMgr::Run() {

		running_ = true;

		while (isRunning())
			bucle_principal();		// <-- Se queda aqui hasta cerrar
		
		return close();
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
		unload_images();

		SYS_INFO("GuiMgr", "UI closed.");
		running_ = false;
		initialized_ = false;

		return !initialized_; // <- true
	}

	bool GuiMgr::isRunning() const {
		return window_ && !glfwWindowShouldClose(window_);
	}
	

	// Captura de teclas --------------------------------------------------------------------

	void GuiMgr::capture_keys() {

		// Modo debug de detección de teclas
		if (captureKeys_)
			for (int key = ImGuiKey_NamedKey_BEGIN; key < ImGuiKey_NamedKey_END; key++)
				if (IsKeyPressed((ImGuiKey)key))
					printf("Key pressed: %d\n", key);

		// Aumentar el tamaño de la fuente Ctrl+"+"
		if (io_->KeyCtrl && (IsKeyPressed((ImGuiKey)605) || IsKeyPressed((ImGuiKey)626)) )
			update_density(1);
		
		// Reducir el tamaño de la fuente Ctrl+"-"
		if (io_->KeyCtrl && (IsKeyPressed(ImGuiKey_Minus) || IsKeyPressed((ImGuiKey)625)) )
			update_density(-1);

		// Alternar modo pantalla completa F11 / Alt+Enter
		if ( IsKeyPressed((ImGuiKey)582) || (io_->KeyAlt && IsKeyPressed((ImGuiKey)525))  )
			set_fullscreen(!fullscreen_);

	}


	// Bucle principal -----------------------------------------------------------------------

	void GuiMgr::init_frame() {
		glfwPollEvents();
		
		ImGui_ImplOpenGL3_NewFrame();
		ImGui_ImplGlfw_NewFrame();
		NewFrame();
	}

	void GuiMgr::end_frame() {
		// Renderiza
		Render();
		glClear(GL_COLOR_BUFFER_BIT);
		ImGui_ImplOpenGL3_RenderDrawData(GetDrawData());
		glfwSwapBuffers(window_);
	}

	void GuiMgr::bucle_principal() {
		init_frame();
		capture_keys();

		//Meto un timer para que refresque todo el rato los devices
		static float device_refresh_timer = 0.0f;
    	device_refresh_timer += io_->DeltaTime;
		if (device_refresh_timer >= deviceRefreshInterval_)
		{
			ctrl_->getSoundsModule()->updateDevices();
			device_refresh_timer = 0.0f;
		}

		//ShowDemoWindow();     // Ventana de demostración
		//ShowMetricsWindow();  // Ventana de métricas

		// vvvvvvvvv Contenido de la ventana vvvvvvvvv

		// BARRA DE MENÚ SUPERIOR
		mainmenu_bar();

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
			main_window();
		End();
		PopStyleVar(4);

		
		// ^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^

		end_frame();
	}


	// Elementos de interfaz ----------------------------------------------------------------

	void GuiMgr::mainmenu_bar() {
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

	void GuiMgr::main_window() {
		// Variables estáticas (solo se crean una vez) para guardar datos
		static float heightRightTop = 0.5f; 
		static float totalHeight = GetContentRegionAvail().y;
		static float sizeX__Izq = GetContentRegionAvail().x * 0.3f;
		static std::string TTS_text;
		
		// COLUMNA IZQUIERDA
		BeginGroup();
		{
			// Panel F1 (Arriba Izquierda)
			BeginChild("##F1", ImVec2(sizeX__Izq, GetContentRegionAvail().y), ImGuiChildFlags_AutoResizeX | ImGuiChildFlags_AutoResizeY | ImGuiChildFlags_FrameStyle);
	
			// Botón de modo
			if (Button(       (ctrl_->isOnlineMode()) ? "ONLINE" : "OFFLINE"        ) ){
				ctrl_->setOnlineMode(!ctrl_->isOnlineMode());
			}

			// Test de temas (Combobox from imgui demo)

			Text("Tema:"); SameLine();
			static const char* theme_items[] = {
				"Dashboard",
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
							case 0: Style_Dashboard(); 			break;
							case 1: Style_AdobeInspired(); 		break;
							case 2: Style_AyuDark(); 			break;
							case 3: Style_Confy(); 				break;
							case 4: Style_DarkCyan(); 			break;
							case 5: Style_DefaultDark(); 		break;
							case 6: Style_DefaultLight();		break;
							case 7: Style_Everforest(); 		break;
							case 8: Style_FutureDark(); 		break;
							case 9: Style_Gold(); 				break;
							case 10: Style_HazyDark(); 			break;
							case 11: Style_KazamsCherry(); 		break;
							case 12: Style_LightOrange(); 		break;
							case 13: Style_QuickMinimalLook(); 	break;
							case 14: Style_Modern(); 			break;
							case 15: Style_Microfrost(); 		break;
							case 16: Style_Moonlight(); 		break;
							case 17: Style_SonicRiders(); 		break;
							case 18: Style_VisualStudio(); 		break;

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
	
	void GuiMgr::playerCard(
		std::string const&           idPrefix,
		std::string const&           name,
		AudioPlaybackModule*         mod,
		std::function<void()> const& onPlay,
		std::function<void()> const& onStop,
		std::function<void()> const& onRemove)
	{
		// idPrefix distingue el tipo (audio/morse/tts): dos players con el mismo
		// nombre pero de distinto tipo no deben compartir ID de ImGui.
		const std::string cardId = idPrefix + "##" + name;
		PushID(cardId.c_str());

		const bool playing = mod && mod->isPlaying();
		BeginChild(
			"##card",
			ImVec2(0, 170),
			ImGuiChildFlags_Borders | ImGuiChildFlags_AlwaysUseWindowPadding
		);

		// Cabecera: nombre grande + estado
		SetWindowFontScale(1.5f);
		Text("%s", name.c_str());
		SetWindowFontScale(1.0f);
		SameLine();
		SetWindowFontScale(1.2f);
		TextColored(
			playing ? ImVec4(0.35f,0.85f,0.35f,1.0f) : ImVec4(0.55f,0.55f,0.55f,1.0f),
			playing ? "● Playing" : "○ Idle"
		);
		SetWindowFontScale(1.0f);

		Dummy(ImVec2(0, 10));

		// Botones grandes: Play / Stop
		const ImVec2 bigBtn(130, 50);
		PushStyleColor(ImGuiCol_Button,        ImVec4(0.18f,0.55f,0.30f,1.0f));
		PushStyleColor(ImGuiCol_ButtonHovered, ImVec4(0.22f,0.68f,0.36f,1.0f));
		PushStyleColor(ImGuiCol_ButtonActive,  ImVec4(0.14f,0.45f,0.24f,1.0f));
		if (Button("Play", bigBtn) && onPlay) onPlay();
		PopStyleColor(3);
		SameLine();
		if (Button("Stop", bigBtn) && onStop) onStop();

		// Slider de volumen del módulo (a la derecha de Play/Stop): más cómodo
		// de manejar con ratón que un knob, y muestra el valor exacto siempre.
		if (mod) {
			int vol = mod->getModuleVolume();
			SameLine();
			Dummy(ImVec2(20, 0));
			SameLine();
			const float sliderWidth = GetContentRegionAvail().x - bigBtn.x - 20.0f;
			PushItemWidth(sliderWidth > 80.0f ? sliderWidth : 80.0f);
			if (SliderInt("Volume", &vol, 0, 100, "%d%%"))
				mod->setModuleVolume(static_cast<unsigned short>(vol));
			PopItemWidth();
		}

		// Botón eliminar en rojo, separado a la derecha del todo
		SameLine(GetContentRegionAvail().x - bigBtn.x + GetCursorPosX());
		PushStyleColor(ImGuiCol_Button,        ImVec4(0.65f,0.15f,0.15f,1.0f));
		PushStyleColor(ImGuiCol_ButtonHovered, ImVec4(0.80f,0.20f,0.20f,1.0f));
		PushStyleColor(ImGuiCol_ButtonActive,  ImVec4(0.50f,0.10f,0.10f,1.0f));
		const bool removeClicked = Button("Remove", bigBtn);
		PopStyleColor(3);

		EndChild();
		PopID();

		Dummy(ImVec2(0, 6));

		// IMPORTANTE: se llama DESPUÉS de EndChild/PopID, cuando ya no se
		// vuelve a tocar 'mod' en este frame (removePlayerX() lo destruye).
		if (removeClicked && onRemove) onRemove();
	}

	void GuiMgr::panelSounds() {
		SoundMgr* snd = ctrl_ ? ctrl_->getSoundsModule() : nullptr;
		if (!snd) {
			Text("Sounds module not available.");
			return;
		}

		// Fuente más grande para toda la pestaña
		SetWindowFontScale(1.2f);

		// ================================================================ Dispositivos
		SeparatorText("Devices");
		Dummy(ImVec2(0, 4));

		if (Button("Refresh devices", ImVec2(190, 40)))
			snd->updateDevices();
		SameLine();
		Dummy(ImVec2(10, 0));
		SameLine();
		Text("Default playback device: %s", snd->getDefaultPlaybackDevice().c_str());

		Dummy(ImVec2(0, 6));
		{
			auto pb  = snd->getAvailablePlaybacks();
			auto cap = snd->getAvailableCaptures();

			if (BeginTable("snd_devices_tbl", 2, ImGuiTableFlags_Borders | ImGuiTableFlags_RowBg, ImVec2(0, 130))) {
				TableSetupColumn("Playback devices");
				TableSetupColumn("Capture devices");
				TableHeadersRow();

				const size_t rows = (pb.size() > cap.size()) ? pb.size() : cap.size();
				for (size_t i = 0; i < rows; ++i) {
					TableNextRow();
					TableSetColumnIndex(0);
					if (i < pb.size())  Text("%s", pb[i].c_str());
					TableSetColumnIndex(1);
					if (i < cap.size()) Text("%s", cap[i].c_str());
				}
				EndTable();
			}
		}

		Dummy(ImVec2(0, 24));

		// ================================================================ Audio players
		SeparatorText("Audio players");
		Dummy(ImVec2(0, 4));

		Text("New player name:");
		SameLine();
		static char audioNewName[64] = "";
		PushItemWidth(260);
		InputTextWithHint("##audioNewName", "e.g. front-speaker", audioNewName, sizeof(audioNewName));
		PopItemWidth();
		SameLine();
		if (Button("+ Add player##audioAdd", ImVec2(180, 36)) && audioNewName[0] != '\0') {
			snd->addPlayerAudio(nullptr, audioNewName, "");
			audioNewName[0] = '\0';
		}

		Dummy(ImVec2(0, 6));

		Text("File to play:");
		SameLine();
		static char audioFilepath[256] = "";
		PushItemWidth(340);
		InputTextWithHint("##audioFilepath", "path/to/sound.wav", audioFilepath, sizeof(audioFilepath));
		PopItemWidth();
		SameLine();
		Dummy(ImVec2(10, 0));
		SameLine();
		Text("Volume:");
		SameLine();
		static int audioPlayVolume = 100;
		PushItemWidth(180);
		SliderInt("##audioPlayVolume", &audioPlayVolume, 0, 100, "%d%%");
		PopItemWidth();

		Dummy(ImVec2(0, 14));

		for (auto const& name : snd->getPlayerAudioNames()) {
			PlayerAudio* pa = snd->getPlayerAudio(name);
			const std::string filepath = audioFilepath;
			const int         volume   = audioPlayVolume;

			playerCard("audio", name, pa,
				[pa, filepath, volume]() { if (pa && !filepath.empty()) pa->playAudio(filepath, static_cast<unsigned short>(volume)); },
				[pa, filepath]()         { if (pa && !filepath.empty()) pa->stop(filepath, false, 0, 0); },
				[snd, name]()            { snd->removePlayerAudio(name); }
			);
		}

		Dummy(ImVec2(0, 24));

		// ================================================================ Morse players
		SeparatorText("Morse players");
		Dummy(ImVec2(0, 4));

		Text("New player name:");
		SameLine();
		static char morseNewName[64] = "";
		PushItemWidth(260);
		InputTextWithHint("##morseNewName", "e.g. beacon", morseNewName, sizeof(morseNewName));
		PopItemWidth();
		SameLine();
		if (Button("+ Add player##morseAdd", ImVec2(180, 36)) && morseNewName[0] != '\0') {
			snd->addPlayerMorse(nullptr, morseNewName);
			morseNewName[0] = '\0';
		}

		Dummy(ImVec2(0, 6));

		Text("Text to play:");
		SameLine();
		static char morseText[256] = "";
		PushItemWidth(340);
		InputTextWithHint("##morseText", "SOS", morseText, sizeof(morseText));
		PopItemWidth();

		Dummy(ImVec2(0, 14));

		for (auto const& name : snd->getPlayerMorseNames()) {
			PlayerMorse* pm = snd->getPlayerMorse(name);
			const std::string text = morseText;

			playerCard("morse", name, pm,
				[pm, text]() { if (pm && !text.empty()) pm->playMorse(text); },
				[pm, text]() { if (pm && !text.empty()) pm->stop(text, false, 0, 0); },
				[snd, name]() { snd->removePlayerMorse(name); }
			);
		}

		Dummy(ImVec2(0, 24));

		// ================================================================ TTS players
		SeparatorText("TTS players");
		Dummy(ImVec2(0, 4));

		Text("New player name:");
		SameLine();
		static char ttsNewName[64] = "";
		PushItemWidth(260);
		InputTextWithHint("##ttsNewName", "e.g. announcer", ttsNewName, sizeof(ttsNewName));
		PopItemWidth();
		SameLine();
		if (Button("+ Add player##ttsAdd", ImVec2(180, 36)) && ttsNewName[0] != '\0') {
			snd->addPlayerTTS(nullptr, ttsNewName, "");
			ttsNewName[0] = '\0';
		}

		Dummy(ImVec2(0, 6));

		Text("Model:");
		SameLine();
		static char ttsModel[64] = "";
		PushItemWidth(180);
		InputTextWithHint("##ttsModel", "model", ttsModel, sizeof(ttsModel));
		PopItemWidth();
		SameLine();
		Dummy(ImVec2(10, 0));
		SameLine();
		Text("Audio name:");
		SameLine();
		static char ttsAudioName[64] = "";
		PushItemWidth(180);
		InputTextWithHint("##ttsAudioName", "id", ttsAudioName, sizeof(ttsAudioName));
		PopItemWidth();

		Dummy(ImVec2(0, 6));

		Text("Text to say:");
		SameLine();
		static char ttsText[256] = "";
		PushItemWidth(340);
		InputTextWithHint("##ttsText", "hello world", ttsText, sizeof(ttsText));
		PopItemWidth();

		Dummy(ImVec2(0, 14));

		for (auto const& name : snd->getPlayerTTSNames()) {
			PlayerTTS* pt = snd->getPlayerTTS(name);
			const std::string model     = ttsModel;
			const std::string audioName = ttsAudioName;
			const std::string text      = ttsText;

			playerCard("tts", name, pt,
				[pt, model, audioName, text]() { if (pt && !model.empty() && !audioName.empty() && !text.empty()) pt->playTTS(model, text, audioName); },
				[pt, audioName]()               { if (pt && !audioName.empty()) pt->stop(audioName, false, 0, 0); },
				[snd, name]()                    { snd->removePlayerTTS(name); }
			);
		}

		SetWindowFontScale(1.0f);
	}

	void GuiMgr::columnaDerecha() {
		//  COLUMNA DERECHA (Layout principal)
		BeginGroup(); 
		{
			static ImGuiTabBarFlags tab_bar_flags = ImGuiTabBarFlags_None;
			if (BeginTabBar("MyTabBar", tab_bar_flags)) {

				if (BeginTabItem("Sounds")) {
					panelSounds();
					EndTabItem();
				}

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
						Image(get_image("imageres/cat.png"), ImVec2(200,100));
						TableNextColumn();
						Image(get_image("imageres/nonexist?"), ImVec2(200,100));
						TableNextColumn();
						Image(get_image("imageres/cat.png"), ImVec2(100,200));

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
					std::string current_model = (!soundsData_.tts.loaded_models.empty()) ? soundsData_.tts.loaded_models[selected_idx] : "";
	
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
						const char* preview_value = (soundsData_.tts.loaded_models.empty()) ? "" : soundsData_.tts.loaded_models[selected_idx].c_str();
						if (BeginCombo("##cb_model", preview_value)) {
							for (int n = 0; n < static_cast<int>(soundsData_.tts.loaded_models.size()); ++n) {
								if (Selectable(soundsData_.tts.loaded_models[n].c_str(), selected_idx == n)) selected_idx = n;
							}
							EndCombo();
						}

						Spacing(); Spacing();

						if (Button("Generate Wav", ImVec2(-FLT_MIN, 40))) { 
							// Usamos el buffer manual que el usuario ha escrito
							ctrl_->getSoundsModule()->getTTSCore()->generateWav(current_model, manual_buffer, current_model);
						}
						if (is_online) EndDisabled(); // Cerramos el bloque de deshabilitado

					EndChild();



					EndTabItem();
				}
				
				if (BeginTabItem("audio")) {

					SoundMgr* snd = ctrl_->getSoundsModule();

					// Pide al controlador la lista de micrófonos disponibles en este momento
					std::vector<std::string> entradas = snd->getAvailableCaptures();

					// Lista con los nombres de los dispositivos que el usuario ha activado
					static std::vector<std::string> dispositivos_activos;
					dispositivos_activos = snd->getCaptureModuleNames();

					// Flag para abrir/cerrar la ventana flotante del selector
					static bool show_device_selector = false;

					// --- Muestra los dispositivos activos ---
					Text("Dispositivos activos:");
					if (dispositivos_activos.empty()) {
						TextDisabled("  (ninguno)");   // Si no hay ninguno, muestra texto gris
					} else {

						// #TODO (rehacer)

						AudioCaptureModule* acm = nullptr;

						short i = 0;
						for (std::string const& captureName : dispositivos_activos) {
							PushID(i);

							// Obtener el dispositivo de captura
							acm = snd->getCapture(captureName);

							if (!acm)
								continue;

							// Si el dispositivo se ha desconectado, mostrar aviso en rojo
							if (!acm->isValid()) {
								TextColored(ImVec4(1, 0, 0, 1), " %s - DESCONECTADO", captureName.c_str());
								SameLine();
								if (SmallButton("x")) 
									snd->removeCaptureDevice(captureName);
								
								
							} else {
								
								Text("%s", captureName.c_str());
								SameLine();
								if (SmallButton("Grabar"))
									acm->StartRec(acm->getModuleName() + "_REC");
								SameLine();
								if (SmallButton("Parar"))
									acm->StopRec();
								SameLine();
								if (SmallButton("x"))
									snd->removeCaptureDevice(captureName);
								
	
								SameLine();
	
								Text(std::to_string(acm->getBufferSize()).c_str());
								Text(std::to_string(acm->getRecBufferSize()).c_str());

								SameLine();

								// Medidor VU - barra vertical RMS
								float LevelVal = acm->getPeakLevel();

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

					// --- Ventana flotante: selector de dispositivos disponibles ---
					if (show_device_selector) {
						SetNextWindowSize(ImVec2(400, 300), ImGuiCond_FirstUseEver);    // Tamaño inicial de la ventana
						if (Begin("Dispositivos de entrada", &show_device_selector)) {  // &show_device_selector: la X de la ventana la cierra
							if (entradas.empty()) {
								TextDisabled("No hay dispositivos disponibles.");
							} else {
								for (int n = 0; n < static_cast<int>(entradas.size()); ++n) {
									if (Selectable(entradas[n].c_str())) {
										snd->addCaptureDevice(nullptr, entradas[n]);
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

	uintptr_t GuiMgr::get_image(std::string path) {
		// Si la imagen no está precacheada, se añade
		if (images_.find(path) == images_.end())
			add_texture_from_file(path);

		return images_[path].tex;
	}

	void GuiMgr::unload_images() {

		// No hace falta borrar si no hay nada
		if (images_.empty())
			return;

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

	void GuiMgr::add_texture_from_file(std::string filename) {

		SYS_INFO("GuiMgr","Saving '" + filename + "' image texture to cache...");

		imageData img_data;		// Variable temporal para almacenar datos de la imagen cargada
		img_data.tex 	  = 0;	// Textura por defecto

		// Generar textura rosa/blanca por defecto si no se ha generado aún
		if (defaultTexture_ == 0) generate_default_texture();

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

	void GuiMgr::generate_default_texture() {
		SYS_INFO("GuiMgr", "Generating default texture image...");

		const int width = 64;
		const int height = 64;
		const int checkSize = 32; // Tamaño de cada cuadro del mosaico
		
		// 4 bytes por píxel (RGBA)
		std::vector<unsigned char> data(width * height * 4);

		// Usa size_t para dimensiones y tamaños para evitar conversiones implícitas
		const size_t uWidth = static_cast<size_t>(width);
		const size_t uHeight = static_cast<size_t>(height);
		const size_t uCheckSize = static_cast<size_t>(checkSize);

		for (size_t y = 0; y < uHeight; ++y) {
			for (size_t x = 0; x < uWidth; ++x) {
				size_t index = (y * uWidth + x) * 4U;
				
				// Lógica del mosaico
				bool isPink = ((x / uCheckSize) + (y / uCheckSize)) % 2U == 0;

				if (isPink) {
					// Rosa (Magenta)
					data[index + 0U] = static_cast<unsigned char>(255);
					data[index + 1U] = static_cast<unsigned char>(0);
					data[index + 2U] = static_cast<unsigned char>(255);
					data[index + 3U] = static_cast<unsigned char>(255);
				} else {
					// Negro
					data[index + 0U] = static_cast<unsigned char>(0);
					data[index + 1U] = static_cast<unsigned char>(0);
					data[index + 2U] = static_cast<unsigned char>(0);
					data[index + 3U] = static_cast<unsigned char>(255);
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

	bool GuiMgr::set_fullscreen(bool enabled) {
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
			glfwSetWindowMonitor(window_, NULL, (int)windowPosX_, (int)windowPosY_, (int)windowSizeX_, (int)windowSizeY_, 0);
		}

		fullscreen_ = enabled;
		return true;
	}

	void GuiMgr::update_density(int delta) {

		int new_size = (int)style_->FontSizeBase + delta;

		if (new_size > (int)MAX_FONT_SIZE_) {
			SYS_WARN("GuiMgr","Font size bigger than max allowed.");
			return;
		}
		if (new_size < static_cast<int>(MIN_FONT_SIZE_)) {
			SYS_WARN("GuiMgr","Font size smaller than min allowed.");
			return;
		}

		// Guardar factor de escala relativo al tamaño anterior
		float prev = style_->FontSizeBase;
		float next = static_cast<float>(new_size);
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

	void GuiMgr::titlebar_dark_mode(bool useDarkMode) {
		#ifdef _WIN32
			std::string st_darkmode=(useDarkMode ? "Dark" : "Light");
			BOOL useDarkMode_ = useDarkMode ? TRUE : FALSE;
			DwmSetWindowAttribute(glfwGetWin32Window(window_), 20, &useDarkMode_, sizeof(useDarkMode_));
			SYS_INFO("GuiMgr",  st_darkmode + " window title set");
		#endif
	}


	// Overrides de interfaces observador ---------------------------------------------------

	/* ITTSOberver */

	void GuiMgr::onSoundsDataChanged(OBS_SoundsData const& data) {
		
		/* Recomendable implementar mutex */
		//std::lock_guard<std::mutex> lock(...);
		
		soundsData_ = data;
    }


	// Temas --------------------------------------------------------------------------------

	/* De aquí en adelante, implementado en GuiMgr_Themes.cpp */


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
    bool GuiMgr::init(void*)		{ return false;}
    void GuiMgr::run()				{ return; }
    bool GuiMgr::isRunning() const 	{ return false; }
    bool GuiMgr::close()			{ return false; }

	// Bucle principal ----------------------------------------------------------------------
    void GuiMgr::init_frame()		{ return; }
    void GuiMgr::end_frame()		{ return; }
    void GuiMgr::capture_keys()		{ return; }
    void GuiMgr::bucle_principal()	{ return; }

	// Elementos de interfaz ----------------------------------------------------------------
    void GuiMgr::mainmenu_bar()		{ return; }
    void GuiMgr::main_window()		{ return; }

	// Carga de imágenes --------------------------------------------------------------------
    uintptr_t GuiMgr::get_image(std::string)					{ return 0; }
    void GuiMgr::unload_images()								{ return; }
    void GuiMgr::add_texture_from_file(std::string)			{ return; }
    void GuiMgr::generate_default_texture()					{ return; }
    
	// Aspecto y temas ----------------------------------------------------------------------
    void update_density(int)						{ return; }
    void titlebar_dark_mode(bool)					{ return; }

#endif
