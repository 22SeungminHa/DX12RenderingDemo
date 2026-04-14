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

class CScene {
public:
	CScene() = default;
	virtual ~CScene() = default;

	virtual SCENE_TYPE GetSceneType() const = 0;

	void Load(ID3D12Device* device, ID3D12GraphicsCommandList* cmdList);
	void Unload();
	void ReleaseUploadBuffers();

	//씬에서 마우스와 키보드 메시지를 처리한다.
	virtual bool ProcessInput(const UCHAR* keysBuffer);
	virtual bool OnProcessingMouseMessage(HWND hWnd, UINT messageID, WPARAM wParam, LPARAM lParam);
	virtual bool OnProcessingKeyboardMessage(HWND hWnd, UINT messageID, WPARAM wParam, LPARAM lParam);
	
	virtual void Animate(float deltaTime);
	virtual void Render(ID3D12GraphicsCommandList* cmdList, CCamera* camera);

protected:
	//그래픽 루트 시그너쳐를 생성한다.
	ComPtr<ID3D12RootSignature> CreateGraphicsRootSignature(ID3D12Device* device);
	virtual void OnLoad(ID3D12Device* device, ID3D12GraphicsCommandList* cmdList) = 0;
	virtual void OnUnload() {}
	virtual void OnReleaseUploadBuffers() {}

protected:
	//씬은 셰이더들의 집합이다. 셰이더들은 게임 객체들의 집합이다.
	std::vector<std::unique_ptr<CGameObject>> mObjects;
	ComPtr<ID3D12RootSignature> mGraphicsRootSignature;
};