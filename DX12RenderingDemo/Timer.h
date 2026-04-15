#pragma once
#include "pch.h"

const ULONG MAX_SAMPLE_COUNT = 50; // 50회의 프레임 처리시간을 누적하여 평균한다.

class Timer
{
public:
	Timer();
	virtual ~Timer();

	void Start() { }
	void Stop() { }
	void Reset();
	void Tick(float fLockFPS = 0.0f);
	unsigned long GetFrameRate(LPTSTR lpszString = NULL, int nCharacters = 0);
	float GetTimeElapsed();

private:
	bool hardwareHasPerformanceCounter_;
	float timeScale_;
	float timeElapsed_;
	__int64 currentTime_;
	__int64 lastTime_;
	__int64 performanceFrequency_;

	float frameTime_[MAX_SAMPLE_COUNT];
	ULONG sampleCount_;

	unsigned long currentFrameRate_;
	unsigned long framesPerSecond_;
	float fpsTimeElapsed_;

	bool stopped_;
};