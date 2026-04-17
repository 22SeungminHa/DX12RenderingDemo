#pragma once
#include "Timer.h"
#include "Scene.h"
#include "SceneManager.h"
#include "Renderer.h"
#include "InputSystem.h"

class GameFramework {
private:
	Timer timer_;
	TCHAR frameRate_[50];

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
	GameFramework();
	~GameFramework();

	bool onCreate(HINSTANCE instance, HWND hwnd);
	void onDestroy();

	void animate();
	void frameAdvance();
	void processSceneChange();

	LRESULT CALLBACK onProcessingWindowMessage(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam);
};