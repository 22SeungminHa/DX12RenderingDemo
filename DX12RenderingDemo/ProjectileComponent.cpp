#include "ProjectileComponent.h"
#include "GameObject.h"

void ProjectileComponent::Initialize(
    const Vector3& velocity,
    const Vector3& gravity)
{
    velocity_ = velocity;
    gravity_ = gravity;
}

void ProjectileComponent::Update(float deltaTime)
{
    if (!owner_)
        return;

    const Vector3 displacement =
        velocity_ * deltaTime +
        gravity_ * (0.5f * deltaTime * deltaTime);

    owner_->Translate(displacement);

    velocity_ += gravity_ * deltaTime;
}