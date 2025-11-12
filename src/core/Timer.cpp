#include "Timer.h"

Timer CreateTimer()
{
	Timer timer{};

	QueryPerformanceFrequency(&timer._perfFrequency);
	QueryPerformanceCounter(&timer._lastFrameTime);

	return timer;
}

void PerFrameTimer(Timer& timer)
{

}
