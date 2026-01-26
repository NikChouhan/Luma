#pragma once
#include <string>


struct Watcher
{
	HANDLE _hDir;
	bool _stopRequested;
	std::string _dir;

	void StartWatching();
	void ProcessNotifications(BYTE* data, DWORD dword);
	//bool WatchDirectory(LAMBDA(Resources*) resources);
};

Watcher CreateWatcher(const wchar_t* directory);