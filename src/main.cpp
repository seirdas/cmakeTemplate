#include <iostream>
#include <thread>
#include <chrono>
#include "functions.h"
#include "nlohmann/json.hpp"
#include <fstream>
#include "defines.h"

#include "imgui.h"
#include "imgui_impl_glfw.h"
#include "imgui_impl_opengl3.h"
#include <GLFW/glfw3.h>

#include <filesystem>

using json = nlohmann::json;
namespace fs = std::filesystem;


void test_imgui(){
    // --- INICIALIZACIÓN DE GLFW & OPENGL ---
    if (!glfwInit()) return;
    GLFWwindow* window = glfwCreateWindow(1280, 720, "Prueba ImGui", NULL, NULL);
    glfwMakeContextCurrent(window);
    glfwSwapInterval(1); // Enable vsync

    // --- INICIALIZACIÓN DE IMGUI ---
    IMGUI_CHECKVERSION();
    ImGui::CreateContext();
    ImGui_ImplGlfw_InitForOpenGL(window, true);
    ImGui_ImplOpenGL3_Init("#version 130");

    // Estado de nuestra app
    float color_fondo[3] = {0.45f, 0.55f, 0.60f};

    // --- BUCLE PRINCIPAL ---
    while (!glfwWindowShouldClose(window)) {
        glfwPollEvents();

        // Iniciar frame de ImGui
        ImGui_ImplOpenGL3_NewFrame();
        ImGui_ImplGlfw_NewFrame();
        ImGui::NewFrame();

        // LOGICA DE PRUEBA: Una ventana sencilla
        {
            ImGui::Begin("Panel de Control");
            
            ImGui::Text("Hola! Esta es una prueba de ImGui.");
            
            if (ImGui::Button("Limpiar Terminal")) {
                #ifdef _WIN32
                    std::system("cls");
                #else
                    std::system("clear");
                #endif
                std::cout << "Terminal limpiada desde la GUI!" << std::endl;
            }

            ImGui::ColorEdit3("Color de Fondo", color_fondo);
            
            if (ImGui::Button("Salir")) {
                glfwSetWindowShouldClose(window, true);
            }

            ImGui::End();
        }

        // RENDERIZADO
        ImGui::Render();
        int display_w, display_h;
        glfwGetFramebufferSize(window, &display_w, &display_h);
        glViewport(0, 0, display_w, display_h);
        glClearColor(color_fondo[0], color_fondo[1], color_fondo[2], 1.0f);
        glClear(GL_COLOR_BUFFER_BIT);
        ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());

        glfwSwapBuffers(window);
    }

    // --- LIMPIEZA FINAL ---
    ImGui_ImplOpenGL3_Shutdown();
    ImGui_ImplGlfw_Shutdown();
    ImGui::DestroyContext();
    glfwDestroyWindow(window);
    glfwTerminate();

    return;
}

int main(int argc, char** argv){


    test_imgui();



    #ifdef _WIN32
        if (AttachConsole(ATTACH_PARENT_PROCESS)) {
            freopen("CONOUT$", "w", stdout);
            freopen("CONOUT$", "w", stderr);
            // Código ANSI para limpiar pantalla y resetear cursor
            std::cout << "\033[2J\033[1;1H";
        }
    #endif

    // Asegurar directorio del exe (para entorno de desarrollo)
    fs::current_path(fs::absolute(argv[0]).parent_path());
        
    std::cout << std::endl;
    std::cout << fs::current_path() << argc << std::endl;
    std::cout << "Hello" << myFunc() << std::endl;

    int value = 45;

    json nuevojson;

    nuevojson["usuario"] = "secrespo";
    nuevojson["id"] = 1024;
    nuevojson["activo"] = true;

    std::ofstream archivo("./config.json");
    if (archivo.is_open()) {
        archivo << nuevojson.dump(4);
        archivo.close();
        std::cout << "¡JSON creado con éxito!" << std::endl;
    } else {
        std::cerr << "No se pudo crear el archivo." << std::endl;
    }


    int value2 = value + 92;
    int value3 = value2 -14;
    std::cout << "Value2: " << value3 << std::endl;

    // Hilo que espera 5 segundos, muestra un mensaje y termina
    std::thread worker([]() {
        std::this_thread::sleep_for(std::chrono::seconds(3));
        std::cout << "Mensaje: el hilo ha esperado 3 segundos y termina ahora." << std::endl;
    });
    
    // Esperar a que el hilo termine antes de salir
    worker.join();

    return value;
}