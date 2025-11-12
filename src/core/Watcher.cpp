#include "Watcher.h"

#include <windows.h>

bool WatchDirectory(const std::wstring& directory) {
    HANDLE hDir = CreateFileW(
        directory.c_str(),
        FILE_LIST_DIRECTORY,
        FILE_SHARE_READ | FILE_SHARE_WRITE | FILE_SHARE_DELETE,
        nullptr,
        OPEN_EXISTING,
        FILE_FLAG_BACKUP_SEMANTICS,
        nullptr
    );
    bool result = false;
    if (hDir == INVALID_HANDLE_VALUE) return false;

    char buffer[1024];
    DWORD bytesReturned;

    while (ReadDirectoryChangesW(
        hDir,
        &buffer,
        sizeof(buffer),
        TRUE,
        FILE_NOTIFY_CHANGE_LAST_WRITE,
        &bytesReturned,
        nullptr,
        nullptr
    )) {
        FILE_NOTIFY_INFORMATION* event = (FILE_NOTIFY_INFORMATION*)buffer;
        // Handle the change event
        if (event->Action == FILE_ACTION_MODIFIED) {
            result = true;
        }
    }
    CloseHandle(hDir);
    return result;  
}