#pragma once
#include <Utils/ConcurrentQueue.h>
#include <Utils/Message.h>
#include <entt.hpp>
#include <cassert>

class MessageHandler;
class Window;
class InputManager;
namespace Renderer
{
    class Renderer;
}
class ServiceLocator
{
public:
    static entt::registry* GetGameRegistry() { return _gameRegistry; }
    static void SetGameRegistry(entt::registry* registry);
    static entt::registry* GetUIRegistry() { return _uiRegistry; }
    static void SetUIRegistry(entt::registry* registry);
    static MessageHandler* GetNetworkMessageHandler() { return _networkMessageHandler; }
    static void SetNetworkMessageHandler(MessageHandler* serverMessageHandler);
    static Window* GetWindow() { return _window; }
    static void SetWindow(Window* window);
    static InputManager* GetInputManager() { return _inputManager; }
    static void SetInputManager(InputManager* inputManager);
    static moodycamel::ConcurrentQueue<Message>* GetMainInputQueue() 
    {
        assert(_mainInputQueue != nullptr);
        return _mainInputQueue; 
    }
    static void SetMainInputQueue(moodycamel::ConcurrentQueue<Message>* mainInputQueue)
    {
        assert(_mainInputQueue == nullptr);
        _mainInputQueue = mainInputQueue;
    }
    static Renderer::Renderer* GetRenderer() 
    {
        assert(_renderer != nullptr);
        return _renderer;
    }
    static void SetRenderer(Renderer::Renderer* renderer);

private:
    ServiceLocator() { }
    static entt::registry* _gameRegistry;
    static entt::registry* _uiRegistry;
    static MessageHandler* _networkMessageHandler;
    static Window* _window;
    static InputManager* _inputManager;
    static moodycamel::ConcurrentQueue<Message>* _mainInputQueue;
    static Renderer::Renderer* _renderer;
};