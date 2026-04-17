#pragma once
#include "Timer.h"
#include "Scene.h"
#include "SceneManager.h"
#include "Renderer.h"
#include "InputSystem.h"

class Application
{
public:
	Application();
	~Application();

	void OnCreate(HINSTANCE instance, HWND hwnd);
	void OnDestroy();

	void FrameAdvance();
	LRESULT CALLBACK OnProcessMessage(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam);

private:
	void ApplyStartupDisplayMode();
	void UpdateSceneChange();
	void Animate();
	void ProcessPendingUploadBufferRelease(bool forceWait = false);
	void HandleResize(UINT width, UINT height); // 추가

private:
	// Constants
	static constexpr const TCHAR* kTitlePrefix = _T("D3DX12Demo (");
	static constexpr size_t kTitlePrefixLength = 12;
	static constexpr size_t kFrameRateBufferSize = 50;
	static constexpr size_t kRemain = kFrameRateBufferSize - kTitlePrefixLength;

	// Window / application state
	HINSTANCE hInstance_{};
	HWND hwnd_{};

	bool startFullscreen_ = false;

	bool isMinimized_ = false;

	bool resizePending_ = false;
	UINT pendingResizeWidth_ = 0;
	UINT pendingResizeHeight_ = 0;

	DWORD windowedStyle_ = 0;
	DWORD windowedExStyle_ = 0;

	// Timing / title text
	Timer timer_;
	TCHAR frameRate_[kFrameRateBufferSize]{};

	// Owned systems
	std::unique_ptr<SceneManager> sceneManager_;
	std::unique_ptr<Renderer> renderer_;
	std::unique_ptr<InputSystem> inputSystem_;

	// scene load 완료 후 upload buffer 지연 해제용
	UINT64 pendingSceneLoadFenceValue_ = 0;
	bool hasPendingUploadBufferRelease_ = false;
};