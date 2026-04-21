#include "TestScene.h"

void TestScene1::OnLoad(ID3D12Device* device, ID3D12GraphicsCommandList* cmdList)
{
    InitializeCamera();

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

void TestScene1::InitializeCamera()
{
    auto camera = std::make_unique<Camera>();

    camera->SetViewport(0, 0, clientWidth_, clientHeight_, 0.0f, 1.0f);
    camera->SetScissorRect(0, 0, clientWidth_, clientHeight_);

    float aspect = (clientHeight_ == 0) ? 1.0f : static_cast<float>(clientWidth_) / clientHeight_;
    camera->SetProjection(1.0f, 500.0f, aspect, 90.0f);
    camera->SetLookAt(Vector3(0.0f, 15.0f, -25.0f), Vector3(0.0f, 0.0f, 0.0f), Vector3::Up);

    activeCamera_ = camera.get();
    cameras_.push_back(std::move(camera));
}

void TestScene1::OnResize(UINT width, UINT height)
{
    if (!activeCamera_) return;

    activeCamera_->SetViewport(0, 0, float(width), float(height));
    activeCamera_->SetScissorRect(0, 0, width, height);

    float aspect = (height == 0) ? 1.0f : static_cast<float>(width) / height;
    activeCamera_->SetProjection(1.0f, 500.0f, aspect, 90.0f);
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