#include "TestScene.h"
#include "FBXLoader.h"
#include "Material.h"
#include "Shader.h"
#include "GameObject.h"
#include "Mesh.h"
#include "Texture.h"
#include "AssetManager.h"
#include "InputSystem.h"
#include "GlassComponent.h"
#include "Camera.h"
#include "MapObjectLoader.h"
#include "CrystalGlassComponent.h"
#include "ColliderComponent.h"
#include "ProjectileComponent.h"
#include "GlassFragmentComponent.h"

void TestScene::OnLoad(
    ID3D12Device* device,
    ID3D12GraphicsCommandList* cmdList,
    ID3D12RootSignature* rootSignature,
    AssetManager& assetManager)
{
    DirectionalLight* mainLight = AddDirectionalLight();
    mainLight->SetDirection({ 0.0f, -0.916f, 0.40f });
    mainLight->SetColor({ 1.0f, 1.0f, 1.0f, 1.0f });
    mainLight->SetIntensity(1.0f);

    DirectionalLight* fillLight = AddDirectionalLight();
    fillLight->SetDirection({ -0.12f, 0.96f, -0.25f });
    fillLight->SetColor({ 1.0f, 1.0f, 1.0f, 1.0f });
    fillLight->SetIntensity(0.13f);

    DirectionalLight* rimLight = AddDirectionalLight();
    rimLight->SetDirection({ 0.12f, 0.96f, -0.25f });
    rimLight->SetColor({ 1.0f, 1.0f, 1.0f, 1.0f });
    rimLight->SetIntensity(0.13f);

    LoadMap(device, cmdList, rootSignature, assetManager);
    LoadObstacles(device, cmdList, rootSignature, assetManager);
    LoadCrystals(device, cmdList, rootSignature, assetManager);

    projectileMesh_ = assetManager.LoadSphereMesh(device, cmdList);
    projectileMaterial_ = assetManager.LoadMaterialFromFile(device, cmdList, rootSignature, AssetPath::Material(L"Default_Marble"));

    if (!projectileMesh_)
        LOG("Projectile sphere mesh load failed");

    if (!projectileMaterial_)
        LOG("Projectile material load failed");

    SetSkybox();

    if (Camera* camera = GetActiveCamera())
    {
        cameraMode_ = CameraControlMode::AutoForward;
        autoCameraSpeed_ = 5.0f;
        autoCameraPosition_ = camera->GetPosition();
        autoCameraForward_ = camera->GetForward();

        if (autoCameraForward_.LengthSquared() < 0.000001f)
            autoCameraForward_ = Vector3(0.0f, 0.0f, 1.0f);
    }
}

void TestScene::LoadMap(
    ID3D12Device* device,
    ID3D12GraphicsCommandList* cmdList,
    ID3D12RootSignature* rootSignature,
    AssetManager& assetManager)
{
    GameObject* mapRoot = CreateGameObject();

    if (!mapRoot)
        return;

    auto visualRoot =
        std::make_unique<GameObject>();

    visualRoot->SetScale(Vector3(0.01f));

    GameObject* visualRootPtr =
        visualRoot.get();

    mapRoot->AddChild(
        std::move(visualRoot));


    auto collisionRoot =
        std::make_unique<GameObject>();

    GameObject* collisionRootPtr =
        collisionRoot.get();

    mapRoot->AddChild(
        std::move(collisionRoot));


    constexpr UINT mapCount = 7;

    UINT loadedMapCount = 0;
    UINT loadedColliderCount = 0;

    for (UINT i = 1; i <= mapCount; ++i)
    {
        wchar_t mapName[16]{};

        swprintf_s(
            mapName,
            L"Map_%02u",
            i);


        // -------------------------
        // Visual
        // -------------------------

        auto modelData =
            FBXLoader::LoadLitModel(
                device,
                cmdList,
                rootSignature,
                assetManager,
                AssetPath::FBX(mapName));

        if (modelData)
        {
            if (CreateFBXChildObject(
                visualRootPtr,
                *modelData))
            {
                ++loadedMapCount;
            }
        }
        else
        {
            LOG(
                "Map FBX load failed: "
                << AssetPath::FBX(mapName).string());
        }


        // -------------------------
        // Collision
        // -------------------------

        std::vector<CubeData> cubes;

        if (!MapObjectLoader::LoadMapCubes(
            AssetPath::Data(mapName),
            cubes))
        {
            LOG(
                "Map collision data load failed: "
                << AssetPath::Data(mapName).string());

            continue;
        }

        for (const CubeData& cube : cubes)
        {
            if (cube.scale.x <= 0.0f ||
                cube.scale.y <= 0.0f ||
                cube.scale.z <= 0.0f)
            {
                LOG(
                    "Invalid map collider: "
                    << cube.name);

                continue;
            }

            auto colliderObject =
                std::make_unique<GameObject>();

            colliderObject->SetPosition(
                cube.position);

            auto* collider =
                colliderObject->AddComponent<MapColliderComponent>();

            collider->SetLocalSize(
                cube.scale);

            collisionRootPtr->AddChild(
                std::move(colliderObject));

            ++loadedColliderCount;
        }
    }

    LOG(
        "Map load complete / Maps: "
        << loadedMapCount
        << " / Colliders: "
        << loadedColliderCount);
}

