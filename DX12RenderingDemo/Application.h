#pragma once
#include "Timer.h"
#include "Scene.h"
#include "SceneManager.h"
#include "Renderer.h"
#include "InputSystem.h"

class Application {
private:
	static constexpr const TCHAR* TitlePrefix = _T("D3DX12Demo (");

	const size_t prefixLen = _tcslen(TitlePrefix);
	const size_t remain = _countof(frameRate_) - prefixLen;

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

	struct SceneLoadGuard {
		Renderer* renderer = nullptr;

		explicit SceneLoadGuard(Renderer* r) : renderer(r) {}
		~SceneLoadGuard() { if (renderer) renderer->EndSceneLoad(); }
		SceneLoadGuard(const SceneLoadGuard&) = delete;
		SceneLoadGuard& operator=(const SceneLoadGuard&) = delete;
	};

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