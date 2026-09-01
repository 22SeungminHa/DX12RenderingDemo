#include "CrystalGlassComponent.h"

#include "Scene.h"
#include "GameObject.h"
#include "Material.h"
#include "Mesh.h"
#include "FBXLoader.h"
#include "GlassFragmentComponent.h"

#include <random>

void CrystalGlassComponent::Initialize(
    const std::shared_ptr<Material>& material,
    const std::shared_ptr<FBXNodeData>& crashedModel)
{
    material_ = material;
    crashedModel_ = crashedModel;

    isBroken_ = false;
}

bool CrystalGlassComponent::Break(Scene& scene)
{
    if (isBroken_ ||
        !owner_ ||
        !material_ ||
        !crashedModel_)
    {
        return false;
    }

    std::vector<GameObject*> pieces;

    GameObject* crashedRoot =
        scene.CreateFBXChildObject(
            owner_,
            *crashedModel_,
            material_,
            &pieces
        );

    if (!crashedRoot || pieces.empty())
        return false;

    // 기존 Crystal Mesh 제거
    owner_->SetMesh(std::shared_ptr<Mesh>{});

    static std::mt19937 randomEngine{
        std::random_device{}()
    };

    std::uniform_real_distribution<float>
        horizontalVelocity(-4.0f, 4.0f);

    std::uniform_real_distribution<float>
        verticalVelocity(3.0f, 7.0f);

    std::uniform_real_distribution<float>
        angularVelocity(-6.0f, 6.0f);

    for (GameObject* piece : pieces)
    {
        if (!piece)
            continue;

        auto* fragment =
            piece->AddComponent<GlassFragmentComponent>();

        fragment->SetVelocity(
            Vector3(
                horizontalVelocity(randomEngine),
                verticalVelocity(randomEngine),
                horizontalVelocity(randomEngine)
            )
        );

        fragment->SetAngularVelocity(
            Vector3(
                angularVelocity(randomEngine),
                angularVelocity(randomEngine),
                angularVelocity(randomEngine)
            )
        );
    }

    isBroken_ = true;

    return true;
}