#include "SceneManager.h"
#include "Scene.h"
#include "GameScene.h"
#include "BenchmarkScene.h"

std::unique_ptr<Scene> SceneManager::CreateSceneByType(SceneType sceneType, UINT width, UINT height)
{
	switch (sceneType)
	{
	case SceneType::Game:
		return std::make_unique<GameScene>(width, height);

	case SceneType::Benchmark:
		return std::make_unique< BenchmarkScene>(width, height);

	default:
		return nullptr;
	}
}

void SceneManager::CreateScene(
	SceneType sceneType,
	ID3D12Device* device,
	ID3D12GraphicsCommandList* cmdList,
	ID3D12RootSignature* rootSignature,
	AssetManager& assetManager,
	UINT width,
	UINT height)
{
	currentScene_ = CreateSceneByType(sceneType, width, height);

	if (currentScene_) {
		currentSceneType_ = sceneType;
		currentScene_->Load(
			device,
			cmdList,
			rootSignature,
			assetManager
		);
	}
	else {
		currentSceneType_ = SceneType::None;
	}
}

void SceneManager::RequestChangeScene(SceneType nextScene)
{
	if (currentScene_ && nextScene == currentSceneType_)
		return;

	sceneChangeRequested_ = true;
	nextSceneType_ = nextScene;
}

void SceneManager::ProcessSceneChange(
	ID3D12Device* device,
	ID3D12GraphicsCommandList* cmdList,
	ID3D12RootSignature* rootSignature,
	AssetManager& assetManager,
	UINT width,
	UINT height)
{
	if (!sceneChangeRequested_) return;

	ReleaseCurrentScene();

	CreateScene(nextSceneType_, device, cmdList, rootSignature, assetManager, width, height);

	sceneChangeRequested_ = false;
	nextSceneType_ = SceneType::None;
}

void SceneManager::ReleaseCurrentScene()
{
	if (currentScene_) {
		currentScene_->Unload();
		currentScene_.reset();
	}

	currentSceneType_ = SceneType::None;
}

void SceneManager::ReleaseCurrentSceneUploadResources()
{
	if (currentScene_) currentScene_->ReleaseUploadResources();
}

void SceneManager::ResizeCurrentScene(UINT width, UINT height)
{
	if (currentScene_) currentScene_->Resize(width, height);
}
