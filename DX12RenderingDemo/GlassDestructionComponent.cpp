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
    if (isBroken_ || breakRequested_ || !material_ || width_ <= 0.0f || height_ <= 0.0f || depth_ <= 0.0f)
        return false;

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
        GlassFragmentGeometry geometry = GlassFracture::BuildFragmentGeometry(fragment, width_, height_, depth_);

        if (geometry.vertices.empty() || geometry.indices.empty())
            continue;

        pendingFragments_.push_back(std::move(geometry));
    }

    if (pendingFragments_.empty())
        return false;

    LOG("Glass fracture generated: " << pendingFragments_.size());

    breakRequested_ = true;

    return true;
}

void GlassDestructionComponent::PrepareRenderResources(
    Scene& scene,
    ID3D12Device* device,
    ID3D12GraphicsCommandList* cmdList,
    std::vector<ComPtr<ID3D12Resource>>& transientUploadResources)
{
    if (!breakRequested_ || isBroken_ || !owner_ || !material_ || !device || !cmdList)
        return;

    struct FragmentMeshSlice
    {
        UINT vertexOffset = 0;
        UINT vertexCount = 0;

        UINT indexOffset = 0;
        UINT indexCount = 0;
    };

    size_t totalVertexCount = 0;
    size_t totalIndexCount = 0;

    for (const GlassFragmentGeometry& geometry : pendingFragments_)
    {
        totalVertexCount += geometry.vertices.size();
        totalIndexCount += geometry.indices.size();
    }

    if (totalVertexCount == 0 || totalIndexCount == 0)
        return;

    std::vector<LitVertex> combinedVertices;
    std::vector<UINT> combinedIndices;
    std::vector<FragmentMeshSlice> slices;

    combinedVertices.reserve(totalVertexCount);
    combinedIndices.reserve(totalIndexCount);
    slices.reserve(pendingFragments_.size());

    for (const GlassFragmentGeometry& geometry : pendingFragments_)
    {
        FragmentMeshSlice slice{};

        slice.vertexOffset = static_cast<UINT>(combinedVertices.size());
        slice.vertexCount = static_cast<UINT>(geometry.vertices.size());

        slice.indexOffset = static_cast<UINT>(combinedIndices.size());
        slice.indexCount = static_cast<UINT>(geometry.indices.size());

        combinedVertices.insert(combinedVertices.end(), geometry.vertices.begin(), geometry.vertices.end());
        combinedIndices.insert(combinedIndices.end(), geometry.indices.begin(), geometry.indices.end());

        slices.push_back(slice);
    }

    owner_->SetMesh(std::shared_ptr<Mesh>{});

    pendingFragments_.clear();

    breakRequested_ = false;
    isBroken_ = true;
}