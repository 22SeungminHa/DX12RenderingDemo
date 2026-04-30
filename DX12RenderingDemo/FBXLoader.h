#pragma once
#include "pch.h"

class Mesh;

class FBXLoader
{
public:
    static std::shared_ptr<Mesh> LoadDiffusedMesh(
        ID3D12Device* device,
        ID3D12GraphicsCommandList* cmdList,
        const std::string& filePath);
};

