#include "EngineLoop.h"
#include <Networking/InputQueue.h>
#include <Utils/Timer.h>
#include <tracy/Tracy.hpp>
#include "Utils/ServiceLocator.h"
#include "Utils/EntityUtils.h"
#include <Networking/MessageHandler.h>
#include "Utils/MapLoader.h"
#include "Rendering/ClientRenderer.h"
#include "Rendering/UIRenderer.h"
#include "Rendering/Camera.h"
#include <Renderer/Renderer.h>
#include <SceneManager.h>

// Component Singletons
#include "ECS/Components/Singletons/MapSingleton.h"
#include "ECS/Components/Singletons/TimeSingleton.h"
#include "ECS/Components/Singletons/ScriptSingleton.h"
#include "ECS/Components/Network/ConnectionSingleton.h"
#include "ECS/Components/Network/AuthenticationSingleton.h"
#include "ECS/Components/LocalplayerSingleton.h"
#include "ECS/Components/UI/UIDataSingleton.h"

// Components
#include "ECS/Components/Rendering/Model.h"
#include "ECS/Components/Transform.h"

// Systems
#include "ECS/Systems/Network/ConnectionSystems.h"
#include "ECS/Systems/UI/AddElementSystem.h"
#include "ECS/Systems/Rendering/RenderModelSystem.h"
#include "ECS/Systems/MovementSystem.h"

// Handlers
#include "Network/Handlers/AuthSocket/AuthHandlers.h"
#include "Network/Handlers/GameSocket/GameHandlers.h"
#include "Scripting/ScriptHandler.h"

#include <InputManager.h>
#include <GLFW/glfw3.h>

EngineLoop::EngineLoop() : _isRunning(false), _inputQueue(256), _outputQueue(256)
{
    _network.asioService = std::make_shared<asio::io_service>(2);
    _network.authSocket = std::make_shared<NetworkClient>(new asio::ip::tcp::socket(*_network.asioService.get()));
    _network.gameSocket = std::make_shared<NetworkClient>(new asio::ip::tcp::socket(*_network.asioService.get()));
}

EngineLoop::~EngineLoop()
{
    delete _clientRenderer;
}

void EngineLoop::Start()
{
    if (_isRunning)
        return;

    ServiceLocator::SetMainInputQueue(&_inputQueue);

    std::thread threadRun = std::thread(&EngineLoop::Run, this);
    std::thread threadRunIoService = std::thread(&EngineLoop::RunIoService, this);
    threadRun.detach();
    threadRunIoService.detach();
}

void EngineLoop::Stop()
{
    if (!_isRunning)
        return;

    Message message;
    message.code = MSG_IN_EXIT;
    PassMessage(message);
}

void EngineLoop::PassMessage(Message& message)
{
    _inputQueue.enqueue(message);
}

bool EngineLoop::TryGetMessage(Message& message)
{
    return _outputQueue.try_dequeue(message);
}

void LoginScreenLoaded(u32 sceneNameHash)
{
    NC_LOG_SUCCESS("Scene: LoginScreen was loaded");
}

