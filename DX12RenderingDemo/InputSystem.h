#pragma once
#include "SceneManager.h"
#include "Renderer.h"

class InputSystem
{
private:
	SceneManager* sceneManager_ = nullptr;
	Renderer* renderer_ = nullptr;
	HWND hwnd_ = nullptr;

public:
	InputSystem() = default;
	~InputSystem() = default;

	void Initialize(HWND hwnd, SceneManager* sceneManager, Renderer* renderer);

	void ProcessInput();

	bool OnProcessingMouseMessage(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam);
	bool OnProcessingKeyboardMessage(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam);
};