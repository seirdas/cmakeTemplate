#pragma once

#include <memory>               // unique_ptr (pimpl)
#include <string>
#include <cstdint>              // uintptr_t
#include <unordered_map>


// Includes de observador
#include "tts/ITTSObserver.hpp"


// Forward declaration
struct GLFWwindow;
struct ImGuiStyle;
struct ImGuiIO;
class IAppControl;


/**
  * @class GuiMgr
  * @brief Gestor de ventana y loop principal usando GLFW, OpenGL e ImGui.
  *  Comunica con el resto de la aplicación mediante la interfaz IAppControl suministrada.
  * @note El destructor virtual garantiza la limpieza correcta en clases derivadas.
  * @note Los métodos privados initCuadro() y endCuadro() encapsulan el inicio y fin de
  *  cada frame de ImGui.
  * @note BuclePrincipal() puede ser sobreescrito para modificar los elementos de la UI.
  * @note Proporciona varias funciones de estilo para configurar la apariencia de ImGui.
  * @see IAppControl
  */
class GuiMgr 
    : public ITTSObserver
{

public:

// General ------------------------------------------------------------------------------

    /**
     * @brief Constructor por defecto.
     * @param ctrl_ Controlador para pedir datos a AppController
     */
    GuiMgr(IAppControl* ctrl_ = nullptr);
    
    /**
     * @brief Destructor. Llama a cerrar() para liberar recursos.
     */
    ~GuiMgr();
    
    /**
     * @brief Establece el controlador que usa para manejar otras clases.
     */
    void setController(IAppControl* controller);
    

// Ejecución ----------------------------------------------------------------------------

    /**
     * @brief Inicializa la gestión de ventanas con GLFW y OpenGL, y configura ImGui.
     * @return	True si la inicialización fue exitosa, false si hubo algún error 
     */
    bool init(void* config);
    
    /**
     * @brief Devuelve si la inicialización ha sido exitosa
     * @return @c true Si ha iniciado bien, @c false en caso contrario
     */
    bool isInitialized() const;

    /**
    * @brief Carga y valida la configuración de la aplicación desde un objeto JSON.
    * Esta función verifica la existencia y el tipo de los campos requeridos en el JSON.
    * Si un campo no existe o es inválido, la función escribe el valor actual por defecto
    * del código en el objeto JSON, asegurando que el archivo de configuración siempre 
    * esté completo y sincronizado.
    * @param config Puntero al objeto JSON que contiene los parámetros de configuración.
    */
    void loadConfig(void* config);

    /**
     * @brief Inicia el bucle principal de la ventana. 
     * @note Este método es bloqueante hasta que la ventana se cierre.
     */
    void run();

    /**
     * @brief Cierra la ventana y libera los recursos asociados.
     */
    bool close();

    /**
    * @brief Comprueba si la ventana sigue abierta.
    * @return	True si la ventana está abierta, false si se ha cerrado.
    */
    bool isRunning() const;


private:

// Captura de teclas --------------------------------------------------------------------

    /**
     * @brief Llama a funciones con combinaciones de teclas.
     */
    void captureKeys();


// Bucle principal ----------------------------------------------------------------------

    /**
    * @brief Inicia un nuevo frame de ImGui.
    */
    void initCuadro();

    /**
    * @brief Renderiza el contenido de ImGui en la ventana.
    */
    void endCuadro();

    /**
     * @brief Bucle principal de la ventana. Se encarga de iniciar un nuevo frame, renderizar el contenido de ImGui, y actualizar la ventana.
     *        Se llama repetidamente mientras la ventana esté abierta.
     */
    virtual void BuclePrincipal();


// Elementos de interfaz ----------------------------------------------------------------

    /**
     * @brief Crea la barra de menú principal.
     */
    void crearMainMenuBar();

    /**
     * @brief Crea la ventana principal
     */
    void ventanaPrincipal();

    /**
     * @brief Gestión de la columna derecha
     */
    void columnaDerecha();


// Carga de imágenes --------------------------------------------------------------------
   
    /**
     * @brief Utiliza una imagen y la precarga en el sistema para usos posteriores.
     */
    uintptr_t getImage(std::string path);

    /**
     * @brief Libera los recursos de las imágenes cargadas de forma dinámica.
     * @note No hace falta actualizar esta función al añadir nuevas imágenes
     */
    void unloadImages();
    
    /**
     * @brief Carga una textura de imagen desde un archivo de imagen.
     * @details Soporte para png.
     * @note Precachea textura por defecto desde defaultTexture_ si falla. 
     * @param filename Ruta del archivo de imagen que se va a cargar.
     */
    void addTextureFromFile(std::string filename);

    /**
     * @brief Genera la textura del clásico mosaico rosa/blanco para texturas fallidas
     * @details Lo guarda en la variable defaultTexture_
     */
    void generateDefaultTexture();
    

// Aspecto ------------------------------------------------------------------------------

    /**
     * @brief Activa/desactiva el modo de pantalla completa
     * @param enable true para pantalla completa, false para modo ventana
     */
    bool setFullscreenMode(bool enable);

    /**
     * @brief Actualiza el tamaño de los elementos de la ui incluyendo fuentes.
     * @param delta diferencia de tamaño (+1, +2, -1, -2...) 
     */
    void updateDensity(int delta);

    /**
     * @brief Cambia la barra de título entre modo claro/oscuro
     * @param useDarkMode true modo oscuro, false modo claro.
     * @note Solo para Windows
     */
    void titleBarDarkMode(bool useDarkMode);


// Overrides de interfaces observador ---------------------------------------------------

    /**
     * @brief Cuando TTSMgr o TTSCore notifiquen un cambio, guardamos los datos en pimpl_
     * @param data Paquete de datos internos de TTS
     */
	void onTTSDataChanged(const TTSCoreData& data) override;

    

// Temas --------------------------------------------------------------------------------

    /**
     * @brief Aplica el tema definido en la variable theme_selected_
     */
    void ApplyTheme();

    /**
     * @brief Guarda el tema en la configuración pasado como parámetro (considerado json)
     */
    void saveConfig();

    /**
     * @brief Dark - AdobeInspired style by nexacopic from ImThemes
     */
    void Style_AdobeInspired();

    /**
     * @brief Dark - Ayu-dark style by usrnatc from ImThemes
     * 
     */
    void Style_AyuDark();

    /**
     * @brief Dark - Comfy style by Giuseppe from ImThemes
     */
    void Style_Confy();

    /**
     * @brief Dark - Comfortable Dark Cyan style by SouthCraftX from ImThemes
     */
    void Style_DarkCyan();

    /**
     * @brief Tema oscuro por defecto ImGui con bordes estilizados
     */
    void Style_DefaultDark();

    /**
     * @brief Tema oscuro por defecto ImGui con bordes estilizados
     */
    void Style_DefaultLight();

    /**
     * @brief Dark - Everforest style by DestroyerDarkNess from ImThemes
     */
    void Style_Everforest();

    /**
     * @brief Dark - Future Dark style by rewrking from ImThemes
     */
    void Style_FutureDark();

    /**
     * @brief Dark - Gold style by CookiePLMonster from ImThemes
     * 
     */
    void Style_Gold();

    /**
     * @brief Dark - Hazy Dark style by kaitabuchi314 from ImThemes
     * 
     */
    void Style_HazyDark();

    /**
     * @brief Dark - Kazam's Cherry style by coyoteclan from ImThemes 
     * 
     */
    void Style_KazamsCherry();

    /**
     * @brief Light - Comfortable Light Orange style by SouthCraftX from ImThemes
     */
    void Style_LightOrange();

    /**
     * @brief Dark - Quick minimal look style by 90th from ImThemes
     * 
     */
    void Style_QuickMinimalLook();

    /**
     * @brief Dark - Modern style by LousyBook-01 from ImThemes 
     * 
     */
    void Style_Modern();

    /**
     * @brief Light - Microsoft style by usernameiwantedwasalreadytaken from ImThemes
     */
    void Style_Microfrost();

    /**
     * @brief Dark - Moonlight style by rewrking from ImThemes
     */
    void Style_Moonlight();

    /**
     * @brief Dark - Sonic Riders style by Sewer56 from ImThemes
     * 
     */
    void Style_SonicRiders();
    
    /**
     * @brief Dark - Rounded Visual Studio style by RedNicStone from ImThemes
     */
    void Style_VisualStudio();

    
/************ Variables ********************************************************/
    
// Pointer to implementation
    struct Impl;                        ///< Estructura PIMPL
    std::unique_ptr<Impl> pimpl_;       ///< Miembros dependientes de IAppController

// Inicialización y ejecución
    void*           config_;            ///< Configuración del módulo (considerado json)
    IAppControl*    ctrl_;              ///< Puntero al controlador de la aplicación para comunicación entre miembros
    bool            running_;           ///< Indica si la ventana se ha cerrado para evitar cerrar varias veces
    bool            initialized_;       ///< Bandera para indicar inicialización exitosa

// Propiedades de la ventana
    GLFWwindow*     window_;            ///< Puntero a la ventana GLFW
    ImGuiStyle*     style_;             ///< Modificar ajustes de estilo
    ImGuiIO*        io_;                ///< Manejar entrada/salida
    bool            captureKeys_;       ///< Modo Debug para detección de teclas en consola

// Parámetros de la ventana
    std::string     AppName_;           ///< Nombre de la aplicación/ventana
    unsigned int    windowSizeX_;       ///< Tamaño horizontal (x) de la ventana
    unsigned int    windowSizeY_;       ///< Tamaño vertical (y) de la ventana
    unsigned int    windowPosX_;        ///< Posición horizontal (x) de la ventana
    unsigned int    windowPosY_;        ///< Posición vertical (y) de la ventana
    bool            fullscreen_;        ///< Modo ventana completa activo
    std::string     theme_selected_;    ///< Tema seleccionado

// Parámetros de fuente de letra
    float              fontSize_;           ///< Tamaño de fuente predeterminado
    unsigned int const MAX_FONT_SIZE_ = 30; ///< Tamaño de fuente máximo permitido
    unsigned int const MIN_FONT_SIZE_ = 14; ///< Tamaño de fuente mínimo permitido

// Tiempos de refresco
    float deviceRefreshInterval_;           ///< Tiempo de refrescar los dispositivos de entrada

// Imágenes
    struct imageData {      ///< Datos de imagen
        uintptr_t tex = 0;  ///< Puntero a textura (lo que usa imgui)
        int x = 0;          ///< resolución width (ancho)
        int y = 0;          ///< resolución height (alto)
        int channels = 0;   ///< Canales de color (no se usa)
    };
    std::unordered_map<std::string, imageData> images_;         ///< Mapa de imágenes cargadas
    uintptr_t defaultTexture_ = 0;                              ///< Textura generada por defecto (mosaico blaco/rosa)

// Variables (MenuBar)
    float           MainMenuBar_Height_       = 0.0f;           ///< Almacena el alto de la barra de menú para ajustar la ventana principal

};

