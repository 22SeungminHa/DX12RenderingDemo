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

	bool isFullscreenChanging_ = false;

	std::unique_ptr<SceneManager> sceneManager_;
	std::unique_ptr<Renderer> renderer_;
	std::unique_ptr<InputSystem> inputSystem_;

public:
	GameFramework();
	~GameFramework();

	// 프레임 워크 초기화 함수(주 윈도우 생성 시 호출)
	bool onCreate(HINSTANCE instance, HWND hwnd);
	void onDestroy();

	void onResize();
	void ToggleFullscreen();

	void animate();
	void frameAdvance();
	void processSceneChange();

	//윈도우의 메시지(키보드, 마우스 입력)를 처리하는 함수이다. 
	LRESULT CALLBACK onProcessingWindowMessage(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam);
};