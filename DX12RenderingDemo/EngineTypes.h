#pragma once
#include "pch.h"

// enums

enum class TextureType
{
    BaseColor,
    Normal,
    //MetallicRoughness,
    //Emissive,

    End
};

enum class RenderMode
{
    Opaque,
    Transparent,

    End
};

enum class SCENE_TYPE
{
    None,
    Title,
    Game,
    Loading,
    Test,

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

struct Light
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

    Light mainLight;

    Vector4 ambientColor = Vector4(0.15f, 0.15f, 0.15f, 1.0f);

    float specularPower = 128.0f;
    Vector2 pad1 = Vector2::Zero;
};

struct MaterialCB
{
    Vector4 baseColorTint = Vector4(1.0f, 1.0f, 1.0f, 1.0f);

    float alpha = 1.0f;
    float fresnelPower = 5.0f;
    float specularStrength = 0.5f;
    float padding0 = 0.0f;
};