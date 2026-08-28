#pragma once
#include "Material.h"

class Texture;
class Shader;
class LitShader;
class Material;
class Mesh;
class LitVertex;

class AssetManager
{
public:
    AssetManager() = default;
    ~AssetManager() = default;

    void InitializeDefaultTextures(ID3D12Device* device, ID3D12GraphicsCommandList* cmdList);
    std::shared_ptr<Texture> GetDefaultTexture(TextureType type) const { return defaultTextures_[static_cast<size_t>(type)]; }

    std::shared_ptr<Texture> LoadTexture(ID3D12Device* device, ID3D12GraphicsCommandList* cmdList, const std::filesystem::path& filePath);
    std::shared_ptr<Shader> LoadLitShader(ID3D12Device* device, ID3D12RootSignature* rootSignature, const std::string& key = "LitShader");
    std::shared_ptr<Shader> LoadGlassShader(ID3D12Device* device, ID3D12RootSignature* rootSignature, const std::string& key = "GlassShader");
    std::shared_ptr<Material> LoadMaterialFromFile(ID3D12Device* device, ID3D12GraphicsCommandList* cmdList, ID3D12RootSignature* rootSignature, const std::filesystem::path& matPath);
    std::shared_ptr<Mesh> LoadLitMesh(ID3D12Device* device, ID3D12GraphicsCommandList* cmdList, const std::string& key, const std::vector<LitVertex>& vertices, const std::vector<UINT>& indices);

    std::shared_ptr<Mesh> LoadCubeMesh(ID3D12Device* device, ID3D12GraphicsCommandList* cmdList, const std::string& key = "Primitive/Cube");
    std::shared_ptr<Mesh> LoadGlassMesh(
        ID3D12Device* device,
        ID3D12GraphicsCommandList* cmdList,
        float width,
        float height,
        float depth);
    std::shared_ptr<Mesh> LoadSphereMesh(
        ID3D12Device* device,
        ID3D12GraphicsCommandList* cmdList,
        const std::string& key = "Primitive/Sphere",
        UINT sliceCount = 32,
        UINT stackCount = 16);

    void Clear();
    void ReleaseUploadResources();

private:
    std::shared_ptr<Shader> LoadShaderByName(ID3D12Device* device, ID3D12RootSignature* rootSignature, const std::string& shaderName);

    std::unordered_map<std::string, std::shared_ptr<Texture>> textures_;
    std::unordered_map<std::string, std::shared_ptr<Material>> materials_;
    std::unordered_map<std::string, std::shared_ptr<Shader>> shaders_;
    std::unordered_map<std::string, std::shared_ptr<Mesh>> meshes_;

    std::array<std::shared_ptr<Texture>, static_cast<size_t>(TextureType::End)> defaultTextures_{};
    std::shared_ptr<Material> defaultMaterial_;
};