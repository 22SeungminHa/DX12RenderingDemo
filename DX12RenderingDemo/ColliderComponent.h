#pragma once
#include "Component.h"

class ColliderComponent : public Component
{
public:
    virtual ~ColliderComponent() = default;

    void SetEnabled(bool enabled) { enabled_ = enabled; }
    bool IsEnabled() const { return enabled_; }

private:
    bool enabled_ = true;
};

class BoxColliderComponent : public ColliderComponent
{
public:
    void SetLocalSize(const Vector3& size);
    void SetLocalCenter(const Vector3& center) { center_ = center; }

    const Vector3& GetLocalSize() const { return size_; }
    const Vector3& GetLocalCenter() const { return center_; }

    void SetLocalBounds(const BoundingBox& bounds);
    BoundingBox GetWorldBounds() const;

private:
    Vector3 size_ = Vector3::One;
    Vector3 center_ = Vector3::Zero;
};

class SphereColliderComponent : public ColliderComponent
{
public:
    void SetLocalRadius(float radius);
    void SetLocalCenter(const Vector3& center) { center_ = center; }

    float GetLocalRadius() const { return radius_; }
    const Vector3& GetLocalCenter() const { return center_; }

    BoundingSphere GetWorldBounds() const;

private:
    float radius_ = 1.0f;
    Vector3 center_ = Vector3::Zero;
};

class CollisionSystem
{
public:
    static bool Intersects(
        const ColliderComponent& lhs,
        const ColliderComponent& rhs);

    static bool SweepIntersects(
        const Vector3& startCenter,
        const ColliderComponent& movingCollider,
        const ColliderComponent& targetCollider,
        float& outHitT,
        Vector3& outHitCenter);

private:
    static bool SweepSphereBox(
        const Vector3& startCenter,
        const SphereColliderComponent& sphere,
        const BoxColliderComponent& box,
        float& outHitT,
        Vector3& outHitCenter);
};