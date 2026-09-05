#include "GameScene.h"
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

void GameScene::OnLoad(
    ID3D12Device* device,
    ID3D12GraphicsCommandList* cmdList,
    ID3D12RootSignature* rootSignature,
    AssetManager& assetManager)
{
    CreateLights();

    LoadMap(device, cmdList, rootSignature, assetManager);
    LoadObstacles(device, cmdList, rootSignature, assetManager);
    LoadCrystals(device, cmdList, rootSignature, assetManager);

    InitializeMapStreaming();

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

void GameScene::CreateLights()
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

void GameScene::LoadMap(
    ID3D12Device* device,
    ID3D12GraphicsCommandList* cmdList,
    ID3D12RootSignature* rootSignature,
    AssetManager& assetManager)
{
    activeMaps_.clear();
    nextMapSequence_ = 0;
    mapLoopLength_ = 0.0f;

    mapRoot_ = CreateGameObject();

    if (!mapRoot_)
        return;

    mapRoot_->SetScale(Vector3(kMapScale));

    for (UINT i = 0; i < kMapCount; ++i)
    {
        wchar_t mapName[16]{};
        swprintf_s(
            mapName,
            L"Map_%02u",
            i + 1);

        auto modelData =
            FBXLoader::LoadLitModel(
                device,
                cmdList,
                rootSignature,
                assetManager,
                AssetPath::FBX(mapName));

        if (!modelData)
        {
            LOG("Map FBX load failed: "
                << AssetPath::FBX(mapName).string());

            return;
        }

        auto model =
            std::make_shared<FBXNodeData>(
                std::move(*modelData));

        float minZ = 0.0f;
        float maxZ = 0.0f;

        if (!CalculateMapZBounds(
            *model,
            minZ,
            maxZ))
        {
            LOG("Map bounds calculation failed: "
                << AssetPath::FBX(mapName).string());

            return;
        }

        mapTemplates_[i].model = std::move(model);

        mapTemplates_[i].minZ =
            minZ * kMapScale;

        mapTemplates_[i].maxZ =
            maxZ * kMapScale;

        LOG(
            "Map_" << (i + 1)
            << " / Z: "
            << mapTemplates_[i].minZ
            << " ~ "
            << mapTemplates_[i].maxZ);
    }

    mapLoopLength_ =
        mapTemplates_[kMapCount - 1].maxZ -
        mapTemplates_[0].minZ;

    if (mapLoopLength_ <= 0.0f)
    {
        LOG("Invalid map loop length");
        return;
    }

    LOG(
        "Map templates loaded / LoopLength: "
        << mapLoopLength_);
}

void GameScene::LoadObstacles(
    ID3D12Device* device,
    ID3D12GraphicsCommandList* cmdList,
    ID3D12RootSignature* rootSignature,
    AssetManager& assetManager)
{
    for (auto& templates : obstacleTemplates_)
        templates.clear();

    std::vector<CubeData> obstacles;

    if (!MapObjectLoader::LoadObstacles(
        AssetPath::Data(L"Obstacles"),
        obstacles))
    {
        LOG("Obstacles.json load failed");
        return;
    }

    obstacleMaterial_ =
        assetManager.LoadMaterialFromFile(
            device,
            cmdList,
            rootSignature,
            AssetPath::Material(L"Obstacle_Glass"));

    if (!obstacleMaterial_)
    {
        LOG("Obstacle_Glass material load failed");
        return;
    }

    for (const CubeData& obstacle : obstacles)
    {
        const int mapIndex =
            FindMapIndex(obstacle.position.z);

        if (mapIndex < 0)
        {
            LOG(
                "Obstacle outside map bounds: "
                << obstacle.name
                << " / Z: "
                << obstacle.position.z);

            continue;
        }

        const float width = obstacle.scale.x;
        const float height = obstacle.scale.y;
        const float depth = obstacle.scale.z;

        if (width <= 0.0f ||
            height <= 0.0f ||
            depth <= 0.0f)
        {
            LOG("Invalid obstacle size: " << obstacle.name);
            continue;
        }

        auto mesh =
            assetManager.LoadGlassMesh(
                device,
                cmdList,
                width,
                height,
                depth);

        if (!mesh)
            continue;

        ObstacleSpawnData data{};

        data.mesh = std::move(mesh);
        data.position = obstacle.position;
        data.size = obstacle.scale;

        obstacleTemplates_[
            static_cast<UINT>(mapIndex)]
            .push_back(std::move(data));
    }
}

void GameScene::LoadCrystals(
    ID3D12Device* device,
    ID3D12GraphicsCommandList* cmdList,
    ID3D12RootSignature* rootSignature,
    AssetManager& assetManager)
{
    for (auto& templates : crystalTemplates_)
        templates.clear();

    std::vector<CrystalData> crystals;

    if (!MapObjectLoader::LoadCrystals(
        AssetPath::Data(L"Crystals"),
        crystals))
    {
        LOG("Crystals.json load failed");
        return;
    }

    crystalMesh_ =
        FBXLoader::LoadLitMeshFromFile(
            device,
            cmdList,
            assetManager,
            AssetPath::FBX(L"Crystal"));

    if (!crystalMesh_)
    {
        LOG("Crystal.fbx load failed");
        return;
    }

    auto crashedModelData =
        FBXLoader::LoadLitModel(
            device,
            cmdList,
            rootSignature,
            assetManager,
            AssetPath::FBX(L"CrashedCrystal"));

    if (!crashedModelData)
    {
        LOG("CrashedCrystal.fbx load failed");
        return;
    }

    crashedCrystalModel_ =
        std::make_shared<FBXNodeData>(
            std::move(*crashedModelData));

    crystalMaterial_ =
        assetManager.LoadMaterialFromFile(
            device,
            cmdList,
            rootSignature,
            AssetPath::Material(L"Crystal_Glass"));

    if (!crystalMaterial_)
    {
        LOG("Crystal_Glass material load failed");
        return;
    }

    for (const CrystalData& crystal : crystals)
    {
        const int mapIndex =
            FindMapIndex(crystal.position.z);

        if (mapIndex < 0)
        {
            LOG(
                "Crystal outside map bounds: "
                << crystal.name
                << " / Z: "
                << crystal.position.z);

            continue;
        }

        CrystalSpawnData data{};
        data.position = crystal.position;

        crystalTemplates_[
            static_cast<UINT>(mapIndex)]
            .push_back(data);
    }
}

CameraDesc GameScene::SetupCameraDesc() const
{
    CameraDesc desc{};

    desc.eye = { 0.0f, 4.0f, -35.0f };
    desc.target = { 0.0f, 4.0f, -34.0f };
    desc.nearZ = 1.0f;
    desc.farZ = 500.0f;
    desc.fovY = 60.0f;

    return desc;
}

SceneLightDesc GameScene::SetupLightDesc() const
{
    SceneLightDesc desc{};

    desc.ambientColor = { 0.0f, 0.0f, 0.0f, 1.0f };
    desc.specularPower = 32.0f;

    return desc;
}

FogDesc GameScene::SetupFogDesc() const
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

void GameScene::OnProcessInput(const InputSystem& input, float deltaTime)
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

void GameScene::Animate(float deltaTime)
{
    UpdateAutoCamera(deltaTime);

    UpdateMapStreaming();

    Scene::Animate(deltaTime);

    CheckProjectileCollisions();

    RemoveObjectsBehindCamera();

    FlushDestroyedGameObjects();
}

void GameScene::OnPrepareRenderResources(
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

void GameScene::FireProjectile(const Vector2& screenPosition)
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

void GameScene::CheckProjectileCollisions()
{
    ForEachObject(ObjectType::Projectile, [&](GameObject& object)
        {
            auto& projectile = static_cast<ProjectileObject&>(object);

            if (projectile.IsPendingDestroy())
                return;

            SphereColliderComponent* sphereCollider = projectile.GetCollider();

            if (!sphereCollider || !sphereCollider->IsEnabled())
                return;

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

                    GameObject* target = boxCollider.GetOwner();
                    if (!target || target == &projectile || target->IsPendingDestroy())
                        return;

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

                    if (hasHit && hitT >= closestHit.hitT)
                        return;

                    hasHit = true;

                    closestHit.collider = &boxCollider;
                    closestHit.target = target;
                    closestHit.hitT = hitT;
                    closestHit.hitCenter = hitCenter;
                    closestHit.hitNormal = hitNormal;
                });

            if (!hasHit || !closestHit.collider || !closestHit.target)
                return;

            CollisionEvent projectileEvent{};
            projectileEvent.scene = this;
            projectileEvent.self = &projectile;
            projectileEvent.other = closestHit.target;
            projectileEvent.selfCollider = sphereCollider;
            projectileEvent.otherCollider = closestHit.collider;
            projectileEvent.hitT = closestHit.hitT;
            projectileEvent.hitPoint = closestHit.hitCenter;
            projectileEvent.hitNormal = closestHit.hitNormal;

            projectile.OnCollision(projectileEvent);

            CollisionEvent targetEvent{};
            targetEvent.scene = this;
            targetEvent.self = closestHit.target;
            targetEvent.other = &projectile;
            targetEvent.selfCollider = closestHit.collider;
            targetEvent.otherCollider = sphereCollider;
            targetEvent.hitT = closestHit.hitT;
            targetEvent.hitPoint = closestHit.hitCenter;
            targetEvent.hitNormal = -closestHit.hitNormal;

            closestHit.target->OnCollision(targetEvent);
        });
}

