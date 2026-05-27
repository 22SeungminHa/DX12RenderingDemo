#pragma once
#include "pch.h"

class Mesh;
class GameObject;
class Material;
class Shader;
class Texture;
class AssetManager;

class FBXLoader
{
public:
    static Matrix ToMatrix(const aiMatrix4x4& m);

    static std::shared_ptr<Mesh> CreateLitMesh(
        ID3D12Device* device,
        ID3D12GraphicsCommandList* cmdList,
        AssetManager& assetManager,
        const std::string& modelPath,
        UINT meshIndex,
        aiMesh* mesh);

    static std::unique_ptr<GameObject> LoadLitModel(
        ID3D12Device* device,
        ID3D12GraphicsCommandList* cmdList,
        ID3D12RootSignature* rootSignature,
        AssetManager& assetManager,
        const std::string& filePath,
        UINT& objectCBIndex);

    static std::unique_ptr<GameObject> ProcessNode(
        ID3D12Device* device,
        ID3D12GraphicsCommandList* cmdList,
        AssetManager& assetManager,
        const std::string& modelPath,
        const aiScene* scene,
        aiNode* node,
        const std::vector<std::shared_ptr<Material>>& materials,
        UINT& objectCBIndex,
        int depth = 0
    );

    static std::vector<std::shared_ptr<Material>> LoadMaterialsFromMatFiles(
        ID3D12Device* device,
        ID3D12GraphicsCommandList* cmdList,
        ID3D12RootSignature* rootSignature,
        AssetManager& assetManager,
        const aiScene* scene,
        const std::string& modelPath);
};

