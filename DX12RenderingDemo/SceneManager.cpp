#include "SceneManager.h"

void CSceneManager::BuildObjects(ID3D12Device* D3DDevice, ID3D12GraphicsCommandList* CommandList)
{
	//씬 객체를 생성하고 씬에 포함될 게임 객체들을 생성한다.
	mScene = new CScene();
	mScene->BuildObjects(D3DDevice, CommandList);

	//그래픽 리소스들을 생성하는 과정에 생성된 업로드 버퍼들을 소멸시킨다.
	if (mScene) mScene->ReleaseUploadBuffers();
}

void CSceneManager::ReleaseObjects()
{
	if (mScene) mScene->ReleaseObjects();
	if (mScene) delete mScene;
}