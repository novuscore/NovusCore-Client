#include "EngineLoop.h"
#include <Utils/Timer.h>
#include "Utils/ServiceLocator.h"
#include <SceneManager.h>
#include <Networking/InputQueue.h>
#include <Networking/MessageHandler.h>
#include <Renderer/Renderer.h>
#include "Rendering/ClientRenderer.h"
#include "Rendering/TerrainRenderer.h"
#include "Rendering/Camera.h"
#include "Gameplay/Map/MapLoader.h"
#include "Gameplay/DBC/DBCLoader.h"

// Component Singletons
#include "ECS/Components/Singletons/TimeSingleton.h"
#include "ECS/Components/Singletons/ScriptSingleton.h"
#include "ECS/Components/Singletons/DataStorageSingleton.h"
#include "ECS/Components/Singletons/SceneManagerSingleton.h"
#include "ECS/Components/Network/ConnectionSingleton.h"
#include "ECS/Components/Network/AuthenticationSingleton.h"
#include "ECS/Components/LocalplayerSingleton.h"

#include "UI/ECS/Components/Singletons/UIDataSingleton.h"

// Components
#include "ECS/Components/Transform.h"

// Systems
#include "ECS/Systems/Network/ConnectionSystems.h"
#include "UI/ECS/Systems/UpdateElementSystem.h"
#include "ECS/Systems/Rendering/RenderModelSystem.h"
#include "ECS/Systems/MovementSystem.h"

// Handlers
#include "Network/Handlers/AuthSocket/AuthHandlers.h"
#include "Network/Handlers/GameSocket/GameHandlers.h"
#include "Scripting/ScriptHandler.h"

#include <InputManager.h>
#include <GLFW/glfw3.h>
#include <tracy/Tracy.hpp>

#include "imgui/imgui.h"
#include "imgui/imgui_impl_vulkan.h"
#include "imgui/imgui_impl_glfw.h"
#include "imgui/misc/cpp/imgui_stdlib.h"

#include "ECS/Components/Singletons/StatsSingleton.h"

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

