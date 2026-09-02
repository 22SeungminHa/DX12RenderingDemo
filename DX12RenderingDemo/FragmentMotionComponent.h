#pragma once
#include "Component.h"

class FragmentMotionComponent : public Component
{
public:
    void Update(float deltaTime) override;

    void SetVelocity(const Vector3& velocity) { velocity_ = velocity; }
    void SetAngularVelocity(const Vector3& angularVelocity) { angularVelocity_ = angularVelocity; }
    void SetGravity(const Vector3& gravity) { gravity_ = gravity; }

    void SetActive(bool active) { active_ = active; }
    bool IsActive() const { return active_; }

private:
    bool active_ = true;

    Vector3 velocity_ = Vector3::Zero;

    // rad / sec
    Vector3 angularVelocity_ = Vector3::Zero;

    Vector3 gravity_ = Vector3(0.0f, -9.8f, 0.0f);
};