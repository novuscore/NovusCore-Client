#include <Utils/DebugHandler.h>
#include <Utils/StringUtils.h>

#include "EngineLoop.h"
#include "ConsoleCommands.h"
#include <Utils/Message.h>

#ifdef _WIN32
#include <Windows.h>
#endif
#include <future>

//The name of the console window.
#define WINDOWNAME "Client"

i32 main()
{
    /* Set up console window title */
#ifdef _WIN32 //Windows
    SetConsoleTitle(WINDOWNAME);
#endif

    EngineLoop engineLoop;
    engineLoop.Start();

    ConsoleCommandHandler consoleCommandHandler;
    auto future = std::async(std::launch::async, StringUtils::GetLineFromCin);
    while (true)
    {
        Message message;
        bool shouldExit = false;

        while (engineLoop.TryGetMessage(message))
        {
            if (message.code == MSG_OUT_EXIT_CONFIRM)
            {
                shouldExit = true;
                break;
            }
            else if (message.code == MSG_OUT_PRINT)
            {
                DebugHandler::Print(*message.message);
                delete message.message;
            }
        }

        if (shouldExit)
            break;

        if (future.wait_for(std::chrono::milliseconds(50)) == std::future_status::ready)
        {
            std::string command = future.get();
            //std::transform(command.begin(), command.end(), command.begin(), ::tolower); // Convert command to lowercase

            consoleCommandHandler.HandleCommand(engineLoop, command);
            future = std::async(std::launch::async, StringUtils::GetLineFromCin);
        }
    }

    return 0;
}
