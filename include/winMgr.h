#pragma once

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

    /// @brief Renderiza el contenido de ImGui en la ventana.
    void renderizar();

    /// @brief Inicia un nuevo frame de ImGui.
    void initCuadro();
    
    /// @brief Cierra la ventana y libera los recursos asociados.
    void close();

    /// @brief Comprueba si la ventana sigue abierta.
    /// @return	True si la ventana está abierta, false si se ha cerrado.
    bool isRunning() const;

private:

    // Puntero a la ventana GLFW
    GLFWwindow* window = nullptr;

// Bucle principal ----------------------------------------------------------------------
public:

    /// @brief Actualiza y renderiza un frame de la ventana.
    void CuadroPrincipal();

    /// @brief Dibuja una barra lateral fija en la ventana.
    void DibujarSidebar();

// Temas --------------------------------------------------------------------------------
private:
    void Style_Confy();
    void Style_FutureDark();
    void Style_Moonlight();
    void Style_VisualStudio();
};
