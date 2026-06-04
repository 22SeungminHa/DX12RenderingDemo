#ifndef COMMON_HLSLI
#define COMMON_HLSLI

cbuffer cbGameObjectInfo : register(b0)
{
    matrix gmtxWorld;
    matrix gmtxWorldInvTranspose;
};

cbuffer cbCameraInfo : register(b1)
{
    matrix gmtxView;
    matrix gmtxProjection;

    float3 gEyePosW;
    float gPad0;

    float3 gLightDir;
    float gPad1;

    float4 gLightColor;
    float4 gAmbientColor;

    float gSpecularPower;
    float2 gPad2;
};

cbuffer cbMaterialInfo : register(b2)
{
    float4 gBaseColorTint;

    float gAlpha;
    float gFresnelPower;
    float gSpecularStrength;
    float gReflectionStrength;
    float gRefractionStrength;
    float3 gMaterialPad0;
};

Texture2D gDiffuseMap : register(t0);
Texture2D gNormalMap : register(t1);
TextureCube gSkyboxMap : register(t2);
Texture2D gRefractionSceneMap : register(t4);

SamplerState gSampler : register(s0);

struct VS_INPUT
{
    float3 position : POSITION;
    float4 color : COLOR;
    float3 normal : NORMAL;
    float3 tangent : TANGENT;
    float2 texCoord : TEXCOORD;
};

struct VS_OUTPUT
{
    float4 position : SV_POSITION;
    float3 positionW : POSITION;
    float4 color : COLOR;
    float3 normalW : NORMAL;
    float3 tangentW : TANGENT;
    float2 texCoord : TEXCOORD;
};

struct VS_SKYBOX_INPUT
{
    float3 position : POSITION;
};

struct VS_SKYBOX_OUTPUT
{
    float4 position : SV_POSITION;
    float3 direction : TEXCOORD;
};

struct VS_FULLSCREEN_OUTPUT
{
    float4 position : SV_POSITION;
    float2 texCoord : TEXCOORD;
};

#endif