void EngineLoop::Run()
{
    _isRunning = true;

    SceneManager* sceneManager = new SceneManager();
    sceneManager->SetAvailableScenes({ "LoginScreen"_h, "CharacterSelection"_h, "CharacterCreation"_h });
    sceneManager->RegisterSceneLoadedCallback("LoginScreen"_h, SceneCallback("Dummy Callback"_h, LoginScreenLoaded));
    ServiceLocator::SetSceneManager(sceneManager);

    // We want to push this to the end of the first frame so anything that runs during the first frame has time to set itself up
    sceneManager->LoadScene("LoginScreen"_h);

    _updateFramework.gameRegistry.create();
    _updateFramework.uiRegistry.create();
    SetupUpdateFramework();

    MapLoader::Load(_updateFramework.gameRegistry);

    TimeSingleton& timeSingleton = _updateFramework.gameRegistry.set<TimeSingleton>();
    ScriptSingleton& scriptSingleton = _updateFramework.gameRegistry.set<ScriptSingleton>();
    ConnectionSingleton& connectionSingleton = _updateFramework.gameRegistry.set<ConnectionSingleton>();
    AuthenticationSingleton& authenticationSingleton = _updateFramework.gameRegistry.set<AuthenticationSingleton>();
    LocalplayerSingleton& localplayerSingleton = _updateFramework.gameRegistry.set<LocalplayerSingleton>();
    connectionSingleton.authConnection = _network.authSocket;
    connectionSingleton.gameConnection = _network.gameSocket;

    Timer timer;
    f32 targetDelta = 1.0f / 60.f;

    _clientRenderer = new ClientRenderer();

    // Bind Movement Keys
    InputManager* inputManager = ServiceLocator::GetInputManager();
    inputManager->RegisterKeybind("Move Forward", GLFW_KEY_W, KEYBIND_ACTION_PRESS, KEYBIND_MOD_NONE);
    inputManager->RegisterKeybind("Move Backward", GLFW_KEY_S, KEYBIND_ACTION_PRESS, KEYBIND_MOD_NONE);
    inputManager->RegisterKeybind("Move Left", GLFW_KEY_A, KEYBIND_ACTION_PRESS, KEYBIND_MOD_NONE);
    inputManager->RegisterKeybind("Move Right", GLFW_KEY_D, KEYBIND_ACTION_PRESS, KEYBIND_MOD_NONE);

    inputManager->RegisterMousePositionCallback("MouseLook - Player", [this](Window* window, f32 xPos, f32 yPos)
        {
            entt::registry* registry = ServiceLocator::GetGameRegistry();

            LocalplayerSingleton& localplayerSingleton = registry->ctx<LocalplayerSingleton>();
            if (localplayerSingleton.entity == entt::null)
                return;

            Camera* camera = ServiceLocator::GetCamera();
            if (camera->IsMouseCaptured())
            {
                ConnectionSingleton& connectionSingleton = registry->ctx<ConnectionSingleton>();
                Transform& transform = registry->get<Transform>(localplayerSingleton.entity);

                std::shared_ptr<Bytebuffer> buffer = Bytebuffer::Borrow<128>();
                buffer->Put(Opcode::MSG_MOVE_ENTITY);

                buffer->PutU16(32);

                vec3 position = camera->GetPosition();
                vec3 rotation = camera->GetRotation();

                buffer->Put(localplayerSingleton.entity);
                buffer->Put(transform.moveFlags);
                buffer->Put(position);
                buffer->Put(rotation);
                connectionSingleton.gameConnection->Send(buffer.get());

                transform.position = position;
                transform.rotation = rotation;
                transform.isDirty = true;
            }
        });

    std::string scriptPath = "./Data/scripts";
    ScriptHandler::LoadScriptDirectory(scriptPath);
    
    _network.authSocket->SetReadHandler(std::bind(&ConnectionUpdateSystem::AuthSocket_HandleRead, std::placeholders::_1));
    _network.authSocket->SetConnectHandler(std::bind(&ConnectionUpdateSystem::AuthSocket_HandleConnect, std::placeholders::_1, std::placeholders::_2));
    _network.authSocket->SetDisconnectHandler(std::bind(&ConnectionUpdateSystem::AuthSocket_HandleDisconnect, std::placeholders::_1));
    _network.authSocket->Connect("127.0.0.1", 3724); // Realmlist should contain these values
    
    _network.gameSocket->SetReadHandler(std::bind(&ConnectionUpdateSystem::GameSocket_HandleRead, std::placeholders::_1));
    _network.gameSocket->SetConnectHandler(std::bind(&ConnectionUpdateSystem::GameSocket_HandleConnect, std::placeholders::_1, std::placeholders::_2));
    _network.gameSocket->SetDisconnectHandler(std::bind(&ConnectionUpdateSystem::GameSocket_HandleDisconnect, std::placeholders::_1));

    while (true)
    {
        f32 deltaTime = timer.GetDeltaTime();
        timer.Tick();

        timeSingleton.lifeTimeInS = timer.GetLifeTime();
        timeSingleton.lifeTimeInMS = timeSingleton.lifeTimeInS * 1000;
        timeSingleton.deltaTime = deltaTime;

        if (!Update(deltaTime))
            break;

        Render();

        // Wait for tick rate, this might be an overkill implementation but it has the most even tickrate I've seen - MPursche
        for (deltaTime = timer.GetDeltaTime(); deltaTime < targetDelta - 0.0025f; deltaTime = timer.GetDeltaTime())
        {
            std::this_thread::sleep_for(std::chrono::milliseconds(1));
        }

        for (deltaTime = timer.GetDeltaTime(); deltaTime < targetDelta; deltaTime = timer.GetDeltaTime())
        {
            std::this_thread::yield();
        }
    }

    // Clean up stuff here

    Message exitMessage;
    exitMessage.code = MSG_OUT_EXIT_CONFIRM;
    _outputQueue.enqueue(exitMessage);
}
void EngineLoop::RunIoService()
{
    asio::io_service::work ioWork(*_network.asioService.get());
    _network.asioService->run();
}

