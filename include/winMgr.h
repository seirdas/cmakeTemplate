#pragma once

#include "imgui.h"
#include "imgui_impl_glfw.h"
#include "imgui_impl_opengl3.h"
#include <GLFW/glfw3.h>

using namespace ImGui;

// Gestión de ventanas
class WinMgr{
public:
    WinMgr();
    ~WinMgr();

    void init();
    void run();
    void cleanup();


private:
    GLFWwindow* window;

};