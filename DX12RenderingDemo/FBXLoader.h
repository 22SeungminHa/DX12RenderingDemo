#pragma once
#include "pch.h"

class Mesh;
class GameObject;
class Material;
class Shader;
class Texture;

class FBXLoader
{
public:
    static Matrix ToMatrix(const aiMatrix4x4& m);

    static std::shared_ptr<Mesh> CreateLitMesh(
        ID3D12Device* device,
        ID3D12GraphicsCommandList* cmdList,
        aiMesh* mesh);

    static std::unique_ptr<GameObject> LoadLitModel(
        ID3D12Device* device,
        ID3D12GraphicsCommandList* cmdList,
        const std::string& filePath,
        const std::shared_ptr<Shader>& shader,
        UINT& objectCBIndex);

    static std::unique_ptr<GameObject> ProcessNode(
        ID3D12Device* device,
        ID3D12GraphicsCommandList* cmdList,
        const aiScene* scene,
        aiNode* node,
        const std::vector<std::shared_ptr<Material>>& materials,
        UINT& objectCBIndex,
        int depth = 0
    );

    static std::vector<std::shared_ptr<Material>> LoadMaterials(
        ID3D12Device* device,
        ID3D12GraphicsCommandList* cmdList,
        const aiScene* scene,
        const std::string& modelPath,
        const std::shared_ptr<Shader>& shader
    );

    static std::shared_ptr<Texture> LoadMaterialTexture(
        ID3D12Device* device,
        ID3D12GraphicsCommandList* cmdList,
        const std::string& modelPath,
        aiMaterial* material
    );

    static void DebugPrintMaterial(aiMaterial* material, UINT index);
};

