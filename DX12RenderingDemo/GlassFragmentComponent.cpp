#include "GlassFragmentComponent.h"
#include "GameObject.h"

void GlassFragmentComponent::Update(float deltaTime)
{
    GameObject* owner = GetOwner();

    if (!owner)
        return;

    velocity_ += gravity_ * deltaTime;

    owner->Translate(velocity_ * deltaTime);
    owner->SetRotation(owner->GetRotation() + angularVelocity_ * deltaTime);
}