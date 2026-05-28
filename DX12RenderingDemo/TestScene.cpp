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
    //
    //if (object)
    //    objects_.push_back(std::move(object));

    auto cubeMesh = assetManager.LoadCubeMesh(device, cmdList);

    auto glassMaterial = assetManager.LoadMaterialFromFile(
        device,
        cmdList,
        rootSignature,
        AssetPath::Material(L"Default_Glass")
    );

    if (!cubeMesh || !glassMaterial)
        return;

    CreateObject(
        cubeMesh,
        glassMaterial,
        Vector3(0.0f, 3.0f, 0.0f),
        Vector3(5.0f, 5.0f, 5.0f)
    );

    SetSkybox(L"Skybox");
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
