#include "SceneManager.h"
#include "Scene.h"
#include "TestScene.h"

std::unique_ptr<Scene> SceneManager::CreateSceneByType(SCENE_TYPE sceneType, UINT width, UINT height)
{
	switch (sceneType) {
	case SCENE_TYPE::TEST1:
		return std::make_unique<TestScene1>(width, height);
	case SCENE_TYPE::TEST2:
		return std::make_unique<TestScene2>(width, height);
	default:
		return nullptr;
	}
}

void SceneManager::CreateScene(SCENE_TYPE sceneType, ID3D12Device* device, ID3D12GraphicsCommandList* cmdList, UINT width, UINT height)
{
	currentScene_ = CreateSceneByType(sceneType, width, height);

	if (currentScene_) {
		currentSceneType_ = sceneType;
		currentScene_->Load(device, cmdList);
	}
	else {
		currentSceneType_ = SCENE_TYPE::NONE;
	}
}

void SceneManager::RequestChangeScene(SCENE_TYPE nextScene)
{
	if (currentScene_ && nextScene == currentSceneType_)
		return;

	sceneChangeRequested_ = true;
	nextSceneType_ = nextScene;
}

void SceneManager::ProcessSceneChange(ID3D12Device* device, ID3D12GraphicsCommandList* cmdList, UINT width, UINT height)
{
	if (!sceneChangeRequested_) return;

	ReleaseCurrentScene();
	CreateScene(nextSceneType_, device, cmdList, width, height);
	
	sceneChangeRequested_ = false;
	nextSceneType_ = SCENE_TYPE::NONE;
}

void SceneManager::ReleaseCurrentScene()
{
	if (currentScene_) {
		currentScene_->Unload();
		currentScene_.reset();
	}

	currentSceneType_ = SCENE_TYPE::NONE;
}

void SceneManager::ReleaseCurrentSceneUploadBuffers()
{
	if (currentScene_) currentScene_->ReleaseUploadBuffers();
}

void SceneManager::ResizeCurrentScene(UINT width, UINT height)
{
	if (currentScene_) currentScene_->Resize(width, height);
}
