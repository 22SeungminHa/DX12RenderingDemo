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
#include "MapObjectLoader.h"

void TestScene::OnLoad(
    ID3D12Device* device,
    ID3D12GraphicsCommandList* cmdList,
    ID3D12RootSignature* rootSignature,
    AssetManager& assetManager)
{
    DirectionalLight* mainLight = AddDirectionalLight();
    mainLight->SetDirection({ 0.0f, -0.916f, 0.40f });
    mainLight->SetColor({ 1.0f, 1.0f, 1.0f, 1.0f });
    mainLight->SetIntensity(1.0f);

    DirectionalLight* fillLight = AddDirectionalLight();
    fillLight->SetDirection({ -0.12f, 0.96f, -0.25f });
    fillLight->SetColor({ 1.0f, 1.0f, 1.0f, 1.0f });
    fillLight->SetIntensity(0.13f);

    DirectionalLight* rimLight = AddDirectionalLight();
    rimLight->SetDirection({ 0.12f, 0.96f, -0.25f });
    rimLight->SetColor({ 1.0f, 1.0f, 1.0f, 1.0f });
    rimLight->SetIntensity(0.13f);

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
        AssetPath::FBX(L"Map1"),
        Vector3::Zero,
        Vector3(0.01f, 0.01f, 0.01f)
    );

    if (!map)
        LOG("Map.fbx load failed");

    LoadObstacles(device, cmdList, rootSignature, assetManager);
    LoadCrystals(device, cmdList, rootSignature, assetManager);

    SetSkybox();
}

void TestScene::LoadObstacles(ID3D12Device* device, ID3D12GraphicsCommandList* cmdList, ID3D12RootSignature* rootSignature, AssetManager& assetManager)
{
    std::vector<MapObstacleData> obstacles;

    if (!MapObjectLoader::LoadObstacles(AssetPath::Data(L"MapObjects"), obstacles))
    {
        LOG("MapObjects.json load failed");
        return;
    }

    auto glassMaterial = assetManager.LoadMaterialFromFile(device, cmdList, rootSignature, AssetPath::Material(L"Default_Glass"));

    if (!glassMaterial)
    {
        LOG("Default_Glass material load failed");
        return;
    }

    UINT loadedCount = 0;

    for (const MapObstacleData& obstacle : obstacles)
    {
        const float width = obstacle.scale.x;
        const float height = obstacle.scale.y;
        const float depth = obstacle.scale.z;

        if (width <= 0.0f || height <= 0.0f || depth <= 0.0f)
        {
            LOG("Invalid obstacle size: " << obstacle.name);
            continue;
        }

        auto glassMesh = assetManager.LoadGlassMesh(device, cmdList, width, height, depth);

        if (!glassMesh)
        {
            LOG("Glass mesh load failed: " << obstacle.name);
            continue;
        }

        GameObject* glassObject = CreateObject(glassMesh, glassMaterial, obstacle.position, Vector3::One);

        if (!glassObject)
            continue;

        auto* glassDestruction = glassObject->AddComponent<GlassComponent>();

        glassDestruction->Initialize(glassMaterial, width, height, depth);

        ++loadedCount;

        LOG(
            obstacle.name
            << " / Position: ("
            << obstacle.position.x << ", "
            << obstacle.position.y << ", "
            << obstacle.position.z << ") / Size: ("
            << width << ", "
            << height << ", "
            << depth << ")"
        );
    }

    LOG("Obstacle load complete: " << loadedCount);
}

void TestScene::LoadCrystals(ID3D12Device* device, ID3D12GraphicsCommandList* cmdList, ID3D12RootSignature* rootSignature, AssetManager& assetManager)
{
    std::vector<MapCrystalData> crystals;

    if (!MapObjectLoader::LoadCrystals(AssetPath::Data(L"MapObjects"), crystals))
    {
        LOG("Crystal data load failed");
        return;
    }

    auto crystalMesh =
        FBXLoader::LoadLitMeshFromFile(
            device,
            cmdList,
            assetManager,
            AssetPath::OBJ(L"Crystal"));

    if (!crystalMesh)
    {
        LOG("Crystal.obj load failed");
        return;
    }

    auto glassMaterial =
        assetManager.LoadMaterialFromFile(
            device,
            cmdList,
            rootSignature,
            AssetPath::Material(L"Default_Glass"));

    if (!glassMaterial)
    {
        LOG("Default_Glass material load failed");
        return;
    }

    UINT loadedCount = 0;

    for (const MapCrystalData& crystal : crystals)
    {
        GameObject* crystalObject =
            CreateObject(
                crystalMesh,
                glassMaterial,
                crystal.position,
                Vector3::Vector3(1.2f));

        if (!crystalObject)
            continue;

        ++loadedCount;

        LOG(
            crystal.name
            << " / Position: ("
            << crystal.position.x << ", "
            << crystal.position.y << ", "
            << crystal.position.z << ")"
        );
    }

    LOG("Crystal load complete: " << loadedCount);
}

CameraDesc TestScene::SetupCameraDesc() const
{
    CameraDesc desc{};
    desc.eye = { 0.0f, 5.15715f, -34.2166f };
    desc.target = { 0.0f, 5.0818f, -33.2204f };
    desc.nearZ = 1.0f;
    desc.farZ = 500.0f;
    desc.fovY = 60.0f;
    return desc;
}

SceneLightDesc TestScene::SetupLightDesc() const
{
    SceneLightDesc desc{};

    desc.ambientColor = { 0.0f, 0.0f, 0.0f, 1.0f };
    desc.specularPower = 32.0f;

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
