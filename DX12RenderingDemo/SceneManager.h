#pragma once
#include "Scene.h"

class AssetManager;

class SceneManager
{
public:
    SceneManager() = default;
    ~SceneManager() = default;

public:
    Scene* GetCurrentScene() const { return currentScene_.get(); }
    SceneType GetCurrentSceneType() const { return currentScene_ ? currentScene_->GetSceneType() : SceneType::None; }
    bool HasSceneChange() const { return sceneChangeRequested_; }

    void RequestChangeScene(SceneType nextScene);
    void ProcessSceneChange(
        ID3D12Device* device,
        ID3D12GraphicsCommandList* cmdList,
        ID3D12RootSignature* rootSignature,
        AssetManager& assetManager,
        UINT width,
        UINT height);

    void ReleaseCurrentScene();
    void ReleaseCurrentSceneUploadResources();
    void ResizeCurrentScene(UINT width, UINT height);

private:
    void CreateScene(
        SceneType sceneType,
        ID3D12Device* device,
        ID3D12GraphicsCommandList* cmdList,
        ID3D12RootSignature* rootSignature,
        AssetManager& assetManager,
        UINT width,
        UINT height);
    std::unique_ptr<Scene> CreateSceneByType(SceneType sceneType, UINT width, UINT height);

private:
    std::unique_ptr<Scene> currentScene_;
    SceneType currentSceneType_ = SceneType::None;
    SceneType nextSceneType_ = SceneType::None;

    bool sceneChangeRequested_ = false;
};