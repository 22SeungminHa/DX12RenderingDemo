#pragma once
#include "pch.h"

class Scene;
class Camera;
class GameObject;
class MeshRenderer;

struct RenderItemDesc
{
    GameObject* object = nullptr;
    MeshRenderer* meshRenderer = nullptr;
    float distanceToCamera = 0.0f;

    D3D12_RECT screenBounds{};
    bool hasScreenBounds = false;
};

class RenderQueueBuilder
{
public:
    void Build(Scene* scene, Camera* camera);
    void Clear();

    const std::vector<RenderItemDesc>& GetOpaqueQueue() const { return opaqueQueue_; }
    const std::vector<RenderItemDesc>& GetTransparentQueue() const { return transparentQueue_; }
    const std::vector<RenderItemDesc>& GetGlassQueue() const { return glassQueue_; }

private:
    void CollectRenderItems(GameObject* object, Camera* camera);
    void SortBackToFront(std::vector<RenderItemDesc>& queue);
    bool CalculateScreenBounds(
        GameObject* object,
        MeshRenderer* meshRenderer,
        Camera* camera,
        D3D12_RECT& outBounds) const;

private:
    std::vector<RenderItemDesc> opaqueQueue_;
    std::vector<RenderItemDesc> transparentQueue_;
    std::vector<RenderItemDesc> glassQueue_;
};