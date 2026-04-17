#pragma once
#include "D3DCore.h"
#include "Camera.h"

class Scene;

class Renderer {
private:
	D3DCore d3dCore_;
	std::unique_ptr<Camera> camera_;

public:
	Renderer();
	~Renderer();

	void Initialize(HWND hwnd, UINT width, UINT height);
	void Shutdown();

	void CreateCamera(UINT width, UINT height);

	void BeginSceneLoad();
	void EndSceneLoad();

	void Render(Scene* scene);

	void WaitForGpuComplete();

	ID3D12Device* GetDevice() const { return d3dCore_.GetDevice(); }
	ID3D12GraphicsCommandList* GetCommandList() const { return d3dCore_.GetCommandList(); }
	Camera* GetCamera() const { return camera_.get(); }
};

