#pragma once
#include "pch.h"

class Scene;
class Camera;
class GameObject;
class MeshRenderer;

struct RenderItem
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

    const std::vector<RenderItem>& GetOpaqueQueue() const { return opaqueQueue_; }
    const std::vector<RenderItem>& GetTransparentQueue() const { return transparentQueue_; }

private:
    void CollectRenderItems(GameObject* object, Camera* camera);
    void SortTransparentQueue();

private:
    std::vector<RenderItem> opaqueQueue_;
    std::vector<RenderItem> transparentQueue_;
};