#pragma once
#include "Scene.h"

class AssetManager;

class TestScene : public Scene {
public:
    TestScene(UINT width, UINT height) : Scene(width, height) {};
    virtual ~TestScene() = default;

    virtual SCENE_TYPE GetSceneType() const override { return SCENE_TYPE::TEST; }
    virtual void OnLoad(
        ID3D12Device* device,
        ID3D12GraphicsCommandList* cmdList,
        ID3D12RootSignature* rootSignature,
        AssetManager& assetManager) override;
    virtual CameraDesc SetupCameraDesc() const override;
};