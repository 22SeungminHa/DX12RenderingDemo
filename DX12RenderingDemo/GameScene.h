#pragma once
#include "Scene.h"

class AssetManager;
class Mesh;
class Material;
class SphereColliderComponent;
class BoxColliderComponent;
struct FBXNodeData;

enum class CameraControlMode
{
    AutoForward,
    Free
};

struct ObstacleSpawnData
{
    std::shared_ptr<Mesh> mesh;

    Vector3 position = Vector3::Zero;
    Vector3 size = Vector3::One;
};

struct CrystalSpawnData
{
    Vector3 position = Vector3::Zero;
};

struct MapTemplate
{
    std::shared_ptr<FBXNodeData> model;

    float minZ = 0.0f;
    float maxZ = 0.0f;
};

struct ActiveMapSegment
{
    GameObject* object = nullptr;

    UINT sequence = 0;

    float startZ = 0.0f;
    float endZ = 0.0f;

    std::vector<GameObject*> objects;
};

class GameScene : public Scene {
public:
    GameScene(UINT width, UINT height) : Scene(width, height) {};
    virtual ~GameScene() = default;

    virtual SceneType GetSceneType() const override { return SceneType::Game; }

    virtual void OnLoad(ID3D12Device* device, ID3D12GraphicsCommandList* cmdList, ID3D12RootSignature* rootSignature, AssetManager& assetManager) override;

    virtual CameraDesc SetupCameraDesc() const override;
    virtual SceneLightDesc SetupLightDesc() const override;
    virtual FogDesc SetupFogDesc() const override;

    virtual void Animate(float deltaTime) override;

protected:
    virtual void OnProcessInput(const InputSystem& input, float deltaTime) override;
    virtual void OnPrepareRenderResources(ID3D12Device* device, ID3D12GraphicsCommandList* cmdList, std::vector<ComPtr<ID3D12Resource>>& transientUploadResources) override;

    virtual bool IsFreeCameraControlEnabled() const override { return cameraMode_ == CameraControlMode::Free; }

private:
    void LoadObstacles(ID3D12Device* device, ID3D12GraphicsCommandList* cmdList, ID3D12RootSignature* rootSignature, AssetManager& assetManager);
    void LoadCrystals(ID3D12Device* device, ID3D12GraphicsCommandList* cmdList, ID3D12RootSignature* rootSignature, AssetManager& assetManager);
    void LoadMap(ID3D12Device* device, ID3D12GraphicsCommandList* cmdList, ID3D12RootSignature* rootSignature, AssetManager& assetManager);

    void FireProjectile(const Vector2& screenPosition);
    void CheckProjectileCollisions();

    void RemoveObjectsBehindCamera();

    void ToggleCameraMode();
    void UpdateAutoCamera(float deltaTime);
    void RestoreAutoCamera();

    void CreateLights();

    GameObject* SpawnMapSegment(UINT sequence);
    void UpdateMapStreaming();

    void SetupMapMeshObjects(
        const std::vector<GameObject*>& meshObjects);

    bool CalculateMapZBounds(
        const FBXNodeData& modelData,
        float& outMinZ,
        float& outMaxZ) const;

    void AccumulateMapZBounds(
        const FBXNodeData& nodeData,
        const Matrix& parentMatrix,
        float& minZ,
        float& maxZ,
        bool& hasBounds) const;

    void InitializeMapStreaming();

    int FindMapIndex(float z) const;

    void SpawnSegmentContents(
        UINT mapIndex,
        float loopOffsetZ,
        std::vector<GameObject*>& outObjects);

private:
    std::shared_ptr<Mesh> projectileMesh_;
    std::shared_ptr<Material> projectileMaterial_;

    CameraControlMode cameraMode_ = CameraControlMode::AutoForward;

    float autoCameraSpeed_ = 7.5f;
    Vector3 autoCameraPosition_ = Vector3::Zero;
    Vector3 autoCameraForward_ = Vector3(0.0f, 0.0f, 1.0f);

    static constexpr UINT kMapCount = 7;
    static constexpr UINT kActiveMapCount = 4;
    static constexpr float kMapScale = 0.01f;

    GameObject* mapRoot_ = nullptr;

    std::array<MapTemplate, kMapCount> mapTemplates_;
    std::vector<ActiveMapSegment> activeMaps_;

    UINT nextMapSequence_ = 0;

    float mapLoopLength_ = 0.0f;

    std::array<std::vector<ObstacleSpawnData>, kMapCount> obstacleTemplates_;
    std::array<std::vector<CrystalSpawnData>, kMapCount> crystalTemplates_;

    std::shared_ptr<Material> obstacleMaterial_;

    std::shared_ptr<Mesh> crystalMesh_;
    std::shared_ptr<Material> crystalMaterial_;
    std::shared_ptr<FBXNodeData> crashedCrystalModel_;
};