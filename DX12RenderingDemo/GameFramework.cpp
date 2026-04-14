#include "pch.h"
#include "GameFramework.h"

CGameFramework::CGameFramework()
{
	m_pSceneManager = std::make_unique<CSceneManager>();
	m_pRenderer = std::make_unique<CRenderer>();
	m_pInputSystem = std::make_unique<CInputSystem>();

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

	m_pInputSystem->Initialize(m_hWnd, m_pSceneManager.get(), m_pRenderer.get());

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

LRESULT CALLBACK CGameFramework::OnProcessingWindowMessage(HWND hWnd, UINT msg,
	WPARAM wParam, LPARAM lParam)
{	
	switch (msg) {
	case WM_SIZE:
		OnResize();
		return 0;
	case WM_LBUTTONDOWN:
	case WM_RBUTTONDOWN:
	case WM_LBUTTONUP:
	case WM_RBUTTONUP:
	case WM_MOUSEMOVE:
		if (m_pInputSystem && m_pInputSystem->OnProcessingMouseMessage(hWnd, msg, wParam, lParam))
			return 0;
		break;
	case WM_KEYDOWN:
	case WM_KEYUP:
		if (m_pInputSystem && m_pInputSystem->OnProcessingKeyboardMessage(hWnd, msg, wParam, lParam))
			return 0;
		break;
	}

	return DefWindowProc(hWnd, msg, wParam, lParam);
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

	if (m_pInputSystem) m_pInputSystem->ProcessInput();

	Animate();

	if (m_pRenderer && m_pSceneManager) m_pRenderer->Render(m_pSceneManager->GetScene());

	m_GameTimer.GetFrameRate(m_pszFrameRate + 12, 37);
	::SetWindowText(m_hWnd, m_pszFrameRate);
}
