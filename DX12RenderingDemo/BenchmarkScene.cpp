#include "BenchmarkScene.h"
#include "AssetManager.h"
#include "Mesh.h"
#include "Material.h"
#include "Camera.h"
#include "Renderer.h"

#include <algorithm>
#include <cmath>
#include <limits>

void BenchmarkScene::OnLoad(ID3D12Device* device, ID3D12GraphicsCommandList* cmdList, ID3D12RootSignature* rootSignature, AssetManager& assetManager)
{
    CreateLights();
    CreateBenchmarkObjects(device, cmdList, rootSignature, assetManager);
    SetSkybox(L"Skybox");
}

void BenchmarkScene::CreateBenchmarkObjects(ID3D12Device* device, ID3D12GraphicsCommandList* cmdList, ID3D12RootSignature* rootSignature, AssetManager& assetManager)
{
    benchmarkMesh_ = assetManager.LoadCubeMesh(device, cmdList);

    if (!benchmarkMesh_)
    {
        LOG("Benchmark CubeMesh load failed");
        return;
    }

    benchmarkMaterial_ = assetManager.LoadMaterialFromFile(device, cmdList, rootSignature, AssetPath::Material(L"Default_Glass"));

    if (!benchmarkMaterial_)
    {
        LOG("Benchmark Default_Glass material load failed");
        return;
    }

    RebuildBenchmarkObjects();
}

void BenchmarkScene::CreateLights()
{
    DirectionalLight* light = AddDirectionalLight();

    light->SetDirection(Vector3(0.2f, -1.0f, 0.3f));
    light->SetColor(Vector4(1.0f, 1.0f, 1.0f, 1.0f));
    light->SetIntensity(1.0f);
}

CameraDesc BenchmarkScene::SetupCameraDesc() const
{
    CameraDesc desc{};

    desc.eye = { 0.0f, 0.0f, -15.0f };
    desc.target = { 0.0f, 0.0f, 0.0f };

    //desc.eye = { 6.62168f, 0.834289f, -2.49053f };
    //desc.target = { 5.99946f, 0.385866f, -1.84885f };

    desc.nearZ = 1.0f;
    desc.farZ = 100.0f;
    desc.fovY = 60.0f;

    return desc;
}

SceneLightDesc BenchmarkScene::SetupLightDesc() const
{
    SceneLightDesc desc{};
    desc.ambientColor = { 0.1f, 0.1f, 0.1f, 1.0f };
    desc.specularPower = 32.0f;

    return desc;
}

FogDesc BenchmarkScene::SetupFogDesc() const
{
    FogDesc desc{};
    desc.enabled = false;

    return desc;
}

void BenchmarkScene::RebuildBenchmarkObjects()
{
    if (!benchmarkMesh_ || !benchmarkMaterial_)
        return;

    objects_.clear();
    nextObjectCBIndex_ = 0;

    const float totalWidth = static_cast<float>(kColumns - 1) * kObjectSpacing;
    const float totalHeight = static_cast<float>(kRows - 1) * kObjectSpacing;
    const float startX = -totalWidth * 0.5f;
    const float startY = -totalHeight * 0.5f;

    for (UINT i = 0; i < glassObjectCount_; ++i)
    {
        const UINT layer = i / kObjectsPerLayer;
        const UINT indexInLayer = i % kObjectsPerLayer;
        const UINT row = indexInLayer / kColumns;
        const UINT column = indexInLayer % kColumns;
        const float x = startX + static_cast<float>(column) * kObjectSpacing;
        const float y = startY + static_cast<float>(row) * kObjectSpacing;
        const float z = static_cast<float>(layer) * kObjectSpacing;

        CreateObject(benchmarkMesh_, benchmarkMaterial_, Vector3(x, y, z), Vector3(0.65f));
    }

    LOG("Benchmark Glass Count: " << glassObjectCount_);
}

void BenchmarkScene::SetGlassObjectCount(UINT count)
{
    if (count != 100 && count != 200 && count != 300)
        return;

    if (glassObjectCount_ == count)
        return;

    glassObjectCount_ = count;

    RebuildBenchmarkObjects();
}

void BenchmarkScene::ResetMeasurementCamera()
{
    Camera* camera = GetActiveCamera();

    if (!camera)
        return;

    const CameraDesc desc = SetupCameraDesc();
    camera->SetLookAt(desc.eye, desc.target);
}

