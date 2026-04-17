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

void InputSystem::OnProcessingMouseMessage(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam)
{
	if (sceneManager_)
		sceneManager_->OnProcessingMouseMessage(hwnd, msg, wParam, lParam);

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
}

void InputSystem::OnProcessingKeyboardMessage(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam)
{
	if (sceneManager_) sceneManager_->OnProcessingKeyboardMessage(hwnd, msg, wParam, lParam);

	switch (msg) {
	case WM_KEYUP:
		switch (wParam) {
		case VK_ESCAPE:
			::PostQuitMessage(0);
			break;

		case VK_SPACE:
			break;

		default:
			break;
		}
		break;
	default:
		break;
	}
}