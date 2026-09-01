#include "BoxColliderComponent.h"
#include "GameObject.h"

void BoxColliderComponent::SetSize(const Vector3& size)
{
    size_.x = std::max(size.x, 0.0f);
    size_.y = std::max(size.y, 0.0f);
    size_.z = std::max(size.z, 0.0f);
}

BoundingBox BoxColliderComponent::GetWorldBounds() const
{
    BoundingBox localBounds{};

    localBounds.Center =
    {
        center_.x,
        center_.y,
        center_.z
    };

    localBounds.Extents =
    {
        size_.x * 0.5f,
        size_.y * 0.5f,
        size_.z * 0.5f
    };

    if (!owner_)
        return localBounds;

    BoundingBox worldBounds{};

    localBounds.Transform(
        worldBounds,
        owner_->GetWorldMatrix()
    );

    return worldBounds;
}

bool BoxColliderComponent::Intersects(
    const BoundingSphere& sphere) const
{
    if (!IsEnabled())
        return false;

    return GetWorldBounds().Intersects(sphere);
}

bool BoxColliderComponent::Intersects(
    const BoundingBox& box) const
{
    if (!IsEnabled())
        return false;

    return GetWorldBounds().Intersects(box);
}

void BoxColliderComponent::SetLocalBounds(
    const BoundingBox& bounds)
{
    center_ =
        Vector3(
            bounds.Center.x,
            bounds.Center.y,
            bounds.Center.z
        );

    size_ =
        Vector3(
            bounds.Extents.x * 2.0f,
            bounds.Extents.y * 2.0f,
            bounds.Extents.z * 2.0f
        );
}