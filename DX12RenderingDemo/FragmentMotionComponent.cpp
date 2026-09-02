#include "FragmentMotionComponent.h"
#include "GameObject.h"

void FragmentMotionComponent::Update(float deltaTime)
{
    if (!active_)
        return;

    GameObject* owner = GetOwner();

    if (!owner)
        return;

    velocity_ += gravity_ * deltaTime;

    owner->TranslateWorld(
        velocity_ * deltaTime
    );

    const Vector3 rotationStep =
        angularVelocity_ * deltaTime;

    const Quaternion deltaRotation =
        Quaternion::CreateFromYawPitchRoll(
            rotationStep.y,
            rotationStep.x,
            rotationStep.z
        );

    owner->Rotate(deltaRotation);
}