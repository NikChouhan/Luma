#include "Timer.h"

Timer::Timer()
{
    QueryPerformanceFrequency(&perfFrequency_);
    QueryPerformanceCounter(&lastFrameTime_);

    startTime_ = lastFrameTime_;
    secondsPerTick_ = 1.0 / static_cast<double>(perfFrequency_.QuadPart);
}

void Timer::Reset()
{
    QueryPerformanceCounter(&lastFrameTime_);
    startTime_ = lastFrameTime_;
    deltaTime_ = 0.0f;
}

void Timer::Tick()
{
    LARGE_INTEGER currentTime;
    QueryPerformanceCounter(&currentTime);

    u64 timeDifference = currentTime.QuadPart - lastFrameTime_.QuadPart;

    deltaTime_ = static_cast<float>(timeDifference * secondsPerTick_);
    lastFrameTime_ = currentTime;
    if (deltaTime_ < 0.0f)
    {
        deltaTime_ = 0.0f;
    }
}

float Timer::DeltaTime() const
{
    return deltaTime_;
}

u64 Timer::TotalTime() const
{
    u64 totalTicks = lastFrameTime_.QuadPart - startTime_.QuadPart;
	return static_cast<u64>(totalTicks * secondsPerTick_ * 1000.0);
}
