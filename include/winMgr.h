#pragma once

#include <string>
#include <iostream>
#include "IAppControl.hpp"     // Interfaz de comunicación entre miembros de la aplicación

struct GLFWwindow;

class WinMgr
{
public:

    // General __________________________________

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

//private:
    // Bucle principal __________________________

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
    
    
    
    
    
    /************ Variables ********************************************************/
    
    // Propiedades de la ventana
    std::string     AppName_        = "Demo";                           // Nombre de la aplicación/ventana
    unsigned int    sizeX_          = 1280;                             // Tamaño horizontal (x) de la ventana
    unsigned int    sizeY_          = 720;                              // Tamaño vertical (y) de la ventana
    float           clearColor_[4]  = {0.45f, 0.55f, 0.60f, 1.00f};     // Color de fondo RGBA
    std::string     customFont_     = "Archivo-Medium.ttf";             // Fuente personalizada
    unsigned int    fontSize_       = 18;                               // Tamaño de fuente predeterminado
    GLFWwindow*     window_         = nullptr;                          // Puntero a la ventana GLFW
    IAppControl*    controller_     = nullptr;                          // Puntero al controlador de la aplicación para comunicación entre miembros
    bool            cerrado_        = false;                            // Indica si la ventana se ha cerrado para evitar cerrar varias veces
    
    float           MainMenuBar_Height_       = 0.0f;                   // Almacena el alto de la barra de menú para ajustar la ventana principal

};
