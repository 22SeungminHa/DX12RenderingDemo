#include "pch.h"
#include "GameFramework.h"

CGameFramework::CGameFramework()
{
	m_pSceneManager = std::make_unique<CSceneManager>();
	m_pRenderer = std::make_unique<CRenderer>();

	_tcscpy_s(m_pszFrameRate, _T("D3DX12Demo ("));
}

CGameFramework::~CGameFramework()
{
}

bool CGameFramework::OnCreate(HINSTANCE hInstance, HWND hMainWnd)
{
	//Direct3D 디바이스, 명령 큐와 명령 리스트, 스왑 체인 등을 생성하는 함수를 호출한다. 
	m_hInstance = hInstance;
	m_hWnd = hMainWnd;

	RECT clientRC;
	::GetClientRect(m_hWnd, &clientRC);

	// 렌더러 초기화
	m_pRenderer->Initialize(m_hWnd, clientRC.right - clientRC.left, clientRC.bottom - clientRC.top);

	// 시작 씬 전환 요청. FrameAdvance()에서 생성한다.
	m_pSceneManager->RequestChangeScene(SCENE_TYPE::TEST1);
	m_GameTimer.Reset();

	return true;
}

void CGameFramework::OnDestroy()
{
	//GPU가 모든 명령 리스트를 실행할 때 까지 기다린다.
	if (m_pRenderer) m_pRenderer->WaitForGpuComplete();

	// 씬과 씬 매니저를 unload 한다. 게임 객체(게임 월드 객체)를 소멸한다.
	if (m_pSceneManager) m_pSceneManager->ReleaseScene();

	if (m_pRenderer) m_pRenderer->Shutdown();
}

void CGameFramework::OnResize()
{
	RECT rc;
	::GetClientRect(m_hWnd, &rc);

	UINT width = rc.right - rc.left;
	UINT height = rc.bottom - rc.top;

	if (m_pRenderer) m_pRenderer->Resize(width, height);
}

void CGameFramework::OnProcessingMouseMessage(HWND hWnd, UINT msg, WPARAM wParam,
	LPARAM lParam)
{
	if (m_pSceneManager && m_pSceneManager->OnProcessingMouseMessage(hWnd, msg, wParam, lParam))
		return;
	
	switch (msg)
	{
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
}

void CGameFramework::OnProcessingKeyboardMessage(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam)
{
	if (m_pSceneManager && m_pSceneManager->OnProcessingKeyboardMessage(hWnd, msg, wParam, lParam))
		return;

	switch (msg) {
	case WM_KEYUP:
		switch (wParam) {
		case VK_ESCAPE:
			::PostQuitMessage(0);
			break;
		case VK_SPACE:
			if (m_pSceneManager->GetSceneType() == SCENE_TYPE::TEST1)
				m_pSceneManager->RequestChangeScene(SCENE_TYPE::TEST2);
			else
				m_pSceneManager->RequestChangeScene(SCENE_TYPE::TEST1);
			break;
		case VK_F8:
			break;
		case VK_F9:
			if (m_pRenderer) m_pRenderer->ChangeSwapChainState();
			break;
		default:
			break;
		}
		break;
	default:
		break;
	}
}

LRESULT CALLBACK CGameFramework::OnProcessingWindowMessage(HWND hWnd, UINT msg,
	WPARAM wParam, LPARAM lParam)
{	
	switch (msg) {
	case WM_SIZE: {
		OnResize();
		return 0;
	}
	case WM_LBUTTONDOWN:
	case WM_RBUTTONDOWN:
	case WM_LBUTTONUP:
	case WM_RBUTTONUP:
	case WM_MOUSEMOVE:
		OnProcessingMouseMessage(hWnd, msg, wParam, lParam);
		return 0;
	case WM_KEYDOWN:
	case WM_KEYUP:
		OnProcessingKeyboardMessage(hWnd, msg, wParam, lParam);
		return 0;
	}
	return DefWindowProc(hWnd, msg, wParam, lParam);
}

void CGameFramework::ProcessInput()
{
	if (!m_pSceneManager) return;

	UCHAR keysBuffer[256];
	::GetKeyboardState(keysBuffer);

	m_pSceneManager->ProcessInput(keysBuffer);
}

void CGameFramework::Animate()
{
	if (m_pSceneManager) m_pSceneManager->Animate(m_GameTimer.GetTimeElapsed());
}

void CGameFramework::ProcessSceneChange()
{
	if (!m_pRenderer || !m_pSceneManager || !m_pSceneManager->HasSceneChange())
		return;

	m_pRenderer->BeginSceneLoad();
	m_pSceneManager->ProcessSceneChange(m_pRenderer->GetDevice(), m_pRenderer->GetCommandList());
	m_pRenderer->EndSceneLoad();

	m_pSceneManager->ReleaseUploadBuffers();
}

void CGameFramework::FrameAdvance()
{
	m_GameTimer.Tick(0.0f);

	ProcessSceneChange();
	ProcessInput();
	Animate();

	if (m_pRenderer && m_pSceneManager) m_pRenderer->Render(m_pSceneManager->GetScene());

	m_GameTimer.GetFrameRate(m_pszFrameRate + 12, 37);
	::SetWindowText(m_hWnd, m_pszFrameRate);
}
