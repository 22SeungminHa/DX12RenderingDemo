#include "InputSystem.h"

void CInputSystem::Initialize(HWND hWnd, CSceneManager* pSceneManager, CRenderer* pRenderer)
{
	m_hWnd = hWnd;
	m_pSceneManager = pSceneManager;
	m_pRenderer = pRenderer;
}

void CInputSystem::ProcessInput()
{
	if (!m_pSceneManager) return;

	UCHAR keysBuffer[256] = {};
	::GetKeyboardState(keysBuffer);

	m_pSceneManager->ProcessInput(keysBuffer);
}

bool CInputSystem::OnProcessingMouseMessage(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam)
{
	if (m_pSceneManager && m_pSceneManager->OnProcessingMouseMessage(hWnd, msg, wParam, lParam))
		return true;

	switch (msg) {
	case WM_LBUTTONDOWN:
	case WM_RBUTTONDOWN:
		break;
	case WM_LBUTTONUP:
	case WM_RBUTTONUP:
		break;
	case WM_MOUSEMOVE:
		break;
	default:
		break;
	}

	return false;
}

bool CInputSystem::OnProcessingKeyboardMessage(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam)
{
	if (m_pSceneManager && m_pSceneManager->OnProcessingKeyboardMessage(hWnd, msg, wParam, lParam))
		return true;

	switch (msg) {
	case WM_KEYUP:
		switch (wParam) {
		case VK_ESCAPE:
			::PostQuitMessage(0);
			return true;
		case VK_SPACE:
			if (!m_pSceneManager) break;

			if (m_pSceneManager->GetSceneType() == SCENE_TYPE::TEST1) {
				m_pSceneManager->RequestChangeScene(SCENE_TYPE::TEST2);
				return true;
			}
			else if (m_pSceneManager->GetSceneType() == SCENE_TYPE::TEST2) {
				m_pSceneManager->RequestChangeScene(SCENE_TYPE::TEST1);
				return true;
			}
			break;
		case VK_F9:
			if (m_pRenderer) {
				m_pRenderer->ChangeSwapChainState();
				return true;
			}
			break;
		default:
			break;
		}
		break;
	default:
		break;
	}

	return false;
}