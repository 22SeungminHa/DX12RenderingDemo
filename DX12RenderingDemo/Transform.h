#pragma once
#include "pch.h"

class Transform
{
public:
    Vector3 position = Vector3::Zero;
    Vector3 rotation = Vector3::Zero; // (pitch, yaw, roll)
    Vector3 scale = Vector3::One;

public:
    Matrix GetWorldMatrix() const
    {
        Matrix S = Matrix::CreateScale(scale);
        Matrix R = Matrix::CreateFromYawPitchRoll(rotation.y, rotation.x, rotation.z);
        Matrix T = Matrix::CreateTranslation(position);

        return S * R * T;
    }
};