void EngineLoop::Run()
{
    _isRunning = true;

    _updateFramework.gameRegistry.create();
    _updateFramework.uiRegistry.create();
    SetupUpdateFramework();

    DBCLoader::Load(&_updateFramework.gameRegistry);
    MapLoader::Init(&_updateFramework.gameRegistry);

    TimeSingleton& timeSingleton = _updateFramework.gameRegistry.set<TimeSingleton>();
    ScriptSingleton& scriptSingleton = _updateFramework.gameRegistry.set<ScriptSingleton>();
    DataStorageSingleton& dataStorageSingleton = _updateFramework.gameRegistry.set<DataStorageSingleton>();
    SceneManagerSingleton& sceneManagerSingleton = _updateFramework.gameRegistry.set<SceneManagerSingleton>();
    ConnectionSingleton& connectionSingleton = _updateFramework.gameRegistry.set<ConnectionSingleton>();
    AuthenticationSingleton& authenticationSingleton = _updateFramework.gameRegistry.set<AuthenticationSingleton>();
    LocalplayerSingleton& localplayerSingleton = _updateFramework.gameRegistry.set<LocalplayerSingleton>();
    EngineStatsSingleton& statsSingleton = _updateFramework.gameRegistry.set<EngineStatsSingleton>();

    connectionSingleton.authConnection = _network.authSocket;
    connectionSingleton.gameConnection = _network.gameSocket;

    Timer timer;
    f32 targetDelta = 1.0f / 60.f;

    // Set up SceneManager. This has to happen before the ClientRenderer is created.
    SceneManager* sceneManager = new SceneManager();
    sceneManager->SetAvailableScenes({ "LoginScreen"_h, "CharacterSelection"_h, "CharacterCreation"_h });
    ServiceLocator::SetSceneManager(sceneManager);

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
                connectionSingleton.gameConnection->Send(buffer);

                transform.position = position;
                transform.rotation = rotation;
                transform.isDirty = true;
            }
        });

    // Load Scripts
    std::string scriptPath = "./Data/scripts";
    ScriptHandler::LoadScriptDirectory(scriptPath);

    sceneManager->LoadScene("LoginScreen"_h);

    _network.authSocket->SetReadHandler(std::bind(&ConnectionUpdateSystem::AuthSocket_HandleRead, std::placeholders::_1));
    _network.authSocket->SetConnectHandler(std::bind(&ConnectionUpdateSystem::AuthSocket_HandleConnect, std::placeholders::_1, std::placeholders::_2));
    _network.authSocket->SetDisconnectHandler(std::bind(&ConnectionUpdateSystem::AuthSocket_HandleDisconnect, std::placeholders::_1));
    
    _network.gameSocket->SetReadHandler(std::bind(&ConnectionUpdateSystem::GameSocket_HandleRead, std::placeholders::_1));
    _network.gameSocket->SetConnectHandler(std::bind(&ConnectionUpdateSystem::GameSocket_HandleConnect, std::placeholders::_1, std::placeholders::_2));
    _network.gameSocket->SetDisconnectHandler(std::bind(&ConnectionUpdateSystem::GameSocket_HandleDisconnect, std::placeholders::_1));

    Timer updateTimer;
    Timer renderTimer;

    EngineStatsSingleton::Frame timings;
    while (true)
    {
        f32 deltaTime = timer.GetDeltaTime();
        timer.Tick();

        timings.deltaTime = deltaTime;

        timeSingleton.lifeTimeInS = timer.GetLifeTime();
        timeSingleton.lifeTimeInMS = timeSingleton.lifeTimeInS * 1000;
        timeSingleton.deltaTime = deltaTime;

        ImGui_ImplVulkan_NewFrame();
        ImGui_ImplGlfw_NewFrame();
        ImGui::NewFrame();

        updateTimer.Reset();
        
        if (!Update(deltaTime))
            break;
        
        DrawEngineStats(&statsSingleton);
        DrawImguiMenuBar();

        timings.simulationFrameTime = updateTimer.GetLifeTime();
        
        renderTimer.Reset();
        
        Render();
        
        timings.renderFrameTime = renderTimer.GetLifeTime();
        
        statsSingleton.AddTimings(timings.deltaTime, timings.simulationFrameTime, timings.renderFrameTime);

        // Wait for tick rate, this might be an overkill implementation but it has the most even tickrate I've seen - MPursche
        for (deltaTime = timer.GetDeltaTime(); deltaTime < targetDelta - 0.0025f; deltaTime = timer.GetDeltaTime())
        {
            ZoneScopedNC("WaitForTickRate::Sleep", tracy::Color::AntiqueWhite1)
            std::this_thread::sleep_for(std::chrono::milliseconds(1));
        }

        for (deltaTime = timer.GetDeltaTime(); deltaTime < targetDelta; deltaTime = timer.GetDeltaTime())
        {
            ZoneScopedNC("WaitForTickRate::Yield", tracy::Color::AntiqueWhite1)
            std::this_thread::yield();
        }

        FrameMark;
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
            entt::registry* uiRegistry = ServiceLocator::GetUIRegistry();
            uiRegistry->ctx<UISingleton::UIDataSingleton>().ClearWidgets();

            ScriptHandler::ReloadScripts();
        }
        else if (message.code == MSG_IN_LOAD_MAP)
        {
            LoadMapInfo* loadMapInfo = reinterpret_cast<LoadMapInfo*>(message.object);

            ServiceLocator::GetClientRenderer()->GetTerrainRenderer()->LoadMap(loadMapInfo->mapInternalNameHash);
            ServiceLocator::GetCamera()->SetPosition(vec3(loadMapInfo->x, 100, loadMapInfo->y));

            delete loadMapInfo;
        }
    }

    _clientRenderer->Update(deltaTime);

    UpdateSystems();
    return true;
}

