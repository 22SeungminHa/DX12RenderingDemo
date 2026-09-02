#pragma once
#include "Component.h"
#include "GlassFracture.h"

class Scene;
class Material;

class GlassComponent : public Component
{
public:
    void Initialize(
        const std::shared_ptr<Material>& material,
        float width,
        float height,
        float depth
    );

    bool Break(
        const Vector2& impactPoint,
        UINT randomRayCount = 9,
        UINT ringCount = 4
    );

    void PrepareRenderResources(
        Scene& scene,
        ID3D12Device* device,
        ID3D12GraphicsCommandList* cmdList,
        std::vector<ComPtr<ID3D12Resource>>& transientUploadResources
    );

    bool IsBroken() const { return isBroken_; }

    void OnCollision(const CollisionEvent& event) override;

private:
    struct PendingFragment
    {
        GlassFragmentGeometry geometry;

        Vector3 velocity = Vector3::Zero;
        Vector3 angularVelocity = Vector3::Zero;

        bool shouldMove = true;
    };

    bool GeneratePendingFragments(
        const Vector2& impactPoint,
        UINT randomRayCount,
        UINT ringCount
    );

    bool CommitPendingFragments(
        Scene& scene,
        ID3D12Device* device,
        ID3D12GraphicsCommandList* cmdList,
        std::vector<ComPtr<ID3D12Resource>>& transientUploadResources
    );

private:
    std::shared_ptr<Material> material_;

    float width_ = 0.0f;
    float height_ = 0.0f;
    float depth_ = 0.0f;

    std::vector<PendingFragment> pendingFragments_;

    bool breakRequested_ = false;
    bool isBroken_ = false;
};

class StaticGlassFragmentComponent : public Component
{
public:
    void OnCollision(const CollisionEvent& event) override;

private:
    bool detached_ = false;
};
