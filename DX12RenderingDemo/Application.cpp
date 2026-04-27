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
	hInstance_ = instance;
	hwnd_ = hwnd;

	windowedStyle_ = static_cast<DWORD>(::GetWindowLongPtr(hwnd_, GWL_STYLE));
	windowedExStyle_ = static_cast<DWORD>(::GetWindowLongPtr(hwnd_, GWL_EXSTYLE));

	ApplyStartupDisplayMode();

	RECT rect;
	::GetClientRect(hwnd_, &rect);

	UINT width = rect.right - rect.left;
	UINT height = rect.bottom - rect.top;

	renderer_->Initialize(hwnd_, width, height);

	sceneManager_->RequestChangeScene(SCENE_TYPE::TEST1);
	inputSystem_->Initialize(hwnd_);

	timer_.Reset();
}

void Application::ApplyStartupDisplayMode()
{
	if (!hwnd_) return;

	if (startFullscreen_) {
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
	else {
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

void Application::ProcessPendingUploadBufferRelease(bool forceWait)
{
	if (!hasPendingUploadBufferRelease_ || !renderer_ || !sceneManager_)
		return;

	if (!forceWait && !renderer_->IsSceneLoadComplete(pendingSceneLoadFenceValue_))
		return;

	renderer_->WaitForSceneLoad(pendingSceneLoadFenceValue_);
	sceneManager_->ReleaseCurrentSceneUploadBuffers();

	pendingSceneLoadFenceValue_ = 0;
	hasPendingUploadBufferRelease_ = false;
}

void Application::OnDestroy()
{
	// scene load 업로드가 남아 있으면 여기서만 정리
	ProcessPendingUploadBufferRelease(true);

	if (renderer_) renderer_->WaitForGpuComplete();
	if (sceneManager_) sceneManager_->ReleaseCurrentScene();
	if (renderer_) renderer_->Shutdown();
}

LRESULT CALLBACK Application::OnProcessMessage(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam)
{
	switch (msg)
	{
	case WM_SIZE:
	{
		const UINT width = LOWORD(lParam);
		const UINT height = HIWORD(lParam);

		pendingResizeWidth_ = width;
		pendingResizeHeight_ = height;

		if (wParam == SIZE_MINIMIZED) {
			isMinimized_ = true;
			timer_.Stop();
			break;
		}

		if (isMinimized_) {
			isMinimized_ = false;
			timer_.Start();
		}

		if (width == 0 || height == 0) break;

		resizePending_ = true;
		break;
	}

	case WM_LBUTTONDOWN:
	case WM_RBUTTONDOWN:
	case WM_LBUTTONUP:
	case WM_RBUTTONUP:
	case WM_MOUSEMOVE:
		if (inputSystem_) inputSystem_->HandleMouseMessage(msg, wParam, lParam);
		break;

	case WM_KEYDOWN:
	case WM_KEYUP:
	case WM_SYSKEYDOWN:
	case WM_SYSKEYUP:
		if (inputSystem_) inputSystem_->HandleKeyboardMessage(msg, wParam, lParam);

		if (msg == WM_KEYUP && wParam == VK_ESCAPE)
			::PostQuitMessage(0);

		if (msg == WM_KEYUP && wParam == VK_SPACE && sceneManager_) {
			SCENE_TYPE type = sceneManager_->GetCurrentSceneType();
			if (type == SCENE_TYPE::TEST1) sceneManager_->RequestChangeScene(SCENE_TYPE::TEST2);
			else if (type == SCENE_TYPE::TEST2) sceneManager_->RequestChangeScene(SCENE_TYPE::TEST1);
		}

		break;

	//case WM_ACTIVATE:
	//{
	//	if (LOWORD(wParam) == WA_INACTIVE)
	//		timer_.Stop();
	//	else if (!isMinimized_)
	//		timer_.Start();
	//	break;
	//}
	}

	return 0;
}

void Application::HandleResize(UINT width, UINT height)
{
	if (!renderer_)
		return;

	if (width == 0 || height == 0)
		return;

	ProcessPendingUploadBufferRelease(true);
	renderer_->Resize(width, height);
	sceneManager_->ResizeCurrentScene(width, height);
}

void Application::ProcessSceneChange()
{
	if (!renderer_ || !sceneManager_ || !sceneManager_->HasSceneChange())
		return;

	// 이전 scene load의 upload buffer가 아직 남아 있다면
	// 새 scene 전환 전에만 안전하게 정리
	ProcessPendingUploadBufferRelease(true);

	renderer_->BeginSceneLoad();

	RECT rect;
	::GetClientRect(hwnd_, &rect);

	UINT width = rect.right - rect.left;
	UINT height = rect.bottom - rect.top;

	sceneManager_->ProcessSceneChange(renderer_->GetDevice(), renderer_->GetCommandList(), width, height);
	pendingSceneLoadFenceValue_ = renderer_->EndSceneLoad();
	hasPendingUploadBufferRelease_ = true;
}

void Application::FrameAdvance()
{
	timer_.Tick();

	ProcessPendingUploadBufferRelease(false);

	if (isMinimized_)
		return;

	if (resizePending_) {
		resizePending_ = false;
		HandleResize(pendingResizeWidth_, pendingResizeHeight_);
		return; // resize한 프레임은 바로 종료
	}

	ProcessSceneChange();

	if (inputSystem_) inputSystem_->Update();

	Scene* currentScene = sceneManager_ ? sceneManager_->GetCurrentScene() : nullptr;
	if (!currentScene) return;

	const float dt = timer_.GetTimeElapsed();

	currentScene->ProcessInput(*inputSystem_, dt);
	currentScene->Animate(dt);

	if (renderer_) renderer_->Render(currentScene);

	if (hwnd_) {
		timer_.GetFrameRate(frameRate_ + kTitlePrefixLength, static_cast<int>(kRemain));
		::SetWindowText(hwnd_, frameRate_);
	}
}