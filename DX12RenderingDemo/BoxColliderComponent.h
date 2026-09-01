#pragma once
#include "ColliderComponent.h"

class BoxColliderComponent : public ColliderComponent
{
public:
    void SetSize(const Vector3& size);
    void SetCenter(const Vector3& center) { center_ = center; }

    const Vector3& GetSize() const { return size_; }
    const Vector3& GetCenter() const { return center_; }

    BoundingBox GetWorldBounds() const;

    bool Intersects(const BoundingSphere& sphere) const;
    bool Intersects(const BoundingBox& box) const;

    void SetLocalBounds(const BoundingBox& bounds);

private:
    Vector3 size_ = Vector3::One;
    Vector3 center_ = Vector3::Zero;
};