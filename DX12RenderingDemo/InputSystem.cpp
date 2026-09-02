#include "InputSystem.h"

void InputSystem::Initialize(HWND hwnd)
{
    hwnd_ = hwnd;
    ::ZeroMemory(currentKeys_, sizeof(currentKeys_));
    ::ZeroMemory(previousKeys_, sizeof(previousKeys_));
}

void InputSystem::Update()
{
    ::memcpy(previousKeys_, currentKeys_, sizeof(currentKeys_));
    ::GetKeyboardState(currentKeys_);

    mouseDelta_.x = mousePosition_.x - previousMousePosition_.x;
    mouseDelta_.y = mousePosition_.y - previousMousePosition_.y;
    previousMousePosition_ = mousePosition_;

    leftMousePressedThisFrame_ = leftMousePressedPending_;
    leftMousePressedPending_ = false;
}

void InputSystem::HandleMouseMessage(UINT msg, WPARAM wParam, LPARAM lParam)
{
    mousePosition_.x = GET_X_LPARAM(lParam);
    mousePosition_.y = GET_Y_LPARAM(lParam);

    switch (msg)
    {
    case WM_LBUTTONDOWN:
        if (!leftMouseDown_)
            leftMousePressedPending_ = true;

        leftMouseDown_ = true;
        break;
    case WM_LBUTTONUP:
        leftMouseDown_ = false;
        break;
    case WM_RBUTTONDOWN:
        rightMouseDown_ = true;
        break;
    case WM_RBUTTONUP:
        rightMouseDown_ = false;
        break;
    default:
        break;
    }
}

void InputSystem::HandleKeyboardMessage(UINT msg, WPARAM wParam, LPARAM lParam)
{
}

bool InputSystem::IsKeyDown(int vk) const
{
    return (currentKeys_[vk] & 0x80) != 0;
}

bool InputSystem::WasKeyPressed(int vk) const
{
    return ((currentKeys_[vk] & 0x80) != 0) &&
        ((previousKeys_[vk] & 0x80) == 0);
}

bool InputSystem::WasKeyReleased(int vk) const
{
    return ((currentKeys_[vk] & 0x80) == 0) &&
        ((previousKeys_[vk] & 0x80) != 0);
}