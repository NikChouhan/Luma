#pragma once

struct Timer
{
	Timer();
	void Reset();
	void Tick();

	// returns total time in Milliseconds
	float DeltaTime() const;
	u64 TotalTime() const;

private:
	LARGE_INTEGER perfFrequency_;
	LARGE_INTEGER lastFrameTime_;

	LARGE_INTEGER startTime_;
	double secondsPerTick_ = 0.0;
	float deltaTime_ = 0.0f;
};
