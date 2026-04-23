#include "TestScene.h"

void TestScene1::OnLoad(ID3D12Device* device, ID3D12GraphicsCommandList* cmdList)
{
    auto cubeMesh = std::make_unique<CubeMeshDiffused>(device, cmdList, 12.0f, 12.0f, 12.0f);

    auto rotatingObject = std::make_unique<RotatingObject>();
    rotatingObject->SetMesh(cubeMesh.release());

    auto shader = std::make_unique<DiffusedShader>();
    shader->CreateShader(device, rootSignature_.Get());
    shader->CreateShaderVariables(device, cmdList);
    rotatingObject->SetShader(shader.release());

    objects_.clear();
    objects_.push_back(std::move(rotatingObject));
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
    auto triMesh = std::make_unique<TriangleMesh>(device, cmdList);

    auto rotatingObject = std::make_unique<RotatingObject>();
    rotatingObject->SetMesh(triMesh.release());

    auto shader = std::make_unique<DiffusedShader>();
    shader->CreateShader(device, rootSignature_.Get());
    shader->CreateShaderVariables(device, cmdList);
    rotatingObject->SetShader(shader.release());

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