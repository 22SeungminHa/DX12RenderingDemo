#include "Timer.h"
#include "pch.h"

Timer::Timer()
{
	if (::QueryPerformanceFrequency((LARGE_INTEGER*)&performanceFrequency_))
	{
		hardwareHasPerformanceCounter_ = TRUE;
		::QueryPerformanceCounter((LARGE_INTEGER*)&lastTime_);
		timeScale_ = 1.0f / performanceFrequency_;
	}
	else
	{
		hardwareHasPerformanceCounter_ = FALSE;
		lastTime_ = ::timeGetTime();
		timeScale_ = 0.001f;
	}
	sampleCount_ = 0;
	currentFrameRate_ = 0;
	framesPerSecond_ = 0;
	fpsTimeElapsed_ = 0.0f;
}

Timer::~Timer()
{
}

void Timer::Tick()
{
	if (hardwareHasPerformanceCounter_)
		::QueryPerformanceCounter(reinterpret_cast<LARGE_INTEGER*>(&currentTime_));
	else
		currentTime_ = ::timeGetTime();

	float deltaTime = static_cast<float>((currentTime_ - lastTime_) * timeScale_);
	lastTime_ = currentTime_;

	if (deltaTime > 0.25f)
		deltaTime = 0.25f;

	timeElapsed_ = deltaTime;

	framesPerSecond_++;
	fpsTimeElapsed_ += deltaTime;
	if (fpsTimeElapsed_ >= 1.0f)
	{
		currentFrameRate_ = framesPerSecond_;
		framesPerSecond_ = 0;
		fpsTimeElapsed_ = 0.0f;
	}
}

void Timer::WaitForFrameRate(float lockFPS)
{
	if (lockFPS <= 0.0f)
		return;

	const float targetElapsed = 1.0f / lockFPS;

	while (timeElapsed_ < targetElapsed)
	{
		::Sleep(1);

		UINT64 now;
		if (hardwareHasPerformanceCounter_)
			::QueryPerformanceCounter(reinterpret_cast<LARGE_INTEGER*>(&now));
		else
			now = ::timeGetTime();

		timeElapsed_ = static_cast<float>((now - lastTime_) * timeScale_);
	}
}

unsigned long Timer::GetFrameRate(LPTSTR lpszString, int nCharacters)
{
	//현재 프레임 레이트를 문자열로 변환하여 lpszString 버퍼에 쓰고 “ FPS”와 결합한다. 

	if (lpszString)
	{
		_itow_s(currentFrameRate_, lpszString, nCharacters, 10);
		wcscat_s(lpszString, nCharacters, _T(" FPS)"));
	}
	return(currentFrameRate_);
};


float Timer::GetTimeElapsed()
{
	return(timeElapsed_);
}

void Timer::Reset()
{
	__int64 nPerformanceCounter;
	::QueryPerformanceCounter((LARGE_INTEGER*)&nPerformanceCounter);

	lastTime_ = nPerformanceCounter;
	currentTime_ = nPerformanceCounter;

	stopped_ = false;
}