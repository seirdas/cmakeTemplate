
#include "winMgr.h"
#include <GLFW/glfw3.h>

#ifdef _WIN32
    #define GLFW_EXPOSE_NATIVE_WIN32
    #include <GLFW/glfw3native.h>
    #include <windows.h>
#endif

#include <imgui.h>
#include <backends/imgui_impl_glfw.h>
#include <backends/imgui_impl_opengl3.h>



// General ---------------------------------------------------------------------


// Inicialización -----------------------------------------------------------------

//@brief Inicializa la gestión de ventanas con GLFW y OpenGL, y configura ImGui.
// https://github.com/ocornut/imgui/wiki/Getting-Started#example-if-you-are-using-glfw--openglwebgl
bool WinMgr::init() {
    if (!glfwInit()) return false;

    // 1. Configuración de la ventana GLFW para que sea un "lienzo" invisible
    glfwWindowHint(GLFW_DECORATED, GLFW_FALSE);      // Sin bordes (WS_POPUP)
    glfwWindowHint(GLFW_TRANSPARENT_FRAMEBUFFER, GLFW_TRUE); // Fondo transparente
    glfwWindowHint(GLFW_FLOATING, GLFW_TRUE);        // Siempre encima (TOPMOST)
    glfwWindowHint(GLFW_RESIZABLE, GLFW_FALSE);      // Evitar cambios raros de tamaño

    // Creamos la ventana al tamaño de la pantalla o el que desees
    window = glfwCreateWindow(1280, 720, "Overlay", NULL, NULL);
    glfwMakeContextCurrent(window);
    glfwSwapInterval(1);

    // 2. MAGIA DE WINDOWS: Hacer que la ventana de GLFW sea "transparente al ratón"
    // Obtenemos el HWND nativo (lo mismo que usa el código de DX11 que viste)
    HWND hwnd = glfwGetWin32Window(window);
    
    // WS_EX_LAYERED permite transparencia
    // WS_EX_TRANSPARENT hace que los clics pasen a través de la ventana
    // (Solo clicas lo que ImGui dibuje si manejas bien los focos)
    SetWindowLong(hwnd, GWL_EXSTYLE, GetWindowLong(hwnd, GWL_EXSTYLE) | WS_EX_LAYERED);

    // --- INICIALIZACIÓN DE IMGUI ---
    IMGUI_CHECKVERSION();
    ImGui::CreateContext();
    ImGuiIO& io = ImGui::GetIO();
    
    // IMPORTANTE: NO uses Viewports aquí. Queremos que ImGui se dibuje 
    // DENTRO de nuestra ventana invisible de GLFW que ocupa toda la zona.
    io.ConfigFlags |= ImGuiConfigFlags_NavEnableKeyboard;

    ImGui_ImplGlfw_InitForOpenGL(window, true);
    ImGui_ImplOpenGL3_Init("#version 130");
    return true;
}

void WinMgr::frame()
{
    glfwPollEvents();

    ImGui_ImplOpenGL3_NewFrame();
    ImGui_ImplGlfw_NewFrame();
    ImGui::NewFrame();

    // 3. Lógica de "Click-through" (Atravesar con el ratón)
    // Si el ratón NO está sobre una ventana de ImGui, dejamos que el clic pase al escritorio
    HWND hwnd = glfwGetWin32Window(window);
    if (ImGui::GetIO().WantCaptureMouse) {
    // El ratón está sobre la UI: Capturamos clics
    SetWindowLong(hwnd, GWL_EXSTYLE, GetWindowLong(hwnd, GWL_EXSTYLE) & ~WS_EX_TRANSPARENT);
    } else {
    // El ratón está en el vacío: Los clics pasan al escritorio
    SetWindowLong(hwnd, GWL_EXSTYLE, GetWindowLong(hwnd, GWL_EXSTYLE) | WS_EX_TRANSPARENT);
    }

    ImGui::Begin("Panel Flotante"); 
    ImGui::Text("Soy un panel real.");
    ImGui::Text("Si clicas fuera de este cuadro,");
    ImGui::Text("clicarás en las carpetas de tu escritorio.");
    
    if (ImGui::Button("Cerrar App")) {
        glfwSetWindowShouldClose(window, true);
    }
    ImGui::End();

    ImGui::Render();

    // Renderizado con fondo totalmente transparente
    int w, h;
    glfwGetFramebufferSize(window, &w, &h);
    glViewport(0, 0, w, h);
    glClearColor(0, 0, 0, 0); // Este es el "ColorKey" (negro transparente)
    glClear(GL_COLOR_BUFFER_BIT);

    ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());
    glfwSwapBuffers(window);
}



//@brief Limpieza de recursos
void WinMgr::close()
{
    ImGui_ImplOpenGL3_Shutdown();
    ImGui_ImplGlfw_Shutdown();
    ImGui::DestroyContext();

    if (window)
    {
        glfwDestroyWindow(window);
        window = nullptr;
    }

    glfwTerminate();
}


bool WinMgr::isRunning() const
{
    return window && !glfwWindowShouldClose(window);
}