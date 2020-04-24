#include "EngineLoop.h"
#include <Networking/InputQueue.h>
#include <Utils/Timer.h>
#include <tracy/Tracy.hpp>
#include "Utils/ServiceLocator.h"
#include "Network/MessageHandler.h"
#include "Rendering/ClientRenderer.h"

// Component Singletons
#include "ECS/Components/Singletons/TimeSingleton.h"
#include "ECS/Components/Singletons/ScriptSingleton.h"
#include "ECS/Components/Network/ConnectionSingleton.h"

// Components

// Systems
#include "ECS/Systems/Network/ConnectionSystems.h"
#include "ECS/Systems/UI/AddElementSystem.h"

// Handlers
#include "Network/Handlers/Client/GeneralHandlers.h"
#include "Scripting/ScriptHandler.h"

EngineLoop::EngineLoop() : _isRunning(false), _inputQueue(256), _outputQueue(256)
{
    _network.asioService = std::make_shared<asio::io_service>(2);
    _network.client = std::make_shared<NetworkClient>(new asio::ip::tcp::socket(*_network.asioService.get()));
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

    // Setup Network Lib
    InputQueue::SetInputQueue(&_inputQueue);

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

void EngineLoop::Run()
{
    _isRunning = true;

    _updateFramework.gameRegistry.create();
    _updateFramework.uiRegistry.create();
    SetupUpdateFramework();

    TimeSingleton& timeSingleton = _updateFramework.gameRegistry.set<TimeSingleton>();
    ScriptSingleton& scriptSingleton = _updateFramework.gameRegistry.set<ScriptSingleton>();
    ConnectionSingleton& connectionSingleton = _updateFramework.gameRegistry.set<ConnectionSingleton>();
    connectionSingleton.connection = _network.client;

    Timer timer;
    f32 targetDelta = 1.0f / 240.0f;

    _clientRenderer = new ClientRenderer();

    std::string scriptPath = "./Data/scripts";
    ScriptHandler::LoadScriptDirectory(scriptPath);

    _network.client->Connect("127.0.0.1", 3724);

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

    // ScriptSingletonTask
    tf::Task ScriptSingletonTask = framework.emplace([&uiRegistry, &gameRegistry]()
        {
            ZoneScopedNC("ScriptSingletonTask::Update", tracy::Color::Blue2)
            gameRegistry.ctx<ScriptSingleton>().ExecuteTransactions();
            gameRegistry.ctx<ScriptSingleton>().ResetCompletedSystems();
        });
    ScriptSingletonTask.gather(addElementSystemTask);
}
void EngineLoop::SetMessageHandler()
{
    auto messageHandler = new MessageHandler();
    ServiceLocator::SetNetworkMessageHandler(messageHandler);

    Client::GeneralHandlers::Setup(messageHandler);
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
