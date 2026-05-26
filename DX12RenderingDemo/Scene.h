#pragma once
#include "EngineTypes.h"

class Renderer;
class InputSystem;
class AssetManager;
class GameObject;
class Camera;

class Scene {
public:
	Scene(UINT width, UINT height);
	virtual ~Scene();

	virtual SCENE_TYPE GetSceneType() const = 0;

	Camera* GetActiveCamera() const { return activeCamera_; }
	const std::vector<std::unique_ptr<GameObject>>& GetObjects() const { return objects_; }

	void Load(
		ID3D12Device* device,
		ID3D12GraphicsCommandList* cmdList,
		ID3D12RootSignature* rootSignature,
		AssetManager& assetManager);
	void Unload();
	void Resize(UINT width, UINT height);
	void ReleaseUploadResources();

	//씬에서 마우스와 키보드 메시지를 처리한다.
	virtual void ProcessInput(const InputSystem& input, float deltaTime);
	virtual void Animate(float deltaTime);
	virtual void AnimateObject(GameObject* object, float deltaTime);

	void SetSkybox(const SkyboxDesc& skybox) { skybox_ = skybox; }
	const SkyboxDesc& GetSkybox() const { return skybox_; }

protected:
	virtual void OnLoad(
		ID3D12Device* device,
		ID3D12GraphicsCommandList* cmdList,
		ID3D12RootSignature* rootSignature,
		AssetManager& assetManager) = 0;
	virtual void OnUnload() {}
	virtual void OnResize(UINT width, UINT height) {}
	virtual void OnReleaseUploadResources() {}
	virtual void OnProcessInput(const InputSystem& input, float deltaTime) {};

	virtual CameraDesc SetupCameraDesc() const { return CameraDesc{}; }
	void CreateCamera();
	void ResizeCamera(UINT width, UINT height);

protected:
	std::vector<std::unique_ptr<GameObject>> objects_;

	std::vector<std::unique_ptr<Camera>> cameras_;
	Camera* activeCamera_ = nullptr;

	SkyboxDesc skybox_;

	UINT clientWidth_ = 0;
	UINT clientHeight_ = 0;
};