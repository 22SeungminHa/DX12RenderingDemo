#include "SceneManager.h"
#include "TestScene.h"

std::unique_ptr<Scene> SceneManager::CreateSceneByType(SCENE_TYPE sceneType)
{
	switch (sceneType) {
	case SCENE_TYPE::TEST1:
		return std::make_unique<TestScene1>();
	case SCENE_TYPE::TEST2:
		return std::make_unique<TestScene2>();
	default:
		return nullptr;
	}
}

void SceneManager::CreateScene(SCENE_TYPE sceneType, ID3D12Device* device, ID3D12GraphicsCommandList* cmdList)
{
	currentScene_ = CreateSceneByType(sceneType);

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

void SceneManager::ProcessSceneChange(ID3D12Device* device, ID3D12GraphicsCommandList* cmdList)
{
	if (!sceneChangeRequested_) return;

	ReleaseScene();
	CreateScene(nextSceneType_, device, cmdList);
	sceneChangeRequested_ = false;
	nextSceneType_ = SCENE_TYPE::NONE;
}

void SceneManager::ReleaseScene()
{
	if (currentScene_) {
		currentScene_->Unload();
		currentScene_.reset();
	}

	currentSceneType_ = SCENE_TYPE::NONE;
}

void SceneManager::ReleaseUploadBuffers()
{
	if (currentScene_) currentScene_->ReleaseUploadBuffers();
}

void SceneManager::ProcessInput(const UCHAR* keysBuffer)
{
	if (currentScene_) currentScene_->ProcessInput(keysBuffer);
}

void SceneManager::OnProcessingKeyboardMessage(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam)
{
	if (currentScene_) currentScene_->OnProcessingKeyboardMessage(hwnd, msg, wParam, lParam);

	switch (msg)
	{
	case WM_KEYUP:
		switch (wParam)
		{
		case VK_SPACE:
			if (currentSceneType_ == SCENE_TYPE::TEST1)
				RequestChangeScene(SCENE_TYPE::TEST2);
			else if (currentSceneType_ == SCENE_TYPE::TEST2)
				RequestChangeScene(SCENE_TYPE::TEST1);
			break;

		default:
			break;
		}
		break;
	default:
		break;
	}
}

void SceneManager::OnProcessingMouseMessage(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam)
{
	if (currentScene_) currentScene_->OnProcessingMouseMessage(hwnd, msg, wParam, lParam);

	switch (msg)
	{
	case WM_LBUTTONDOWN:
	case WM_RBUTTONDOWN:
		break;
	case WM_LBUTTONUP:
	case WM_RBUTTONUP:
		break;
	case WM_MOUSEMOVE:
		break;
	default:
		break;
	}
}

void SceneManager::Animate(float deltaTime)
{
	if (currentScene_) currentScene_->Animate(deltaTime);
}

void SceneManager::Render(ID3D12GraphicsCommandList* cmdList, Camera* camera)
{
	if (currentScene_) currentScene_->Render(cmdList, camera);
}