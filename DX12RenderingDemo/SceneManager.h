#pragma once
#include "Scene.h"

class SceneManager {
private:
    std::unique_ptr<Scene> currentScene_;
    SCENE_TYPE currentSceneType_ = SCENE_TYPE::NONE;
    SCENE_TYPE nextSceneType_ = SCENE_TYPE::NONE;
    bool sceneChangeRequested_ = false;

    void CreateScene(SCENE_TYPE sceneType, ID3D12Device* device, ID3D12GraphicsCommandList* cmdList);

public:
    SceneManager() = default;
    ~SceneManager() = default;

    Scene* GetScene() const { return currentScene_.get(); }
    SCENE_TYPE GetSceneType() const { return currentSceneType_; }
    bool HasSceneChange() const { return sceneChangeRequested_; }

    void RequestChangeScene(SCENE_TYPE nextScene);
    void ProcessSceneChange(ID3D12Device* device, ID3D12GraphicsCommandList* cmdList);

    void ReleaseScene();
    void ReleaseUploadBuffers();

    void ProcessInput(const UCHAR* keysBuffer);
    void OnProcessingKeyboardMessage(HWND hWnd, UINT messageID, WPARAM wParam, LPARAM lParam);
    void OnProcessingMouseMessage(HWND hWnd, UINT messageID, WPARAM wParam, LPARAM lParam);

    void Animate(float deltaTime);
    void Render(ID3D12GraphicsCommandList* cmdList, Camera* camera);

private:
    std::unique_ptr<Scene> CreateSceneByType(SCENE_TYPE sceneType);
};