bool EngineLoop::Update(f32 deltaTime)
{
    bool shouldExit = _clientRenderer->UpdateWindow(deltaTime) == false;
    if (shouldExit)
        return false;

    Message message;
    while (_inputQueue.try_dequeue(message))
    {
        if (message.code == -1)
            assert(false);

        if (message.code == MSG_IN_EXIT)
        {
            return false;
        }
        else if (message.code == MSG_IN_PRINT)
        {
            _outputQueue.enqueue(message);
        }
        else if (message.code == MSG_IN_PING)
        {
            Message pongMessage;
            pongMessage.code = MSG_OUT_PRINT;
            pongMessage.message = new std::string("PONG!");
            _outputQueue.enqueue(pongMessage);
        }
        else if (message.code == MSG_IN_RELOAD)
        {
            _clientRenderer->GetUIRenderer()->ClearWidgets();

            ScriptHandler::ReloadScripts();
        }
    }

    _clientRenderer->Update(deltaTime);

    UpdateSystems();
    return true;
}

void EngineLoop::Render()
{
    _clientRenderer->Render();
}

void EngineLoop::SetupUpdateFramework()
{
    tf::Framework& framework = _updateFramework.framework;
    entt::registry& gameRegistry = _updateFramework.gameRegistry;
    entt::registry& uiRegistry = _updateFramework.uiRegistry;

    ServiceLocator::SetGameRegistry(&gameRegistry);
    ServiceLocator::SetUIRegistry(&uiRegistry);
    SetMessageHandler();

    // ConnectionUpdateSystem
    tf::Task connectionUpdateSystemTask = framework.emplace([&gameRegistry]()
        {
            ZoneScopedNC("ConnectionUpdateSystem::Update", tracy::Color::Blue2)
                ConnectionUpdateSystem::Update(gameRegistry);
            gameRegistry.ctx<ScriptSingleton>().CompleteSystem();
        });

    // AddElementSystem
    tf::Task addElementSystemTask = framework.emplace([&uiRegistry, &gameRegistry]()
        {
            ZoneScopedNC("AddElementSystem::Update", tracy::Color::Blue2)
                AddElementSystem::Update(uiRegistry);
            gameRegistry.ctx<ScriptSingleton>().CompleteSystem();
        });
    addElementSystemTask.gather(connectionUpdateSystemTask);

    // MovementSystem
    tf::Task movementSystemTask = framework.emplace([&gameRegistry]()
        {
            ZoneScopedNC("MovementSystem::Update", tracy::Color::Blue2)
                MovementSystem::Update(gameRegistry);
            gameRegistry.ctx<ScriptSingleton>().CompleteSystem();
        });
    movementSystemTask.gather(connectionUpdateSystemTask);

    // RenderModelSystem
    tf::Task renderModelSystemTask = framework.emplace([this, &gameRegistry]()
        {
            ZoneScopedNC("RenderModelSystem::Update", tracy::Color::Blue2)
                RenderModelSystem::Update(gameRegistry, _clientRenderer);
            gameRegistry.ctx<ScriptSingleton>().CompleteSystem();
        });
    renderModelSystemTask.gather(movementSystemTask);

    // ScriptSingletonTask
    tf::Task scriptSingletonTask = framework.emplace([&uiRegistry, &gameRegistry]()
        {
            ZoneScopedNC("ScriptSingletonTask::Update", tracy::Color::Blue2)
            gameRegistry.ctx<ScriptSingleton>().ExecuteTransactions();
            gameRegistry.ctx<ScriptSingleton>().ResetCompletedSystems();
        });
    scriptSingletonTask.gather(addElementSystemTask);
    scriptSingletonTask.gather(renderModelSystemTask);
}
void EngineLoop::SetMessageHandler()
{
    // Setup Auth Message Handler
    MessageHandler* authSocketMessageHandler = new MessageHandler();
    AuthSocket::AuthHandlers::Setup(authSocketMessageHandler);
    ServiceLocator::SetAuthSocketMessageHandler(authSocketMessageHandler);

    // Setup Game Message Handler
    MessageHandler* gameSocketMessageHandler = new MessageHandler();
    ServiceLocator::SetGameSocketMessageHandler(gameSocketMessageHandler);
    GameSocket::GameHandlers::Setup(gameSocketMessageHandler);
}
void EngineLoop::UpdateSystems()
{
    ZoneScopedNC("UpdateSystems", tracy::Color::Blue2)
    {
        ZoneScopedNC("Taskflow::Run", tracy::Color::Blue2)
            _updateFramework.taskflow.run(_updateFramework.framework);
    }
    {
        ZoneScopedNC("Taskflow::WaitForAll", tracy::Color::Blue2)
            _updateFramework.taskflow.wait_for_all();
    }
}
