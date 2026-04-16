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
	RECT rect;
	::GetClientRect(hwnd_, &rect);
	UINT width = rect.right - rect.left;
	UINT height = rect.bottom - rect.top;

	if (renderer_) renderer_->Resize(width, height);
}

void GameFramework::ToggleFullscreen()
{
	if (!renderer_ || isFullscreenChanging_) return;

	isFullscreenChanging_ = true;

	renderer_->WaitForGpuComplete();
	renderer_->ChangeSwapChainState();

	RECT rect{};
	::GetClientRect(hwnd_, &rect);
	UINT width = rect.right - rect.left;
	UINT height = rect.bottom - rect.top;

	isFullscreenChanging_ = false;
}

LRESULT CALLBACK GameFramework::onProcessingWindowMessage(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam)
{	
	switch (msg) {
	case WM_SIZE:
	{
		if (wParam == SIZE_MINIMIZED) return 0;
		if (isFullscreenChanging_) return 0;

		UINT width = LOWORD(lParam);
		UINT height = HIWORD(lParam);
		if (width == 0 || height == 0) return 0;

		onResize();
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
		if (inputSystem_ && inputSystem_->OnProcessingKeyboardMessage(hwnd, msg, wParam, lParam))
			return 0;
		break;
	case WM_KEYUP:
		if (wParam == VK_F9) {
			ToggleFullscreen();
			return 0;
		}
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

	processSceneChange();

	if (inputSystem_) inputSystem_->ProcessInput();

	animate();

	if (renderer_ && sceneManager_) renderer_->Render(sceneManager_->GetScene());

	timer_.GetFrameRate(frameRate_ + 12, 37);
	::SetWindowText(hwnd_, frameRate_);
}
