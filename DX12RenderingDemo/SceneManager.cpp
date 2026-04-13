#include "SceneManager.h"

CSceneManager::CSceneManager()
{
}

CSceneManager::~CSceneManager()
{
	ReleaseScene();
}

void CSceneManager::BuildScene(ID3D12Device* device, ID3D12GraphicsCommandList* cmdList)
{
	m_pCurrentScene = std::make_unique<CScene>();
	m_pCurrentScene->BuildObjects(device, cmdList);
}

void CSceneManager::ReleaseScene()
{
	if (m_pCurrentScene) {
		m_pCurrentScene->ReleaseObjects();
		m_pCurrentScene.reset();
	}
}

void CSceneManager::ReleaseUploadBuffers()
{
	if (m_pCurrentScene)
		m_pCurrentScene->ReleaseUploadBuffers();
}

void CSceneManager::Animate(float deltaTime)
{
	if (m_pCurrentScene)
		m_pCurrentScene->Animate(deltaTime);
}

void CSceneManager::Render(ID3D12GraphicsCommandList* cmdList, CCamera* camera)
{
	if (m_pCurrentScene)
		m_pCurrentScene->Render(cmdList, camera);
}