void BenchmarkScene::StartMeasurement(Renderer& renderer)
{
    if (IsMeasurementActive())
        return;

    benchmarkResults_.clear();
    benchmarkResults_.reserve(kBenchmarkCases.size());

    currentBenchmarkCaseIndex_ = 0;

    measurementState_ = BenchmarkMeasurementState::Warmup;

    renderer.SetGlassGpuMeasurementEnabled(false);

    LOG("========================================");
    LOG("REFRACTION BENCHMARK: START");
    LOG("Total Cases: " << kBenchmarkCases.size());
    LOG("Warmup: " << kWarmupFrameCount << " frames / case");
    LOG("Samples: " << kMeasurementSampleCount << " / case");
    LOG("========================================");

    BeginCurrentBenchmarkCase(renderer);
}

void BenchmarkScene::CancelMeasurement(Renderer& renderer)
{
    if (!IsMeasurementActive())
        return;

    renderer.SetGlassGpuMeasurementEnabled(false);

    measurementState_ = BenchmarkMeasurementState::Idle;

    benchmarkResults_.clear();

    LOG("========================================");
    LOG("REFRACTION BENCHMARK: CANCELED");
    LOG("Camera Control: UNLOCKED");
    LOG("========================================");
}

void BenchmarkScene::UpdateMeasurement(Renderer& renderer)
{
    switch (measurementState_)
    {
    case BenchmarkMeasurementState::Idle:
        return;

    case BenchmarkMeasurementState::Warmup:
    {
        ++warmupFrameCount_;

        if (warmupFrameCount_ < kWarmupFrameCount)
            return;

        lastConsumedSampleSerial_ = renderer.GetGlassGpuSampleSerial();
        measurementState_ = BenchmarkMeasurementState::Measuring;
        renderer.SetGlassGpuMeasurementEnabled(true);

        LOG("Benchmark Warmup: COMPLETE");
        LOG("Benchmark Sampling: START / " << kMeasurementSampleCount << " samples");

        return;
    }

    case BenchmarkMeasurementState::Measuring:
        break;
    }

    const UINT64 sampleSerial = renderer.GetGlassGpuSampleSerial();

    if (sampleSerial == lastConsumedSampleSerial_)
        return;

    lastConsumedSampleSerial_ = sampleSerial;

    const double gpuTimeMs = renderer.GetLastGlassGpuTimeMs();

    const BenchmarkCase& benchmarkCase =
        kBenchmarkCases[currentBenchmarkCaseIndex_];

    if (benchmarkCase.mode ==
        RefractionMode::PartialPerGlassCapture)
    {
        partialCopyAreaPixelsSum_ +=
            renderer.GetPartialCopyAreaPixels();

        partialCopyCountSum_ +=
            renderer.GetPartialCopyCount();

        partialFullCopyFallbackCountSum_ +=
            renderer.GetPartialFullCopyFallbackCount();

        partialFullScreenPixels_ =
            renderer.GetPartialFullScreenPixels();
    }

    measurementSamplesMs_.push_back(gpuTimeMs);
    measurementSumMs_ += gpuTimeMs;
    measurementMinMs_ = std::min(measurementMinMs_, gpuTimeMs);
    measurementMaxMs_ = std::max(measurementMaxMs_, gpuTimeMs);

    ++measurementSampleCount_;

    if (measurementSampleCount_ % 50 == 0)
        LOG("Benchmark Progress: " << measurementSampleCount_ << " / " << kMeasurementSampleCount);

    if (measurementSampleCount_ < kMeasurementSampleCount)
        return;

    CompleteCurrentBenchmarkCase(renderer);
}

void BenchmarkScene::BeginCurrentBenchmarkCase(Renderer& renderer)
{
    if (currentBenchmarkCaseIndex_ >= kBenchmarkCases.size())
        return;

    renderer.SetGlassGpuMeasurementEnabled(false);

    const BenchmarkCase& benchmarkCase = kBenchmarkCases[currentBenchmarkCaseIndex_];

    renderer.SetRefractionMode(benchmarkCase.mode);
    SetGlassObjectCount(benchmarkCase.glassCount);

    ResetMeasurementCamera();

    warmupFrameCount_ = 0;
    measurementSampleCount_ = 0;
    measurementSumMs_ = 0.0;
    measurementMinMs_ = std::numeric_limits<double>::max();
    measurementMaxMs_ = 0.0;
    partialCopyAreaPixelsSum_ = 0;
    partialCopyCountSum_ = 0;
    partialFullCopyFallbackCountSum_ = 0;
    partialFullScreenPixels_ = 0;

    measurementSamplesMs_.clear();
    measurementSamplesMs_.reserve(kMeasurementSampleCount);

    lastConsumedSampleSerial_ = renderer.GetGlassGpuSampleSerial();

    measurementState_ = BenchmarkMeasurementState::Warmup;

    LOG("----------------------------------------");
    LOG("Benchmark Case " << (currentBenchmarkCaseIndex_ + 1) << " / " << kBenchmarkCases.size());
    LOG("Mode: " << GetRefractionModeName(benchmarkCase.mode));
    LOG("Glass Count: " << benchmarkCase.glassCount);
    LOG("Warmup: START");
}

