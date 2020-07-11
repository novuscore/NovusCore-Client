#pragma once
#include <NovusTypes.h>
#include "../Renderer/Swapchain.h"

struct GLFWwindow;
class Window
{
public:
    Window();
    ~Window();

    bool Init(u32 width, u32 height);

    bool Update(f32 deltaTime);
    void Present();

    GLFWwindow* GetWindow() { return _window; }

    // The types of some of these things will depend on the rendering backend, we store them as void pointers and let the backend keep track of what it is
    void SetSwapChain(Renderer::SwapChain* swapChain) { _swapChain = swapChain; }
    Renderer::SwapChain* GetSwapChain() { return _swapChain; }

    bool IsMinimized() { return _isMinimized; }
    void SetIsMinimized(bool isMinimized) { _isMinimized = isMinimized; }
private:

private:
    GLFWwindow* _window;
    Renderer::SwapChain* _swapChain;

    bool _isMinimized = false;

    static bool _glfwInitialized;
};