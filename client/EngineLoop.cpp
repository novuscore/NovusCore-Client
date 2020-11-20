#include "EngineLoop.h"
#include <Utils/Timer.h>
#include "Utils/ServiceLocator.h"
#include "Utils/EntityUtils.h"
#include "Utils/MapUtils.h"
#include <SceneManager.h>
#include <Networking/InputQueue.h>
#include <Networking/MessageHandler.h>
#include <Renderer/Renderer.h>
#include <Memory/MemoryTracker.h>
#include "Rendering/ClientRenderer.h"
#include "Rendering/TerrainRenderer.h"
#include "Rendering/MapObjectRenderer.h"
#include "Rendering/CModelRenderer.h"
#include "Rendering/CameraFreelook.h"
#include "Rendering/CameraOrbital.h"
#include "Editor/Editor.h"
#include "Loaders/Config/ConfigLoader.h"
#include "Loaders/Texture/TextureLoader.h"
#include "Loaders/Map/MapLoader.h"
#include "Loaders/NDBC/NDBCLoader.h"
#include "Loaders/DisplayInfo/DisplayInfoLoader.h"

// Component Singletons
#include "ECS/Components/Singletons/TimeSingleton.h"
#include "ECS/Components/Singletons/StatsSingleton.h"
#include "ECS/Components/Singletons/ScriptSingleton.h"
#include "ECS/Components/Singletons/DataStorageSingleton.h"
#include "ECS/Components/Singletons/SceneManagerSingleton.h"
#include "ECS/Components/Singletons/LocalplayerSingleton.h"
#include "ECS/Components/Singletons/ConfigSingleton.h"
#include "ECS/Components/Network/ConnectionSingleton.h"
#include "ECS/Components/Network/AuthenticationSingleton.h"

// Components
#include "ECS/Components/Transform.h"
#include "ECS/Components/Physics/Rigidbody.h"
#include "ECS/Components/Rendering/DebugBox.h"

#include "UI/ECS/Components/Transform.h"
#include "UI/ECS/Components/NotCulled.h"

// Systems
#include "ECS/Systems/Network/ConnectionSystems.h"
#include "ECS/Systems/Rendering/RenderModelSystem.h"
#include "ECS/Systems/Physics/SimulateDebugCubeSystem.h"
#include "ECS/Systems/MovementSystem.h"
#include "ECS/Systems/AreaUpdateSystem.h"
#include "ECS/Systems/DayNightSystem.h"

#include "UI/ECS/Systems/ElementDeleteSystem.h"
#include "UI/ECS/Systems/UpdateElementSystem.h"
#include "UI/ECS/Systems/ElementBoundsSystem.h"
#include "UI/ECS/Systems/ElementCullingSystem.h"
#include "UI/ECS/Systems/ElementSortingSystem.h"
#include "UI/ECS/Systems/ElementFinalSystem.h"

// Utils
#include "UI/Utils/ElementUtils.h"

// Handlers
#include "Network/Handlers/AuthSocket/AuthHandlers.h"
#include "Network/Handlers/GameSocket/GameHandlers.h"
#include "Scripting/ScriptHandler.h"

#include <InputManager.h>
#include <GLFW/glfw3.h>
#include <tracy/Tracy.hpp>

#include "imgui/imgui.h"
#include "imgui/imgui_internal.h"
#include "imgui/imgui_impl_vulkan.h"
#include "imgui/imgui_impl_glfw.h"
#include "imgui/misc/cpp/imgui_stdlib.h"

#include "imgui/implot.h"

#include "CVar/CVarSystem.h"

AutoCVar_Int CVAR_FramerateLock("framerate.lock", "enable locking framerate", 1, CVarFlags::EditCheckbox);
AutoCVar_Int CVAR_FramerateTarget("framerate.target", "target framerate", 60);

