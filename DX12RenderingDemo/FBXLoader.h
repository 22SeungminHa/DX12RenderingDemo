#pragma once
#include "pch.h"
#include <assimp/matrix4x4.h>

struct aiScene;
struct aiNode;
struct aiMesh;
struct aiMaterial;

class Mesh;
class Material;
class Shader;
class Texture;
class AssetManager;

struct FBXMeshData
{
    std::shared_ptr<Mesh> mesh;
    std::shared_ptr<Material> material;
};

struct FBXNodeData
{
    std::string name;
    Matrix localMatrix = Matrix::Identity;

    std::vector<FBXMeshData> meshes;
    std::vector<FBXNodeData> children;
};

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

    static std::optional<FBXNodeData> LoadLitModel(
        ID3D12Device* device,
        ID3D12GraphicsCommandList* cmdList,
        ID3D12RootSignature* rootSignature,
        AssetManager& assetManager,
        const std::filesystem::path& filePath);

    static std::shared_ptr<Mesh> LoadLitMeshFromFile(
        ID3D12Device* device,
        ID3D12GraphicsCommandList* cmdList,
        AssetManager& assetManager,
        const std::filesystem::path& filePath);

    static FBXNodeData ProcessNode(
        ID3D12Device* device,
        ID3D12GraphicsCommandList* cmdList,
        AssetManager& assetManager,
        const std::string& modelPath,
        const aiScene* scene,
        aiNode* node,
        const std::vector<std::shared_ptr<Material>>& materials,
        int depth = 0);

    static std::vector<std::shared_ptr<Material>> LoadMaterialsFromMatFiles(
        ID3D12Device* device,
        ID3D12GraphicsCommandList* cmdList,
        ID3D12RootSignature* rootSignature,
        AssetManager& assetManager,
        const aiScene* scene,
        const std::string& modelPath);
};

