#pragma once

#include <string>
#include <iostream>
#include <GL/gl.h>
#include "IAppControl.hpp"      // Interfaz de comunicación entre miembros de la aplicación

struct GLFWwindow;


/**
  * @class WinMgr
  * @brief Gestor de ventana y loop principal usando GLFW, OpenGL e ImGui.
  *  WinMgr inicializa y gestiona la ventana principal de la aplicación, configura
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
class WinMgr {

public:

// General ------------------------------------------------------------------------------

    /**
     * @brief Constructor por defecto.
     */
    WinMgr(IAppControl* controller = nullptr);
    
    /**
     * @brief Destructor. Llama a cerrar() para liberar recursos.
     * @note Virtual para evitar comportamientos inesperados en clases derivadas.
     */
    virtual ~WinMgr();
    
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
     * @brief Bucle principal de la ventana. Se encarga de iniciar un nuevo frame, renderizar el contenido de ImGui, y actualizar la ventana.
     *        Se llama repetidamente mientras la ventana esté abierta.
     */
    virtual void BuclePrincipal();

    /**
     * @brief Crea la barra de menú principal.
     */
    void crearMainMenuBar();

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
     * @param filename Ruta del archivo de imagen que se va a cargar.
     * @param outWidth Salida del ancho de la textura cargada.
     * @param outHeight Salida de la altura de la textura cargada.
     * @return intptr_t El identificador de la textura cargada.
     */
    intptr_t LoadTextureFromFile(const char* filename, int& outWidth, int& outHeight);
    

    
// Temas --------------------------------------------------------------------------------
    
    /**
    * @brief Comfy style by Giuseppe from ImThemes
    */
    void Style_Confy();

    /**
    * @brief Future Dark style by rewrking from ImThemes
    */
    void Style_FutureDark();

    /**
    * @brief Moonlight style by rewrking from ImThemes
    */
    void Style_Moonlight();
    
    /**
    * @brief Rounded Visual Studio style by RedNicStone from ImThemes
    */
    void Style_VisualStudio();
    
    /**
     * @brief Microsoft style by usernameiwantedwasalreadytaken from ImThemes
     */
    void Style_Microfost();
    
    
    
    /************ Variables ********************************************************/
    
    // Propiedades de la ventana
    std::string     AppName_        = "Demo";                           // Nombre de la aplicación/ventana
    unsigned int    sizeX_          = 1280;                             // Tamaño horizontal (x) de la ventana
    unsigned int    sizeY_          = 720;                              // Tamaño vertical (y) de la ventana
    float           clearColor_[4]  = {1.00f, 1.00f, 1.00f, 1.00f};     // Color de fondo RGBA
    std::string     customFont_     = "Archivo-Medium.ttf";             // Fuente personalizada
    unsigned int    fontSize_       = 18;                               // Tamaño de fuente predeterminado
    GLFWwindow*     window_         = nullptr;                          // Puntero a la ventana GLFW
    IAppControl*    controller_     = nullptr;                          // Puntero al controlador de la aplicación para comunicación entre miembros
    bool            cerrado_        = false;                            // Indica si la ventana se ha cerrado para evitar cerrar varias veces
    
    // Imágenes
    GLuint img_cat_ = 0;
    int catW_ = 0;
    int catH_ = 0;

    // Variables (MenuBar)
    float           MainMenuBar_Height_       = 0.0f;                   // Almacena el alto de la barra de menú para ajustar la ventana principal
    
    // Elementos (1)
    int      sl_volume = 2;
    float    sl_pitch = 0;

};
