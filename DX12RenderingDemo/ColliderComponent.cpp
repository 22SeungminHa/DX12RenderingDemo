#include "ColliderComponent.h"
#include "GameObject.h"

void BoxColliderComponent::SetLocalSize(const Vector3& size)
{
    size_.x = std::max(size.x, 0.0f);
    size_.y = std::max(size.y, 0.0f);
    size_.z = std::max(size.z, 0.0f);
}

BoundingBox BoxColliderComponent::GetWorldBounds() const
{
    BoundingBox localBounds{};

    localBounds.Center = { center_.x, center_.y, center_.z };
    localBounds.Extents = { size_.x * 0.5f, size_.y * 0.5f, size_.z * 0.5f };

    if (!owner_)
        return localBounds;

    BoundingBox worldBounds{};

    localBounds.Transform(worldBounds, owner_->GetWorldMatrix());

    return worldBounds;
}

void BoxColliderComponent::SetLocalBounds(const BoundingBox& bounds)
{
    center_ = Vector3(bounds.Center.x, bounds.Center.y, bounds.Center.z);
    size_ = Vector3(bounds.Extents.x * 2.0f, bounds.Extents.y * 2.0f, bounds.Extents.z * 2.0f);
}

void SphereColliderComponent::SetLocalRadius(float radius)
{
    radius_ = std::max(radius, 0.0f);
}

BoundingSphere SphereColliderComponent::GetWorldBounds() const
{
    BoundingSphere localBounds{};

    localBounds.Center = { center_.x, center_.y, center_.z };

    localBounds.Radius = radius_;

    if (!owner_)
        return localBounds;

    BoundingSphere worldBounds{};

    localBounds.Transform(worldBounds, owner_->GetWorldMatrix());

    return worldBounds;
}

bool CollisionSystem::Intersects(const SphereColliderComponent& sphere, const BoxColliderComponent& box)
{
    if (!sphere.IsEnabled() || !box.IsEnabled())
        return false;

    return sphere.GetWorldBounds().Intersects(box.GetWorldBounds());
}

bool CollisionSystem::Intersects(const SphereColliderComponent& lhs, const SphereColliderComponent& rhs)
{
    if (!lhs.IsEnabled() || !rhs.IsEnabled())
        return false;

    return lhs.GetWorldBounds().Intersects(rhs.GetWorldBounds());
}

bool CollisionSystem::Intersects(const BoxColliderComponent& lhs, const BoxColliderComponent& rhs)
{
    if (!lhs.IsEnabled() || !rhs.IsEnabled())
        return false;

    return lhs.GetWorldBounds().Intersects(rhs.GetWorldBounds());
}

bool CollisionSystem::SweepIntersects(
    const Vector3& startCenter,
    const SphereColliderComponent& sphere,
    const BoxColliderComponent& box,
    float& outHitT,
    Vector3& outHitCenter)
{
    if (!sphere.IsEnabled() || !box.IsEnabled())
        return false;

    const BoundingSphere sphereBounds = sphere.GetWorldBounds();

    const BoundingBox boxBounds = box.GetWorldBounds();

    const Vector3 endCenter(sphereBounds.Center.x, sphereBounds.Center.y, sphereBounds.Center.z);

    const float radius = sphereBounds.Radius;

    const Vector3 boxCenter(boxBounds.Center.x, boxBounds.Center.y, boxBounds.Center.z);

    const Vector3 boxExtents(boxBounds.Extents.x, boxBounds.Extents.y, boxBounds.Extents.z);

    // Sphere 반지름만큼 AABB 확장
    const Vector3 expandedMin = boxCenter - boxExtents - Vector3(radius);
    const Vector3 expandedMax = boxCenter + boxExtents + Vector3(radius);
    const Vector3 delta = endCenter - startCenter;

    float tMin = 0.0f;
    float tMax = 1.0f;

    auto testAxis = [&](float start, float direction, float minValue, float maxValue) -> bool
    {
        constexpr float epsilon = 0.000001f;

        if (std::abs(direction) < epsilon)
            return start >= minValue && start <= maxValue;

        const float inverseDirection = 1.0f / direction;
        float t1 = (minValue - start) * inverseDirection;
        float t2 = (maxValue - start) * inverseDirection;

        if (t1 > t2)
            std::swap(t1, t2);

        tMin = std::max(tMin, t1);
        tMax = std::min(tMax, t2);

        return tMin <= tMax;
    };

    if (!testAxis(startCenter.x, delta.x, expandedMin.x, expandedMax.x))
        return false;

    if (!testAxis(startCenter.y, delta.y, expandedMin.y, expandedMax.y))
        return false;

    if (!testAxis(startCenter.z, delta.z, expandedMin.z, expandedMax.z))
        return false;

    outHitT = tMin;
    outHitCenter = startCenter + delta * tMin;

    return true;
}