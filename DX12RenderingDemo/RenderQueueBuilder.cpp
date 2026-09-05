#include "RenderQueueBuilder.h"
#include "Scene.h"
#include "GameObject.h"
#include "Camera.h"
#include "MeshRenderer.h"
#include "Material.h"
#include "Mesh.h"

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

    SortBackToFront(transparentQueue_);
    SortBackToFront(glassQueue_);
}

void RenderQueueBuilder::Clear()
{
    opaqueQueue_.clear();
    transparentQueue_.clear();
    glassQueue_.clear();
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

        RenderItemDesc item{};
        item.object = object;
        item.meshRenderer = meshRenderer;

        const Vector3 objectPos = object->GetWorldMatrix().Translation();
        const Vector3 cameraPos = camera->GetPosition();

        item.distanceToCamera = Vector3::DistanceSquared(objectPos, cameraPos);

        if (material && material->IsGlassMaterial())
        {
            item.hasScreenBounds =
                CalculateScreenBounds(
                    object,
                    meshRenderer,
                    camera,
                    item.screenBounds);

            glassQueue_.push_back(item);
        }
        else if (material && material->GetRenderMode() == RenderMode::Transparent)
            transparentQueue_.push_back(item);
        else
            opaqueQueue_.push_back(item);
    }

    for (const auto& child : object->GetChildren())
        CollectRenderItems(child.get(), camera);
}

void RenderQueueBuilder::SortBackToFront(std::vector<RenderItemDesc>& queue)
{
    std::sort(queue.begin(), queue.end(),
        [](const RenderItemDesc& a, const RenderItemDesc& b)
        {
            return a.distanceToCamera > b.distanceToCamera;
        });
}

bool RenderQueueBuilder::CalculateScreenBounds(
    GameObject* object,
    MeshRenderer* meshRenderer,
    Camera* camera,
    D3D12_RECT& outBounds) const
{
    if (!object || !meshRenderer || !camera)
        return false;

    Mesh* mesh = meshRenderer->GetMesh();

    if (!mesh || !mesh->HasLocalBounds())
        return false;

    const BoundingBox& localBounds = mesh->GetLocalBounds();

    XMFLOAT3 corners[8]{};
    localBounds.GetCorners(corners);

    const Matrix world = object->GetWorldMatrix();
    const Matrix& view = camera->GetViewMatrix();
    const Matrix& projection = camera->GetProjectionMatrix();
    const D3D12_VIEWPORT& viewport = camera->GetViewport();
    const D3D12_RECT& scissor = camera->GetScissorRect();

    const XMMATRIX worldMatrix = XMLoadFloat4x4(&world);
    const XMMATRIX viewMatrix = XMLoadFloat4x4(&view);
    const XMMATRIX projectionMatrix = XMLoadFloat4x4(&projection);

    const XMMATRIX worldViewMatrix =
        XMMatrixMultiply(worldMatrix, viewMatrix);

    const float nearZ = camera->GetDesc().nearZ;
    const float nearPlaneZ = -nearZ;

    bool hasPointInFrontOfNearPlane = false;
    bool hasPointBehindNearPlane = false;

    for (const XMFLOAT3& corner : corners)
    {
        const XMVECTOR position =
            XMVector3TransformCoord(
                XMLoadFloat3(&corner),
                worldViewMatrix);

        const float viewZ = XMVectorGetZ(position);

        // SimpleMath Camera는 Right-Handed 좌표계를 사용한다.
        // 카메라 앞쪽은 View Space의 -Z 방향이다.
        if (viewZ <= nearPlaneZ)
            hasPointInFrontOfNearPlane = true;
        else
            hasPointBehindNearPlane = true;
    }

    if (!hasPointInFrontOfNearPlane)
        return false;

    const LONG viewportLeft =
        static_cast<LONG>(std::floor(viewport.TopLeftX));

    const LONG viewportTop =
        static_cast<LONG>(std::floor(viewport.TopLeftY));

    const LONG viewportRight =
        static_cast<LONG>(
            std::ceil(viewport.TopLeftX + viewport.Width));

    const LONG viewportBottom =
        static_cast<LONG>(
            std::ceil(viewport.TopLeftY + viewport.Height));

    const LONG renderLeft =
        std::max(viewportLeft, scissor.left);

    const LONG renderTop =
        std::max(viewportTop, scissor.top);

    const LONG renderRight =
        std::min(viewportRight, scissor.right);

    const LONG renderBottom =
        std::min(viewportBottom, scissor.bottom);

    if (renderRight <= renderLeft ||
        renderBottom <= renderTop)
    {
        return false;
    }

    // BoundingBox가 Near Plane을 가로지르는 경우
    // 잘못된 Projection Bounds가 만들어지는 것을 피하기 위해
    // 일단 전체 Viewport를 사용한다.
    if (hasPointBehindNearPlane)
    {
        outBounds =
        {
            renderLeft,
            renderTop,
            renderRight,
            renderBottom
        };

        return true;
    }

    float minX = FLT_MAX;
    float minY = FLT_MAX;
    float maxX = -FLT_MAX;
    float maxY = -FLT_MAX;

    for (const XMFLOAT3& corner : corners)
    {
        const XMVECTOR projected =
            XMVector3Project(
                XMLoadFloat3(&corner),
                viewport.TopLeftX,
                viewport.TopLeftY,
                viewport.Width,
                viewport.Height,
                viewport.MinDepth,
                viewport.MaxDepth,
                projectionMatrix,
                viewMatrix,
                worldMatrix);

        const float x = XMVectorGetX(projected);
        const float y = XMVectorGetY(projected);

        minX = std::min(minX, x);
        minY = std::min(minY, y);

        maxX = std::max(maxX, x);
        maxY = std::max(maxY, y);
    }

    LONG left =
        static_cast<LONG>(std::floor(minX));

    LONG top =
        static_cast<LONG>(std::floor(minY));

    LONG right =
        static_cast<LONG>(std::ceil(maxX));

    LONG bottom =
        static_cast<LONG>(std::ceil(maxY));

    Material* material = meshRenderer->GetMaterial();

    if (material)
    {
        const float refractionStrength =
            std::abs(material->GetRefractionStrength());

        // gRefractionStrength는 UV 단위이므로
        // 화면 크기를 곱해 픽셀 단위 최대 굴절 거리를 구한다.
        const LONG marginX =
            static_cast<LONG>(
                std::ceil(refractionStrength * viewport.Width)) + 1;

        const LONG marginY =
            static_cast<LONG>(
                std::ceil(refractionStrength * viewport.Height)) + 1;

        left -= marginX;
        right += marginX;

        top -= marginY;
        bottom += marginY;
    }

    left = std::clamp(left, renderLeft, renderRight);
    top = std::clamp(top, renderTop, renderBottom);

    right = std::clamp(right, renderLeft, renderRight);
    bottom = std::clamp(bottom, renderTop, renderBottom);

    if (right <= left || bottom <= top)
        return false;

    outBounds =
    {
        left,
        top,
        right,
        bottom
    };

    return true;
}