void EngineLoop::Render()
{
    ZoneScopedNC("EngineLoop::Render", tracy::Color::Red2)

    ImGui::Render();
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

    // UpdateElementSystem
    tf::Task updateElementSystemTask = framework.emplace([&uiRegistry, &gameRegistry]()
        {
            ZoneScopedNC("UpdateElementSystem::Update", tracy::Color::Gainsboro)
                UISystem::UpdateElementSystem::Update(uiRegistry);
            gameRegistry.ctx<ScriptSingleton>().CompleteSystem();
        });

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
    scriptSingletonTask.gather(updateElementSystemTask);
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

void EngineLoop::DrawEngineStats(EngineStatsSingleton* stats)
{
    ImGui::Begin("Engine Info");

    EngineStatsSingleton::Frame average = stats->AverageFrame(60);

    ImGui::Text("FPS : %f ", 1.f/  average.deltaTime);
    ImGui::Text("global frametime : %f ms", average.deltaTime * 1000);

    vec3 cameraLocation = ServiceLocator::GetCamera()->GetPosition();
    vec3 cameraRotation = ServiceLocator::GetCamera()->GetRotation();

    ImGui::Text("Camera Location : %f x, %f y, %f z", cameraLocation.x, cameraLocation.y, cameraLocation.z);
    ImGui::Text("Camera Rotation : %f x, %f y, %f z", cameraRotation.x, cameraRotation.y, cameraRotation.z);

    static bool advancedStats = false;
    ImGui::Checkbox("Advanced Stats", &advancedStats);

    if(advancedStats)
    {
        ImGui::Text("update time : %f ms", average.simulationFrameTime * 1000);
        ImGui::Text("render time (CPU): %f ms", average.renderFrameTime * 1000);

        //read the frame buffer to gather timings for the histograms
        std::vector<float> updateTimes;
        updateTimes.reserve(stats->frameStats.size());

        std::vector<float> renderTimes;
        renderTimes.reserve(stats->frameStats.size());

        for (int i = 0; i < stats->frameStats.size(); i++)
        {
            updateTimes.push_back(stats->frameStats[i].simulationFrameTime * 1000);
            renderTimes.push_back(stats->frameStats[i].renderFrameTime * 1000);
        }

        ImGui::PlotHistogram("Update Times", updateTimes.data(), (int)updateTimes.size());
        ImGui::PlotHistogram("Render Times", renderTimes.data(), (int)renderTimes.size());
    }

    ImGui::End();
}


void EngineLoop::DrawImguiMenuBar()
{
    if (ImGui::BeginMainMenuBar())
    {
        if (ImGui::BeginMenu("File"))
        {
            if (ImGui::BeginMenu("Load Map", "CTRL+Z")) 
            {
                static std::string mapload = "Azeroth";

                ImGui::InputText("Map to load", &mapload);

                if (ImGui::Button("Load Map"))
                {
                    u32 namehash = StringUtils::fnv1a_32(mapload.data(), mapload.size());
                    ServiceLocator::GetClientRenderer()->GetTerrainRenderer()->LoadMap(namehash);
                }
                ImGui::EndMenu();
            }
           
            ImGui::EndMenu();
        }
        //mockup
        if (ImGui::BeginMenu("Edit"))
        {
            if (ImGui::MenuItem("Undo", "CTRL+Z")) {}
            if (ImGui::MenuItem("Redo", "CTRL+Y", false, false)) {}
            ImGui::Separator();
            if (ImGui::MenuItem("Cut", "CTRL+X")) {}
            if (ImGui::MenuItem("Copy", "CTRL+C")) {}
            if (ImGui::MenuItem("Paste", "CTRL+V")) {}
            ImGui::EndMenu();
        }
        ImGui::EndMainMenuBar();
    }
}

void EngineLoop::UpdateSystems()
{
    ZoneScopedNC("UpdateSystems", tracy::Color::DarkBlue)
    {
        ZoneScopedNC("Taskflow::Run", tracy::Color::DarkBlue)
            _updateFramework.taskflow.run(_updateFramework.framework);
    }
    {
        ZoneScopedNC("Taskflow::WaitForAll", tracy::Color::DarkBlue)
            _updateFramework.taskflow.wait_for_all();
    }
}
