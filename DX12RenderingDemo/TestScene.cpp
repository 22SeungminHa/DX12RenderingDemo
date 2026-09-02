#include "TestScene.h"
#include "FBXLoader.h"
#include "Material.h"
#include "GameObject.h"
#include "Mesh.h"
#include "AssetManager.h"
#include "InputSystem.h"
#include "GlassComponent.h"
#include "Camera.h"
#include "MapObjectLoader.h"
#include "CrystalObject.h"
#include "ColliderComponent.h"
#include "ProjectileObject.h"
#include "FragmentMotionComponent.h"
#include "ObstacleObject.h"

void TestScene::OnLoad(
    ID3D12Device* device,
    ID3D12GraphicsCommandList* cmdList,
    ID3D12RootSignature* rootSignature,
    AssetManager& assetManager)
{
    CreateLights();

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

void TestScene::CreateLights()
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
    mapRoot->SetScale(Vector3(0.01f));

    constexpr UINT mapCount = 7;
    UINT loadedMapCount = 0;
    UINT loadedColliderCount = 0;

    for (UINT i = 1; i <= mapCount; ++i)
    {
        wchar_t mapName[16]{};
        swprintf_s(mapName, L"Map_%02u", i);

        auto modelData = FBXLoader::LoadLitModel(device, cmdList, rootSignature, assetManager, AssetPath::FBX(mapName));
        if (!modelData)
        {
            LOG("Map FBX load failed: " << AssetPath::FBX(mapName).string());
            continue;
        }

        std::vector<GameObject*> meshObjects;
        GameObject* mapObject = CreateFBXChildObject(mapRoot, *modelData, nullptr, &meshObjects);
        if (!mapObject)
            continue;

        ++loadedMapCount;

        for (GameObject* meshObject : meshObjects)
        {
            if (!meshObject)
                continue;

            auto* renderer = meshObject->GetComponent<MeshRenderer>();
            if (!renderer)
                continue;

            Mesh* mesh = renderer->GetMesh();
            if (!mesh || !mesh->HasLocalBounds())
                continue;

            meshObject->SetObjectType(ObjectType::Map);

            auto* collider = meshObject->AddComponent<BoxColliderComponent>();
            collider->SetLocalBounds(mesh->GetLocalBounds());

            ++loadedColliderCount;
        }
    }

    LOG("Map load complete / Maps: " << loadedMapCount << " / Colliders: " << loadedColliderCount);
}

void TestScene::LoadObstacles(ID3D12Device* device, ID3D12GraphicsCommandList* cmdList, ID3D12RootSignature* rootSignature, AssetManager& assetManager)
{
    std::vector<CubeData> obstacles;
    if (!MapObjectLoader::LoadObstacles(AssetPath::Data(L"Obstacles"), obstacles))
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

        ObstacleObject* obstacleObject = CreateGameObject<ObstacleObject>();
        if (!obstacleObject)
            continue;
        obstacleObject->Initialize(glassMesh, obstacleMaterial, obstacle.position, width, height, depth);

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
    if (!MapObjectLoader::LoadCrystals(AssetPath::Data(L"Crystals"), crystals))
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
        CrystalObject* crystalObject = CreateGameObject<CrystalObject>();
        if (!crystalObject)
            continue;

        crystalObject->Initialize(crystalMesh, crystalMaterial, crashedCrystalModel, crystal.position, Vector3(0.013f));

        ++loadedCount;

        LOG(crystal.name << " / Position: (" << crystal.position.x << ", " << crystal.position.y << ", " << crystal.position.z << ")");
    }

    LOG("Crystal load complete: " << loadedCount);
}

CameraDesc TestScene::SetupCameraDesc() const
{
    CameraDesc desc{};

    desc.eye = { 0.0f, 4.0f, -35.0f };
    desc.target = { 0.0f, 4.0f, -34.0f };
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

FogDesc TestScene::SetupFogDesc() const
{
    FogDesc desc{};

    desc.enabled = true;

    desc.topColor = { 0.015f, 0.003f, 0.012f, 1.0f };
    desc.middleColor = { 0.15f, 0.21f, 0.53f, 1.0f };
    desc.bottomColor = { 0.46f, 0.78f, 0.87f, 1.0f };

    desc.startDistance = 0.0f;
    desc.endDistance = 50.0f;

    return desc;
}

void TestScene::OnProcessInput(const InputSystem& input, float deltaTime)
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
    ForEachComponent<GlassComponent>([&](GlassComponent& destruction)
    {
        destruction.PrepareRenderResources(
            *this,
            device,
            cmdList,
            transientUploadResources
        );
    });
}

