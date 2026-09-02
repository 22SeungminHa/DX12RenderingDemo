#include "Transform.h"

Matrix Transform::GetLocalMatrix() const
{
    return localMatrix_;
}

Matrix Transform::GetWorldMatrix() const
{
    Matrix local = GetLocalMatrix();

    if (parent_)
        return local * parent_->GetWorldMatrix();

    return local;
}

void Transform::SetPosition(const Vector3& value)
{
    position = value;
    RebuildLocalMatrix();
}

void Transform::SetRotation(const Quaternion& value)
{
    rotation = value;
    rotation.Normalize();

    RebuildLocalMatrix();
}

void Transform::SetRotationEuler(const Vector3& radians)
{
    rotation = Quaternion::CreateFromYawPitchRoll(
        radians.y,
        radians.x,
        radians.z
    );

    rotation.Normalize();

    RebuildLocalMatrix();
}

void Transform::SetScale(const Vector3& value)
{
    scale = value;
    RebuildLocalMatrix();
}

void Transform::Translate(const Vector3& offset)
{
    position += offset;
    RebuildLocalMatrix();
}

void Transform::TranslateWorld(const Vector3& worldOffset)
{
    Vector3 localOffset = worldOffset;

    if (parent_)
    {
        const Matrix inverseParent =
            parent_->GetWorldMatrix().Invert();

        localOffset = Vector3::TransformNormal(
            worldOffset,
            inverseParent
        );
    }

    position += localOffset;

    RebuildLocalMatrix();
}

void Transform::Rotate(const Quaternion& deltaRotation)
{
    rotation = Quaternion::Concatenate(
        rotation,
        deltaRotation
    );

    rotation.Normalize();

    RebuildLocalMatrix();
}

void Transform::SetLocalMatrix(const Matrix& matrix)
{
    localMatrix_ = matrix;
    DecomposeLocalMatrix();
}

void Transform::RebuildLocalMatrix()
{
    const Matrix S = Matrix::CreateScale(scale);
    const Matrix R = Matrix::CreateFromQuaternion(rotation);
    const Matrix T = Matrix::CreateTranslation(position);

    localMatrix_ = S * R * T;
}

void Transform::DecomposeLocalMatrix()
{
    Vector3 decomposedScale;
    Quaternion decomposedRotation;
    Vector3 decomposedTranslation;

    if (!localMatrix_.Decompose(
        decomposedScale,
        decomposedRotation,
        decomposedTranslation))
    {
        return;
    }

    scale = decomposedScale;
    rotation = decomposedRotation;
    position = decomposedTranslation;

    rotation.Normalize();
}