#include "GlassComponent.h"
#include "Scene.h"
#include "GameObject.h"
#include "Material.h"
#include "Mesh.h"
#include "GlassFragmentComponent.h"

#include <random>

void GlassComponent::Initialize(const std::shared_ptr<Material>& material, float width, float height, float depth)
{
    material_ = material;

    width_ = width;
    height_ = height;
    depth_ = depth;

    pendingFragments_.clear();

    breakRequested_ = false;
    isBroken_ = false;
}

bool GlassComponent::Break(const Vector2& impactPoint, UINT randomRayCount, UINT ringCount)
{
    if (isBroken_ || breakRequested_ || !material_ || width_ <= 0.0f || height_ <= 0.0f || depth_ <= 0.0f)
        return false;

    if (!GeneratePendingFragments(impactPoint, randomRayCount, ringCount))
        return false;

    LOG("Glass fracture generated: " << pendingFragments_.size());

    breakRequested_ = true;

    return true;
}

bool GlassComponent::GeneratePendingFragments(const Vector2& impactPoint, UINT randomRayCount, UINT ringCount)
{
    auto fragments = GlassFracture::GenerateRingFragments(
        width_,
        height_,
        impactPoint,
        randomRayCount,
        ringCount
    );

    pendingFragments_.clear();
    pendingFragments_.reserve(fragments.size());

    static std::mt19937 randomEngine{ std::random_device{}() };

    std::uniform_real_distribution<float> speedScaleDistribution(0.5f, 0.8f);
    std::uniform_real_distribution<float> angularVelocityDistribution(-4.0f, 4.0f);
    std::uniform_real_distribution<float> depthImpulseDistribution(0.2f, 0.5f);

    constexpr float baseFragmentSpeed = 8.0f;

    for (const GlassFragmentData& fragment : fragments)
    {
        GlassFragmentGeometry geometry = GlassFracture::BuildFragmentGeometry(fragment, width_, height_, depth_);

        if (geometry.vertices.empty() || geometry.indices.empty())
            continue;

        Vector3 direction(
            geometry.localPosition.x - impactPoint.x,
            geometry.localPosition.y - impactPoint.y,
            depthImpulseDistribution(randomEngine)
        );

        if (direction.LengthSquared() > 0.000001f)
            direction.Normalize();
        else
            direction = Vector3(0.0f, 0.0f, 1.0f);

        PendingFragment pendingFragment{};

        pendingFragment.velocity = direction * (baseFragmentSpeed * speedScaleDistribution(randomEngine));

        pendingFragment.angularVelocity =
            Vector3(
                angularVelocityDistribution(randomEngine),
                angularVelocityDistribution(randomEngine),
                angularVelocityDistribution(randomEngine)
            );

        pendingFragment.geometry = std::move(geometry);

        pendingFragments_.push_back(std::move(pendingFragment));
    }

    return !pendingFragments_.empty();
}

void GlassComponent::PrepareRenderResources(
    Scene& scene,
    ID3D12Device* device,
    ID3D12GraphicsCommandList* cmdList,
    std::vector<ComPtr<ID3D12Resource>>& transientUploadResources)
{
    if (!breakRequested_ || isBroken_ || !owner_ || !material_ || !device || !cmdList)
        return;

    if (!CommitPendingFragments(scene, device, cmdList, transientUploadResources))
        return;

    owner_->SetMesh(std::shared_ptr<Mesh>{});

    pendingFragments_.clear();

    breakRequested_ = false;
    isBroken_ = true;
}

bool GlassComponent::CommitPendingFragments(
    Scene& scene,
    ID3D12Device* device,
    ID3D12GraphicsCommandList* cmdList,
    std::vector<ComPtr<ID3D12Resource>>& transientUploadResources)
{
    struct FragmentMeshSlice
    {
        UINT vertexOffset = 0;
        UINT vertexCount = 0;

        UINT indexOffset = 0;
        UINT indexCount = 0;
    };

    size_t totalVertexCount = 0;
    size_t totalIndexCount = 0;

    for (const PendingFragment& fragment : pendingFragments_)
    {
        totalVertexCount += fragment.geometry.vertices.size();
        totalIndexCount += fragment.geometry.indices.size();
    }

    if (totalVertexCount == 0 || totalIndexCount == 0)
        return false;

    std::vector<LitVertex> combinedVertices;
    std::vector<UINT> combinedIndices;
    std::vector<FragmentMeshSlice> slices;

    combinedVertices.reserve(totalVertexCount);
    combinedIndices.reserve(totalIndexCount);
    slices.reserve(pendingFragments_.size());

    for (const PendingFragment& fragment : pendingFragments_)
    {
        const GlassFragmentGeometry& geometry = fragment.geometry;

        FragmentMeshSlice slice{};

        slice.vertexOffset = static_cast<UINT>(combinedVertices.size());
        slice.vertexCount = static_cast<UINT>(geometry.vertices.size());

        slice.indexOffset = static_cast<UINT>(combinedIndices.size());
        slice.indexCount = static_cast<UINT>(geometry.indices.size());

        combinedVertices.insert(combinedVertices.end(), geometry.vertices.begin(), geometry.vertices.end());
        combinedIndices.insert(combinedIndices.end(), geometry.indices.begin(), geometry.indices.end());

        slices.push_back(slice);
    }

    auto runtimeBuffer =
        std::make_shared<RuntimeMeshBufferLit>(
            device,
            cmdList,
            combinedVertices,
            combinedIndices,
            transientUploadResources
        );

    if (!runtimeBuffer->IsValid())
        return false;

    size_t createdFragmentCount = 0;

    for (size_t i = 0; i < pendingFragments_.size(); ++i)
    {
        const PendingFragment& fragment = pendingFragments_[i];
        const FragmentMeshSlice& slice = slices[i];

        auto fragmentMesh =
            std::make_shared<RuntimeMeshLit>(
                runtimeBuffer,
                slice.vertexOffset,
                slice.vertexCount,
                slice.indexOffset,
                slice.indexCount
            );

        GameObject* fragmentObject = scene.CreateChildObject(owner_, fragmentMesh, material_, fragment.geometry.localPosition);

        if (!fragmentObject)
            continue;

        auto* fragmentComponent = fragmentObject->AddComponent<GlassFragmentComponent>();

        fragmentComponent->SetVelocity(fragment.velocity);
        fragmentComponent->SetAngularVelocity(fragment.angularVelocity);

        ++createdFragmentCount;
    }

    return createdFragmentCount > 0;
}