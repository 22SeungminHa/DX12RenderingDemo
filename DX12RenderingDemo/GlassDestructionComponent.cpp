#include "GlassDestructionComponent.h"
#include "Scene.h"
#include "GameObject.h"
#include "Material.h"
#include "Mesh.h"
#include "GlassFragmentComponent.h"

#include <random>

void GlassDestructionComponent::Initialize(
    const std::shared_ptr<Material>& material,
    float width,
    float height,
    float depth)
{
    material_ = material;

    width_ = width;
    height_ = height;
    depth_ = depth;

    pendingFragments_.clear();

    breakRequested_ = false;
    isBroken_ = false;
}

bool GlassDestructionComponent::Break(
    const Vector2& impactPoint,
    UINT randomRayCount,
    UINT ringCount)
{
    if (isBroken_ ||
        breakRequested_ ||
        !material_ ||
        width_ <= 0.0f ||
        height_ <= 0.0f ||
        depth_ <= 0.0f)
    {
        return false;
    }

    auto fragments = GlassFracture::GenerateRingFragments(
        width_,
        height_,
        impactPoint,
        randomRayCount,
        ringCount
    );

    pendingFragments_.clear();
    pendingFragments_.reserve(fragments.size());

    for (const GlassFragmentData& fragment : fragments)
    {
        GlassFragmentGeometry geometry =
            GlassFracture::BuildFragmentGeometry(
                fragment,
                width_,
                height_,
                depth_
            );

        if (geometry.vertices.empty() ||
            geometry.indices.empty())
        {
            continue;
        }

        pendingFragments_.push_back(
            std::move(geometry)
        );
    }

    if (pendingFragments_.empty())
        return false;

    LOG("Glass fracture generated: "
        << pendingFragments_.size());

    breakRequested_ = true;

    return true;
}

void GlassDestructionComponent::PrepareRenderResources(
    Scene& scene,
    ID3D12Device* device,
    ID3D12GraphicsCommandList* cmdList,
    std::vector<ComPtr<ID3D12Resource>>& transientUploadResources)
{
    if (!breakRequested_ ||
        isBroken_ ||
        !owner_ ||
        !material_ ||
        !device ||
        !cmdList)
    {
        return;
    }

    static std::mt19937 randomEngine{
        std::random_device{}()
    };

    std::uniform_real_distribution<float> speedDistribution(
        4.0f,
        7.0f
    );

    std::uniform_real_distribution<float> zDistribution(
        1.0f,
        2.5f
    );

    std::uniform_real_distribution<float> angularDistribution(
        -4.0f,
        4.0f
    );

    for (const GlassFragmentGeometry& geometry : pendingFragments_)
    {
        auto fragmentMesh =
            std::make_shared<RuntimeMeshLit>(
                device,
                cmdList,
                geometry.vertices,
                geometry.indices,
                transientUploadResources
            );

        GameObject* fragmentObject =
            scene.CreateChildObject(
                owner_,
                fragmentMesh,
                material_,
                geometry.localPosition
            );

        if (!fragmentObject)
            continue;

        Vector3 direction(
            geometry.localPosition.x,
            geometry.localPosition.y,
            zDistribution(randomEngine)
        );

        direction.Normalize();

        auto* fragmentMotion =
            fragmentObject->AddComponent<GlassFragmentComponent>();

        fragmentMotion->SetVelocity(
            direction *
            speedDistribution(randomEngine)
        );

        fragmentMotion->SetAngularVelocity(
            Vector3(
                angularDistribution(randomEngine),
                angularDistribution(randomEngine),
                angularDistribution(randomEngine)
            )
        );
    }

    owner_->SetMesh(std::shared_ptr<Mesh>{});

    pendingFragments_.clear();

    breakRequested_ = false;
    isBroken_ = true;
}