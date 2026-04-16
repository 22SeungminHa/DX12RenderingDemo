#include "pch.h"
#include "GameFramework.h"

GameFramework::GameFramework()
{
	sceneManager_ = std::make_unique<SceneManager>();
	renderer_ = std::make_unique<Renderer>();
	inputSystem_ = std::make_unique<InputSystem>();

	_tcscpy_s(frameRate_, _T("D3DX12Demo ("));
}

GameFramework::~GameFramework()
{
}

bool GameFramework::onCreate(HINSTANCE instance, HWND hwnd)
{
	//Direct3D 디바이스, 명령 큐와 명령 리스트, 스왑 체인 등을 생성하는 함수를 호출한다. 
	instance_ = instance;
	hwnd_ = hwnd;

	windowPlacement_.length = sizeof(WINDOWPLACEMENT);
	windowedStyle_ = static_cast<DWORD>(::GetWindowLongPtr(hwnd_, GWL_STYLE));
	windowedExStyle_ = static_cast<DWORD>(::GetWindowLongPtr(hwnd_, GWL_EXSTYLE));

	RECT rect;
	::GetClientRect(hwnd_, &rect);

	UINT width = rect.right - rect.left;
	UINT height = rect.bottom - rect.top;

	renderer_->Initialize(hwnd_, width, height);

	sceneManager_->RequestChangeScene(SCENE_TYPE::TEST1);
	inputSystem_->Initialize(hwnd_, sceneManager_.get(), renderer_.get());
	
	timer_.Reset();

	return true;
}

void GameFramework::onDestroy()
{
	if (renderer_) renderer_->WaitForGpuComplete();
	if (sceneManager_) sceneManager_->ReleaseScene();
	if (renderer_) renderer_->Shutdown();
}

void GameFramework::onResize()
{
	if (!renderer_) return;

	RECT rect{};
	::GetClientRect(hwnd_, &rect);

	UINT width = rect.right - rect.left;
	UINT height = rect.bottom - rect.top;

	if (width == 0 || height == 0) return;

	renderer_->Resize(width, height);
}

void GameFramework::ToggleFullscreen()
{
	if (!hwnd_ || isFullscreenChanging_) return;

	isFullscreenChanging_ = true;

	if (renderer_) renderer_->WaitForGpuComplete();

	if (!isBorderlessFullscreen_)
	{
		windowPlacement_.length = sizeof(WINDOWPLACEMENT);
		::GetWindowPlacement(hwnd_, &windowPlacement_);

		HMONITOR hMonitor = ::MonitorFromWindow(hwnd_, MONITOR_DEFAULTTONEAREST);
		MONITORINFO mi{};
		mi.cbSize = sizeof(MONITORINFO);
		::GetMonitorInfo(hMonitor, &mi);

		::SetWindowLongPtr(hwnd_, GWL_STYLE, WS_POPUP | WS_VISIBLE);
		::SetWindowLongPtr(hwnd_, GWL_EXSTYLE, windowedExStyle_);

		::SetWindowPos(
			hwnd_,
			HWND_TOP,
			mi.rcMonitor.left,
			mi.rcMonitor.top,
			mi.rcMonitor.right - mi.rcMonitor.left,
			mi.rcMonitor.bottom - mi.rcMonitor.top,
			SWP_FRAMECHANGED | SWP_SHOWWINDOW);

		isBorderlessFullscreen_ = true;
	}
	else
	{
		::SetWindowLongPtr(hwnd_, GWL_STYLE, windowedStyle_);
		::SetWindowLongPtr(hwnd_, GWL_EXSTYLE, windowedExStyle_);

		// showCmd 보정
		if (windowPlacement_.showCmd == SW_SHOWMINIMIZED)
			windowPlacement_.showCmd = SW_SHOWNORMAL;

		::SetWindowPlacement(hwnd_, &windowPlacement_);

		::SetWindowPos(
			hwnd_,
			HWND_NOTOPMOST,
			0, 0, 0, 0,
			SWP_NOMOVE | SWP_NOSIZE | SWP_NOOWNERZORDER |
			SWP_FRAMECHANGED | SWP_SHOWWINDOW);

		isBorderlessFullscreen_ = false;
	}

	::ShowWindow(hwnd_, SW_SHOW);
	::UpdateWindow(hwnd_);

	isFullscreenChanging_ = false;

	// 여기서 바로 onResize() 호출하지 말고 플래그만 세팅
	pendingResizeAfterFullscreen_ = true;
}
LRESULT CALLBACK GameFramework::onProcessingWindowMessage(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam)
{	
	switch (msg) {
	case WM_APP_TOGGLE_FULLSCREEN:
		ToggleFullscreen();
		return 0;

	case WM_SIZE:
	{
		if (wParam == SIZE_MINIMIZED) return 0;
		if (isFullscreenChanging_) return 0;

		UINT width = LOWORD(lParam);
		UINT height = HIWORD(lParam);
		if (width == 0 || height == 0) return 0;

		// 너무 즉시 ResizeBuffers 하지 말고 지연
		pendingResizeAfterFullscreen_ = true;
		return 0;
	}

	case WM_LBUTTONDOWN:
	case WM_RBUTTONDOWN:
	case WM_LBUTTONUP:
	case WM_RBUTTONUP:
	case WM_MOUSEMOVE:
		if (inputSystem_ && inputSystem_->OnProcessingMouseMessage(hwnd, msg, wParam, lParam))
			return 0;
		break;

	case WM_KEYDOWN:
		if (wParam == VK_F9 && !(lParam & (1 << 30))) {
			::PostMessage(hwnd, WM_APP_TOGGLE_FULLSCREEN, 0, 0);
			return 0;
		}
		if (inputSystem_ && inputSystem_->OnProcessingKeyboardMessage(hwnd, msg, wParam, lParam))
			return 0;
		break;
	case WM_KEYUP:
		if (inputSystem_ && inputSystem_->OnProcessingKeyboardMessage(hwnd, msg, wParam, lParam))
			return 0;
		break;
	}

	return DefWindowProc(hwnd, msg, wParam, lParam);
}

void GameFramework::animate()
{
	if (sceneManager_) sceneManager_->Animate(timer_.GetTimeElapsed());
}

void GameFramework::processSceneChange()
{
	if (!renderer_ || !sceneManager_ || !sceneManager_->HasSceneChange())
		return;

	renderer_->BeginSceneLoad();
	sceneManager_->ProcessSceneChange(renderer_->GetDevice(), renderer_->GetCommandList());
	renderer_->EndSceneLoad();

	sceneManager_->ReleaseUploadBuffers();
}

void GameFramework::frameAdvance()
{
	timer_.Tick(0.0f);

	if (pendingResizeAfterFullscreen_)
	{
		pendingResizeAfterFullscreen_ = false;
		onResize();
	}

	processSceneChange();

	if (inputSystem_) inputSystem_->ProcessInput();

	animate();

	if (renderer_ && sceneManager_) renderer_->Render(sceneManager_->GetScene());

	timer_.GetFrameRate(frameRate_ + 12, 37);
	::SetWindowText(hwnd_, frameRate_);
}
