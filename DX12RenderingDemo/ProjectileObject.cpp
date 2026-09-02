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

    if (event.other->GetObjectType() != ObjectType::Map)
        return;

    if (collider_)
        collider_->SetEnabled(false);

    if (event.scene)
        event.scene->DestroyGameObject(this);

    LOG("Projectile hit map");
}