#include "ObstacleObject.h"
#include "Mesh.h"
#include "Material.h"
#include "GlassComponent.h"
#include "ColliderComponent.h"

ObstacleObject::ObstacleObject()
{
    SetObjectType(ObjectType::Obstacle);
}

void ObstacleObject::Initialize(
    const std::shared_ptr<Mesh>& mesh,
    const std::shared_ptr<Material>& material,
    const Vector3& position,
    float width,
    float height,
    float depth)
{
    SetPosition(position);
    SetScale(Vector3::One);

    SetMesh(mesh);
    SetMaterial(material);

    if (!glassComponent_)
        glassComponent_ = AddComponent<GlassComponent>();

    glassComponent_->Initialize(material, width, height, depth);

    if (!collider_)
        collider_ = AddComponent<BoxColliderComponent>();

    collider_->SetLocalSize(Vector3(width, height, depth));
}