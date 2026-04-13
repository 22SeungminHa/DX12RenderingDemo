#pragma once
#include "Scene.h"
#include "Camera.h"

class CSceneManager
{
private:
    std::unique_ptr<CScene> m_pCurrentScene;

public:
    CSceneManager();
    ~CSceneManager();

    CScene* GetScene() const { return m_pCurrentScene.get(); }

    void BuildScene(ID3D12Device* pd3dDevice, ID3D12GraphicsCommandList* pd3dCommandList);
    void ReleaseScene();
    void ReleaseUploadBuffers();

    void Animate(float fTimeElapsed);
    void Render(ID3D12GraphicsCommandList* pd3dCommandList, CCamera* pCamera);
    
    void ProcessInput(UCHAR* pKeysBuffer);
};