EngineLoop::EngineLoop() : _isRunning(false), _inputQueue(256), _outputQueue(256)
{
    _network.asioService = std::make_shared<asio::io_service>(2);
    _network.authSocket = std::make_shared<NetworkClient>(new asio::ip::tcp::socket(*_network.asioService.get()));
    _network.gameSocket = std::make_shared<NetworkClient>(new asio::ip::tcp::socket(*_network.asioService.get()));
}

EngineLoop::~EngineLoop()
{
    delete _clientRenderer;
    delete _editor;
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

void EngineLoop::Abort()
{
    Message exitMessage;
    exitMessage.code = MSG_OUT_EXIT_CONFIRM;
    _outputQueue.enqueue(exitMessage);
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

    bool failedToLoad = false;
    failedToLoad |= !ConfigLoader::Init(&_updateFramework.gameRegistry);
    failedToLoad |= !TextureLoader::Load(&_updateFramework.gameRegistry);
    failedToLoad |= !NDBCLoader::Load(&_updateFramework.gameRegistry);
    failedToLoad |= !DisplayInfoLoader::Init(&_updateFramework.gameRegistry);
    failedToLoad |= !MapLoader::Init(&_updateFramework.gameRegistry);

    if (failedToLoad)
    {
        Abort();
        return;
    }

    DayNightSystem::Init(_updateFramework.gameRegistry);
    AreaUpdateSystem::Init(_updateFramework.gameRegistry);

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

    // Set up SceneManager. This has to happen before the ClientRenderer is created.
    SceneManager* sceneManager = new SceneManager();
    sceneManager->SetAvailableScenes({ "LoginScreen"_h, "CharacterSelection"_h, "CharacterCreation"_h });
    ServiceLocator::SetSceneManager(sceneManager);

    _clientRenderer = new ClientRenderer();
    _editor = new Editor::Editor();

    CameraFreeLook* cameraFreeLook = new CameraFreeLook(vec3(0.0f, 0.0f, 0.0f)); // Center of Map (0, 0)
    cameraFreeLook->Init();
    ServiceLocator::SetCameraFreeLook(cameraFreeLook);

    CameraOrbital* cameraOrbital = new CameraOrbital();
    cameraOrbital->Init();
    ServiceLocator::SetCameraOrbital(cameraOrbital);

    ConfigSingleton& configSingleton = _updateFramework.gameRegistry.ctx<ConfigSingleton>();

    // Check Startup Settings for UI
    UIConfig& uiConfig = configSingleton.uiConfig;
    {
        const std::string& defaultMap = uiConfig.GetDefaultMap();

        if (defaultMap.length() != 0)
        {
            MapSingleton& mapSingleton = _updateFramework.gameRegistry.ctx<MapSingleton>();

            u32 defaultMapHash = StringUtils::fnv1a_32(defaultMap.c_str(), defaultMap.length());
            auto itr = mapSingleton.mapNameToDBC.find(defaultMapHash);
            if (itr != mapSingleton.mapNameToDBC.end())
            {
                const NDBC::Map* map = itr->second;

                cameraFreeLook->LoadFromFile("freelook.cameradata");
                _clientRenderer->GetTerrainRenderer()->LoadMap(map);
            }
        }
    }

    // Bind Movement Keys
    InputManager* inputManager = ServiceLocator::GetInputManager();
    inputManager->RegisterKeybind("Switch Camera Mode", GLFW_KEY_C, KEYBIND_ACTION_PRESS, KEYBIND_MOD_NONE, [this](Window* window, std::shared_ptr<Keybind> keybind)
    {
        CameraFreeLook* freeLook = ServiceLocator::GetCameraFreeLook();
        CameraOrbital* orbital = ServiceLocator::GetCameraOrbital();

        if (freeLook->IsActive())
        {
            freeLook->SetActive(false);
            freeLook->Disabled();

            orbital->SetActive(true);
            orbital->Enabled();
        }
        else if (orbital->IsActive())
        {
            orbital->SetActive(false);
            orbital->Disabled();

            freeLook->SetActive(true);
            freeLook->Enabled();
        }

        return true;
    });
    inputManager->RegisterKeybind("Move Forward", GLFW_KEY_W, KEYBIND_ACTION_PRESS, KEYBIND_MOD_NONE);
    inputManager->RegisterKeybind("Move Backward", GLFW_KEY_S, KEYBIND_ACTION_PRESS, KEYBIND_MOD_NONE);
    inputManager->RegisterKeybind("Move Left", GLFW_KEY_A, KEYBIND_ACTION_PRESS, KEYBIND_MOD_NONE);
    inputManager->RegisterKeybind("Move Right", GLFW_KEY_D, KEYBIND_ACTION_PRESS, KEYBIND_MOD_NONE);
    inputManager->RegisterKeybind("Move Jump", GLFW_KEY_SPACE, KEYBIND_ACTION_PRESS, KEYBIND_MOD_NONE);

    // Initialize Localplayer
    localplayerSingleton.entity = _updateFramework.gameRegistry.create();
    Transform& transform = _updateFramework.gameRegistry.emplace<Transform>(localplayerSingleton.entity);

    transform.position = vec3(-9321.f, 108.11f, 50.f);
    transform.scale = vec3(0.5f, 2.f, 0.5f); // "Ish" scale for humans

    _updateFramework.gameRegistry.emplace<DebugBox>(localplayerSingleton.entity);
    //Model& model = EntityUtils::CreateModelComponent(_updateFramework.gameRegistry, localplayerSingleton.entity, "Data/models/Cube.novusmodel");

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

    MovementSystem::Init(_updateFramework.gameRegistry);
    SimulateDebugCubeSystem::Init(_updateFramework.gameRegistry);

    Timer timer;
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

        bool lockFrameRate = CVAR_FramerateLock.Get() == 1;
        if (lockFrameRate)
        {
            f32 targetFramerate = static_cast<f32>(CVAR_FramerateTarget.Get());
            targetFramerate = Math::Max(targetFramerate, 10.0f);
            f32 targetDelta = 1.0f / targetFramerate;

            // Wait for tick rate, this might be an overkill implementation but it has the most even tickrate I've seen - MPursche
            /*for (deltaTime = timer.GetDeltaTime(); deltaTime < targetDelta - 0.0025f; deltaTime = timer.GetDeltaTime())
            {
                ZoneScopedNC("WaitForTickRate::Sleep", tracy::Color::AntiqueWhite1)
                std::this_thread::sleep_for(std::chrono::milliseconds(1));
            }*/

            for (deltaTime = timer.GetDeltaTime(); deltaTime < targetDelta; deltaTime = timer.GetDeltaTime())
            {
                ZoneScopedNC("WaitForTickRate::Yield", tracy::Color::AntiqueWhite1)
                    std::this_thread::yield();
            }
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

    ImguiNewFrame();

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
            UIUtils::ClearAllElements();

            ScriptHandler::ReloadScripts();

            // Resend "LoadScene" to trigger UI events
            SceneManager* sceneManager = ServiceLocator::GetSceneManager();
            sceneManager->LoadScene(sceneManager->GetScene());
        }
    }

    // Update Systems will modify the Camera, so we wait with updating the Camera 
    // until we are sure it is static for the rest of the frame
    UpdateSystems();

    Camera* camera = ServiceLocator::GetCamera();
    camera->Update(deltaTime, 75.0f, static_cast<f32>(_clientRenderer->WIDTH) / static_cast<f32>(_clientRenderer->HEIGHT));

    i32* editorEnabledCVAR = CVarSystem::Get()->GetIntCVar("editor.Enable"_h);
    if (*editorEnabledCVAR)
    {
        _editor->Update(deltaTime);
    }

    _clientRenderer->Update(deltaTime);

    ConfigSingleton& configSingleton = _updateFramework.gameRegistry.ctx<ConfigSingleton>();
    {
        if (CVarSystem::Get()->IsDirty())
        {
            ConfigLoader::Save(ConfigSaveType::CVAR);
            CVarSystem::Get()->ClearDirty();
        }

        if (configSingleton.uiConfig.IsDirty())
        {
            ConfigLoader::Save(ConfigSaveType::UI);
            configSingleton.uiConfig.ClearDirty();
        }
    }
    
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

    // ElementDeleteSystem
    tf::Task elementDeleteSystemTask = framework.emplace([&uiRegistry, &gameRegistry]()
        {
            ZoneScopedNC("ElementDeleteSystem::Update", tracy::Color::Gainsboro)
                UISystem::ElementDeleteSystem::Update(uiRegistry);
            gameRegistry.ctx<ScriptSingleton>().CompleteSystem();
        });

    // UpdateElementSystem
    tf::Task updateElementSystemTask = framework.emplace([&uiRegistry, &gameRegistry]()
    {
        ZoneScopedNC("UpdateElementSystem::Update", tracy::Color::Gainsboro)
            UISystem::UpdateElementSystem::Update(uiRegistry);
        gameRegistry.ctx<ScriptSingleton>().CompleteSystem();
    });
    updateElementSystemTask.gather(elementDeleteSystemTask);

    // ElementBoundsSystem
    tf::Task elementBoundsSystemTask = framework.emplace([&uiRegistry, &gameRegistry]()
        {
            ZoneScopedNC("ElementBoundsSystem::Update", tracy::Color::Gainsboro)
                UISystem::ElementBoundsSystem::Update(uiRegistry);
            gameRegistry.ctx<ScriptSingleton>().CompleteSystem();
        });
    updateElementSystemTask.gather(elementDeleteSystemTask);

    // ElementCullingSystem
    tf::Task elementCullingSystemTask = framework.emplace([&uiRegistry, &gameRegistry]()
        {
            ZoneScopedNC("ElementCullingSystem::Update", tracy::Color::Gainsboro)
                UISystem::ElementCullingSystem::Update(uiRegistry);
            gameRegistry.ctx<ScriptSingleton>().CompleteSystem();
        });
    updateElementSystemTask.gather(elementDeleteSystemTask);
    
    // UpdateElementSystem
    tf::Task elementSortingSystemTask = framework.emplace([&uiRegistry, &gameRegistry]()
        {
            ZoneScopedNC("ElementSortingSystem::Update", tracy::Color::Gainsboro)
                UISystem::ElementSortingSystem::Update(uiRegistry);
            gameRegistry.ctx<ScriptSingleton>().CompleteSystem();
        });
    updateElementSystemTask.gather(elementDeleteSystemTask);

    // ElementFinalSystem
    tf::Task elementFinalSystemTask = framework.emplace([&uiRegistry, &gameRegistry]()
        {
            ZoneScopedNC("UpdateElementSystem::Update", tracy::Color::Gainsboro)
                UISystem::ElementFinalSystem::Update(uiRegistry);
            gameRegistry.ctx<ScriptSingleton>().CompleteSystem();
        });
    elementFinalSystemTask.gather(updateElementSystemTask);
    elementFinalSystemTask.gather(elementCullingSystemTask);
    elementFinalSystemTask.gather(elementSortingSystemTask);

    // MovementSystem
    tf::Task movementSystemTask = framework.emplace([&gameRegistry]()
    {
        ZoneScopedNC("MovementSystem::Update", tracy::Color::Blue2)
            MovementSystem::Update(gameRegistry);
        gameRegistry.ctx<ScriptSingleton>().CompleteSystem();
    });
    movementSystemTask.gather(connectionUpdateSystemTask);

    // DayNightSystem
    tf::Task dayNightSystemTask = framework.emplace([&gameRegistry]()
    {
        ZoneScopedNC("DayNightSystem::Update", tracy::Color::Blue2)
        DayNightSystem::Update(gameRegistry);
        gameRegistry.ctx<ScriptSingleton>().CompleteSystem();
    });
    dayNightSystemTask.gather(movementSystemTask);

    // AreaUpdateSystem
    tf::Task areaUpdateSystemTask = framework.emplace([&gameRegistry]()
    {
        ZoneScopedNC("AreaUpdateSystem::Update", tracy::Color::Blue2)
        AreaUpdateSystem::Update(gameRegistry);
        gameRegistry.ctx<ScriptSingleton>().CompleteSystem();
    });
    areaUpdateSystemTask.gather(dayNightSystemTask);

    // SimulateDebugCubeSystem
    tf::Task simulateDebugCubeSystemTask = framework.emplace([this, &gameRegistry]()
    {
        ZoneScopedNC("SimulateDebugCubeSystem::Update", tracy::Color::Blue2)
            SimulateDebugCubeSystem::Update(gameRegistry, _clientRenderer->GetDebugRenderer());
        gameRegistry.ctx<ScriptSingleton>().CompleteSystem();
    });
    simulateDebugCubeSystemTask.gather(areaUpdateSystemTask);

    // RenderModelSystem
    tf::Task renderModelSystemTask = framework.emplace([this, &gameRegistry]()
    {
        ZoneScopedNC("RenderModelSystem::Update", tracy::Color::Blue2)
            RenderModelSystem::Update(gameRegistry, _clientRenderer);
        gameRegistry.ctx<ScriptSingleton>().CompleteSystem();
    });
    renderModelSystemTask.gather(simulateDebugCubeSystemTask);

    // ScriptSingletonTask
    tf::Task scriptSingletonTask = framework.emplace([&uiRegistry, &gameRegistry]()
    {
        ZoneScopedNC("ScriptSingletonTask::Update", tracy::Color::Blue2)
        gameRegistry.ctx<ScriptSingleton>().ExecuteTransactions();
        gameRegistry.ctx<ScriptSingleton>().ResetCompletedSystems();
    });
    scriptSingletonTask.gather(elementFinalSystemTask);
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

void EngineLoop::ImguiNewFrame()
{
    ImGui_ImplVulkan_NewFrame();
    ImGui_ImplGlfw_NewFrame();
    ImGui::NewFrame();
}

void EngineLoop::DrawEngineStats(EngineStatsSingleton* stats)
{
    if (ImGui::Begin("Engine Info"))
    {
        EngineStatsSingleton::Frame average = stats->AverageFrame(240);

        ImGui::Text("FPS : %f ", 1.f / average.deltaTime);
        ImGui::Text("global frametime : %f ms", average.deltaTime * 1000);

        if (ImGui::BeginTabBar("Information"))
        {
            if (ImGui::BeginTabItem("Map"))
            {
                ImGui::Spacing();
                DrawMapStats();
                ImGui::EndTabItem();
            }

            if (ImGui::BeginTabItem("Position"))
            {
                ImGui::Spacing();
                DrawPositionStats();
                ImGui::EndTabItem();
            }

            if (ImGui::BeginTabItem("UI"))
            {
                ImGui::Spacing();
                DrawUIStats();
                ImGui::EndTabItem();
            }

            if (ImGui::BeginTabItem("Memory"))
            {
                ImGui::Spacing();
                DrawMemoryStats();
                ImGui::EndTabItem();
            }

            if (ImGui::BeginTabItem("Frame Timings"))
            {
                ImGui::Spacing();
                // Draw Timing Graph
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

                    ImPlot::SetNextPlotLimits(0.0, 120.0, 0, 33.0);

                    //lock minimum Y to 0 (cant have negative ms)
                    //lock X completely as its fixed 120 frames
                    if (ImPlot::BeginPlot("Timing", "frame", "ms", ImVec2(400, 300), 0, ImPlotAxisFlags_Lock, ImPlotAxisFlags_LockMin))
                    {
                        ImPlot::PlotLine("Update Time", updateTimes.data(), (int)updateTimes.size());
                        ImPlot::PlotLine("Render Time", renderTimes.data(), (int)renderTimes.size());
                        ImPlot::EndPlot();
                    }
                }
                ImGui::EndTabItem();
            }

            ImGui::EndTabBar();
        }
    }

    ImGui::End();
}

