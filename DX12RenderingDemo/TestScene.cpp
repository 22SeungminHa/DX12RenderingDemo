#include "TestScene.h"
#include "FBXLoader.h"
#include "Material.h"
#include "Shader.h"
#include "GameObject.h"
#include "Mesh.h"
#include "Texture.h"
#include "AssetManager.h"
#include "InputSystem.h"
#include "GlassDestructionComponent.h"

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

    constexpr float glassWidth = 10.0f;
    constexpr float glassHeight = 8.0f;
    constexpr float glassDepth = 1.0f;

    auto glassMesh = assetManager.LoadGlassMesh(
        device,
        cmdList,
        glassWidth,
        glassHeight,
        glassDepth
    );

    auto glassMaterial = assetManager.LoadMaterialFromFile(
        device,
        cmdList,
        rootSignature,
        AssetPath::Material(L"Default_Glass")
    );

    if (!glassMesh || !glassMaterial)
        return;

    GameObject* glassObject = CreateObject(
        glassMesh,
        glassMaterial,
        Vector3(0.0f, 4.0f, 0.0f),
        Vector3::One
    );

    auto* glassDestruction = glassObject->AddComponent<GlassDestructionComponent>();

    glassDestruction->Initialize(
        glassMaterial,
        glassWidth,
        glassHeight,
        glassDepth
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

void TestScene::OnProcessInput(
    const InputSystem& input,
    float deltaTime)
{
    if (!input.WasKeyPressed(VK_SPACE))
        return;

    ForEachComponent<GlassDestructionComponent>(
        [](GlassDestructionComponent& destruction)
        {
            destruction.Break(Vector2(0.0f, 0.0f), 8, 4);
        });
}

void TestScene::OnPrepareRenderResources(
    ID3D12Device* device,
    ID3D12GraphicsCommandList* cmdList,
    std::vector<ComPtr<ID3D12Resource>>& transientUploadResources)
{
    ForEachComponent<GlassDestructionComponent>(
        [&](GlassDestructionComponent& destruction)
        {
            destruction.PrepareRenderResources(
                *this,
                device,
                cmdList,
                transientUploadResources
            );
        });
}
