#include "SphereColliderComponent.h"
#include "GameObject.h"

void SphereColliderComponent::SetRadius(float radius)
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

bool SphereColliderComponent::Intersects(
    const BoundingBox& box) const
{
    if (!IsEnabled())
        return false;

    return GetWorldBounds().Intersects(box);
}

bool SphereColliderComponent::Intersects(
    const BoundingSphere& sphere) const
{
    if (!IsEnabled())
        return false;

    return GetWorldBounds().Intersects(sphere);
}