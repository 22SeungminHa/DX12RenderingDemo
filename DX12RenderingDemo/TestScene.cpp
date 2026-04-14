#include "TestScene.h"

void CTestScene1::Load(ID3D12Device* device, ID3D12GraphicsCommandList* cmdList)
{
    mGraphicsRootSignature = CreateGraphicsRootSignature(device);

    auto cubeMesh = std::make_unique<CCubeMeshDiffused>(device, cmdList, 12.0f, 12.0f, 12.0f);

    auto rotatingObject = std::make_unique<CRotatingObject>();
    rotatingObject->SetMesh(cubeMesh.release());

    auto shader = std::make_unique<CDiffusedShader>();
    shader->CreateShader(device, mGraphicsRootSignature.Get());
    shader->CreateShaderVariables(device, cmdList);
    rotatingObject->SetShader(shader.release());

    mObjects.clear();
    mObjects.push_back(std::move(rotatingObject));
}

void CTestScene2::Load(ID3D12Device* device, ID3D12GraphicsCommandList* cmdList)
{
    mGraphicsRootSignature = CreateGraphicsRootSignature(device);

    auto triMesh = std::make_unique<CTriangleMesh>(device, cmdList);

    auto rotatingObject = std::make_unique<CRotatingObject>();
    rotatingObject->SetMesh(triMesh.release());

    auto shader = std::make_unique<CDiffusedShader>();
    shader->CreateShader(device, mGraphicsRootSignature.Get());
    shader->CreateShaderVariables(device, cmdList);
    rotatingObject->SetShader(shader.release());

    mObjects.clear();
    mObjects.push_back(std::move(rotatingObject));
}