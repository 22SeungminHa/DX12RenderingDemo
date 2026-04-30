#pragma once
#include "pch.h"

class Mesh;
class GameObject;
class Material;

class FBXLoader
{
public:
    static Matrix ToMatrix(const aiMatrix4x4& m);

    static std::shared_ptr<Mesh> CreateDiffusedMesh(
        ID3D12Device* device,
        ID3D12GraphicsCommandList* cmdList,
        aiMesh* mesh);

    static std::unique_ptr<GameObject> LoadDiffusedModel(
        ID3D12Device* device,
        ID3D12GraphicsCommandList* cmdList,
        const std::string& filePath,
        const std::shared_ptr<Material>& material,
        UINT& objectCBIndex);
    
    static std::unique_ptr<GameObject> ProcessNode(
        ID3D12Device* device,
        ID3D12GraphicsCommandList* cmdList,
        const aiScene* scene,
        aiNode* node,
        const std::shared_ptr<Material>& material,
        UINT& objectCBIndex,
        int depth = 0
    );
};