void GameScene::RemoveObjectsBehindCamera()
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

void GameScene::ToggleCameraMode()
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

void GameScene::RestoreAutoCamera()
{
    Camera* camera = GetActiveCamera();

    if (!camera)
        return;

    camera->SetLookAt(autoCameraPosition_, autoCameraPosition_ + autoCameraForward_);
}

void GameScene::UpdateAutoCamera(float deltaTime)
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

bool GameScene::CalculateMapZBounds(
    const FBXNodeData& modelData,
    float& outMinZ,
    float& outMaxZ) const
{
    bool hasBounds = false;

    outMinZ = 0.0f;
    outMaxZ = 0.0f;

    AccumulateMapZBounds(
        modelData,
        Matrix::Identity,
        outMinZ,
        outMaxZ,
        hasBounds);

    return hasBounds;
}

void GameScene::AccumulateMapZBounds(
    const FBXNodeData& nodeData,
    const Matrix& parentMatrix,
    float& minZ,
    float& maxZ,
    bool& hasBounds) const
{
    const Matrix nodeMatrix =
        nodeData.localMatrix * parentMatrix;

    for (const FBXMeshData& meshData :
        nodeData.meshes)
    {
        if (!meshData.mesh ||
            !meshData.mesh->HasLocalBounds())
        {
            continue;
        }

        const BoundingBox& bounds =
            meshData.mesh->GetLocalBounds();

        const Vector3 center(
            bounds.Center.x,
            bounds.Center.y,
            bounds.Center.z);

        const Vector3 extents(
            bounds.Extents.x,
            bounds.Extents.y,
            bounds.Extents.z);

        for (int x = -1; x <= 1; x += 2)
        {
            for (int y = -1; y <= 1; y += 2)
            {
                for (int z = -1; z <= 1; z += 2)
                {
                    const Vector3 localPosition(
                        center.x + extents.x * x,
                        center.y + extents.y * y,
                        center.z + extents.z * z);

                    const Vector3 position =
                        Vector3::Transform(
                            localPosition,
                            nodeMatrix);

                    if (!hasBounds)
                    {
                        minZ = position.z;
                        maxZ = position.z;
                        hasBounds = true;

                        continue;
                    }

                    minZ =
                        std::min(minZ, position.z);

                    maxZ =
                        std::max(maxZ, position.z);
                }
            }
        }
    }

    for (const FBXNodeData& child :
        nodeData.children)
    {
        AccumulateMapZBounds(
            child,
            nodeMatrix,
            minZ,
            maxZ,
            hasBounds);
    }
}

