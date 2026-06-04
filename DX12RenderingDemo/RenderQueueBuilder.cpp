#include "RenderQueueBuilder.h"
#include "Scene.h"
#include "GameObject.h"
#include "Camera.h"
#include "MeshRenderer.h"
#include "Material.h"

void RenderQueueBuilder::Build(Scene* scene, Camera* camera)
{
    Clear();

    if (!scene || !camera)
        return;

    const auto& objects = scene->GetObjects();

    for (const auto& object : objects)
    {
        if (!object)
            continue;

        CollectRenderItems(object.get(), camera);
    }

    SortTransparentQueue();
}

void RenderQueueBuilder::Clear()
{
    opaqueQueue_.clear();
    transparentQueue_.clear();
}

void RenderQueueBuilder::CollectRenderItems(GameObject* object, Camera* camera)
{
    if (!object || !camera)
        return;

    object->OnPrepareRender();

    MeshRenderer* meshRenderer = object->GetComponent<MeshRenderer>();

    if (meshRenderer && meshRenderer->IsRenderable())
    {
        Material* material = meshRenderer->GetMaterial();

        RenderItem item{};
        item.object = object;
        item.meshRenderer = meshRenderer;

        const Vector3 objectPos = object->GetWorldMatrix().Translation();
        const Vector3 cameraPos = camera->GetPosition();

        item.distanceToCamera = Vector3::DistanceSquared(objectPos, cameraPos);

        if (material && material->GetRenderMode() == RenderMode::Transparent)
            transparentQueue_.push_back(item);
        else
            opaqueQueue_.push_back(item);
    }

    for (const auto& child : object->GetChildren())
        CollectRenderItems(child.get(), camera);
}

void RenderQueueBuilder::SortTransparentQueue()
{
    std::sort(transparentQueue_.begin(), transparentQueue_.end(), [](const RenderItem& a, const RenderItem& b) {
        return a.distanceToCamera > b.distanceToCamera;
    });
}