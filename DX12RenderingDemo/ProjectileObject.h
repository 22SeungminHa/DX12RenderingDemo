#pragma once
#include "GameObject.h"

class Mesh;
class Material;
class SphereColliderComponent;

class ProjectileObject : public GameObject
{
public:
    ProjectileObject();

    void Initialize(
        const std::shared_ptr<Mesh>& mesh,
        const std::shared_ptr<Material>& material,
        const Vector3& position,
        float radius,
        const Vector3& velocity,
        const Vector3& gravity = Vector3(0.0f, -9.8f, 0.0f)
    );

    void Animate(float deltaTime) override;
    void OnCollision(const CollisionEvent& event) override;

    const Vector3& GetVelocity() const { return velocity_; }
    const Vector3& GetPreviousPosition() const { return previousPosition_; }

    SphereColliderComponent* GetCollider() const
    {
        return collider_;
    }

private:
    Vector3 velocity_ = Vector3::Zero;
    Vector3 gravity_ = Vector3(0.0f, -9.8f, 0.0f);
    Vector3 previousPosition_ = Vector3::Zero;

    SphereColliderComponent* collider_ = nullptr;
};