void GameScene::SetupMapMeshObjects(
    const std::vector<GameObject*>& meshObjects)
{
    for (GameObject* meshObject :
        meshObjects)
    {
        if (!meshObject)
            continue;

        auto* renderer =
            meshObject->GetComponent<MeshRenderer>();

        if (!renderer)
            continue;

        Mesh* mesh =
            renderer->GetMesh();

        if (!mesh ||
            !mesh->HasLocalBounds())
        {
            continue;
        }

        meshObject->SetObjectType(
            ObjectType::Map);

        auto* collider =
            meshObject->AddComponent<
            BoxColliderComponent>();

        collider->SetLocalBounds(
            mesh->GetLocalBounds());
    }
}

GameObject* GameScene::SpawnMapSegment(
    UINT sequence)
{
    if (!mapRoot_ ||
        mapLoopLength_ <= 0.0f)
    {
        return nullptr;
    }

    const UINT mapIndex =
        sequence % kMapCount;

    const UINT loopIndex =
        sequence / kMapCount;

    MapTemplate& mapTemplate =
        mapTemplates_[mapIndex];

    if (!mapTemplate.model)
        return nullptr;

    std::vector<GameObject*> meshObjects;

    GameObject* mapObject =
        CreateFBXChildObject(
            mapRoot_,
            *mapTemplate.model,
            nullptr,
            &meshObjects);

    if (!mapObject)
        return nullptr;

    const float loopOffsetZ =
        static_cast<float>(loopIndex) *
        mapLoopLength_;

    if (loopOffsetZ != 0.0f)
    {
        mapObject->TranslateWorld(
            Vector3(
                0.0f,
                0.0f,
                loopOffsetZ));
    }

    SetupMapMeshObjects(meshObjects);

    ActiveMapSegment segment{};

    segment.object = mapObject;
    segment.sequence = sequence;

    segment.startZ =
        mapTemplate.minZ + loopOffsetZ;

    segment.endZ =
        mapTemplate.maxZ + loopOffsetZ;

    SpawnSegmentContents(
        mapIndex,
        loopOffsetZ,
        segment.objects);

    activeMaps_.push_back(
        std::move(segment));

    LOG("Spawn Map_"
        << (mapIndex + 1)
        << " / Sequence: "
        << sequence
        << " / Z: "
        << segment.startZ
        << " ~ "
        << segment.endZ);

    return mapObject;
}

