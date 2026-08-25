#include "TestScene.h"
#include "FBXLoader.h"
#include "Material.h"
#include "Shader.h"
#include "GameObject.h"
#include "Mesh.h"
#include "Texture.h"
#include "AssetManager.h"
#include "InputSystem.h"
#include "Camera.h"

void TestScene::OnLoad(
    ID3D12Device* device,
    ID3D12GraphicsCommandList* cmdList,
    ID3D12RootSignature* rootSignature,
    AssetManager& assetManager)
{
    //auto object = FBXLoader::LoadLitModel(
    //    device,
    //    cmdList,
    //    rootSignature,
    //    assetManager,
    //    "../Assets/Meshes/MicroSub.fbx",
    //    objectCBIndex
    //);
    //
    //if (object)
    //    objects_.push_back(std::move(object));

    auto cubeMesh = assetManager.LoadCubeMesh(device, cmdList);

    auto glassMaterial = assetManager.LoadMaterialFromFile(
        device,
        cmdList,
        rootSignature,
        AssetPath::Material(L"Default_Glass")
    );

    if (!cubeMesh || !glassMaterial)
        return;

    glassObject_ = CreateObject(
        cubeMesh,
        glassMaterial,
        Vector3(0.0f, 5.0f, 0.0f),
        Vector3(7.0f, 3.0f, 1.0f)
    );

    //CreateFBXObject(
    //    device, cmdList, rootSignature, assetManager,
    //    AssetPath::FBX(L"MicroSub"),
    //    Vector3(0.0f, 0.0f, 0.0f),
    //    Vector3(1.0f, 1.0f, 1.0f)
    //);

    SetSkybox();
}

CameraDesc TestScene::SetupCameraDesc() const
{
    CameraDesc desc{};
    desc.eye = { 0.0f, 15.0f, -25.0f };
    desc.target = { 0.0f, 0.0f, 0.0f };
    desc.nearZ = 1.0f;
    desc.farZ = 500.0f;
    desc.fovY = 60.0f;
    return desc;
}

bool TestScene::RaycastGlass(const Vector3& rayOrigin, const Vector3& rayDirection, Vector3& localHitPoint) const
{
    if (!glassObject_)
        return false;

    Matrix inverseWorld = glassObject_->GetWorldMatrix().Invert();
    Vector3 localOrigin = Vector3::Transform(rayOrigin, inverseWorld);
    Vector3 localDirection = Vector3::TransformNormal(rayDirection, inverseWorld);

    localDirection.Normalize();

    constexpr float minBound = -1.0f;
    constexpr float maxBound = 1.0f;
    constexpr float epsilon = 0.000001f;

    float tMin = 0.0f;
    float tMax = FLT_MAX;

    auto intersectAxis =
        [&](float origin, float direction) -> bool
        {
            if (std::abs(direction) < epsilon)
                return origin >= minBound && origin <= maxBound;

            float t1 = (minBound - origin) / direction;
            float t2 = (maxBound - origin) / direction;

            if (t1 > t2)
                std::swap(t1, t2);

            tMin = std::max(tMin, t1);
            tMax = std::min(tMax, t2);

            return tMin <= tMax;
        };

    if (!intersectAxis(localOrigin.x, localDirection.x))
        return false;

    if (!intersectAxis(localOrigin.y, localDirection.y))
        return false;

    if (!intersectAxis(localOrigin.z, localDirection.z))
        return false;

    if (tMax < 0.0f)
        return false;

    const float hitT = tMin >= 0.0f ? tMin : tMax;

    localHitPoint = localOrigin + localDirection * hitT;

    return true;
}

void TestScene::OnProcessInput(const InputSystem& input, float deltaTime)
{
    if (!input.WasLeftMousePressed())
        return;

    Camera* camera = GetActiveCamera();

    if (!camera || !glassObject_)
        return;

    POINT mousePos = input.GetMousePosition();

    Vector3 rayOrigin;
    Vector3 rayDirection;

    camera->ScreenPointToRay(static_cast<float>(mousePos.x), static_cast<float>(mousePos.y), rayOrigin, rayDirection);

    Vector3 localHitPoint;

    if (!RaycastGlass(rayOrigin, rayDirection, localHitPoint))
        return;

    Vector2 impactPoint(-localHitPoint.x, localHitPoint.y);

    char buffer[128];

    LOG("Glass Impact : " << impactPoint.x << ", " << impactPoint.y);
}