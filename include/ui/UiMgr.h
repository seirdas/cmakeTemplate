#pragma once

#include <iostream>
#include <string>
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
  *      Comprobar isRunning() para decidir acciones desde fuera.
  *      Llamar cerrar() para liberar recursos manualmente.
  *  Características y notas:
  * @note El destructor virtual garantiza la limpieza correcta en clases derivadas.
  * @note Los métodos privados initCuadro() y endCuadro() encapsulan el inicio y fin de
  *  cada frame de ImGui.
  * @note BuclePrincipal() puede ser sobreescrito para modificar los elementos de la UI.
  * @note Proporciona varias funciones de estilo (Style_Confy, Style_FutureDark, etc.)
  *  para configurar la apariencia de ImGui.
  * @note Evita cerrar recursos múltiples mediante la bandera cerrado_.
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
     * @brief Libera los recursos de las imágenes cargadas.
     * @note Se hace al cerrar la aplicación
     */
    void unloadImages();
    
    /**
     * @brief Carga una textura de imagen desde un archivo de imagen.
     * @note Soporte para png.
     * @example USO: images_["filename.png"].tex
     * @param filename Ruta del archivo de imagen que se va a cargar.
     * @param outWidth Salida del ancho de la textura cargada.
     * @param outHeight Salida de la altura de la textura cargada.
     * @return intptr_t El identificador de la textura cargada.
     */
    void addTextureFromFile(std::string filename);
    
    /**
     * @brief Actualiza el tamaño de la fuente mostrada.
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
    ImGuiStyle*     style_          = nullptr;                          // Modificar ajustes de estilo
    ImGuiIO*        io_             = nullptr;                          // Manejar entrada/salida
    bool            captureKeys_    = false;                            // Modo Debug de detección de teclas

    std::string     AppName_        = "Demo";                           // Nombre de la aplicación/ventana
    unsigned int    sizeX_          = 1280;                             // Tamaño horizontal (x) de la ventana
    unsigned int    sizeY_          = 720;                              // Tamaño vertical (y) de la ventana
    
    std::string     customFont_     = "Archivo-Medium.ttf";             // Fuente personalizada
    unsigned int    fontSize_       = 16;                               // Tamaño de fuente predeterminado
    unsigned int const MAX_FONT_SIZE_ = 30;
    unsigned int const MIN_FONT_SIZE_ = 14;

    GLFWwindow*     window_         = nullptr;                          // Puntero a la ventana GLFW
    IAppControl*    ctrl_           = nullptr;                          // Puntero al controlador de la aplicación para comunicación entre miembros
    bool            cerrado_        = false;                            // Indica si la ventana se ha cerrado para evitar cerrar varias veces
    
    // Imágenes _________________________________

    struct imageData {      // Datos de imagen
        const char* name={};// Nombre de la imagen (para ID)
        uintptr_t tex = 0;  // Puntero a textura (lo que usa imgui)
        int x = 0;          // resolución width (ancho)
        int y = 0;          // resolución height (alto)
    };
    // Cargar las imágenes en la función loadImages
    std::unordered_map<std::string, imageData> images_;       // Mapa de imágenes cargadas

    // Variables (MenuBar) ______________________
    float           MainMenuBar_Height_       = 0.0f;                   // Almacena el alto de la barra de menú para ajustar la ventana principal
    
    // Elementos (1) ____________________________
    int      sl_volume = 2;
    float    sl_pitch = 0;
    float    sl_position = 0;
};
