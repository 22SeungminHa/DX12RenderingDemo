#pragma once
#include "pch.h"

class Texture;
class Shader;
class LitShader;
class Material;

class AssetManager
{
public:
    AssetManager() = default;
    ~AssetManager() = default;

    std::shared_ptr<Texture> LoadTexture(
        ID3D12Device* device,
        ID3D12GraphicsCommandList* cmdList,
        const std::wstring& filePath);

    std::shared_ptr<Shader> LoadLitShader(
        ID3D12Device* device,
        ID3D12RootSignature* rootSignature,
        const std::string& key = "LitShader");

    std::shared_ptr<Material> LoadMaterialFromFile(
        ID3D12Device* device,
        ID3D12GraphicsCommandList* cmdList,
        ID3D12RootSignature* rootSignature,
        const std::filesystem::path& matPath);

    void Clear();

private:
    std::shared_ptr<Shader> LoadShaderByName(
        ID3D12Device* device,
        ID3D12RootSignature* rootSignature,
        const std::string& shaderName);

    std::unordered_map<std::wstring, std::shared_ptr<Texture>> textures_;
    std::unordered_map<std::string, std::shared_ptr<Shader>> shaders_;
    std::unordered_map<std::wstring, std::shared_ptr<Material>> materials_;
};