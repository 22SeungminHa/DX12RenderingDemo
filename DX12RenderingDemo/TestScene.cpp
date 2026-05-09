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
    UINT objectCBIndex = 0;

    auto object = FBXLoader::LoadLitModel(
        device,
        cmdList,
        rootSignature,
        assetManager,
        "../Assets/Meshes/MicroSub.fbx",
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
