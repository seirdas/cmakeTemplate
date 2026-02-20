#pragma once

#include <string>

struct GLFWwindow;

class WinMgr
{
// General ------------------------------------------------------------------------------
public:
    // Constructor y destructor
    WinMgr();
    ~WinMgr();

    /**
     * @brief Inicializa la gestión de ventanas con GLFW y OpenGL, y configura ImGui.
     * @return	True si la inicialización fue exitosa, false si hubo algún error (como no poder crear la ventana).
     */
    bool init();

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
    /**
    * @brief Inicia un nuevo frame de ImGui.
    */
    void initCuadro();

    /**
    * @brief Renderiza el contenido de ImGui en la ventana.
    */
    void endCuadro();

    // Nombre de la aplicación/ventana
    std::string AppName = "Demo";

    // Tamaño de la ventana
    unsigned int sizeX = 1280;
    unsigned int sizeY = 720;

    // Color de fondo RGBA
    float clearColor[4] = {0.45f, 0.55f, 0.60f, 1.00f};

    // Fuente personalizada
    std::string customFont = "Archivo-Medium.ttf";
    unsigned int fontSize = 18;

    // Puntero a la ventana GLFW
    GLFWwindow* window = nullptr;

    // Indica si la ventana se ha cerrado para evitar cerrar varias veces
    bool cerrado;  

// Bucle principal ----------------------------------------------------------------------
public:

    /**
     * @brief Bucle principal de la ventana. Se encarga de iniciar un nuevo frame, renderizar el contenido de ImGui, y actualizar la ventana.
     *        Se llama repetidamente mientras la ventana esté abierta.
     */
    void BuclePrincipal();

private:
    /**
     * @brief Crea la barra de menú principal.
     */
    void crearMainMenuBar();
    float MainMenuBar_Height = 0.0f; // Almacena el alto de la barra de menú para ajustar la ventana principal

// Temas --------------------------------------------------------------------------------
private:
    void Style_Confy();
    void Style_FutureDark();
    void Style_Moonlight();
    void Style_VisualStudio();
};
