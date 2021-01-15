/*
# MIT License

# Copyright(c) 2018-2019 NovusCore

# Permission is hereby granted, free of charge, to any person obtaining a copy
# of this software and associated documentation files(the "Software"), to deal
# in the Software without restriction, including without limitation the rights
# to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
# copies of the Software, and to permit persons to whom the Software is
# furnished to do so, subject to the following conditions :

# The above copyright notice and this permission notice shall be included in all
# copies or substantial portions of the Software.

# THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
# IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
# FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.IN NO EVENT SHALL THE
# AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
# LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
# OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
# SOFTWARE.
*/
#pragma once
#include <NovusTypes.h>
#include <Utils/Message.h>
#include <Utils/StringUtils.h>
#include <Utils/ConcurrentQueue.h>
#include <taskflow/taskflow.hpp>
#include <entity/fwd.hpp>
#include <asio/io_service.hpp>

namespace tf
{
    class Framework;
}

namespace Editor
{
    class Editor;
}

struct FrameworkRegistryPair
{
    entt::registry gameRegistry;
    entt::registry uiRegistry;
    tf::Framework framework;
    tf::Taskflow taskflow;
};

class ClientRenderer;
class EngineLoop
{
public:
    EngineLoop();
    ~EngineLoop();

    void Start();
    void Stop();
    void Abort();

    void PassMessage(Message& message);
    bool TryGetMessage(Message& message);

    template <typename... Args>
    void PrintMessage(std::string message, Args... args)
    {
        char str[256];
        StringUtils::FormatString(str, sizeof(str), message.c_str(), args...);

        Message printMessage;
        printMessage.code = MSG_OUT_PRINT;
        printMessage.message = new std::string(str);
        _outputQueue.enqueue(printMessage);
    }

private:
    bool Init();
    void Run();
    void RunIoService();
    void Cleanup();

    bool Update(f32 deltaTime);
    void UpdateSystems();
    void Render();

    void SetupUpdateFramework();
    void SetupMessageHandler();

    void ImguiNewFrame();
    void DrawEngineStats(struct EngineStatsSingleton* stats);
    void DrawMapStats();
    void DrawPositionStats();
    void DrawUIStats();
    void DrawMemoryStats();
    void DrawImguiMenuBar();
    void DrawPerformance(struct EngineStatsSingleton* stats);
    void DrawCullingStatsEntry(std::string_view name, u32 drawCalls, u32 survivedDrawCalls, bool isCollapsed);

private:
    bool _isInitialized = false;
    bool _isRunning = false;

    moodycamel::ConcurrentQueue<Message> _inputQueue;
    moodycamel::ConcurrentQueue<Message> _outputQueue;
    FrameworkRegistryPair _updateFramework;

    ClientRenderer* _clientRenderer;
    Editor::Editor* _editor;
    std::shared_ptr<asio::io_service> _asioService;
};