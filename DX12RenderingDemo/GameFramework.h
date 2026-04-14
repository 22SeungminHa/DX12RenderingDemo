#pragma once
#include "Timer.h"
#include "Scene.h"
#include "Camera.h"
#include "D3DCore.h"
#include "SceneManager.h"
#include "Renderer.h"

class CGameFramework {
private:
	CGameTimer m_GameTimer;
	_TCHAR m_pszFrameRate[50];

	HINSTANCE m_hInstance;
	HWND m_hWnd;

	std::unique_ptr<CSceneManager> m_pSceneManager;
	std::unique_ptr<CRenderer> m_pRenderer;

public:
	CGameFramework();
	~CGameFramework();

	// 프레임 워크 초기화 함수(주 윈도우 생성 시 호출)
	bool OnCreate(HINSTANCE hInstance, HWND hMainwnd);
	void OnDestroy();
	void OnResize();

	void ProcessInput();
	void Animate();
	void FrameAdvance();

	void ProcessSceneChange();

	//윈도우의 메시지(키보드, 마우스 입력)를 처리하는 함수이다. 
	void OnProcessingMouseMessage(HWND hWnd, UINT nMessageID, WPARAM wParam, LPARAM lParam);
	void OnProcessingKeyboardMessage(HWND hWnd, UINT nMessageID, WPARAM wParam, LPARAM lParam);
	LRESULT CALLBACK OnProcessingWindowMessage(HWND hWnd, UINT nMessageID, WPARAM wParam, LPARAM lParam);
};