void BenchmarkScene::CompleteCurrentBenchmarkCase(Renderer& renderer)
{
    renderer.SetGlassGpuMeasurementEnabled(false);

    const BenchmarkCase& benchmarkCase = kBenchmarkCases[currentBenchmarkCaseIndex_];
    const double averageMs = measurementSumMs_ / static_cast<double>(measurementSampleCount_);

    std::vector<double> sortedSamples = measurementSamplesMs_;
    std::sort(sortedSamples.begin(), sortedSamples.end());
    const size_t sampleCount = sortedSamples.size();

    // Median
    double medianMs = 0.0;

    if (!sortedSamples.empty())
    {
        if (sampleCount % 2 == 0)
        {
            const size_t rightIndex = sampleCount / 2;
            const size_t leftIndex = rightIndex - 1;

            medianMs = (sortedSamples[leftIndex] + sortedSamples[rightIndex]) * 0.5;
        }
        else
            medianMs = sortedSamples[sampleCount / 2];
    }

    // P95
    double p95Ms = 0.0;
    if (!sortedSamples.empty())
    {
        const size_t p95Rank = static_cast<size_t>(std::ceil(0.95 * static_cast<double>(sampleCount)));
        const size_t p95Index = std::min(p95Rank - 1, sampleCount - 1);

        p95Ms = sortedSamples[p95Index];
    }

    // Standard Deviation
    double variance = 0.0;

    for (double sampleMs :
    measurementSamplesMs_)
    {
        const double difference = sampleMs - averageMs;
        variance += difference * difference;
    }

    if (!measurementSamplesMs_.empty())
        variance /= static_cast<double>(measurementSamplesMs_.size());

    const double stdDevMs = std::sqrt(variance);

    BenchmarkResult result{};

    result.mode = benchmarkCase.mode;
    result.glassCount = benchmarkCase.glassCount;
    result.averageMs = averageMs;
    result.minMs = measurementMinMs_;
    result.maxMs = measurementMaxMs_;
    result.medianMs = medianMs;
    result.p95Ms = p95Ms;
    result.stdDevMs = stdDevMs;

    benchmarkResults_.push_back(result);

    LOG("Benchmark Case: COMPLETE");
    LOG("GPU AVG: " << averageMs << " ms");
    LOG("GPU MIN: " << measurementMinMs_ << " ms");
    LOG("GPU MAX: " << measurementMaxMs_ << " ms");
    LOG("GPU MEDIAN: " << medianMs << " ms");
    LOG("GPU P95: " << p95Ms << " ms");
    LOG("GPU STDDEV: " << stdDevMs << " ms");

    if (benchmarkCase.mode ==
        RefractionMode::PartialPerGlassCapture)
    {
        const double averageCopyAreaPixels =
            partialCopyCountSum_ > 0
            ? static_cast<double>(partialCopyAreaPixelsSum_) /
            static_cast<double>(partialCopyCountSum_)
            : 0.0;

        const double averageCopyAreaRatio =
            partialFullScreenPixels_ > 0
            ? averageCopyAreaPixels /
            static_cast<double>(partialFullScreenPixels_) *
            100.0
            : 0.0;

        const double averagePartialCopiesPerFrame =
            measurementSampleCount_ > 0
            ? static_cast<double>(partialCopyCountSum_) /
            static_cast<double>(measurementSampleCount_)
            : 0.0;

        const double averageFallbacksPerFrame =
            measurementSampleCount_ > 0
            ? static_cast<double>(
                partialFullCopyFallbackCountSum_) /
            static_cast<double>(measurementSampleCount_)
            : 0.0;

        const UINT64 effectiveCopiedPixels =
            partialCopyAreaPixelsSum_ +
            partialFullCopyFallbackCountSum_ *
            partialFullScreenPixels_;

        const UINT64 totalCopyCount =
            partialCopyCountSum_ +
            partialFullCopyFallbackCountSum_;

        const UINT64 fullCopyBaselinePixels =
            totalCopyCount *
            partialFullScreenPixels_;

        const double effectiveCopyRatio =
            fullCopyBaselinePixels > 0
            ? static_cast<double>(effectiveCopiedPixels) /
            static_cast<double>(fullCopyBaselinePixels) *
            100.0
            : 0.0;

        const double copyAreaReduction =
            100.0 - effectiveCopyRatio;

        LOG("Partial Copy Statistics");
        LOG("Full Screen Area: "
            << partialFullScreenPixels_
            << " pixels");

        LOG("Average Partial Copy Area: "
            << averageCopyAreaPixels
            << " pixels");

        LOG("Average Partial Copy Area Ratio: "
            << averageCopyAreaRatio
            << "%");

        LOG("Average Partial Copies / Frame: "
            << averagePartialCopiesPerFrame);

        LOG("Full Copy Fallback Total: "
            << partialFullCopyFallbackCountSum_);

        LOG("Average Full Copy Fallback / Frame: "
            << averageFallbacksPerFrame);

        LOG("Effective Copy Area Ratio vs Full PerGlass: "
            << effectiveCopyRatio
            << "%");

        LOG("Copy Area Reduction vs Full PerGlass: "
            << copyAreaReduction
            << "%");
    }

    ++currentBenchmarkCaseIndex_;

    // 다음 Case
    if (currentBenchmarkCaseIndex_ < kBenchmarkCases.size())
    {
        BeginCurrentBenchmarkCase(renderer);
        return;
    }

    // 전체 Benchmark 완료.
    measurementState_ = BenchmarkMeasurementState::Idle;

    PrintBenchmarkSummary();

    LOG("REFRACTION BENCHMARK: COMPLETE");
    LOG("Camera Control: UNLOCKED");
}