void TestScene::FireProjectile(const Vector2& screenPosition)
{
    if (!projectileMesh_ || !projectileMaterial_)
        return;

    Camera* camera = GetActiveCamera();
    if (!camera)
        return;

    constexpr float projectileRadius = 0.4f;
    constexpr float spawnDistance = 1.5f;
    constexpr float launchSpeed = 40.0f;

    const Vector3 direction = camera->ScreenPointToWorldDirection(screenPosition);
    const Vector3 spawnPosition = camera->GetPosition() + direction * spawnDistance;

    ProjectileObject* projectile = CreateGameObject<ProjectileObject>();
    if (!projectile)
        return;
    projectile->Initialize(projectileMesh_, projectileMaterial_, spawnPosition, projectileRadius, direction * launchSpeed);
}

void TestScene::CheckProjectileCollisions()
{
    ForEachObject(ObjectType::Projectile, [&](GameObject& object)
        {
            auto& projectile =
                static_cast<ProjectileObject&>(object);

            if (projectile.IsPendingDestroy())
                return;

            SphereColliderComponent* sphereCollider =
                projectile.GetCollider();

            if (!sphereCollider ||
                !sphereCollider->IsEnabled())
            {
                return;
            }

            struct ClosestHit
            {
                BoxColliderComponent* collider = nullptr;
                GameObject* target = nullptr;

                float hitT = 0.0f;
                Vector3 hitCenter = Vector3::Zero;
                Vector3 hitNormal = Vector3::Zero;
            };

            ClosestHit closestHit{};
            bool hasHit = false;

            ForEachComponent<BoxColliderComponent>(
                [&](BoxColliderComponent& boxCollider)
                {
                    if (!boxCollider.IsEnabled())
                        return;

                    GameObject* target =
                        boxCollider.GetOwner();

                    if (!target ||
                        target == &projectile ||
                        target->IsPendingDestroy())
                    {
                        return;
                    }

                    float hitT = 0.0f;
                    Vector3 hitCenter = Vector3::Zero;
                    Vector3 hitNormal = Vector3::Zero;

                    if (!CollisionSystem::SweepIntersects(
                        projectile.GetPreviousPosition(),
                        *sphereCollider,
                        boxCollider,
                        hitT,
                        hitCenter,
                        hitNormal))
                    {
                        return;
                    }

                    if (hasHit &&
                        hitT >= closestHit.hitT)
                    {
                        return;
                    }

                    hasHit = true;

                    closestHit.collider =
                        &boxCollider;

                    closestHit.target =
                        target;

                    closestHit.hitT =
                        hitT;

                    closestHit.hitCenter =
                        hitCenter;

                    closestHit.hitNormal =
                        hitNormal;
                });

            if (!hasHit ||
                !closestHit.collider ||
                !closestHit.target)
            {
                return;
            }

            CollisionEvent projectileEvent{};

            projectileEvent.scene = this;
            projectileEvent.self = &projectile;
            projectileEvent.other = closestHit.target;

            projectileEvent.selfCollider =
                sphereCollider;

            projectileEvent.otherCollider =
                closestHit.collider;

            projectileEvent.hitT =
                closestHit.hitT;

            projectileEvent.hitPoint =
                closestHit.hitCenter;

            projectileEvent.hitNormal =
                closestHit.hitNormal;

            projectile.OnCollision(
                projectileEvent);


            CollisionEvent targetEvent{};

            targetEvent.scene = this;
            targetEvent.self = closestHit.target;
            targetEvent.other = &projectile;

            targetEvent.selfCollider =
                closestHit.collider;

            targetEvent.otherCollider =
                sphereCollider;

            targetEvent.hitT =
                closestHit.hitT;

            targetEvent.hitPoint =
                closestHit.hitCenter;

            targetEvent.hitNormal =
                -closestHit.hitNormal;

            closestHit.target->OnCollision(
                targetEvent);
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

    auto addCandidate = [&](GameObject* object)
    {
        if (!object || std::find(candidates.begin(), candidates.end(), object) != candidates.end())
            return;

        candidates.push_back(object);
    };

    auto addObjectsByType = [&](ObjectType type)
    {
        ForEachObject(type, [&](GameObject& object) { addCandidate(&object); });
    };

    addObjectsByType(ObjectType::Projectile);
    addObjectsByType(ObjectType::Obstacle);
    addObjectsByType(ObjectType::Crystal);
    addObjectsByType(ObjectType::GlassFragment);

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
