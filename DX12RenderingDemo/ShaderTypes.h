#pragma once
#include "pch.h"

struct ObjectCB
{
    Matrix world;
    Matrix worldInvTranspose;
};

struct Light
{
    Vector3 direction;
    float pad0 = 0.0f;

    Vector4 color;
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
    float specularStrength = 0.5f;
    Vector2 pad1 = Vector2::Zero;
};