void EngineLoop::DrawMapStats()
{
    entt::registry* registry = ServiceLocator::GetGameRegistry();
    MapSingleton& mapSingleton = registry->ctx<MapSingleton>();
    NDBCSingleton& ndbcSingleton = registry->ctx<NDBCSingleton>();

    static const std::string* selectedMap = nullptr;
    static std::string selectedMapToLower;

    if (selectedMap == nullptr && mapSingleton.mapNames.size() > 0)
        selectedMap = mapSingleton.mapNames[0];

    selectedMapToLower.resize(selectedMap->length());
    std::transform(selectedMap->begin(), selectedMap->end(), selectedMapToLower.begin(), std::tolower);

    // Map Selection
    {
        ImGui::Text("Select a map");

        static std::string searchText = "";
        static std::string searchTextToLower = "";
        static std::string mapNameCopy = "";
        ImGui::InputText("Filter", &searchText);

        searchTextToLower.resize(searchText.length());
        std::transform(searchText.begin(), searchText.end(), searchTextToLower.begin(), std::tolower);

        bool hasFilter = searchText.length() != 0;

        static const char* preview = nullptr;
        if (!hasFilter)
            preview = selectedMap->c_str();

        if (ImGui::BeginCombo("##", preview)) // The second parameter is the label previewed before opening the combo.
        {
            for (const std::string* mapName : mapSingleton.mapNames)
            {
                mapNameCopy.resize(mapName->length());
                std::transform(mapName->begin(), mapName->end(), mapNameCopy.begin(), std::tolower);

                if (mapNameCopy.find(searchTextToLower) == std::string::npos)
                    continue;

                bool isSelected = selectedMap == mapName;

                if (ImGui::Selectable(mapName->c_str(), &isSelected))
                {
                    selectedMap = mapName;
                    preview = selectedMap->c_str();
                }

                if (isSelected)
                    ImGui::SetItemDefaultFocus();
            }

            ImGui::EndCombo();
        }
        else
        {
            if (hasFilter)
            {
                if (selectedMapToLower.find(searchTextToLower) != std::string::npos)
                {
                    preview = selectedMap->c_str();
                }
                else
                {
                    for (const std::string* mapName : mapSingleton.mapNames)
                    {
                        mapNameCopy.resize(mapName->length());
                        std::transform(mapName->begin(), mapName->end(), mapNameCopy.begin(), std::tolower);

                        if (mapNameCopy.find(searchTextToLower) == std::string::npos)
                            continue;

                        preview = mapName->c_str();
                        break;
                    }
                }
            }
        }

        if (ImGui::Button("Load"))
        {
            u32 mapNamehash = StringUtils::fnv1a_32(preview, strlen(preview));
            const NDBC::Map* map = mapSingleton.mapNameToDBC[mapNamehash];

            ServiceLocator::GetClientRenderer()->GetTerrainRenderer()->LoadMap(map);
        }

        ImGui::SameLine();

        if (ImGui::Button("Set Default"))
        {
            ConfigSingleton& configSingleton = registry->ctx<ConfigSingleton>();
            configSingleton.uiConfig.SetDefaultMap(preview);
        }

        ImGui::SameLine();

        if (ImGui::Button("Clear Default"))
        {
            ConfigSingleton& configSingleton = registry->ctx<ConfigSingleton>();
            configSingleton.uiConfig.SetDefaultMap("");
        }

        ImGui::Spacing();
    }

    if (ImGui::BeginTabBar("Map Information"))
    {
        bool mapIsLoaded = mapSingleton.currentMap.IsLoadedMap();

        if (ImGui::BeginTabItem("Basic Info"))
        {
            if (!mapIsLoaded)
            {
                ImGui::Text("No Map Loaded");
            }
            else
            {
                const NDBC::File& ndbcFile = ndbcSingleton.nameHashToDBCFile["Maps"_h];
                const NDBC::Map* map = mapSingleton.mapIdToDBC[mapSingleton.currentMap.id];

                const std::string& publicMapName = ndbcFile.stringTable->GetString(map->name);
                const std::string& internalMapName = ndbcFile.stringTable->GetString(map->internalName);

                static std::string instanceType = "............."; // Default to 13 Characters (Max that can be set to force default size to not need reallocation)
                {
                    if (map->instanceType == 0 || map->instanceType >= 5)
                        instanceType = "Open World";
                    else
                    {
                        if (map->instanceType == 1)
                            instanceType = "Dungeon";
                        else if (map->instanceType == 2)
                            instanceType = "Raid";
                        else if (map->instanceType == 3)
                            instanceType = "Battleground";
                        else if (map->instanceType == 4)
                            instanceType = "Arena";
                    }
                }

                ImGui::Text("Map Id:            %u", map->id);
                ImGui::Text("Public Name:       %s", publicMapName.c_str());
                ImGui::Text("Internal name:     %s", internalMapName.c_str());
                ImGui::Text("Instance Type:     %s", instanceType.c_str());
                ImGui::Text("Max Players:       %u", map->maxPlayers);
                ImGui::Text("Expansion:         %u", map->expansion);
            }

            ImGui::EndTabItem();
        }

        if (ImGui::BeginTabItem("Placement Info"))
        {
            if (!mapIsLoaded)
            {
                ImGui::Text("No Map Loaded");
            }
            else
            {
                if (mapSingleton.currentMap.header.flags.UseMapObjectInsteadOfTerrain)
                {
                    ImGui::Text("Loaded World Object:           %s", mapSingleton.currentMap.header.mapObjectName.c_str());
                }
                else
                {
                    ImGui::Text("Loaded Chunks:                 %u", mapSingleton.currentMap.chunks.size());
                }

                TerrainRenderer* terrainRenderer = _clientRenderer->GetTerrainRenderer();
                MapObjectRenderer* mapObjectRenderer = terrainRenderer->GetMapObjectRenderer();
                CModelRenderer* cModelRenderer = _clientRenderer->GetCModelRenderer();

                ImGui::Text("Loaded Map Objects:            %u", mapObjectRenderer->GetNumLoadedMapObjects());
                ImGui::Text("Loaded Complex Models:         %u", cModelRenderer->GetNumLoadedCModels());

                ImGui::Separator();

                ImGui::Text("Map Object Placements:         %u", mapObjectRenderer->GetNumMapObjectPlacements());
                ImGui::Text("Complex Models Placements:     %u", cModelRenderer->GetNumCModelPlacements());
            }
            
            ImGui::EndTabItem();
        }
        ImGui::EndTabBar();
    }
}

