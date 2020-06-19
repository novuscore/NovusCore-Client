#pragma once
#include <Utils/ConcurrentQueue.h>
#include <Utils/Message.h>
#include <entt.hpp>
#include <cassert>

class MessageHandler;
class Window;
class InputManager;
class Camera;
class SceneManager;
namespace Renderer
{
    class Renderer;
}
class ServiceLocator
{
public:
    static entt::registry* GetGameRegistry() 
    {
        assert(_gameRegistry != nullptr);
        return _gameRegistry; 
    }
    static void SetGameRegistry(entt::registry* registry);
    static entt::registry* GetUIRegistry() 
    {
        assert(_uiRegistry != nullptr);
        return _uiRegistry; 
    }
    static void SetUIRegistry(entt::registry* registry);
    static MessageHandler* GetNetworkMessageHandler() 
    {
        assert(_networkMessageHandler != nullptr);
        return _networkMessageHandler; 
    }
    static void SetNetworkMessageHandler(MessageHandler* serverMessageHandler);
    static Window* GetWindow() 
    {
        assert(_window != nullptr);
        return _window; 
    }
    static void SetWindow(Window* window);
    static InputManager* GetInputManager() 
    {
        assert(_inputManager != nullptr);
        return _inputManager; 
    }
    static void SetInputManager(InputManager* inputManager);
    static Camera* GetCamera() 
    {
        assert(_camera != nullptr);
        return _camera; 
    }
    static void SetCamera(Camera* camera);
    static moodycamel::ConcurrentQueue<Message>* GetMainInputQueue() 
    {
        assert(_mainInputQueue != nullptr);
        return _mainInputQueue; 
    }
    static void SetMainInputQueue(moodycamel::ConcurrentQueue<Message>* mainInputQueue);
    static Renderer::Renderer* GetRenderer() 
    {
        assert(_renderer != nullptr);
        return _renderer;
    }
    static void SetRenderer(Renderer::Renderer* renderer);
    static SceneManager* GetSceneManager()
    {
        assert(_sceneManager != nullptr);
        return _sceneManager;
    }
    static void SetSceneManager(SceneManager* sceneManager);

private:
    ServiceLocator() { }
    static entt::registry* _gameRegistry;
    static entt::registry* _uiRegistry;
    static MessageHandler* _networkMessageHandler;
    static Window* _window;
    static InputManager* _inputManager;
    static Camera* _camera;
    static moodycamel::ConcurrentQueue<Message>* _mainInputQueue;
    static Renderer::Renderer* _renderer;
    static SceneManager* _sceneManager;
};