void GameScene::SpawnSegmentContents(
    UINT mapIndex,
    float loopOffsetZ,
    std::vector<GameObject*>& outObjects)
{
    if (mapIndex >= kMapCount)
        return;

    for (const ObstacleSpawnData& data :
        obstacleTemplates_[mapIndex])
    {
        if (!data.mesh || !obstacleMaterial_)
            continue;

        Vector3 position = data.position;
        position.z += loopOffsetZ;

        ObstacleObject* obstacle =
            CreateGameObject<ObstacleObject>();

        if (!obstacle)
            continue;

        obstacle->Initialize(
            data.mesh,
            obstacleMaterial_,
            position,
            data.size.x,
            data.size.y,
            data.size.z);

        outObjects.push_back(obstacle);
    }

    if (crystalMesh_ &&
        crystalMaterial_ &&
        crashedCrystalModel_)
    {
        for (const CrystalSpawnData& data :
            crystalTemplates_[mapIndex])
        {
            Vector3 position = data.position;
            position.z += loopOffsetZ;

            CrystalObject* crystal =
                CreateGameObject<CrystalObject>();

            if (!crystal)
                continue;

            crystal->Initialize(
                crystalMesh_,
                crystalMaterial_,
                crashedCrystalModel_,
                position,
                Vector3(0.013f));

            outObjects.push_back(crystal);
        }
    }
}

void GameScene::UpdateMapStreaming()
{
    if (cameraMode_ !=
        CameraControlMode::AutoForward)
    {
        return;
    }

    Camera* camera =
        GetActiveCamera();

    if (!camera)
        return;

    constexpr float passOffset = 0.05f;

    const float cameraZ =
        camera->GetPosition().z;

    while (!activeMaps_.empty())
    {
        const ActiveMapSegment& current =
            activeMaps_.front();

        if (cameraZ <
            current.endZ + passOffset)
        {
            break;
        }

        if (current.object &&
            !current.object->IsPendingDestroy())
        {
            DestroyGameObject(
                current.object);

            for (GameObject* object : current.objects)
            {
                if (!object ||
                    object->IsPendingDestroy())
                {
                    continue;
                }

                DestroyGameObject(object);
            }
        }

        LOG(
            "Remove Map Sequence: "
            << current.sequence);

        activeMaps_.erase(
            activeMaps_.begin());

        SpawnMapSegment(
            nextMapSequence_);

        ++nextMapSequence_;
    }
}

void GameScene::InitializeMapStreaming()
{
    if (!mapRoot_ || mapLoopLength_ <= 0.0f)
        return;

    activeMaps_.clear();
    nextMapSequence_ = 0;

    for (UINT i = 0; i < kActiveMapCount; ++i)
    {
        SpawnMapSegment(nextMapSequence_);
        ++nextMapSequence_;
    }

    LOG("Map streaming initialized");
}

int GameScene::FindMapIndex(float z) const
{
    constexpr float epsilon = 0.01f;

    for (UINT i = 0; i < kMapCount; ++i)
    {
        const MapTemplate& mapTemplate =
            mapTemplates_[i];

        if (z >= mapTemplate.minZ - epsilon &&
            z <= mapTemplate.maxZ + epsilon)
        {
            return static_cast<int>(i);
        }
    }

    return -1;
}