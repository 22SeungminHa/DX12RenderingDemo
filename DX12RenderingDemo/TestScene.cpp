#include "TestScene.h"
#include "FBXLoader.h"
#include "Material.h"
#include "Shader.h"
#include "GameObject.h"
#include "Mesh.h"

void TestScene1::OnLoad(ID3D12Device* device, ID3D12GraphicsCommandList* cmdList)
{
    auto shader = std::make_shared<DiffusedShader>();
    shader->CreateShader(device, rootSignature_.Get());

    auto material = std::make_shared<Material>();
    material->SetShader(shader);

    UINT objectCBIndex = 0;

    auto object = FBXLoader::LoadDiffusedModel(
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

void TestScene1::SetupCameraDesc()
{
    cameraDesc_.eye = { 0.0f, 15.0f, -25.0f };
    cameraDesc_.target = { 0.0f, 0.0f, 0.0f };
    cameraDesc_.nearZ = 1.0f;
    cameraDesc_.farZ = 500.0f;
    cameraDesc_.fovY = 90.0f;
}

void TestScene2::OnLoad(ID3D12Device* device, ID3D12GraphicsCommandList* cmdList)
{
    auto triMesh = std::make_shared<TriangleMesh>(device, cmdList);

    auto shader = std::make_shared<DiffusedShader>();
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

void TestScene2::SetupCameraDesc()
{
    cameraDesc_.eye = { 0.0f, 15.0f, -25.0f };
    cameraDesc_.target = { 0.0f, 0.0f, 0.0f };
    cameraDesc_.nearZ = 1.0f;
    cameraDesc_.farZ = 500.0f;
    cameraDesc_.fovY = 90.0f;
}