#include "AssetManager.h"
#include "Texture.h"
#include "Shader.h"

std::shared_ptr<Texture> AssetManager::LoadTexture(
    ID3D12Device* device,
    ID3D12GraphicsCommandList* cmdList,
    const std::wstring& filePath)
{
    if (auto iter = textures_.find(filePath); iter != textures_.end())
        return iter->second;

    auto texture = std::make_shared<Texture>();
    texture->LoadDDS(device, cmdList, filePath);
    texture->SetName(std::filesystem::path(filePath).filename().string());

    textures_[filePath] = texture;
    return texture;
}

std::shared_ptr<Shader> AssetManager::LoadLitShader(
    ID3D12Device* device,
    ID3D12RootSignature* rootSignature,
    const std::string& key)
{
    if (auto iter = shaders_.find(key); iter != shaders_.end())
        return iter->second;

    auto shader = std::make_shared<LitShader>();
    shader->CreateShader(device, rootSignature);
    shader->SetName(key);

    shaders_[key] = shader;
    return shader;
}

void AssetManager::Clear()
{
    textures_.clear();
    shaders_.clear();
}