#pragma once
#include <string>

struct Watcher
{
	HANDLE _hDir;
	bool _stopRequested;
	std::string _dir;

	void StartWatching();
	bool WatchDirectory();
};

Watcher CreateWatcher(const LPWSTR& directory);