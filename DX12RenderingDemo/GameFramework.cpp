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

	// 렌더러 초기화
	renderer_->Initialize(hwnd_, width, height);

	// 시작 씬 전환 요청. FrameAdvance()에서 생성한다.
	sceneManager_->RequestChangeScene(SCENE_TYPE::TEST1);

	inputSystem_->Initialize(hwnd_, sceneManager_.get(), renderer_.get());

	timer_.Reset();

	return true;
}

void GameFramework::onDestroy()
{
	//GPU가 모든 명령 리스트를 실행할 때 까지 기다린다.
	if (renderer_) renderer_->WaitForGpuComplete();

	// 씬과 씬 매니저를 unload 한다. 게임 객체(게임 월드 객체)를 소멸한다.
	if (sceneManager_) sceneManager_->ReleaseScene();

	if (renderer_) renderer_->Shutdown();
}

void GameFramework::ToggleFullscreen()
{
	if (!renderer_) return;

	isFullscreenChanging_ = true;

	renderer_->WaitForGpuComplete();
	renderer_->ChangeSwapChainState();

	RECT rect{};
	::GetClientRect(hwnd_, &rect);
	UINT width = rect.right - rect.left;
	UINT height = rect.bottom - rect.top;

	if (width > 0 && height > 0)
		renderer_->Resize(width, height);

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

		ToggleFullscreen();
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

	processSceneChange();

	if (inputSystem_) inputSystem_->ProcessInput();

	animate();

	if (renderer_ && sceneManager_) renderer_->Render(sceneManager_->GetScene());

	timer_.GetFrameRate(frameRate_ + 12, 37);
	::SetWindowText(hwnd_, frameRate_);
}
