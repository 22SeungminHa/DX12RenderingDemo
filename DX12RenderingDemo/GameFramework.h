#pragma once
#include "Timer.h"
#include "Scene.h"
#include "Camera.h"
#include "D3DCore.h"

class CGameFramework
{
private:
	CGameTimer m_GameTimer;
	_TCHAR m_pszFrameRate[50];

	HINSTANCE m_hInstance;
	HWND m_hWnd;

	std::unique_ptr<CScene> m_pScene;

public:
	CGameFramework();
	~CGameFramework();

	CD3DCore mD3DCore;
	std::unique_ptr<CCamera> m_pCamera;

	bool OnCreate(HINSTANCE hInstance, HWND hMainwnd);
	// 프레임 워크 초기화 함수(주 윈도우 생성 시 호출)

	void OnDestroy();

	void BuildObjects();
	void ReleaseObjects();
	// 렌더링할 메쉬와 게임 객체를 생성하고 소멸하는 함수

	void ProcessInput();
	void AnimateObjects();
	void FrameAdvance();

	void OnProcessingMouseMessage(HWND hWnd, UINT nMessageID, WPARAM wParam, LPARAM lParam);
	void OnProcessingKeyboardMessage(HWND hWnd, UINT nMessageID, WPARAM wParam, LPARAM lParam);
	LRESULT CALLBACK OnProcessingWindowMessage(HWND hWnd, UINT nMessageID, WPARAM wParam, LPARAM lParam);
	//윈도우의 메시지(키보드, 마우스 입력)를 처리하는 함수이다. 
};