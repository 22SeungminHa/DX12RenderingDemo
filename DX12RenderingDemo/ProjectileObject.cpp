#include "ProjectileObject.h"
#include "ColliderComponent.h"
#include "Scene.h"
#include "Mesh.h"
#include "Material.h"

ProjectileObject::ProjectileObject()
{
    SetObjectType(ObjectType::Projectile);
}

void ProjectileObject::Initialize(
    const std::shared_ptr<Mesh>& mesh,
    const std::shared_ptr<Material>& material,
    const Vector3& position,
    float radius,
    const Vector3& velocity,
    const Vector3& gravity)
{
    SetPosition(position);
    SetScale(Vector3(radius));

    SetMesh(mesh);
    SetMaterial(material);

    velocity_ = velocity;
    gravity_ = gravity;
    previousPosition_ = position;

    if (!collider_)
        collider_ = AddComponent<SphereColliderComponent>();

    collider_->SetLocalRadius(1.0f);
}

void ProjectileObject::Animate(float deltaTime)
{
    GameObject::Animate(deltaTime);

    previousPosition_ = GetPosition();

    const Vector3 displacement = velocity_ * deltaTime + gravity_ * (0.5f * deltaTime * deltaTime);
    Translate(displacement);

    velocity_ += gravity_ * deltaTime;
}

void ProjectileObject::OnCollision(const CollisionEvent& event)
{
    GameObject::OnCollision(event);

    if (!event.other)
        return;

    switch (event.other->GetObjectType())
    {
    case ObjectType::Map:
        Bounce(event.hitPoint, event.hitNormal);
        break;

    case ObjectType::Obstacle:
        ReduceSpeed(bounceSpeedMultiplier_);
        break;

    case ObjectType::Crystal:
        BounceUpOnCrystal();
        break;

    default:
        break;
    }
}

void ProjectileObject::Bounce(const Vector3& hitCenter, const Vector3& hitNormal)
{
    if (hitNormal.LengthSquared() < 0.000001f)
        return;

    Vector3 normal = hitNormal;
    normal.Normalize();

    const float velocityAlongNormal = velocity_.Dot(normal);
    if (velocityAlongNormal >= 0.0f)
        return;

    velocity_ -= (1.0f + restitution_) * velocityAlongNormal * normal;

    ReduceSpeed(bounceSpeedMultiplier_);

    constexpr float collisionSkin = 0.01f;
    SetPosition(hitCenter + normal * collisionSkin);
}

void ProjectileObject::BounceUpOnCrystal()
{
    ReduceSpeed(impactSpeedMultiplier_);
    velocity_.y = std::max(velocity_.y, 0.0f) + crystalUpwardImpulse_;
}

void ProjectileObject::ReduceSpeed(float multiplier)
{
    velocity_.x *= multiplier * 0.5;
    velocity_.y *= multiplier;
    velocity_.z *= multiplier * 0.5;
}