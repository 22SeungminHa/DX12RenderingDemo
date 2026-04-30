#include "TestScene.h"
#include "FBXLoader.h"
#include "Material.h"
#include "Shader.h"
#include "GameObject.h"
#include "Mesh.h"

void TestScene1::OnLoad(ID3D12Device* device, ID3D12GraphicsCommandList* cmdList)
{
    auto shader = std::make_shared<LitShader>();
    shader->CreateShader(device, rootSignature_.Get());

    auto material = std::make_shared<Material>();
    material->SetShader(shader);

    UINT objectCBIndex = 0;

    auto object = FBXLoader::LoadLitModel(
        device,
        cmdList,
        "../Assets/Meshes/MicroSub.fbx",
        material,
        objectCBIndex
    );

    objects_.clear();

    if (object)
        objects_.push_back(std::move(object));
}

CameraDesc TestScene1::SetupCameraDesc() const
{
    CameraDesc desc{};
    desc.eye = { 0.0f, 15.0f, -25.0f };
    desc.target = { 0.0f, 0.0f, 0.0f };
    desc.nearZ = 1.0f;
    desc.farZ = 500.0f;
    desc.fovY = 90.0f;
    return desc;
}

void TestScene2::OnLoad(ID3D12Device* device, ID3D12GraphicsCommandList* cmdList)
{
    auto triMesh = std::make_shared<TriangleMesh>(device, cmdList);

    auto shader = std::make_shared<LitShader>();
    shader->CreateShader(device, rootSignature_.Get());

    auto material = std::make_shared<Material>();
    material->SetShader(shader);

    auto rotatingObject = std::make_unique<RotatingObject>();
    rotatingObject->SetObjectCBIndex(0);
    rotatingObject->SetMesh(triMesh);
    rotatingObject->SetMaterial(material);

    objects_.clear();
    objects_.push_back(std::move(rotatingObject));
}

CameraDesc TestScene2::SetupCameraDesc() const
{
    CameraDesc desc{};
    desc.eye = { 0.0f, 15.0f, -25.0f };
    desc.target = { 0.0f, 0.0f, 0.0f };
    desc.nearZ = 1.0f;
    desc.farZ = 500.0f;
    desc.fovY = 90.0f;
    return desc;
}