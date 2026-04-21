#pragma once
#include "pch.h"

struct ObjectCB
{
    Matrix world;
};

struct PassCB
{
    Matrix view;
    Matrix proj;
};
