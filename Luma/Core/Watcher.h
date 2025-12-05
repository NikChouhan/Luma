#pragma once
#include <string>

#include "Graphics/D3D12/Shader.h"

struct Watcher
{
	HANDLE _hDir;
	bool _stopRequested;
	std::string _dir;

	void StartWatching();
	void ProcessNotifications(BYTE* data, DWORD dword);
	bool WatchDirectory(LAMBDA(Resources*) resources);
};

Watcher CreateWatcher(const wchar_t* directory);