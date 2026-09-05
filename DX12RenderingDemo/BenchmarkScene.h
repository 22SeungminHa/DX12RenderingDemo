#pragma once
#include "Scene.h"

class AssetManager;
class Mesh;
class Material;
class Renderer;

enum class BenchmarkMeasurementState
{
    Idle,
    Warmup,
    Measuring
};

struct BenchmarkCase
{
    RefractionMode mode;
    UINT glassCount;
};

struct BenchmarkResult
{
    RefractionMode mode = RefractionMode::SingleCapture;

    UINT glassCount = 0;

    double averageMs = 0.0;
    double minMs = 0.0;
    double maxMs = 0.0;

    double medianMs = 0.0;
    double p95Ms = 0.0;
    double stdDevMs = 0.0;
};

class BenchmarkScene : public Scene
{
public:
    BenchmarkScene(UINT width, UINT height) : Scene(width, height) {}
    virtual ~BenchmarkScene() = default;

    virtual SceneType GetSceneType() const override { return SceneType::Benchmark; }

    void StartMeasurement(Renderer& renderer);
    void CancelMeasurement(Renderer& renderer);
    void UpdateMeasurement(Renderer& renderer);

    bool IsMeasurementActive() const { return measurementState_ != BenchmarkMeasurementState::Idle; }

    void SetGlassObjectCount(UINT count);
    UINT GetGlassObjectCount() const { return glassObjectCount_; }

protected:
    virtual void OnLoad(ID3D12Device* device, ID3D12GraphicsCommandList* cmdList, ID3D12RootSignature* rootSignature, AssetManager& assetManager) override;

    virtual CameraDesc SetupCameraDesc() const override;
    virtual SceneLightDesc SetupLightDesc() const override;
    virtual FogDesc SetupFogDesc() const override;

    virtual bool IsFreeCameraControlEnabled() const override { return measurementState_ == BenchmarkMeasurementState::Idle; }

private:
    void CreateBenchmarkObjects(ID3D12Device* device, ID3D12GraphicsCommandList* cmdList, ID3D12RootSignature* rootSignature, AssetManager& assetManager);

    void CreateLights();

    void RebuildBenchmarkObjects();
    void ResetMeasurementCamera();

    void BeginCurrentBenchmarkCase(Renderer& renderer);
    void CompleteCurrentBenchmarkCase(Renderer& renderer);
    void PrintBenchmarkSummary() const;

    static const char* GetRefractionModeName(RefractionMode mode);

private:
    std::shared_ptr<Mesh> benchmarkMesh_;
    std::shared_ptr<Material> benchmarkMaterial_;

    UINT glassObjectCount_ = 100;

    BenchmarkMeasurementState measurementState_ = BenchmarkMeasurementState::Idle;

    UINT warmupFrameCount_ = 0;
    UINT measurementSampleCount_ = 0;

    UINT64 lastConsumedSampleSerial_ = 0;

    double measurementSumMs_ = 0.0;
    double measurementMinMs_ = 0.0;
    double measurementMaxMs_ = 0.0;

    std::vector<double> measurementSamplesMs_;

    UINT currentBenchmarkCaseIndex_ = 0;

    std::vector<BenchmarkResult> benchmarkResults_;

    static constexpr UINT kWarmupFrameCount = 120;
    static constexpr UINT kMeasurementSampleCount = 300;

    static constexpr std::array<BenchmarkCase, 9> kBenchmarkCases =
    { {
        { RefractionMode::SingleCapture,      100 },
        { RefractionMode::SingleCapture,      200 },
        { RefractionMode::SingleCapture,      300 },

        { RefractionMode::PerGlassCapture,    100 },
        { RefractionMode::PerGlassCapture,    200 },
        { RefractionMode::PerGlassCapture,    300 },

        { RefractionMode::AccumulationBuffer, 100 },
        { RefractionMode::AccumulationBuffer, 200 },
        { RefractionMode::AccumulationBuffer, 300 }
    } };

    static constexpr UINT kColumns = 10;
    static constexpr UINT kRows = 10;
    static constexpr UINT kObjectsPerLayer = kColumns * kRows;

    static constexpr float kObjectSpacing = 1.6f;
};