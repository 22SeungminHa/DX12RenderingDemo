#include "TestScene.h"
#include "FBXLoader.h"
#include "Material.h"
#include "Shader.h"
#include "GameObject.h"
#include "Mesh.h"
#include "Texture.h"
#include "AssetManager.h"
#include "InputSystem.h"
#include "GlassComponent.h"
#include "Camera.h"

void TestScene::OnLoad(
    ID3D12Device* device,
    ID3D12GraphicsCommandList* cmdList,
    ID3D12RootSignature* rootSignature,
    AssetManager& assetManager)
{
    //constexpr float glassWidth = 10.0f;
    //constexpr float glassHeight = 10.0f;
    //constexpr float glassDepth = 10.0f;

    //auto glassMesh = assetManager.LoadGlassMesh(
    //    device,
    //    cmdList,
    //    glassWidth,
    //    glassHeight,
    //    glassDepth
    //);

    //auto glassMaterial = assetManager.LoadMaterialFromFile(
    //    device,
    //    cmdList,
    //    rootSignature,
    //    AssetPath::Material(L"Default_Glass")
    //);

    //if (!glassMesh || !glassMaterial)
    //    return;

    //GameObject* glassObject = CreateObject(
    //    glassMesh,
    //    glassMaterial,
    //    Vector3(0.0f, 10.0f, 0.0f),
    //    Vector3::One
    //);

    //auto* glassDestruction = glassObject->AddComponent<GlassComponent>();

    //glassDestruction->Initialize(
    //    glassMaterial,
    //    glassWidth,
    //    glassHeight,
    //    glassDepth
    //);

    GameObject* map = CreateFBXObject(
        device,
        cmdList,
        rootSignature,
        assetManager,
        AssetPath::FBX(L"Map"),
        Vector3::Zero,
        Vector3(0.01f, 0.01f, 0.01f)
    );

    if (!map)
        LOG("Map.fbx load failed");

    SetSkybox();
}

CameraDesc TestScene::SetupCameraDesc() const
{
    CameraDesc desc{};
    desc.eye = { -0.12562f, 5.15715f, -34.2166f };
    desc.target = { -0.170477f, 5.0818f, -33.2204f };
    desc.nearZ = 1.0f;
    desc.farZ = 500.0f;
    desc.fovY = 60.0f;
    return desc;
}

void TestScene::OnProcessInput(
    const InputSystem& input,
    float deltaTime)
{
    if (input.WasKeyPressed('P'))
    {
        Camera* camera = GetActiveCamera();

        if (camera)
        {
            const Vector3 position = camera->GetPosition();
            const Vector3 target = position + camera->GetForward();

            LOG("desc.eye = { " << position.x << "f, " << position.y << "f, " << position.z << "f };");
            LOG("desc.target = { " << target.x << "f, " << target.y << "f, " << target.z << "f };");
        }
    }

    if (input.WasKeyPressed(VK_SPACE))
    {
        ForEachComponent<GlassComponent>(
            [](GlassComponent& destruction)
            {
                destruction.Break(Vector2(0.0f, 0.0f), 8, 4);
            });
    }
}

void TestScene::OnPrepareRenderResources(
    ID3D12Device* device,
    ID3D12GraphicsCommandList* cmdList,
    std::vector<ComPtr<ID3D12Resource>>& transientUploadResources)
{
    ForEachComponent<GlassComponent>(
        [&](GlassComponent& destruction)
        {
            destruction.PrepareRenderResources(
                *this,
                device,
                cmdList,
                transientUploadResources
            );
        });
}
