#include "ServiceLocator.h"

entt::registry* ServiceLocator::_gameRegistry = nullptr;
entt::registry* ServiceLocator::_uiRegistry = nullptr;
MessageHandler* ServiceLocator::_networkMessageHandler = nullptr;
Window* ServiceLocator::_window = nullptr;
InputManager* ServiceLocator::_inputManager = nullptr;
Camera* ServiceLocator::_camera = nullptr;
Renderer::Renderer* ServiceLocator::_renderer = nullptr;
SceneManager* ServiceLocator::_sceneManager = nullptr;

moodycamel::ConcurrentQueue<Message>* ServiceLocator::_mainInputQueue = nullptr;

void ServiceLocator::SetGameRegistry(entt::registry* registry)
{
    assert(_gameRegistry == nullptr);
    _gameRegistry = registry;
}
void ServiceLocator::SetUIRegistry(entt::registry* registry)
{
    assert(_uiRegistry == nullptr);
    _uiRegistry = registry;
}
void ServiceLocator::SetNetworkMessageHandler(MessageHandler* networkMessageHandler)
{
    assert(_networkMessageHandler == nullptr);
    _networkMessageHandler = networkMessageHandler;
}
void ServiceLocator::SetWindow(Window* window)
{
    assert(_window == nullptr);
    _window = window;
}

void ServiceLocator::SetInputManager(InputManager* inputManager)
{
    assert(_inputManager == nullptr);
    _inputManager = inputManager;
}

void ServiceLocator::SetCamera(Camera* camera)
{
    assert(_camera == nullptr);
    _camera = camera;
}

void ServiceLocator::SetMainInputQueue(moodycamel::ConcurrentQueue<Message>* mainInputQueue)
{
    assert(_mainInputQueue == nullptr);
    _mainInputQueue = mainInputQueue;
}

void ServiceLocator::SetRenderer(Renderer::Renderer* renderer)
{
    assert(_renderer == nullptr);
    _renderer = renderer;
}

void ServiceLocator::SetSceneManager(SceneManager* sceneManager)
{
    assert(_sceneManager == nullptr);
    _sceneManager = sceneManager;
}
