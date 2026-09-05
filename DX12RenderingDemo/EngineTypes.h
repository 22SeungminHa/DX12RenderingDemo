#pragma once
#include "pch.h"

// enums

enum class RootParam : UINT
{
    ObjectCB = 0,
    PassCB = 1,
    MaterialCB = 2,
    MaterialTextures = 3,
    SkyboxTexture = 4,
    PostProcessTexture = 5,
    SceneColorTexture = 6,
    GlassAccumTexture = 7,
    GlassRevealageTexture = 8,

    End
};

enum class TextureType
{
    BaseColor,
    Normal,

    End
};

enum class RenderMode
{
    Opaque,
    Transparent,

    End
};

enum class RenderPass
{
    Default,
    GlassAccumulation,

    End
};

enum class RefractionMode
{
    SingleCapture,
    PerGlassCapture,
    PartialPerGlassCapture,
    AccumulationBuffer,

    End
};

enum class SceneType
{
    None,
    Title,
    Game,
    Benchmark,

    End
};

// render structs

struct SkyboxDesc
{
    bool enabled = false;
    std::filesystem::path cubemapPath{};

    void SetCubemap(const std::wstring& name)
    {
        enabled = true;
        cubemapPath = AssetPath::Texture(name);
    }
};

struct CameraDesc
{
    Vector3 eye = { 0.0f, 0.0f, -10.0f };
    Vector3 target = { 0.0f, 0.0f, 0.0f };
    Vector3 up = Vector3::Up;

    float nearZ = 1.0f;
    float farZ = 1000.0f;
    float fovY = 90.0f;
};

struct FogDesc
{
    bool enabled = false;

    Vector4 topColor =
    {
        0.015f,
        0.003f,
        0.012f,
        1.0f
    };

    Vector4 middleColor =
    {
        0.15f,
        0.21f,
        0.53f,
        1.0f
    };

    Vector4 bottomColor =
    {
        0.46f,
        0.78f,
        0.87f,
        1.0f
    };

    float startDistance = 100.0f;
    float endDistance = 300.0f;
};

constexpr UINT kMaxDirectionalLights = 3;

struct SceneLightDesc
{
    Vector4 ambientColor = { 0.15f, 0.15f, 0.15f, 1.0f };
    float specularPower = 32.0f;
};

struct DirectionalLightData
{
    Vector3 direction;
    float pad0 = 0.0f;

    Vector4 color;
};

struct ObjectCB
{
    Matrix world;
    Matrix worldInvTranspose;
};

struct PassCB
{
    Matrix view;
    Matrix proj;

    Vector3 eyePosW;
    float pad0 = 0.0f;

    std::array<DirectionalLightData, kMaxDirectionalLights> directionalLights{};

    Vector4 ambientColor = Vector4(0.15f, 0.15f, 0.15f, 1.0f);

    float specularPower = 128.0f;
    UINT directionalLightCount = 0;
    Vector2 pad1 = Vector2::Zero;

    Vector4 fogTopColor = Vector4::Zero;
    Vector4 fogMiddleColor = Vector4::Zero;
    Vector4 fogBottomColor = Vector4::Zero;

    float fogStart = 0.0f;
    float fogEnd = 0.0f;
    float viewportHeight = 1.0f;
    UINT fogEnabled = 0;
};

struct MaterialCB
{
    Vector4 baseColorTint = Vector4(1.0f, 1.0f, 1.0f, 1.0f);

    float alpha = 1.0f;
    float fresnelPower = 5.0f;
    float specularStrength = 0.5f;
    float reflectionStrength = 0.0f;

    float refractionStrength = 0.0f;
    Vector2 tiling = Vector2::One;
    float pad0 = 0.0f;
};