/**** imgui knobs
*  ## Variants
*  ImGuiKnobVariant_Tick
*  ImGuiKnobVariant_Dot
*  ImGuiKnobVariant_Wiper
*  ImGuiKnobVariant_WiperOnly
*  ImGuiKnobVariant_WiperDot
*  ImGuiKnobVariant_Stepped
*  ImGuiKnobVariant_Space
*  ## Flags
*  ImGuiKnobFlags_NoTitle:         Hide the top title.
*  ImGuiKnobFlags_NoInput:         Hide the bottom drag input.
*  ImGuiKnobFlags_ValueTooltip:    Show a tooltip with the current value on hover.
*  ImGuiKnobFlags_DragHorizontal:  Use horizontal dragging only.
*  ImGuiKnobFlags_DragVertical:    Use vertical dragging only.
*  ImGuiKnobFlags_AlwaysClamp:     Clamp input values that the user types into the input field.
*  ImGuiKnobFlags_Logarithmic:     Use logarithmic scale for the knob.
*  ## Custom colors
*  Push/PopStyleColor() for each colors used (namely ImGuiCol_ButtonActive and ImGuiCol_ButtonHovered for primary and ImGuiCol_Framebg for Track)
*/

/**** imgui spinners
* Ejemplo
* ImSpinner::SpinnerDots(
*     "NombreUnico",     // ID para ImGui
*     15.0f,            // Radio
*     4.0f,             // Grosor de los puntos/línea
*     ImColor(1.0f, 1.0f, 1.0f, 1.0f), // Color
*     0.6f              // Velocidad
* );
* 1. Spinners de Puntos (Dots)
*   SpinnerDots: Puntos moviéndose en círculo.
*   SpinnerBounceDots: Puntos que rebotan verticalmente.
*   SpinnerFadeDots: Puntos que aparecen y desaparecen.
*   SpinnerScaleDots: Puntos que cambian de tamaño.
*   SpinnerMovingDots: Puntos que se desplazan linealmente.
*   SpinnerPulsar: Un punto central con ondas expansivas.
* 2. Spinners de Arco y Anillo (Arcs / Ring)
*   SpinnerArc: Un arco simple rotando.
*   SpinnerArcFade: Un arco que cambia su opacidad.
*   SpinnerArcRotation: Arcos concéntricos rotando a diferentes velocidades.
*   SpinnerDoubleArc: Dos arcos enfrentados.
*   SpinnerFilledArc: Un arco con grosor variable.
*   SpinnerRing: Un anillo completo que se llena y vacía.
* 3. Spinners de Figuras Geométricas
*   SpinnerSquare: Un cuadrado que rota y se deforma.
*   SpinnerTriangle: Un triángulo rotatorio.
*   SpinnerDoubleTriangle: Dos triángulos entrelazados.
*   SpinnerAng: Arcos con ángulos específicos.
*   SpinnerTwinAng: Dos arcos angulares simétricos.
* 4. Spinners Especiales y Complejos
*   SpinnerClock: Un reloj con manecillas moviéndose rápido.
*   SpinnerDNA: Una hélice de ADN rotando en 2D.
*   SpinnerSolar: Bolas rotando alrededor de un centro (estilo planetario).
*   SpinnerLoading68: Un spinner estilo retro.
*   SpinnerLoading78: Una variante circular con efectos de estela.
*   SpinnerGooey: Efecto de gotas líquidas fusionándose.
*/
