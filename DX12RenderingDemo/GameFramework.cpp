#include "pch.h"
#include "GameFramework.h"

CGameFramework::CGameFramework()
{
	m_pSceneManager = std::make_unique<CSceneManager>();
	_tcscpy_s(m_pszFrameRate, _T("D3DX12Demo ("));
}

CGameFramework::~CGameFramework()
{
}

bool CGameFramework::OnCreate(HINSTANCE hInstance, HWND hMainWnd)
{
	m_hInstance = hInstance;
	m_hWnd = hMainWnd;
	//Direct3D 디바이스, 명령 큐와 명령 리스트, 스왑 체인 등을 생성하는 함수를 호출한다. 

	RECT clientRC;
	::GetClientRect(m_hWnd, &clientRC);
	mD3DCore.Initialize(m_hWnd, clientRC.right - clientRC.left, clientRC.bottom - clientRC.top);

	BuildObjects();
	//렌더링할 게임 객체를 생성한다. 

	return(true);
}

void CGameFramework::OnDestroy()
{
	//GPU가 모든 명령 리스트를 실행할 때 까지 기다린다.
	mD3DCore.WaitForGpuComplete();

	//게임 객체(게임 월드 객체)를 소멸한다.
	ReleaseObjects();

	mD3DCore.Shutdown();
}

void CGameFramework::BuildObjects()
{
	mD3DCore.ResetCommandList();

	// 카메라 객체를 생성하여 뷰포트, 씨저 사각형, 투영 변환 행렬, 카메라 변환 행렬을 생성하고 설정한다.
	UINT width = mD3DCore.GetClientWidth();
	UINT height = mD3DCore.GetClientHeight();

	m_pCamera = std::make_unique<CCamera>();
	m_pCamera->SetViewport(0, 0, width, height, 0.0f, 1.0f);
	m_pCamera->SetScissorRect(0, 0, width, height);
	m_pCamera->GenerateProjectionMatrix(1.0f, 500.0f, float(width) / float(height), 90.0f);
	m_pCamera->GenerateViewMatrix(Vector3(0.0f, 15.0f, -25.0f), Vector3(0.0f, 0.0f, 0.0f), Vector3::Up);

	// 씬 객체를 생성하고 씬에 포함될 게임 객체들을 생성한다.
	m_pSceneManager->BuildScene(mD3DCore.GetDevice(), mD3DCore.GetCommandList());

	// 그래픽 명령 리스트를 제출하고 모두 실행될 때까지 기다린다.
	mD3DCore.ExecuteCommandList();
	mD3DCore.WaitForGpuComplete();

	//그래픽 리소스들을 생성하는 과정에 생성된 업로드 버퍼들을 소멸시킨다.
	m_pSceneManager->ReleaseUploadBuffers();
	m_GameTimer.Reset();
}

void CGameFramework::ReleaseObjects()
{
	if (m_pSceneManager)
		m_pSceneManager->ReleaseScene();
}

void CGameFramework::OnProcessingMouseMessage(HWND hWnd, UINT nMessageID, WPARAM wParam,
	LPARAM lParam)
{
	switch (nMessageID)
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

void CGameFramework::OnProcessingKeyboardMessage(HWND hWnd, UINT nMessageID, WPARAM
	wParam, LPARAM lParam)
{
	switch (nMessageID)
	{
	case WM_KEYUP:
		switch (wParam)
		{
		case VK_ESCAPE:
			::PostQuitMessage(0);
			break;
		case VK_RETURN:
			break;
		case VK_F8:
			break;
		case VK_F9:
			mD3DCore.ChangeSwapChainState();
			break;
		default:
			break;
		}
		break;
	default:
		break;
	}
}

LRESULT CALLBACK CGameFramework::OnProcessingWindowMessage(HWND hWnd, UINT nMessageID,
	WPARAM wParam, LPARAM lParam)
{
	switch (nMessageID)
	{
	case WM_SIZE:
	{
		mD3DCore.Resize(LOWORD(lParam), HIWORD(lParam));
		break;
	}
	case WM_LBUTTONDOWN:
	case WM_RBUTTONDOWN:
	case WM_LBUTTONUP:
	case WM_RBUTTONUP:
	case WM_MOUSEMOVE:
		OnProcessingMouseMessage(hWnd, nMessageID, wParam, lParam);
		break;
	case WM_KEYDOWN:
	case WM_KEYUP:
		OnProcessingKeyboardMessage(hWnd, nMessageID, wParam, lParam);
		break;
	}
	return(0);
}

void CGameFramework::ProcessInput()
{
}

void CGameFramework::Animate()
{
	if (m_pSceneManager)
		m_pSceneManager->Animate(m_GameTimer.GetTimeElapsed());
}

void CGameFramework::FrameAdvance()
{
	m_GameTimer.Tick(0.0f);

	ProcessInput();
	Animate();

	float clearColor[4] = { 0.0f, 0.125f, 0.3f, 1.0f };

	mD3DCore.ResetCommandList();
	mD3DCore.BeginRender(clearColor);

	if (m_pSceneManager)
		m_pSceneManager->Render(mD3DCore.GetCommandList(), m_pCamera.get());

	mD3DCore.EndRender();
	mD3DCore.ExecuteCommandList();
	mD3DCore.Present(0, 0);
	mD3DCore.MoveToNextFrame();

	m_GameTimer.GetFrameRate(m_pszFrameRate + 12, 37);
	::SetWindowText(m_hWnd, m_pszFrameRate);
}
