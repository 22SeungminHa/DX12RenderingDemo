#pragma once
#include "Scene.h"

class AssetManager;
class GameObject;
class InputSystem;

class TestScene : public Scene {
public:
    TestScene(UINT width, UINT height) : Scene(width, height) {};
    virtual ~TestScene() = default;

    virtual SCENE_TYPE GetSceneType() const override { return SCENE_TYPE::Test; }
    virtual void OnLoad(
        ID3D12Device* device,
        ID3D12GraphicsCommandList* cmdList,
        ID3D12RootSignature* rootSignature,
        AssetManager& assetManager) override;

    virtual CameraDesc SetupCameraDesc() const override;

protected:
    virtual void OnProcessInput(const InputSystem& input, float deltaTime) override;

private:
    bool RaycastGlass(const Vector3& rayOrigin, const Vector3& rayDirection, Vector3& localHitPoint) const;

private:
    GameObject* glassObject_ = nullptr;
};