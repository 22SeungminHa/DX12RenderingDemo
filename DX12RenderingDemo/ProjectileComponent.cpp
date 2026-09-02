#include "ProjectileComponent.h"
#include "GameObject.h"

void ProjectileComponent::Initialize(const Vector3& velocity, const Vector3& gravity)
{
    velocity_ = velocity;
    gravity_ = gravity;

    if (owner_)
        previousPosition_ = owner_->GetPosition();
}

void ProjectileComponent::Update(float deltaTime)
{
    if (!owner_)
        return;

    previousPosition_ = owner_->GetPosition();

    const Vector3 displacement = velocity_ * deltaTime + gravity_ * (0.5f * deltaTime * deltaTime);

    owner_->Translate(displacement);

    velocity_ += gravity_ * deltaTime;
}