#pragma once

struct GLFWwindow;

class WinMgr
{
public:
    bool init();
    void frame();
    void close();

    bool isRunning() const;

private:
    GLFWwindow* window = nullptr;

    void renderizar();
    void DibujarSidebar();
    void StartFrame();

// Themes _______________________________-
private:
    void Style_Confy();
    void Style_FutureDark();
    void Style_Moonlight();
    void Style_VisualStudio();

};
