#pragma once

struct Timer
{
	LARGE_INTEGER _perfFrequency;
	LARGE_INTEGER _lastFrameTime;
};

Timer CreateTimer();
void PerFrameTimer(Timer& timer);