void TestScene::LoadObstacles(ID3D12Device* device, ID3D12GraphicsCommandList* cmdList, ID3D12RootSignature* rootSignature, AssetManager& assetManager)
{
    std::vector<CubeData> obstacles;

    if (!MapObjectLoader::LoadObstacles(
        AssetPath::Data(L"Obstacles"),
        obstacles))
    {
        LOG("Obstacles.json load failed");
        return;
    }

    auto obstacleMaterial = assetManager.LoadMaterialFromFile(device, cmdList, rootSignature, AssetPath::Material(L"Obstacle_Glass"));

    if (!obstacleMaterial)
    {
        LOG("Default_Glass material load failed");
        return;
    }

    UINT loadedCount = 0;

    for (const CubeData& obstacle : obstacles)
    {
        const float width = obstacle.scale.x;
        const float height = obstacle.scale.y;
        const float depth = obstacle.scale.z;

        if (width <= 0.0f || height <= 0.0f || depth <= 0.0f)
        {
            LOG("Invalid obstacle size: " << obstacle.name);
            continue;
        }

        auto glassMesh = assetManager.LoadGlassMesh(device, cmdList, width, height, depth);

        if (!glassMesh)
        {
            LOG("Glass mesh load failed: " << obstacle.name);
            continue;
        }

        GameObject* glassObject = CreateObject(glassMesh, obstacleMaterial, obstacle.position, Vector3::One);

        if (!glassObject)
            continue;

        auto* glassDestruction = glassObject->AddComponent<GlassComponent>();
        glassDestruction->Initialize(obstacleMaterial, width, height, depth);

        auto* collider = glassObject->AddComponent<BoxColliderComponent>();
        collider->SetLocalSize(Vector3(width, height, depth));

        ++loadedCount;

        LOG(
            obstacle.name
            << " / Position: ("
            << obstacle.position.x << ", "
            << obstacle.position.y << ", "
            << obstacle.position.z << ") / Size: ("
            << width << ", "
            << height << ", "
            << depth << ")"
        );
    }

    LOG("Obstacle load complete: " << loadedCount);
}

void TestScene::LoadCrystals(ID3D12Device* device, ID3D12GraphicsCommandList* cmdList, ID3D12RootSignature* rootSignature, AssetManager& assetManager)
{
    std::vector<CrystalData> crystals;

    if (!MapObjectLoader::LoadCrystals(
        AssetPath::Data(L"Crystals"),
        crystals))
    {
        LOG("Crystals.json load failed");
        return;
    }

    auto crystalMesh = FBXLoader::LoadLitMeshFromFile(device, cmdList, assetManager, AssetPath::FBX(L"Crystal"));

    if (!crystalMesh)
    {
        LOG("Crystal.fbx load failed");
        return;
    }

    auto crashedModelData = FBXLoader::LoadLitModel(device, cmdList, rootSignature, assetManager, AssetPath::FBX(L"CrashedCrystal"));

    if (!crashedModelData)
    {
        LOG("CrashedCrystal.fbx load failed");
        return;
    }

    auto crashedCrystalModel = std::make_shared<FBXNodeData>(std::move(*crashedModelData));

    auto crystalMaterial = assetManager.LoadMaterialFromFile(device, cmdList, rootSignature, AssetPath::Material(L"Crystal_Glass"));

    if (!crystalMaterial)
    {
        LOG("Crystal_Glass material load failed");
        return;
    }

    UINT loadedCount = 0;

    for (const CrystalData& crystal : crystals)
    {
        GameObject* crystalObject = CreateObject(crystalMesh, crystalMaterial, crystal.position, Vector3(0.013f));

        if (!crystalObject)
            continue;

        auto* crystalGlass = crystalObject->AddComponent<CrystalGlassComponent>();

        crystalGlass->Initialize(crystalMaterial, crashedCrystalModel);

        auto* collider = crystalObject->AddComponent<BoxColliderComponent>();

        if (crystalMesh->HasLocalBounds())
            collider->SetLocalBounds(crystalMesh->GetLocalBounds());

        ++loadedCount;

        LOG(
            crystal.name
            << " / Position: ("
            << crystal.position.x << ", "
            << crystal.position.y << ", "
            << crystal.position.z << ")"
        );
    }

    LOG("Crystal load complete: " << loadedCount);
}

