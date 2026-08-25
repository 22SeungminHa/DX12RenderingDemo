#pragma once
#include "pch.h"

class InputSystem
{
public:
    InputSystem() = default;
    ~InputSystem() = default;

    void Initialize(HWND hwnd);

    void Update();

    void HandleMouseMessage(UINT msg, WPARAM wParam, LPARAM lParam);
    void HandleKeyboardMessage(UINT msg, WPARAM wParam, LPARAM lParam);

    bool IsKeyDown(int vk) const;
    bool WasKeyPressed(int vk) const;
    bool WasKeyReleased(int vk) const;
    bool WasLeftMousePressed() const { return leftMousePressed_; }
    bool WasRightMousePressed() const { return rightMousePressed_; }

    bool IsLeftMouseDown() const { return leftMouseDown_; }
    bool IsRightMouseDown() const { return rightMouseDown_; }

    POINT GetMousePosition() const { return mousePosition_; }
    POINT GetMouseDelta() const { return mouseDelta_; }

private:
    HWND hwnd_ = nullptr;

    UCHAR currentKeys_[256]{};
    UCHAR previousKeys_[256]{};

    bool leftMouseDown_ = false;
    bool rightMouseDown_ = false;

    bool previousLeftMouseDown_ = false;
    bool previousRightMouseDown_ = false;

    bool leftMousePressed_ = false;
    bool rightMousePressed_ = false;

    POINT mousePosition_{};
    POINT previousMousePosition_{};
    POINT mouseDelta_{};
};