const char* BenchmarkScene::GetRefractionModeName(RefractionMode mode)
{
    switch (mode)
    {
    case RefractionMode::SingleCapture:
        return "SingleCapture";

    case RefractionMode::PerGlassCapture:
        return "PerGlassCapture";

    case RefractionMode::PartialPerGlassCapture:
        return "PartialPerGlassCapture";

    case RefractionMode::AccumulationBuffer:
        return "AccumulationBuffer";

    default:
        return "Unknown";
    }
}

void BenchmarkScene::PrintBenchmarkSummary() const
{
    LOG("========================================");
    LOG("REFRACTION BENCHMARK SUMMARY");
    LOG("========================================");

    for (const BenchmarkResult& result : benchmarkResults_)
        LOG(GetRefractionModeName(result.mode) << " | Glass: " << result.glassCount
            << " | AVG: " << result.averageMs << " ms"
            << " | MEDIAN: " << result.medianMs << " ms"
            << " | P95: " << result.p95Ms << " ms"
            << " | STDDEV: " << result.stdDevMs << " ms"
            << " | MIN: " << result.minMs << " ms"
            << " | MAX: " << result.maxMs << " ms");

    LOG("----------------------------------------");
    LOG("PartialPerGlassCapture vs PerGlassCapture");

    const UINT glassCounts[] = { 100, 200, 300 };

    for (UINT glassCount : glassCounts)
    {
        double perGlassMs = 0.0;
        double partialPerGlassMs = 0.0;

        for (const BenchmarkResult& result : benchmarkResults_)
        {
            if (result.glassCount != glassCount)
                continue;

            if (result.mode == RefractionMode::PerGlassCapture)
                perGlassMs = result.averageMs;

            if (result.mode == RefractionMode::PartialPerGlassCapture)
                partialPerGlassMs = result.averageMs;
        }

        if (perGlassMs <= 0.0 || partialPerGlassMs <= 0.0)
            continue;

        const double improvementPercent =
            (perGlassMs - partialPerGlassMs) / perGlassMs * 100.0;

        const double speedup =
            perGlassMs / partialPerGlassMs;

        LOG("Glass " << glassCount
            << " | PerGlass: " << perGlassMs << " ms"
            << " | Partial: " << partialPerGlassMs << " ms"
            << " | Improvement: " << improvementPercent << "%"
            << " | Speedup: " << speedup << "x");
    }

    LOG("----------------------------------------");
    LOG("AccumulationBuffer vs PerGlassCapture");

    for (UINT glassCount : glassCounts)
    {
        double perGlassMs = 0.0;
        double accumulationMs = 0.0;

        for (const BenchmarkResult& result : benchmarkResults_)
        {
            if (result.glassCount != glassCount)
                continue;

            if (result.mode == RefractionMode::PerGlassCapture)
                perGlassMs = result.averageMs;

            if (result.mode == RefractionMode::AccumulationBuffer)
                accumulationMs = result.averageMs;
        }

        if (perGlassMs <= 0.0 || accumulationMs <= 0.0)
            continue;

        const double improvementPercent = (perGlassMs - accumulationMs) / perGlassMs * 100.0;
        const double speedup = perGlassMs / accumulationMs;

        LOG("Glass " << glassCount
            << " | PerGlass: " << perGlassMs << " ms"
            << " | Accumulation: " << accumulationMs << " ms"
            << " | Improvement: " << improvementPercent << "%"
            << " | Speedup: " << speedup << "x");
    }

    LOG("========================================");
}