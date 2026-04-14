#pragma once
#include "SceneManager.h"
#include "Renderer.h"

class CInputSystem
{
private:
	CSceneManager* m_pSceneManager = nullptr;
	CRenderer* m_pRenderer = nullptr;
	HWND m_hWnd = nullptr;

public:
	CInputSystem() = default;
	~CInputSystem() = default;

	void Initialize(HWND hWnd, CSceneManager* pSceneManager, CRenderer* pRenderer);

	void ProcessInput();

	bool OnProcessingMouseMessage(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam);
	bool OnProcessingKeyboardMessage(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam);
};