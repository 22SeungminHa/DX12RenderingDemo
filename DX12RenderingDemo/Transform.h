#pragma once
#include "pch.h"

class Transform
{
public:
    Vector3 position = Vector3::Zero;
    Quaternion rotation = Quaternion::Identity;
    Vector3 scale = Vector3::One;

public:
    void SetParent(Transform* parent) { parent_ = parent; }
    Transform* GetParent() const { return parent_; }

    Matrix GetLocalMatrix() const;
    Matrix GetWorldMatrix() const;

    void SetPosition(const Vector3& value);
    void SetRotation(const Quaternion& value);
    void SetRotationEuler(const Vector3& radians);
    void SetScale(const Vector3& value);

    void Translate(const Vector3& offset);
    void TranslateWorld(const Vector3& worldOffset);

    void Rotate(const Quaternion& deltaRotation);

    void SetLocalMatrix(const Matrix& matrix);

private:
    void RebuildLocalMatrix();
    void DecomposeLocalMatrix();

private:
    Transform* parent_ = nullptr;

    Matrix localMatrix_ = Matrix::Identity;
};

