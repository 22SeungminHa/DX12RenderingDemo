#pragma once
#include "pch.h"

class Transform
{
public:
    Vector3 position = Vector3::Zero;
    Vector3 rotation = Vector3::Zero; // (pitch, yaw, roll)
    Vector3 scale = Vector3::One;

public:
    void SetParent(Transform* parent) { parent_ = parent; }
    Transform* GetParent() const { return parent_; }

    Matrix GetLocalMatrix() const;
    Matrix GetWorldMatrix() const;

private:
    Transform* parent_ = nullptr;
};

