#pragma once
#include "Scene.h"
#include "GlassFracture.h"

class AssetManager;

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
    virtual void OnProcessInput(
        const InputSystem& input,
        float deltaTime) override;

    virtual void OnPrepareRenderResources(
        ID3D12Device* device,
        ID3D12GraphicsCommandList* cmdList,
        std::vector<ComPtr<ID3D12Resource>>& transientUploadResources) override;

private:
    std::shared_ptr<Material> glassMaterial_;

    GameObject* glassObject_ = nullptr;

    float glassWidth_ = 10.0f;
    float glassHeight_ = 8.0f;
    float glassDepth_ = 1.0f;

    std::vector<GlassFragmentGeometry> pendingFragmentGeometries_;

    bool breakRequested_ = false;

    bool isBroken_ = false;
};