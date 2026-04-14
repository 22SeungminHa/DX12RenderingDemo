#pragma once
#include "D3DCore.h"
#include "Camera.h"
#include "SceneManager.h"

class CRenderer {
private:
	CD3DCore mD3DCore;
	std::unique_ptr<CCamera> mCamera;

public:
	CRenderer();
	~CRenderer();

	bool Initialize(HWND hWnd, UINT width, UINT height);
	void Shutdown();

	void Resize(UINT width, UINT height);

	void CreateCamera(UINT width, UINT height);
	void Render(CSceneManager* sceneManager);

	void ProcessSceneChange(CSceneManager* sceneManager);

	void ChangeSwapChainState();
	void WaitForGpuComplete();

	ID3D12Device* GetDevice() const { return mD3DCore.GetDevice(); }
	ID3D12GraphicsCommandList* GetCommandList() const { return mD3DCore.GetCommandList(); }
	CCamera* GetCamera() const { return mCamera.get(); }
};