CameraDesc TestScene::SetupCameraDesc() const
{
    CameraDesc desc{};

    desc.eye = { 0.0f, 5.0f, -35.0f };
    desc.target = { 0.0f, 5.0f, -35.0f };
    desc.nearZ = 1.0f;
    desc.farZ = 500.0f;
    desc.fovY = 60.0f;

    return desc;
}

SceneLightDesc TestScene::SetupLightDesc() const
{
    SceneLightDesc desc{};

    desc.ambientColor = { 0.0f, 0.0f, 0.0f, 1.0f };
    desc.specularPower = 32.0f;

    return desc;
}

void TestScene::OnProcessInput(
    const InputSystem& input,
    float deltaTime)
{
    if (input.WasKeyPressed('C'))
    {
        ToggleCameraMode();
    }

    if (input.WasKeyPressed('P'))
    {
        Camera* camera = GetActiveCamera();

        if (camera)
        {
            const Vector3 position = camera->GetPosition();
            const Vector3 target = position + camera->GetForward();

            LOG("desc.eye = { " << position.x << "f, " << position.y << "f, " << position.z << "f };");
            LOG("desc.target = { " << target.x << "f, " << target.y << "f, " << target.z << "f };");
        }
    }

    if (input.WasLeftMousePressed())
    {
        const POINT mousePosition = input.GetMousePosition();
        FireProjectile(Vector2(static_cast<float>(mousePosition.x), static_cast<float>(mousePosition.y)));
    }
}

void TestScene::Animate(float deltaTime)
{
    UpdateAutoCamera(deltaTime);

    Scene::Animate(deltaTime);

    CheckProjectileCollisions();

    RemoveObjectsBehindCamera();

    FlushDestroyedGameObjects();
}

void TestScene::OnPrepareRenderResources(
    ID3D12Device* device,
    ID3D12GraphicsCommandList* cmdList,
    std::vector<ComPtr<ID3D12Resource>>& transientUploadResources)
{
    ForEachComponent<GlassComponent>(
        [&](GlassComponent& destruction)
        {
            destruction.PrepareRenderResources(
                *this,
                device,
                cmdList,
                transientUploadResources
            );
        });
}

void TestScene::FireProjectile(
    const Vector2& screenPosition)
{
    if (!projectileMesh_ || !projectileMaterial_)
        return;

    Camera* camera = GetActiveCamera();

    if (!camera)
        return;

    constexpr float projectileRadius = 0.4f;
    constexpr float spawnDistance = 1.5f;
    constexpr float launchSpeed = 25.0f;

    const Vector3 direction = camera->ScreenPointToWorldDirection(screenPosition);

    const Vector3 spawnPosition = camera->GetPosition() + direction * spawnDistance;

    GameObject* projectile = CreateObject(projectileMesh_, projectileMaterial_, spawnPosition, Vector3(projectileRadius));

    if (!projectile)
        return;

    auto* projectileComponent = projectile->AddComponent<ProjectileComponent>();
    projectileComponent->Initialize(direction * launchSpeed);

    auto* collider = projectile->AddComponent<SphereColliderComponent>();
    collider->SetLocalRadius(1.0f);
}