void EngineLoop::DrawPositionStats()
{
    Camera* camera = ServiceLocator::GetCamera();
    vec3 cameraLocation = camera->GetPosition();
    vec3 cameraRotation = camera->GetRotation();

    ImGui::Text("Camera Location : %f x, %f y, %f z", cameraLocation.x, cameraLocation.y, cameraLocation.z);
    ImGui::Text("Camera Rotation : %f x, %f y, %f z", cameraRotation.x, cameraRotation.y, cameraRotation.z);

    ImGui::Spacing();
    ImGui::Spacing();

    vec2 adtPos = Terrain::MapUtils::WorldPositionToADTCoordinates(cameraLocation);

    vec2 chunkPos = Terrain::MapUtils::GetChunkFromAdtPosition(adtPos);
    vec2 chunkRemainder = chunkPos - glm::floor(chunkPos);

    vec2 cellLocalPos = (chunkRemainder * Terrain::MAP_CHUNK_SIZE);
    vec2 cellPos = cellLocalPos / Terrain::MAP_CELL_SIZE;
    vec2 cellRemainder = cellPos - glm::floor(cellPos);

    vec2 patchLocalPos = (cellRemainder * Terrain::MAP_CELL_SIZE);
    vec2 patchPos = patchLocalPos / Terrain::MAP_PATCH_SIZE;
    vec2 patchRemainder = patchPos - glm::floor(patchPos);

    ImGui::Text("ADT : %f x, %f y", adtPos.x, adtPos.y);
    ImGui::Text("Chunk : %f x, %f y", chunkPos.x, chunkPos.y);
    ImGui::Text("cellPos : %f x, %f y", cellLocalPos.x, cellLocalPos.y);
    ImGui::Text("patchPos : %f x, %f y", patchLocalPos.x, patchLocalPos.y);

    ImGui::Spacing();
    ImGui::Text("Chunk Remainder : %f x, %f y", chunkRemainder.x, chunkRemainder.y);
    ImGui::Text("Cell  Remainder : %f x, %f y", cellRemainder.x, cellRemainder.y);
    ImGui::Text("Patch Remainder : %f x, %f y", patchRemainder.x, patchRemainder.y);
}

