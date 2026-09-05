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

bool CrystalObject::Break(Scene& scene, const Vector3& impactPoint) 
{
    if (isBroken_ || !material_ || !crashedModel_)
        return false;

    std::vector<GameObject*> pieces;
    GameObject* crashedRoot = scene.CreateFBXChildObject(this, *crashedModel_, material_, &pieces, true);

    if (!crashedRoot || pieces.empty())
        return false;

    SetMesh(std::shared_ptr<Mesh>{});

    static std::mt19937 randomEngine{ std::random_device{}() };

    std::uniform_int_distribution<int> quarterTurnDistribution(0, 1);

    std::uniform_real_distribution<float> horizontalVelocity(-2.5f, 2.5f);
    std::uniform_real_distribution<float> verticalVelocity(1.5f, 4.0f);
    std::uniform_real_distribution<float> angularVelocity(-3.0f, 3.0f);

    std::bernoulli_distribution remainRandom(0.5);

    const int quarterTurn = quarterTurnDistribution(randomEngine);
    crashedRoot->Rotate(Vector3::Up, static_cast<float>(quarterTurn * 180));

    const float alwaysRemainOffset = 0.6f;
    const float randomRemainOffset = 0.0f;

    for (GameObject* piece : pieces)
    {
        if (!piece)
            continue;

        piece->SetObjectType(ObjectType::GlassFragment);

        const float pieceY = piece->GetWorldPosition().y;

        bool remain = false;

        if (pieceY < impactPoint.y - alwaysRemainOffset)
            remain = true;
        else if (pieceY < impactPoint.y - randomRemainOffset)
            remain = remainRandom(randomEngine);

        if (remain)
            continue;

        auto* fragment = piece->AddComponent<FragmentMotionComponent>();

        fragment->SetVelocity(Vector3(horizontalVelocity(randomEngine), verticalVelocity(randomEngine), horizontalVelocity(randomEngine) ));
        fragment->SetAngularVelocity(Vector3(angularVelocity(randomEngine), angularVelocity(randomEngine), angularVelocity(randomEngine) ));
    }

    isBroken_ = true;

    return true;
}

void CrystalObject::OnCollision(const CollisionEvent& event)
{
    GameObject::OnCollision(event);

    if (!event.other || !event.scene || event.other->GetObjectType() != ObjectType::Projectile)
        return;

    if (!Break(*event.scene, event.hitPoint))
        return;

    if (collider_)
        collider_->SetEnabled(false);
}