void TestScene::CheckProjectileCollisions()
{
    ForEachComponent<ProjectileComponent>([&](ProjectileComponent& projectile)
    {
        GameObject* projectileObject = projectile.GetOwner();

        if (!projectileObject)
            return;

        auto* sphereCollider = projectileObject->GetComponent<SphereColliderComponent>();

        if (!sphereCollider || !sphereCollider->IsEnabled())
            return;

        ForEachComponent<BoxColliderComponent>([&](BoxColliderComponent& boxCollider)
        {
            if (!boxCollider.IsEnabled())
                return;
            if (projectileObject->IsPendingDestroy())
                return;

            float hitT = 0.0f;
            Vector3 hitCenter = Vector3::Zero;

            if (!CollisionSystem::SweepIntersects(
                projectile.GetPreviousPosition(),
                *sphereCollider,
                boxCollider,
                hitT,
                hitCenter))
            {
                return;
            }

            GameObject* target = boxCollider.GetOwner();

            if (!target)
                return;

            if (auto* glass = target->GetComponent<GlassComponent>())
            {
                const Vector3 worldImpactPoint = hitCenter;
                const Matrix inverseWorld = target->GetWorldMatrix().Invert();
                const Vector3 localImpactPoint = Vector3::Transform(worldImpactPoint, inverseWorld );
                const Vector3 localSize = boxCollider.GetLocalSize();
                const Vector2 impactPoint(
                    std::clamp(localImpactPoint.x, -localSize.x * 0.5f, localSize.x * 0.5f),
                    std::clamp(localImpactPoint.y, -localSize.y * 0.5f, localSize.y * 0.5f)
                );

                if (glass->Break(impactPoint))
                {
                    boxCollider.SetEnabled(false);

                    LOG(
                        "Projectile hit obstacle glass / Impact: ("
                        << impactPoint.x << ", "
                        << impactPoint.y << ")"
                    );
                }

                return;
            }

            if (auto* crystal = target->GetComponent<CrystalGlassComponent>())
            {
                if (crystal->Break(*this))
                {
                    boxCollider.SetEnabled(false);
                    LOG("Projectile hit crystal");
                }

                return;
            }

            if (target->GetComponent<MapColliderComponent>())
            {
                sphereCollider->SetEnabled(false);

                DestroyGameObject(projectileObject);

                LOG("Projectile hit map");

                return;
            }
        });
    });
}

void TestScene::RemoveObjectsBehindCamera()
{
    Camera* camera = GetActiveCamera();

    if (!camera || cameraMode_ != CameraControlMode::AutoForward)
        return;

    const Vector3 cameraPosition = camera->GetPosition();
    const Vector3 cameraForward = camera->GetForward();
    constexpr float removeDistanceBehind = 1.0f;

    std::vector<GameObject*> candidates;

    auto addCandidate =
        [&](GameObject* object)
        {
            if (!object || std::find(candidates.begin(), candidates.end(), object) != candidates.end())
                return;

            candidates.push_back(object);
        };

    // 쇠구슬
    ForEachComponent<ProjectileComponent>(
        [&](ProjectileComponent& projectile)
        {
            addCandidate(projectile.GetOwner());
        });

    // 장애물 유리
    ForEachComponent<GlassComponent>(
        [&](GlassComponent& glass)
        {
            addCandidate(glass.GetOwner());
        });

    // 크리스탈
    ForEachComponent<CrystalGlassComponent>(
        [&](CrystalGlassComponent& crystal)
        {
            addCandidate(crystal.GetOwner());
        });

    // 크리스탈/장애물 유리 파편
    ForEachComponent<GlassFragmentComponent>(
        [&](GlassFragmentComponent& fragment)
        {
            addCandidate(fragment.GetOwner());
        });

    for (GameObject* object : candidates)
    {
        if (!object || object->IsPendingDestroy())
            continue;

        const Vector3 worldPosition = object->GetWorldPosition();
        const Vector3 cameraToObject = worldPosition - cameraPosition;
        const float forwardDistance = cameraToObject.Dot(cameraForward);

        if (forwardDistance < -removeDistanceBehind)
            DestroyGameObject(object);
    }
}

void TestScene::ToggleCameraMode()
{
    Camera* camera = GetActiveCamera();

    if (!camera)
        return;

    if (cameraMode_ == CameraControlMode::AutoForward)
    {
        cameraMode_ = CameraControlMode::Free;
        LOG("Camera Mode: Free");

        return;
    }

    cameraMode_ = CameraControlMode::AutoForward;

    RestoreAutoCamera();

    LOG("Camera Mode: AutoForward");
}

void TestScene::RestoreAutoCamera()
{
    Camera* camera = GetActiveCamera();

    if (!camera)
        return;

    camera->SetLookAt(autoCameraPosition_, autoCameraPosition_ + autoCameraForward_);
}
void TestScene::UpdateAutoCamera(float deltaTime)
{
    if (cameraMode_ != CameraControlMode::AutoForward)
        return;

    if (deltaTime <= 0.0f)
        return;

    Camera* camera = GetActiveCamera();

    if (!camera)
        return;

    autoCameraPosition_ += autoCameraForward_ * (autoCameraSpeed_ * deltaTime);

    camera->SetLookAt(autoCameraPosition_, autoCameraPosition_ + autoCameraForward_);
}