void EngineLoop::DrawUIStats()
{
    size_t count = ServiceLocator::GetUIRegistry()->size<UIComponent::Transform>();
    size_t notCulled = ServiceLocator::GetUIRegistry()->size<UIComponent::NotCulled>();

    ImGui::Text("Total Elements : %d", count);
    ImGui::Text("Culled elements : %d", (count-notCulled));
    
    ImGui::Spacing();
    ImGui::Spacing();
    ImGui::Checkbox("Show Collision Bounds", reinterpret_cast<bool*>(CVarSystem::Get()->GetIntCVar(StringUtils::StringHash("ui.drawCollisionBounds"))));
}

void EngineLoop::DrawMemoryStats()
{
    // RAM
    size_t ramUsage = Memory::MemoryTracker::GetMemoryUsage() / 1000000;
    size_t ramBudget = Memory::MemoryTracker::GetMemoryBudget() / 1000000;
    f32 ramPercent = (static_cast<f32>(ramUsage) / static_cast<f32>(ramBudget)) * 100;

    ImGui::Text("RAM Usage: %luMB / %luMB (%.2f%%)", ramUsage, ramBudget, ramPercent);

    size_t ramMinBudget = 3500;
    f32 ramMinPercent = (static_cast<f32>(ramUsage) / static_cast<f32>(ramMinBudget)) * 100;
    ImGui::Text("RAM Usage (Min specs): %luMB / %luMB (%.2f%%)", ramUsage, ramMinBudget, ramMinPercent);

    size_t ramUsagePeak = Memory::MemoryTracker::GetMemoryUsagePeak() / 1000000;
    f32 ramPeakPercent = (static_cast<f32>(ramUsagePeak) / static_cast<f32>(ramBudget)) * 100;

    ImGui::Text("RAM Usage (Peak): %luMB / %luMB (%.2f%%)", ramUsagePeak, ramBudget, ramPeakPercent);

    f32 ramMinPeakPercent = (static_cast<f32>(ramUsagePeak) / static_cast<f32>(ramMinBudget)) * 100;
    ImGui::Text("RAM Usage (Peak, Min specs): %luMB / %luMB (%.2f%%)", ramUsagePeak, ramMinBudget, ramMinPeakPercent);

    // VRAM
    ImGui::Spacing();

    size_t vramUsage = _clientRenderer->GetVRAMUsage() / 1000000;
    
    size_t vramBudget = _clientRenderer->GetVRAMBudget() / 1000000;
    f32 vramPercent = (static_cast<f32>(vramUsage) / static_cast<f32>(vramBudget)) * 100;

    ImGui::Text("VRAM Usage: %luMB / %luMB (%.2f%%)", vramUsage, vramBudget, vramPercent);

    size_t vramMinBudget = 1500;
    f32 vramMinPercent = (static_cast<f32>(vramUsage) / static_cast<f32>(vramMinBudget)) * 100;

    ImGui::Text("VRAM Usage (Min specs): %luMB / %luMB (%.2f%%)", vramUsage, vramMinBudget, vramMinPercent);
}

void EngineLoop::DrawImguiMenuBar()
{
    if (ImGui::BeginMainMenuBar())
    {
        _editor->DrawImguiMenuBar();

        if (ImGui::BeginMenu("Debug"))
        {
            if (ImGui::BeginMenu("CVAR"))
            {
                CVarSystem::Get()->DrawImguiEditor();
                ImGui::EndMenu();
            }
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
