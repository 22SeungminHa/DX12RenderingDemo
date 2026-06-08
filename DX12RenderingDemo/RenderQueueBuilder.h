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
};

class RenderQueueBuilder
{
public:
    void Build(Scene* scene, Camera* camera);
    void Clear();

    const std::vector<RenderItemDesc>& GetOpaqueQueue() const { return opaqueQueue_; }
    const std::vector<RenderItemDesc>& GetTransparentQueue() const { return transparentQueue_; }

private:
    void CollectRenderItems(GameObject* object, Camera* camera);
    void SortTransparentQueue();

private:
    std::vector<RenderItemDesc> opaqueQueue_;
    std::vector<RenderItemDesc> transparentQueue_;
};