#pragma once
#include "Scene.h"

class SceneManager
{
public:
    SceneManager() = default;
    ~SceneManager() = default;

public:
    Scene* GetCurrentScene() const { return currentScene_.get(); }
    bool HasSceneChange() const { return sceneChangeRequested_; }

    void RequestChangeScene(SCENE_TYPE nextScene);
    void ProcessSceneChange(ID3D12Device* device, ID3D12GraphicsCommandList* cmdList, UINT width, UINT height);

    void ReleaseCurrentScene();
    void ReleaseCurrentSceneUploadBuffers();

    void ProcessInput(const UCHAR* keysBuffer);
    void OnProcessingKeyboardMessage(HWND hWnd, UINT messageID, WPARAM wParam, LPARAM lParam);
    void OnProcessingMouseMessage(HWND hWnd, UINT messageID, WPARAM wParam, LPARAM lParam);

    void ResizeCurrentScene(UINT width, UINT height);

private:
    void CreateScene(SCENE_TYPE sceneType, ID3D12Device* device, ID3D12GraphicsCommandList* cmdList, UINT width, UINT height);
    std::unique_ptr<Scene> CreateSceneByType(SCENE_TYPE sceneType, UINT width, UINT height);

private:
    std::unique_ptr<Scene> currentScene_;
    SCENE_TYPE currentSceneType_ = SCENE_TYPE::NONE;
    SCENE_TYPE nextSceneType_ = SCENE_TYPE::NONE;

    bool sceneChangeRequested_ = false;
};