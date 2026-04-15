#include "InputSystem.h"

void InputSystem::Initialize(HWND hwnd, SceneManager* sceneManager, Renderer* renderer)
{
	hwnd_ = hwnd;
	sceneManager_ = sceneManager;
	renderer_ = renderer;
}

void InputSystem::ProcessInput()
{
	if (!sceneManager_) return;

	UCHAR keysBuffer[256] = {};
	::GetKeyboardState(keysBuffer);

	sceneManager_->ProcessInput(keysBuffer);
}

bool InputSystem::OnProcessingMouseMessage(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam)
{
	if (sceneManager_ && sceneManager_->OnProcessingMouseMessage(hwnd, msg, wParam, lParam))
		return true;

	switch (msg) {
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

	return false;
}

bool InputSystem::OnProcessingKeyboardMessage(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam)
{
	if (sceneManager_ && sceneManager_->OnProcessingKeyboardMessage(hwnd, msg, wParam, lParam))
		return true;

	switch (msg) {
	case WM_KEYUP:
		switch (wParam) {
		case VK_ESCAPE:
			::PostQuitMessage(0);
			return true;
		case VK_SPACE:
			if (!sceneManager_) break;

			if (sceneManager_->GetSceneType() == SCENE_TYPE::TEST1) {
				sceneManager_->RequestChangeScene(SCENE_TYPE::TEST2);
				return true;
			}
			else if (sceneManager_->GetSceneType() == SCENE_TYPE::TEST2) {
				sceneManager_->RequestChangeScene(SCENE_TYPE::TEST1);
				return true;
			}
			break;
		case VK_F9:
			if (renderer_) {
				renderer_->ChangeSwapChainState();
				return true;
			}
			break;
		default:
			break;
		}
		break;
	default:
		break;
	}

	return false;
}