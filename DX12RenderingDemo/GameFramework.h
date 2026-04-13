#pragma once
#include "Timer.h"
#include "Scene.h"
#include "Camera.h"
#include "D3DCore.h"
#include "SceneManager.h"

class CGameFramework {
private:
	CGameTimer m_GameTimer;
	_TCHAR m_pszFrameRate[50];

	HINSTANCE m_hInstance;
	HWND m_hWnd;

	CD3DCore mD3DCore;
	std::unique_ptr<CSceneManager> m_pSceneManager;

public:
	CGameFramework();
	~CGameFramework();

	std::unique_ptr<CCamera> m_pCamera;

	// 프레임 워크 초기화 함수(주 윈도우 생성 시 호출)
	bool OnCreate(HINSTANCE hInstance, HWND hMainwnd);
	void OnDestroy();
	void OnResize();

	// 렌더링할 메쉬와 게임 객체를 생성하고 소멸하는 함수
	void BuildObjects();
	void ReleaseObjects();

	void ProcessInput();
	void Animate();
	void FrameAdvance();

	void ProcessSceneChange();

	//윈도우의 메시지(키보드, 마우스 입력)를 처리하는 함수이다. 
	void OnProcessingMouseMessage(HWND hWnd, UINT nMessageID, WPARAM wParam, LPARAM lParam);
	void OnProcessingKeyboardMessage(HWND hWnd, UINT nMessageID, WPARAM wParam, LPARAM lParam);
	LRESULT CALLBACK OnProcessingWindowMessage(HWND hWnd, UINT nMessageID, WPARAM wParam, LPARAM lParam);
};