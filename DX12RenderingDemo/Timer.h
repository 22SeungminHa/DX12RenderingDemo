#pragma once
#include "pch.h"

class Timer
{
public:
    Timer();
    ~Timer() = default;

public:
    void Reset();
    void Start();
    void Stop();
    void Tick();

    unsigned long GetFrameRate(LPTSTR lpszString = nullptr, int nCharacters = 0) const;
    float GetTimeElapsed() const;
    float GetTotalTime() const;
    bool IsStopped() const { return stopped_; }

private:
    __int64 GetCurrentTime() const;

private:
    bool hardwareHasPerformanceCounter_ = false;
    bool stopped_ = false;

    float timeScale_ = 0.0f;
    float timeElapsed_ = 0.0f;

    __int64 baseTime_ = 0;
    __int64 pausedTime_ = 0;
    __int64 stopTime_ = 0;
    __int64 currentTime_ = 0;
    __int64 lastTime_ = 0;
    __int64 performanceFrequency_ = 0;

    unsigned long currentFrameRate_ = 0;
    unsigned long framesPerSecond_ = 0;
    float fpsTimeElapsed_ = 0.0f;
};