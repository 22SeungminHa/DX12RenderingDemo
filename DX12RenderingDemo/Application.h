#pragma once
#include "Timer.h"
#include "Scene.h"
#include "SceneManager.h"
#include "Renderer.h"
#include "InputSystem.h"

class Application {
private:
	static constexpr const TCHAR* TitlePrefix = _T("D3DX12Demo (");
	static constexpr size_t kFrameRateBufferSize = 50;
	TCHAR frameRate_[kFrameRateBufferSize]{};

	const size_t prefixLen = _tcslen(TitlePrefix);
	const size_t remain = kFrameRateBufferSize - prefixLen;

	Timer timer_;

	HINSTANCE instance_{};
	HWND hwnd_{};

	// 시작 모드만 결정하는 변수
	bool startFullscreen_ = false;

	DWORD windowedStyle_ = 0;
	DWORD windowedExStyle_ = 0;

	std::unique_ptr<SceneManager> sceneManager_;
	std::unique_ptr<Renderer> renderer_;
	std::unique_ptr<InputSystem> inputSystem_;

private:
	void ApplyStartupDisplayMode();

public:
	Application();
	~Application();

	bool OnCreate(HINSTANCE instance, HWND hwnd);
	void OnDestroy();

	void Animate();
	void FrameAdvance();
	void ProcessSceneChange();

	LRESULT CALLBACK OnProcessingMessage(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam);
};