#include "Timer.h"

Timer::Timer()
{
    if (::QueryPerformanceFrequency(reinterpret_cast<LARGE_INTEGER*>(&performanceFrequency_)))
    {
        hardwareHasPerformanceCounter_ = true;
        timeScale_ = 1.0f / static_cast<float>(performanceFrequency_);
    }
    else
    {
        hardwareHasPerformanceCounter_ = false;
        performanceFrequency_ = 1000;
        timeScale_ = 0.001f;
    }

    Reset();
}

__int64 Timer::GetCurrentTime() const
{
    if (hardwareHasPerformanceCounter_)
    {
        __int64 currentTime = 0;
        ::QueryPerformanceCounter(reinterpret_cast<LARGE_INTEGER*>(&currentTime));
        return currentTime;
    }

    return static_cast<__int64>(::timeGetTime());
}

void Timer::Reset()
{
    const __int64 currentTime = GetCurrentTime();

    baseTime_ = currentTime;
    pausedTime_ = 0;
    stopTime_ = 0;
    currentTime_ = currentTime;
    lastTime_ = currentTime;

    timeElapsed_ = 0.0f;
    fpsTimeElapsed_ = 0.0f;
    framesPerSecond_ = 0;
    currentFrameRate_ = 0;

    stopped_ = false;
}

void Timer::Start()
{
    if (!stopped_)
        return;

    const __int64 startTime = GetCurrentTime();

    pausedTime_ += (startTime - stopTime_);
    lastTime_ = startTime;
    stopTime_ = 0;
    stopped_ = false;
}

void Timer::Stop()
{
    if (stopped_)
        return;

    stopTime_ = GetCurrentTime();
    stopped_ = true;
}

void Timer::Tick()
{
    if (stopped_)
    {
        timeElapsed_ = 0.0f;
        return;
    }

    currentTime_ = GetCurrentTime();
    timeElapsed_ = static_cast<float>((currentTime_ - lastTime_) * timeScale_);
    lastTime_ = currentTime_;

    if (timeElapsed_ < 0.0f)
        timeElapsed_ = 0.0f;

    if (timeElapsed_ > 0.25f)
        timeElapsed_ = 0.25f;

    ++framesPerSecond_;
    fpsTimeElapsed_ += timeElapsed_;

    if (fpsTimeElapsed_ >= 1.0f)
    {
        currentFrameRate_ = framesPerSecond_;
        framesPerSecond_ = 0;
        fpsTimeElapsed_ -= 1.0f;
    }
}

unsigned long Timer::GetFrameRate(LPTSTR lpszString, int nCharacters) const
{
    if (lpszString && nCharacters > 0)
    {
        _itow_s(currentFrameRate_, lpszString, nCharacters, 10);
        wcscat_s(lpszString, nCharacters, _T(" FPS"));
    }

    return currentFrameRate_;
}

float Timer::GetTimeElapsed() const
{
    return timeElapsed_;
}

float Timer::GetTotalTime() const
{
    if (stopped_)
        return static_cast<float>(((stopTime_ - pausedTime_) - baseTime_) * timeScale_);

    return static_cast<float>(((currentTime_ - pausedTime_) - baseTime_) * timeScale_);
}