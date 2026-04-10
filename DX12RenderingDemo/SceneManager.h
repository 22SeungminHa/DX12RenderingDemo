#pragma once
#include "Scene.h"
#include "Camera.h"

class CSceneManager
{
public:
    CSceneManager();
    ~CSceneManager();

    void SetScene(CScene* pScene);
    CScene* GetScene() const { return mScene; }

    void BuildObjects(ID3D12Device* pd3dDevice, ID3D12GraphicsCommandList* pd3dCommandList);
    void ReleaseObjects();

    void ProcessInput(UCHAR* pKeysBuffer);
    void AnimateObjects(float fTimeElapsed);
    void Render(ID3D12GraphicsCommandList* pd3dCommandList, CCamera* pCamera);

private:
    CScene* mScene = nullptr;
};