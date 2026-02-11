#pragma once

#include <string>

struct GLFWwindow;

class WinMgr
{
// General ------------------------------------------------------------------------------
public:
    // Constructor y destructor
    WinMgr() = default;
    ~WinMgr() = default;

    /// @brief Inicializa la gestión de ventanas con GLFW y OpenGL, y configura ImGui.
    bool init();

    /// @brief Comprueba si la ventana sigue abierta.
    /// @return	True si la ventana está abierta, false si se ha cerrado.
    bool isRunning() const;

    /// @brief Cierra la ventana y libera los recursos asociados.
    void cerrar();

private:
    /// @brief Inicia un nuevo frame de ImGui.
    void initCuadro();

    /// @brief Renderiza el contenido de ImGui en la ventana.
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

// Bucle principal ----------------------------------------------------------------------
public:
    /// @brief Actualiza y renderiza un frame de la ventana.
    void BuclePrincipal();

private:
    /// @brief Crea la barra de menú principal con opciones básicas.
    void crearMainMenuBar();
    float MainMenuBar_Height = 0.0f; // Almacena el alto de la barra de menú para ajustar la ventana principal

// Temas --------------------------------------------------------------------------------
private:
    void Style_Confy();
    void Style_FutureDark();
    void Style_Moonlight();
    void Style_VisualStudio();
};
