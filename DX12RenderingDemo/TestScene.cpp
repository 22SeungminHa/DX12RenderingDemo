#include "TestScene.h"
#include "FBXLoader.h"
#include "Material.h"
#include "Shader.h"
#include "GameObject.h"
#include "Mesh.h"
#include "Texture.h"
#include "AssetManager.h"
#include "InputSystem.h"
#include "GlassFracture.h"
#include "GlassFragmentComponent.h"
#include <random>

void TestScene::OnLoad(
    ID3D12Device* device,
    ID3D12GraphicsCommandList* cmdList,
    ID3D12RootSignature* rootSignature,
    AssetManager& assetManager)
{
    device_ = device;

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

    auto glassMesh = assetManager.LoadGlassMesh(
        device,
        cmdList,
        glassWidth_,
        glassHeight_,
        glassDepth_
    );

    glassMaterial_ = assetManager.LoadMaterialFromFile(
        device,
        cmdList,
        rootSignature,
        AssetPath::Material(L"Default_Glass")
    );

    if (!glassMesh || !glassMaterial_)
        return;

    glassObject_ = CreateObject(
        glassMesh,
        glassMaterial_,
        Vector3(0.0f, 4.0f, 0.0f),
        Vector3::One
    );

    //CreateFBXObject(
    //    device, cmdList, rootSignature, assetManager,
    //    AssetPath::FBX(L"MicroSub"),
    //    Vector3(0.0f, 0.0f, 0.0f),
    //    Vector3(1.0f, 1.0f, 1.0f)
    //);

    SetSkybox();
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

void TestScene::OnProcessInput(
    const InputSystem& input,
    float deltaTime)
{
    if (isBroken_)
        return;

    if (!input.WasKeyPressed(VK_SPACE))
        return;

    auto fragments = GlassFracture::GenerateRadialFragments(
        glassWidth_,
        glassHeight_,
        Vector2(0.0f, 0.0f),
        8
    );

    LOG("Glass fracture generated: " << fragments.size());

    static std::mt19937 randomEngine{ std::random_device{}() };

    std::uniform_real_distribution<float> speedDistribution(4.0f, 7.0f);
    std::uniform_real_distribution<float> zDistribution(1.0f, 2.5f);
    std::uniform_real_distribution<float> angularDistribution(-4.0f, 4.0f);

    for (size_t i = 0; i < fragments.size(); ++i)
    {
        GlassFragmentGeometry geometry =
            GlassFracture::BuildFragmentGeometry(
                fragments[i],
                glassWidth_,
                glassHeight_,
                glassDepth_
            );

        if (geometry.vertices.empty() ||
            geometry.indices.empty())
        {
            continue;
        }

        auto fragmentMesh =
            std::make_shared<RuntimeMeshLit>(
                device_,
                geometry.vertices,
                geometry.indices
            );

        GameObject* fragmentObject =
            CreateChildObject(
                glassObject_,
                fragmentMesh,
                glassMaterial_,
                geometry.localPosition
            );

        if (!fragmentObject)
            continue;

        Vector3 direction(
            geometry.localPosition.x,
            geometry.localPosition.y,
            zDistribution(randomEngine)
        );

        direction.Normalize();

        auto* fragmentMotion =
            fragmentObject->AddComponent<GlassFragmentComponent>();

        fragmentMotion->SetVelocity(
            direction * speedDistribution(randomEngine)
        );

        fragmentMotion->SetAngularVelocity(
            Vector3(
                angularDistribution(randomEngine),
                angularDistribution(randomEngine),
                angularDistribution(randomEngine)
            )
        );
    }

    glassObject_->SetMesh(std::shared_ptr<Mesh>{});

    isBroken_ = true;
}