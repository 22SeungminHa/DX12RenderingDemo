#pragma once
#include "pch.h"

class Texture;
class Shader;
class LitShader;

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

    void Clear();

private:
    std::unordered_map<std::wstring, std::shared_ptr<Texture>> textures_;
    std::unordered_map<std::string, std::shared_ptr<Shader>> shaders_;
};