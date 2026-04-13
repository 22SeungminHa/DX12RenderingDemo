#include "SceneManager.h"
#include "TestScene.h"

CSceneManager::CSceneManager()
{
}

CSceneManager::~CSceneManager()
{
	ReleaseScene();
}

std::unique_ptr<CScene> CSceneManager::CreateSceneByType(SCENE_TYPE sceneType)
{
	switch (sceneType) {
	case SCENE_TYPE::TEST1:
		return std::make_unique<CTestScene1>();
	case SCENE_TYPE::TEST2:
		return std::make_unique<CTestScene2>();
	default:
		return nullptr;
	}
}

void CSceneManager::CreateScene(SCENE_TYPE sceneType, ID3D12Device* device, ID3D12GraphicsCommandList* cmdList)
{
	ReleaseScene();

	mCurrentScene = CreateSceneByType(sceneType);

	if (mCurrentScene) {
		mCurrentSceneType = sceneType;
		mCurrentScene->BuildObjects(device, cmdList);
	}
}

void CSceneManager::RequestChangeScene(SCENE_TYPE nextScene)
{
	if (nextScene == mCurrentSceneType)
		return;

	mSceneChangeRequested = true;
	mNextSceneType = nextScene;
}

void CSceneManager::ProcessSceneChange(ID3D12Device* device, ID3D12GraphicsCommandList* cmdList)
{
	if (!mSceneChangeRequested) return;

	ReleaseScene();
	CreateScene(mNextSceneType, device, cmdList);
	mSceneChangeRequested = false;
}

void CSceneManager::ReleaseScene()
{
	if (mCurrentScene) {
		mCurrentScene->ReleaseObjects();
		mCurrentScene.reset();
	}
}

void CSceneManager::ReleaseUploadBuffers()
{
	if (mCurrentScene) mCurrentScene->ReleaseUploadBuffers();
}

bool CSceneManager::ProcessInput(UCHAR* keysBuffer)
{
	if (mCurrentScene) return mCurrentScene->ProcessInput(keysBuffer);
	return false;
}

bool CSceneManager::OnProcessingKeyboardMessage(HWND hWnd, UINT messageID, WPARAM wParam, LPARAM lParam)
{
	if (mCurrentScene) return mCurrentScene->OnProcessingKeyboardMessage(hWnd, messageID, wParam, lParam);
	return false;
}

bool CSceneManager::OnProcessingMouseMessage(HWND hWnd, UINT messageID, WPARAM wParam, LPARAM lParam)
{
	if (mCurrentScene) return mCurrentScene->OnProcessingMouseMessage(hWnd, messageID, wParam, lParam);
	return false;
}

void CSceneManager::Animate(float deltaTime)
{
	if (mCurrentScene) mCurrentScene->Animate(deltaTime);
}

void CSceneManager::Render(ID3D12GraphicsCommandList* cmdList, CCamera* camera)
{
	if (mCurrentScene) mCurrentScene->Render(cmdList, camera);
}