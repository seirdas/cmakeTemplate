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

};
