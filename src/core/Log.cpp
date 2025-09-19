#include <iostream>
#include <iomanip>
#include "Log.h"

#include <cstdarg>
#include <cstdio>
#include "common.h"
#include <print>

namespace Luma
{
    bool Log::m_initialized = false;
    static constexpr u32 bufferSize = 1024 * 1024;
    static char logBuffer[bufferSize];

    void Log::Init()
    {
        m_initialized = true;
        std::println("\033[32mLogger Init!\033[0m");
    }

    void Log::Shutdown()
    {
        m_initialized = false;
    }

    // temporarily removed until I find the way to do GUI
    // along with a console windows (without Alloc-ing a separate console
    // or other hacks
    /*void Log::LogMessage(LogLevel level, cstring msg)
    {
        if (!m_initialized)
        {
            return;
        }

        switch (level)
        {
        default: break;
        case LogLevel::Info:
            std::println("\033[32m{}\033[0m", msg);
            break;
        case LogLevel::Warn:
            std::println("\033[33m{}\033[0m", msg);
            break;
        case LogLevel::Error:
            std::println(("\033[31m{}\033[0m"), msg);
            break;
        case LogLevel::InfoDebug:
            std::println(("\033[36m{}\033[0m"), msg);
            break;
        }
    }*/

    void Log::LogMessage(LogLevel level, cstring msg)
    {
        if (!m_initialized)
        {
            return;
        }

        std::string formatted_msg;
        switch (level)
        {
        case LogLevel::Info:      formatted_msg = "[INFO] " + std::string(msg) + "\n"; break;
        case LogLevel::Warn:      formatted_msg = "[WARN] " + std::string(msg) + "\n"; break;
        case LogLevel::Error:     formatted_msg = "[ERROR] " + std::string(msg) + "\n"; break;
        case LogLevel::InfoDebug: formatted_msg = "[DEBUG] " + std::string(msg) + "\n"; break;
        default:                  formatted_msg = "[UNKN] " + std::string(msg) + "\n"; break;
        }

        int sizeNeeded = MultiByteToWideChar(CP_UTF8, 0, &formatted_msg[0],
            (int)formatted_msg.size(), nullptr, 0);
        std::wstring wstr(sizeNeeded, 0);
        MultiByteToWideChar(CP_UTF8, 0, &formatted_msg[0],
            (int)formatted_msg.size(), &wstr[0], sizeNeeded);

        OutputDebugStringW(wstr.c_str());
    }
}