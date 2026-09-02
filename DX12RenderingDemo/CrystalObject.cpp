#include "CrystalObject.h"

#include "Scene.h"
#include "Mesh.h"
#include "Material.h"
#include "FBXLoader.h"
#include "ColliderComponent.h"
#include "FragmentMotionComponent.h"

#include <random>

CrystalObject::CrystalObject()
{
    SetObjectType(ObjectType::Crystal);
}

void CrystalObject::Initialize(
    const std::shared_ptr<Mesh>& mesh,
    const std::shared_ptr<Material>& material,
    const std::shared_ptr<FBXNodeData>& crashedModel,
    const Vector3& position,
    const Vector3& scale)
{
    SetPosition(position);
    SetScale(scale);

    SetMesh(mesh);
    SetMaterial(material);

    material_ = material;
    crashedModel_ = crashedModel;

    isBroken_ = false;

    if (!collider_)
        collider_ = AddComponent<BoxColliderComponent>();

    if (mesh && mesh->HasLocalBounds())
        collider_->SetLocalBounds(mesh->GetLocalBounds());
}

bool CrystalObject::Break(Scene& scene)
{
    if (isBroken_ || !material_ || !crashedModel_)
        return false;

    std::vector<GameObject*> pieces;
    GameObject* crashedRoot = scene.CreateFBXChildObject(this, *crashedModel_, material_, &pieces);

    if (!crashedRoot || pieces.empty())
        return false;

    SetMesh(std::shared_ptr<Mesh>{});

    static std::mt19937 randomEngine{ std::random_device{}() };

    std::uniform_real_distribution<float> horizontalVelocity(-4.0f, 4.0f);
    std::uniform_real_distribution<float> verticalVelocity(3.0f, 7.0f);
    std::uniform_real_distribution<float> angularVelocity(-6.0f, 6.0f);

    for (GameObject* piece : pieces)
    {
        if (!piece)
            continue;

        piece->SetObjectType(ObjectType::GlassFragment);

        auto* fragment = piece->AddComponent<FragmentMotionComponent>();

        fragment->SetVelocity(Vector3(horizontalVelocity(randomEngine), verticalVelocity(randomEngine), horizontalVelocity(randomEngine)));
        fragment->SetAngularVelocity(Vector3(angularVelocity(randomEngine), angularVelocity(randomEngine), angularVelocity(randomEngine)));
    }

    isBroken_ = true;

    return true;
}

void CrystalObject::OnCollision(const CollisionEvent& event)
{
    GameObject::OnCollision(event);

    if (!event.other || !event.scene || event.other->GetObjectType() != ObjectType::Projectile)
        return;

    if (!Break(*event.scene))
        return;

    if (collider_)
        collider_->SetEnabled(false);

    LOG("Projectile hit crystal");
}