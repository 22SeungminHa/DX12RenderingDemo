#include "GlassComponent.h"
#include "Scene.h"
#include "GameObject.h"
#include "Material.h"
#include "Mesh.h"
#include "FragmentMotionComponent.h"
#include "ColliderComponent.h"
#include "ProjectileObject.h"

#include <random>

namespace
{
    BoundingBox CalculateFragmentBounds(const GlassFragmentGeometry& geometry)
    {
        BoundingBox bounds{};
        if (geometry.vertices.empty())
            return bounds;

        Vector3 minPosition = geometry.vertices[0].GetPosition();
        Vector3 maxPosition = minPosition;

        for (const LitVertex& vertex : geometry.vertices)
        {
            const Vector3& position = vertex.GetPosition();

            minPosition.x = std::min(minPosition.x, position.x);
            minPosition.y = std::min(minPosition.y, position.y);
            minPosition.z = std::min(minPosition.z, position.z);

            maxPosition.x = std::max(maxPosition.x, position.x);
            maxPosition.y = std::max(maxPosition.y, position.y);
            maxPosition.z = std::max(maxPosition.z, position.z);
        }

        const Vector3 center = (minPosition + maxPosition) * 0.5f;
        const Vector3 extents = (maxPosition - minPosition) * 0.5f;

        bounds.Center = { center.x, center.y, center.z };
        bounds.Extents = { extents.x, extents.y, extents.z };

        return bounds;
    }
}

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

    std::uniform_real_distribution<float> speedScaleDistribution(0.4f, 0.65f);
    std::uniform_real_distribution<float> angularVelocityDistribution(-2.5f, 2.5f);
    std::uniform_real_distribution<float> depthImpulseDistribution(0.1f, 0.25f);

    constexpr float baseFragmentSpeed = 5.0f;
    const float moveRadius = std::min(width_, height_) * 0.25f;

    for (const GlassFragmentData& fragment : fragments)
    {
        GlassFragmentGeometry geometry = GlassFracture::BuildFragmentGeometry(fragment, width_, height_, depth_);
        if (geometry.vertices.empty() || geometry.indices.empty())
            continue;

        const Vector2 fragmentOffset(geometry.localPosition.x - impactPoint.x, geometry.localPosition.y - impactPoint.y);
        const float distanceFromImpact = fragmentOffset.Length();

        PendingFragment pendingFragment{};
        pendingFragment.geometry = std::move(geometry);

        pendingFragment.shouldMove = distanceFromImpact <= moveRadius;

        if (pendingFragment.shouldMove)
        {
            Vector3 direction(fragmentOffset.x, fragmentOffset.y, depthImpulseDistribution(randomEngine));

            if (direction.LengthSquared() > 0.000001f)
                direction.Normalize();
            else
                direction = Vector3(0.0f, 0.0f, 1.0f);

            pendingFragment.velocity = direction * (baseFragmentSpeed * speedScaleDistribution(randomEngine));
            pendingFragment.angularVelocity = Vector3(angularVelocityDistribution(randomEngine), angularVelocityDistribution(randomEngine), angularVelocityDistribution(randomEngine) );
        }

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

    auto runtimeBuffer = std::make_shared<RuntimeMeshBufferLit>(device, cmdList, combinedVertices, combinedIndices, transientUploadResources);
    if (!runtimeBuffer->IsValid())
        return false;

    size_t createdFragmentCount = 0;

    for (size_t i = 0; i < pendingFragments_.size(); ++i)
    {
        const PendingFragment& fragment = pendingFragments_[i];
        const FragmentMeshSlice& slice = slices[i];

        auto fragmentMesh = std::make_shared<RuntimeMeshLit>(runtimeBuffer, slice.vertexOffset, slice.vertexCount, slice.indexOffset, slice.indexCount);

        GameObject* fragmentObject = scene.CreateChildObject(owner_, fragmentMesh, material_, fragment.geometry.localPosition);
        if (!fragmentObject)
            continue;

        fragmentObject->SetObjectType(ObjectType::GlassFragment);

        auto* fragmentComponent = fragmentObject->AddComponent<FragmentMotionComponent>();
        fragmentComponent->SetVelocity(fragment.velocity);
        fragmentComponent->SetAngularVelocity(fragment.angularVelocity);

        if (!fragment.shouldMove)
        {
            fragmentComponent->SetActive(false);

            auto* fragmentCollider = fragmentObject->AddComponent<BoxColliderComponent>();
            fragmentCollider->SetLocalBounds(CalculateFragmentBounds(fragment.geometry));
            fragmentObject->AddComponent<StaticGlassFragmentComponent>();
        }

        ++createdFragmentCount;
    }

    return createdFragmentCount > 0;
}

void GlassComponent::OnCollision(const CollisionEvent& event)
{
    if (!owner_ || !event.other || event.other->GetObjectType() != ObjectType::Projectile)
        return;

    auto* boxCollider = dynamic_cast<BoxColliderComponent*>(event.selfCollider);
    if (!boxCollider)
        return;

    const Matrix inverseWorld = owner_->GetWorldMatrix().Invert();
    const Vector3 localImpactPoint = Vector3::Transform(event.hitPoint, inverseWorld);
    const Vector3 localSize = boxCollider->GetLocalSize();
    const Vector2 impactPoint(
        std::clamp(localImpactPoint.x, -localSize.x * 0.5f, localSize.x * 0.5f),
        std::clamp(localImpactPoint.y, -localSize.y * 0.5f, localSize.y * 0.5f));

    if (!Break(impactPoint))
        return;

    boxCollider->SetEnabled(false);

    LOG(
        "Projectile hit obstacle glass / Impact: ("
        << impactPoint.x << ", "
        << impactPoint.y << ")"
    );
}

void StaticGlassFragmentComponent::OnCollision(const CollisionEvent& event)
{
    if (detached_ || !owner_ || !event.other || event.other->GetObjectType() != ObjectType::Projectile)
        return;

    auto* projectile = dynamic_cast<ProjectileObject*>(event.other);
    auto* motion = owner_->GetComponent<FragmentMotionComponent>();
    auto* collider = dynamic_cast<BoxColliderComponent*>(event.selfCollider);

    if (!projectile || !motion || !collider)
        return;

    Vector3 direction = owner_->GetWorldPosition() - projectile->GetWorldPosition();
    direction.z -= 0.5f;

    if (direction.LengthSquared() > 0.000001f)
        direction.Normalize();
    else
        direction = Vector3(0.0f, 0.0f, -1.0f);

    static std::mt19937 randomEngine{ std::random_device{}() };

    std::uniform_real_distribution<float> speedScale(0.5f, 0.8f);
    std::uniform_real_distribution<float> angularVelocity(-2.0f, 2.0f);

    const float projectileSpeed = projectile->GetVelocity().Length();
    const float fragmentSpeed = projectileSpeed / 30.0f * speedScale(randomEngine);

    motion->SetVelocity(direction * fragmentSpeed);
    motion->SetAngularVelocity(Vector3(angularVelocity(randomEngine), angularVelocity(randomEngine), angularVelocity(randomEngine)));
    motion->SetActive(true);

    collider->SetEnabled(false);

    detached_ = true;
}
