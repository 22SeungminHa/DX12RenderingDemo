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

void Timer::Tick(float fLockFPS)
{
	if (hardwareHasPerformanceCounter_)
	{
		::QueryPerformanceCounter((LARGE_INTEGER*)&currentTime_);
	}
	else
	{
		currentTime_ = ::timeGetTime();
	}
	//마지막으로 이 함수를 호출한 이후 경과한 시간을 계산한다.

	float fTimeElapsed = (currentTime_ - lastTime_) * timeScale_;
	if (fLockFPS > 0.0f)
	{
		//이 함수의 파라메터(fLockFPS)가 0보다 크면 이 시간만큼 호출한 함수를 기다리게 한다. 

		while (fTimeElapsed < (1.0f / fLockFPS))
		{
			if (hardwareHasPerformanceCounter_)
			{
				::QueryPerformanceCounter((LARGE_INTEGER*)&currentTime_);
			}
			else
			{
				currentTime_ = ::timeGetTime();
			}
			//마지막으로 이 함수를 호출한 이후 경과한 시간을 계산한다.

			fTimeElapsed = (currentTime_ - lastTime_) * timeScale_;
		}
	}
	//현재 시간을 m_nLastTime에 저장한다. 
	lastTime_ = currentTime_;

	/* 마지막 프레임 처리 시간과 현재 프레임 처리 시간의 차이가 1초보다 작으면 현재 프레임 처리 시간
	을 frameTime_[0]에 저장한다. */
	if (fabsf(fTimeElapsed - timeElapsed_) < 1.0f)
	{
		::memmove(&frameTime_[1], frameTime_, (MAX_SAMPLE_COUNT - 1) *
			sizeof(float));
		frameTime_[0] = fTimeElapsed;
		if (sampleCount_ < MAX_SAMPLE_COUNT) sampleCount_++;
	}
	//초당 프레임 수를 1 증가시키고 현재 프레임 처리 시간을 누적하여 저장한다. 

	framesPerSecond_++;
	fpsTimeElapsed_ += fTimeElapsed;
	if (fpsTimeElapsed_ > 1.0f)
	{
		currentFrameRate_ = framesPerSecond_;
		framesPerSecond_ = 0;
		fpsTimeElapsed_ = 0.0f;
	}
	//누적된 프레임 처리 시간의 평균을 구하여 프레임 처리 시간을 구한다. 
	timeElapsed_ = 0.0f;
	for (ULONG i = 0; i < sampleCount_; i++) timeElapsed_ += frameTime_[i];
	if (sampleCount_ > 0) timeElapsed_ /= sampleCount_;
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