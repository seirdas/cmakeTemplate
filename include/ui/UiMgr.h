#pragma once

#include <string>
#include <cstdint>              // uintptr_t
#include <unordered_map>
#include "IAppControl.hpp"      // Interfaz de comunicación entre miembros de la aplicación

// Para evitar añadir el include, declarar las estructuras indicando que existen

struct GLFWwindow;
struct ImGuiStyle;
struct ImGuiIO;

/**
  * @class UiMgr
  * @brief Gestor de ventana y loop principal usando GLFW, OpenGL e ImGui.
  *  UiMgr inicializa y gestiona la ventana principal de la aplicación, configura
  *  ImGui, proporciona el bucle principal de renderizado y utilidades para
  *  estilos visuales. 
  *  Comunica con el resto de la aplicación mediante la interfaz IAppControl suministrada.
  *  Uso:
  *      Crear una instancia (opcionalmente pasando un IAppControl).
  *      Llamar init() para inicializar GLFW/OpenGL/ImGui y crear la ventana.
  *      Llamar run() para ejecutar el bucle principal (bloqueante hasta el cierre).
  *      Llamar cerrar() para liberar recursos manualmente.
  * @note El destructor virtual garantiza la limpieza correcta en clases derivadas.
  * @note Los métodos privados initCuadro() y endCuadro() encapsulan el inicio y fin de
  *  cada frame de ImGui.
  * @note BuclePrincipal() puede ser sobreescrito para modificar los elementos de la UI.
  * @note Proporciona varias funciones de estilo para configurar la apariencia de ImGui.
  * @see IAppControl
  * @date March 2, 2026 
  */
class UiMgr {

public:

// General ------------------------------------------------------------------------------

    /**
     * @brief Constructor por defecto.
     */
    UiMgr(IAppControl* ctrl_ = nullptr);
    
    /**
     * @brief Destructor. Llama a cerrar() para liberar recursos.
     * @note Virtual para evitar comportamientos inesperados en clases derivadas.
     */
    virtual ~UiMgr();
    
    /**
     * @brief Establece el controlador que usa para manejar otras clases.
     */
    void setController(IAppControl* controller);

    // Estados __________________________________

    /**
     * @brief Inicializa la gestión de ventanas con GLFW y OpenGL, y configura ImGui.
     * @return	True si la inicialización fue exitosa, false si hubo algún error (como no poder crear la ventana).
     */
    bool init();

    /**
     * @brief Inicia el bucle principal de la ventana. Este método bloquea hasta que la ventana se cierre.
     */
    void run();

    /**
    * @brief Comprueba si la ventana sigue abierta.
    * @return	True si la ventana está abierta, false si se ha cerrado.
    */
    bool isRunning() const;

    /**
     * @brief Cierra la ventana y libera los recursos asociados.
     */
    void cerrar();


private:
    // Bucle principal ____________________________________

    /**
    * @brief Inicia un nuevo frame de ImGui.
    */
    void initCuadro();

    /**
    * @brief Renderiza el contenido de ImGui en la ventana.
    */
    void endCuadro();

    /**
     * @brief Llama a funciones con combinaciones de teclas.
     */
    void captureKeys();

    /**
     * @brief Bucle principal de la ventana. Se encarga de iniciar un nuevo frame, renderizar el contenido de ImGui, y actualizar la ventana.
     *        Se llama repetidamente mientras la ventana esté abierta.
     */
    virtual void BuclePrincipal();

    /**
     * @brief Crea la barra de menú principal.
     */
    void crearMainMenuBar();

    /**
     * @brief Crea la ventana principal
     */
    void ventanaPrincipal();

    // Carga de imágenes __________________________________
    
    /**
     * @brief Prepara las imágenes que se van a representar.
     */
    void loadImages();

    /**
     * @brief Libera los recursos de las imágenes cargadas de forma dinámica.
     * @note No hace falta actualizar esta función al añadir nuevas imágenes
     */
    void unloadImages();
    
    /**
     * @brief Carga una textura de imagen desde un archivo de imagen.
     *  Ejemplo: images_["filename.png"].tex
     * @note Soporte para png.
     * @param filename Ruta del archivo de imagen que se va a cargar.
     * @return intptr_t El identificador de la textura cargada.
     */
    void addTextureFromFile(std::string filename);
    
    /**
     * @brief Actualiza el tamaño de los elementos de la ui incluyendo fuentes.
     * @param delta diferencia de tamaño (+1, +2, -1, -2...) 
     */
    void updateDensity(int delta);


// Temas --------------------------------------------------------------------------------

    /**
     * @brief Cambia la barra de título entre modo claro/oscuro
     * @param useDarkMode true modo oscuro, false modo claro.
     * @note Solo para Windows
     */
    void titleBarDarkMode(bool useDarkMode);
    
    /**
     * @brief Dark - Comfy style by Giuseppe from ImThemes
     */
    void Style_Confy();

    /**
     * @brief Dark - Future Dark style by rewrking from ImThemes
     */
    void Style_FutureDark();

    /**
     * @brief Dark - Moonlight style by rewrking from ImThemes
     */
    void Style_Moonlight();
    
    /**
     * @brief Dark - Rounded Visual Studio style by RedNicStone from ImThemes
     */
    void Style_VisualStudio();
    
    /**
     * @brief Light - Microsoft style by usernameiwantedwasalreadytaken from ImThemes
     */
    void Style_Microfrost();
    
    
    
/************ Variables ********************************************************/
    
    // Propiedades de la ventana ________________
    ImGuiStyle*     style_          = nullptr;                  // Modificar ajustes de estilo
    ImGuiIO*        io_             = nullptr;                  // Manejar entrada/salida
    bool            captureKeys_    = false;                    // Modo Debug para detección de teclas en consola

    std::string     AppName_        = "Demo";                   // Nombre de la aplicación/ventana
    unsigned int    sizeX_          = 1280;                     // Tamaño horizontal (x) de la ventana
    unsigned int    sizeY_          = 720;                      // Tamaño vertical (y) de la ventana

    float           fontSize_         = 16.0f;                  // Tamaño de fuente predeterminado
    unsigned int const MAX_FONT_SIZE_ = 30;                     // Tamaño de fuente máximo permitido
    unsigned int const MIN_FONT_SIZE_ = 14;                     // Tamaño de fuente mínimo permitido

    GLFWwindow*     window_         = nullptr;                  // Puntero a la ventana GLFW
    IAppControl*    ctrl_           = nullptr;                  // Puntero al controlador de la aplicación para comunicación entre miembros
    bool            running_        = false;                    // Indica si la ventana se ha cerrado para evitar cerrar varias veces
    
// Imágenes _________________________________

    struct imageData {      // Datos de imagen
        const char* name={};// Nombre de la imagen (para ID)
        uintptr_t tex = 0;  // Puntero a textura (lo que usa imgui)
        int x = 0;          // resolución width (ancho)
        int y = 0;          // resolución height (alto)
    };
    // Cargar las imágenes en la función loadImages
    std::unordered_map<std::string, imageData> images_;         // Mapa de imágenes cargadas

// Variables (MenuBar) ______________________
    float           MainMenuBar_Height_       = 0.0f;           // Almacena el alto de la barra de menú para ajustar la ventana principal
    
// Elementos (1) ____________________________
    int      sl_volume = 2;
    float    sl_pitch = 0;
    float    sl_position = 0;
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