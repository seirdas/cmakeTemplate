
#include "winMgr.h"


// General ---------------------------------------------------------------------
WinMgr::WinMgr(){
    
}

WinMgr::~WinMgr(){
    
}

// Inicialización -----------------------------------------------------------------

//@brief Inicializa la gestión de ventanas con GLFW y OpenGL, y configura ImGui.
void WinMgr::init(){

        // --- INICIALIZACIÓN DE GLFW & OPENGL ---
    if (!glfwInit()) return;
    window = glfwCreateWindow(1280, 720, "Prueba ImGui", NULL, NULL);
    glfwMakeContextCurrent(window);
    glfwSwapInterval(1); // Enable vsync

    // --- INICIALIZACIÓN DE IMGUI ---
    IMGUI_CHECKVERSION();
    CreateContext();
    ImGui_ImplGlfw_InitForOpenGL(window, true);
    ImGui_ImplOpenGL3_Init("#version 130");
}

//@brief Limpieza de recursos
void WinMgr::cleanup(){
        // --- LIMPIEZA FINAL ---
    ImGui_ImplOpenGL3_Shutdown();
    ImGui_ImplGlfw_Shutdown();
    DestroyContext();
    glfwDestroyWindow(window);
    glfwTerminate();
}

// @brief Bucle principal de la ventana
void WinMgr::run(){
    while (!glfwWindowShouldClose(window)) {
        glfwPollEvents();

        // Iniciar frame de ImGui
        ImGui_ImplOpenGL3_NewFrame();
        ImGui_ImplGlfw_NewFrame();
        NewFrame();

        // LOGICA DE PRUEBA: Una ventana sencilla
        {
            Begin("Panel de Control");

            Text("Hola! Esta es una prueba de ImGui.");
            
            if (Button("Salir")) {
                glfwSetWindowShouldClose(window, true);
            }

            Button("Juan Andres");

            End();
        }

        // RENDERIZADO
        Render();
        int display_w, display_h;
        glfwGetFramebufferSize(window, &display_w, &display_h);
        glViewport(0, 0, display_w, display_h);
        glClear(GL_COLOR_BUFFER_BIT);
        ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());

        glfwSwapBuffers(window);
    }

    return;
}