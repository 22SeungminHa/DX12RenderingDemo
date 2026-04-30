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
};