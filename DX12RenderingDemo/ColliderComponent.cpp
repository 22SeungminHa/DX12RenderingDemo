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

    localBounds.Transform(
        worldBounds,
        owner_->GetWorldMatrix()
    );

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

    localBounds.Center =
    {
        center_.x,
        center_.y,
        center_.z
    };

    localBounds.Radius = radius_;

    if (!owner_)
        return localBounds;

    BoundingSphere worldBounds{};

    localBounds.Transform(
        worldBounds,
        owner_->GetWorldMatrix()
    );

    return worldBounds;
}

bool CollisionSystem::Intersects(
    const SphereColliderComponent& sphere,
    const BoxColliderComponent& box)
{
    if (!sphere.IsEnabled() || !box.IsEnabled())
        return false;

    return sphere.GetWorldBounds().Intersects(
        box.GetWorldBounds()
    );
}

bool CollisionSystem::Intersects(
    const SphereColliderComponent& lhs,
    const SphereColliderComponent& rhs)
{
    if (!lhs.IsEnabled() || !rhs.IsEnabled())
        return false;

    return lhs.GetWorldBounds().Intersects(
        rhs.GetWorldBounds()
    );
}

bool CollisionSystem::Intersects(
    const BoxColliderComponent& lhs,
    const BoxColliderComponent& rhs)
{
    if (!lhs.IsEnabled() || !rhs.IsEnabled())
        return false;

    return lhs.GetWorldBounds().Intersects(
        rhs.GetWorldBounds()
    );
}