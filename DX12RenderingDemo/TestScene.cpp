#include "TestScene.h"
#include "FBXLoader.h"
#include "Material.h"
#include "Shader.h"
#include "GameObject.h"
#include "Mesh.h"
#include "Texture.h"

void TestScene::OnLoad(ID3D12Device* device, ID3D12GraphicsCommandList* cmdList, ID3D12RootSignature* rootSignature)
{
    auto shader = std::make_shared<LitShader>();
    shader->CreateShader(device, rootSignature);

    std::vector<std::shared_ptr<Material>> materials;

    auto bodyTexture = std::make_shared<Texture>();
    bodyTexture->LoadDDS(
        device,
        cmdList,
        L"../Assets/Textures/MicroSub_Albedo.dds"
    );

    auto bodyMaterial = std::make_shared<Material>();
    bodyMaterial->SetShader(shader);
    bodyMaterial->SetTexture(bodyTexture);

    materials.push_back(bodyMaterial);

    auto glassTexture = std::make_shared<Texture>();
    glassTexture->LoadDDS(
        device,
        cmdList,
        L"../Assets/Textures/MicroSub_Glass_MetallicSmoothness.dds"
    );

    auto glassMaterial = std::make_shared<Material>();
    glassMaterial->SetShader(shader);
    glassMaterial->SetTexture(glassTexture);

    materials.push_back(glassMaterial);

    UINT objectCBIndex = 0;

    auto object = FBXLoader::LoadLitModel(
        device,
        cmdList,
        "../Assets/Meshes/MicroSub.fbx",
        materials,
        objectCBIndex
    );

    objects_.clear();

    if (object)
        objects_.push_back(std::move(object));
}

CameraDesc TestScene::SetupCameraDesc() const
{
    CameraDesc desc{};
    desc.eye = { 0.0f, 15.0f, -25.0f };
    desc.target = { 0.0f, 0.0f, 0.0f };
    desc.nearZ = 1.0f;
    desc.farZ = 500.0f;
    desc.fovY = 90.0f;
    return desc;
}
