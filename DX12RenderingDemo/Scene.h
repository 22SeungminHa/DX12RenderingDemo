#pragma once
#include "Shader.h"

enum class SCENE_TYPE
{
	NONE,
	TITLE,
	GAME,
	LOADING,
	TEST1,
	TEST2
};

class Scene {
public:
	Scene() = default;
	virtual ~Scene() = default;

	virtual SCENE_TYPE GetSceneType() const = 0;

	void Load(ID3D12Device* device, ID3D12GraphicsCommandList* cmdList);
	void Unload();
	void ReleaseUploadBuffers();

	//씬에서 마우스와 키보드 메시지를 처리한다.
	virtual void ProcessInput(const UCHAR* keysBuffer);
	virtual void OnProcessingMouseMessage(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam);
	virtual void OnProcessingKeyboardMessage(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam);
	
	virtual void Animate(float deltaTime);
	virtual void Render(ID3D12GraphicsCommandList* cmdList, Camera* camera);

protected:
	//그래픽 루트 시그너쳐를 생성한다.
	ComPtr<ID3D12RootSignature> CreateGraphicsRootSignature(ID3D12Device* device);
	virtual void OnLoad(ID3D12Device* device, ID3D12GraphicsCommandList* cmdList) = 0;
	virtual void OnUnload() {}
	virtual void OnReleaseUploadBuffers() {}

protected:
	//씬은 셰이더들의 집합이다. 셰이더들은 게임 객체들의 집합이다.
	std::vector<std::unique_ptr<GameObject>> objects_;
	ComPtr<ID3D12RootSignature> rootSignature_;
};