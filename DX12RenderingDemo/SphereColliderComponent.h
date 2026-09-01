#pragma once
#include "ColliderComponent.h"

class SphereColliderComponent : public ColliderComponent
{
public:
    void SetRadius(float radius);
    void SetCenter(const Vector3& center) { center_ = center; }

    float GetRadius() const { return radius_; }
    const Vector3& GetCenter() const { return center_; }

    BoundingSphere GetWorldBounds() const;

    bool Intersects(const BoundingBox& box) const;
    bool Intersects(const BoundingSphere& sphere) const;

private:
    float radius_ = 1.0f;
    Vector3 center_ = Vector3::Zero;
};