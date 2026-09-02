#pragma once
#include "Component.h"

class ProjectileComponent : public Component
{
public:
    void Initialize(const Vector3& velocity, const Vector3& gravity = Vector3(0.0f, -9.8f, 0.0f));

    virtual void Update(float deltaTime) override;

    const Vector3& GetVelocity() const { return velocity_; }
    const Vector3& GetPreviousPosition() const { return previousPosition_; }

private:
    Vector3 velocity_ = Vector3::Zero;
    Vector3 gravity_ = Vector3(0.0f, -9.8f, 0.0f);
    Vector3 previousPosition_ = Vector3::Zero;
};