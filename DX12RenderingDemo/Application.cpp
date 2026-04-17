#include "pch.h"
#include "Application.h"

Application::Application()
{
	sceneManager_ = std::make_unique<SceneManager>();
	renderer_ = std::make_unique<Renderer>();
	inputSystem_ = std::make_unique<InputSystem>();

	_tcscpy_s(frameRate_, kTitlePrefix);
}

Application::~Application()
{
}

void Application::OnCreate(HINSTANCE instance, HWND hwnd)
{
	instance_ = instance;
	hwnd_ = hwnd;

	windowedStyle_ = static_cast<DWORD>(::GetWindowLongPtr(hwnd_, GWL_STYLE));
	windowedExStyle_ = static_cast<DWORD>(::GetWindowLongPtr(hwnd_, GWL_EXSTYLE));

	// 시작 모드 1회 적용
	ApplyStartupDisplayMode();

	RECT rect;
	::GetClientRect(hwnd_, &rect);

	UINT width = rect.right - rect.left;
	UINT height = rect.bottom - rect.top;

	renderer_->Initialize(hwnd_, width, height);

	sceneManager_->RequestChangeScene(SCENE_TYPE::TEST1);
	inputSystem_->Initialize(hwnd_, sceneManager_.get(), renderer_.get());

	timer_.Reset();
}

void Application::ApplyStartupDisplayMode()
{
	if (!hwnd_) return;

	if (startFullscreen_)
	{
		HMONITOR hMonitor = ::MonitorFromWindow(hwnd_, MONITOR_DEFAULTTONEAREST);
		MONITORINFO monitorInfo{};
		monitorInfo.cbSize = sizeof(MONITORINFO);
		::GetMonitorInfo(hMonitor, &monitorInfo);

		::SetWindowLongPtr(hwnd_, GWL_STYLE, WS_POPUP | WS_VISIBLE);
		::SetWindowLongPtr(hwnd_, GWL_EXSTYLE, windowedExStyle_);

		::SetWindowPos(
			hwnd_,
			HWND_TOP,
			monitorInfo.rcMonitor.left,
			monitorInfo.rcMonitor.top,
			monitorInfo.rcMonitor.right - monitorInfo.rcMonitor.left,
			monitorInfo.rcMonitor.bottom - monitorInfo.rcMonitor.top,
			SWP_FRAMECHANGED | SWP_SHOWWINDOW);
	}
	else
	{
		::SetWindowLongPtr(hwnd_, GWL_STYLE, windowedStyle_);
		::SetWindowLongPtr(hwnd_, GWL_EXSTYLE, windowedExStyle_);

		RECT windowRect{ 0, 0, FRAME_BUFFER_WIDTH, FRAME_BUFFER_HEIGHT };
		::AdjustWindowRectEx(&windowRect, windowedStyle_, FALSE, windowedExStyle_);

		const int windowWidth = windowRect.right - windowRect.left;
		const int windowHeight = windowRect.bottom - windowRect.top;

		::SetWindowPos(
			hwnd_,
			nullptr,
			100, 100,
			windowWidth, windowHeight,
			SWP_NOZORDER | SWP_FRAMECHANGED | SWP_SHOWWINDOW);
	}
}

void Application::OnDestroy()
{
	if (renderer_) renderer_->WaitForGpuComplete();
	if (sceneManager_) sceneManager_->ReleaseScene();
	if (renderer_) renderer_->Shutdown();
}

LRESULT CALLBACK Application::OnProcessingMessage(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam)
{
	switch (msg) {
	case WM_LBUTTONDOWN:
	case WM_RBUTTONDOWN:
	case WM_LBUTTONUP:
	case WM_RBUTTONUP:
	case WM_MOUSEMOVE:
		if (inputSystem_) inputSystem_->OnProcessingMouseMessage(hwnd, msg, wParam, lParam);
		break;

	case WM_KEYDOWN:
	case WM_KEYUP:
		if (inputSystem_) inputSystem_->OnProcessingKeyboardMessage(hwnd, msg, wParam, lParam);
		break;
	}

	return 0;
}

void Application::Animate()
{
	if (sceneManager_) sceneManager_->Animate(timer_.GetTimeElapsed());
}

void Application::ProcessSceneChange()
{
	if (!renderer_ || !sceneManager_ || !sceneManager_->HasSceneChange())
		return;

	renderer_->BeginSceneLoad();
	sceneManager_->ProcessSceneChange(renderer_->GetDevice(), renderer_->GetCommandList());
	renderer_->EndSceneLoad();

	sceneManager_->ReleaseUploadBuffers();
}

void Application::FrameAdvance()
{
	timer_.Tick();

	ProcessSceneChange();

	if (inputSystem_) inputSystem_->ProcessInput();

	Animate();

	if (renderer_ && sceneManager_) renderer_->Render(sceneManager_->GetScene());

	if (hwnd_) {
		timer_.GetFrameRate(frameRate_ + prefixLen, static_cast<int>(remain));
		::SetWindowText(hwnd_, frameRate_);
	}
}