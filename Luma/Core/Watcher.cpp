#include "Watcher.h"

#include <windows.h>

#include "Log.h"

void Watcher::StartWatching()
{
    
}

void Watcher::ProcessNotifications(BYTE* data, DWORD dword)
{

}

//bool Watcher::WatchDirectory(LAMBDA(Resources*) resources)
//{
//    constexpr DWORD bufferSize = 64 * 1024;
//    std::vector<BYTE> buffer(bufferSize);
//    DWORD bytesReturned;
//
//    bool result = false;
//    while (!_stopRequested) 
//    {
//        if(!ReadDirectoryChangesW(
//            _hDir,
//            buffer.data(),
//            bufferSize,
//            TRUE, // Watch subdirectories
//            FILE_NOTIFY_CHANGE_LAST_WRITE |
//            FILE_NOTIFY_CHANGE_FILE_NAME |
//            FILE_NOTIFY_CHANGE_DIR_NAME,
//            &bytesReturned,
//            nullptr,
//            nullptr
//        )) 
//        {
//            return false;
//        }
//        if (bytesReturned == 0) break;
//
//        ProcessNotifications(buffer.data(), bytesReturned);
//    }
//    CloseHandle(_hDir);
//    return result;  
//}

Watcher CreateWatcher(const wchar_t* directory)
{
    Watcher watcher{};
    watcher._hDir = CreateFileW(
        directory,
        FILE_LIST_DIRECTORY,
        FILE_SHARE_READ | FILE_SHARE_WRITE | FILE_SHARE_DELETE,
        nullptr,
        OPEN_EXISTING,
        FILE_FLAG_BACKUP_SEMANTICS,
        nullptr
    );
    if (watcher._hDir == INVALID_HANDLE_VALUE)
    {
        printl(Luma::Log::LogLevel::Error, "Watcher failed!");
        abort();
    }
    watcher._stopRequested = false;
    return watcher;
}
