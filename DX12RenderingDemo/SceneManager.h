#pragma once
#include "Scene.h"
#include "Camera.h"

class CSceneManager
{
private:
    std::unique_ptr<CScene> mCurrentScene;
    SCENE_TYPE mCurrentSceneType = SCENE_TYPE::TEST1;
    bool mSceneChangeRequested = false;
    SCENE_TYPE mNextSceneType = SCENE_TYPE::TEST2;

public:
    CSceneManager();
    ~CSceneManager();

    CScene* GetScene() const { return mCurrentScene.get(); }
    SCENE_TYPE GetSceneType() const { return mCurrentSceneType; }
    bool HasSceneChange() const { return mSceneChangeRequested; }

    void CreateScene(SCENE_TYPE sceneType, ID3D12Device* device, ID3D12GraphicsCommandList* cmdList);
    void RequestChangeScene(SCENE_TYPE nextScene);
    void ProcessSceneChange(ID3D12Device* device, ID3D12GraphicsCommandList* cmdList);

    void ReleaseScene();
    void ReleaseUploadBuffers();

    bool ProcessInput(UCHAR* keysBuffer);
    bool OnProcessingKeyboardMessage(HWND hWnd, UINT messageID, WPARAM wParam, LPARAM lParam);
    bool OnProcessingMouseMessage(HWND hWnd, UINT messageID, WPARAM wParam, LPARAM lParam);

    void Animate(float deltaTime);
    void Render(ID3D12GraphicsCommandList* cmdList, CCamera* camera);

private:
    std::unique_ptr<CScene> CreateSceneByType(SCENE_TYPE sceneType);
};