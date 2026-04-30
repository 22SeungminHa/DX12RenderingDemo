#include "Transform.h"

Matrix Transform::GetLocalMatrix() const
{
    Matrix S = Matrix::CreateScale(scale);
    Matrix R = Matrix::CreateFromYawPitchRoll(rotation.y, rotation.x, rotation.z);
    Matrix T = Matrix::CreateTranslation(position);

    return S * R * T;
}

Matrix Transform::GetWorldMatrix() const
{
    Matrix local = GetLocalMatrix();

    if (parent_)
        return local * parent_->GetWorldMatrix();

    return local;
}