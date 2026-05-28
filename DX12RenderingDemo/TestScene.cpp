#include "TestScene.h"
#include "FBXLoader.h"
#include "Material.h"
#include "Shader.h"
#include "GameObject.h"
#include "Mesh.h"
#include "Texture.h"
#include "AssetManager.h"

void TestScene::OnLoad(
    ID3D12Device* device,
    ID3D12GraphicsCommandList* cmdList,
    ID3D12RootSignature* rootSignature,
    AssetManager& assetManager)
{
    objects_.clear();

    UINT objectCBIndex = 0;

    //auto object = FBXLoader::LoadLitModel(
    //    device,
    //    cmdList,
    //    rootSignature,
    //    assetManager,
    //    "../Assets/Meshes/MicroSub.fbx",
    //    objectCBIndex
    //);

    //if (object)
    //    objects_.push_back(std::move(object));

    auto glassCube = std::make_unique<GameObject>();
    glassCube->SetObjectCBIndex(++objectCBIndex);

    //glassCube->GetTransform()->SetPosition({ 0.0f, 3.0f, 0.0f });
    //glassCube->GetTransform()->SetScale({ 5.0f, 5.0f, 5.0f });

    auto cubeRenderer = glassCube->AddComponent<MeshRenderer>();

    cubeRenderer->SetMesh(assetManager.LoadCubeMesh(device, cmdList));

    auto glassMaterial = std::make_shared<Material>();
    glassMaterial->SetKey("TestGlassCubeMaterial");
    glassMaterial->SetShader(assetManager.LoadGlassShader(device, rootSignature));
    glassMaterial->SetRenderMode(RenderMode::Transparent);
    glassMaterial->SetBaseColor({ 0.7f, 0.9f, 1.0f, 1.0f });
    glassMaterial->SetAlpha(0.25f);
    glassMaterial->SetFresnelPower(3.0f);
    glassMaterial->SetSpecularStrength(1.5f);

    glassMaterial->SetTexture(TextureType::BaseColor, assetManager.GetDefaultTexture(TextureType::BaseColor));
    glassMaterial->SetTexture(
        TextureType::Normal,
        assetManager.LoadTexture(
            device,
            cmdList,
            L"../Assets/Textures/T_YFSM_01_n.dds"
        )
    );

    cubeRenderer->SetMaterial(glassMaterial);

    objects_.push_back(std::move(glassCube));

    SkyboxDesc skybox{};
    skybox.enabled = true;
    skybox.cubemapPath = L"../Assets/Textures/Skybox.dds";

    SetSkybox(skybox);
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
