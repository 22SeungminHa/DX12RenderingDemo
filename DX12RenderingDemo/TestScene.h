#pragma once
#include "Scene.h"

class AssetManager;
class Mesh;
class Material;

enum class CameraControlMode
{
    AutoForward,
    Free
};

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
    virtual SceneLightDesc SetupLightDesc() const override;

    virtual void Animate(float deltaTime) override;

protected:
    virtual void OnProcessInput(
        const InputSystem& input,
        float deltaTime) override;

    virtual void OnPrepareRenderResources(
        ID3D12Device* device,
        ID3D12GraphicsCommandList* cmdList,
        std::vector<ComPtr<ID3D12Resource>>& transientUploadResources) override;

    virtual bool IsFreeCameraControlEnabled() const override
    {
        return cameraMode_ == CameraControlMode::Free;
    }

private:
    void LoadObstacles(
        ID3D12Device* device,
        ID3D12GraphicsCommandList* cmdList,
        ID3D12RootSignature* rootSignature,
        AssetManager& assetManager);

    void LoadCrystals(
        ID3D12Device* device,
        ID3D12GraphicsCommandList* cmdList,
        ID3D12RootSignature* rootSignature,
        AssetManager& assetManager);

    void LoadMap(
        ID3D12Device* device,
        ID3D12GraphicsCommandList* cmdList,
        ID3D12RootSignature* rootSignature,
        AssetManager& assetManager);

    void FireProjectile(const Vector2& screenPosition);

    void CheckProjectileCollisions();

    void RemoveObjectsBehindCamera();

    void ToggleCameraMode();
    void UpdateAutoCamera(float deltaTime);
    void RestoreAutoCamera();

private:
    std::shared_ptr<Mesh> projectileMesh_;
    std::shared_ptr<Material> projectileMaterial_;

    CameraControlMode cameraMode_ =
        CameraControlMode::AutoForward;

    float autoCameraSpeed_ = 5.0f;

    Vector3 autoCameraPosition_ =
        Vector3::Zero;

    Vector3 autoCameraForward_ =
        Vector3(0.0f, 0.0f, 1.0f);
};