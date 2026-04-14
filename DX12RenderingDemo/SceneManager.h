#pragma once
#include "Scene.h"

class CSceneManager {
private:
    std::unique_ptr<CScene> mCurrentScene;
    SCENE_TYPE mCurrentSceneType = SCENE_TYPE::NONE;
    SCENE_TYPE mNextSceneType = SCENE_TYPE::NONE;
    bool mSceneChangeRequested = false;

    void CreateScene(SCENE_TYPE sceneType, ID3D12Device* device, ID3D12GraphicsCommandList* cmdList);

public:
    CSceneManager() = default;
    ~CSceneManager() = default;

    CScene* GetScene() const { return mCurrentScene.get(); }
    SCENE_TYPE GetSceneType() const { return mCurrentSceneType; }
    bool HasSceneChange() const { return mSceneChangeRequested; }

    void RequestChangeScene(SCENE_TYPE nextScene);
    void ProcessSceneChange(ID3D12Device* device, ID3D12GraphicsCommandList* cmdList);

    void ReleaseScene();
    void ReleaseUploadBuffers();

    bool ProcessInput(const UCHAR* keysBuffer);
    bool OnProcessingKeyboardMessage(HWND hWnd, UINT messageID, WPARAM wParam, LPARAM lParam);
    bool OnProcessingMouseMessage(HWND hWnd, UINT messageID, WPARAM wParam, LPARAM lParam);

    void Animate(float deltaTime);
    void Render(ID3D12GraphicsCommandList* cmdList, CCamera* camera);

private:
    std::unique_ptr<CScene> CreateSceneByType(SCENE_TYPE sceneType);
};