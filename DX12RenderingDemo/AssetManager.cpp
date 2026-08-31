#include "AssetManager.h"
#include "Texture.h"
#include "Shader.h"
#include "Material.h"
#include "Mesh.h"
#include <sstream>
#include <fstream>

namespace
{
    static std::string Trim(const std::string& str)
    {
        const auto begin = str.find_first_not_of(" \t\r\n");
        if (begin == std::string::npos)
            return "";

        const auto end = str.find_last_not_of(" \t\r\n");
        return str.substr(begin, end - begin + 1);
    }
}

void AssetManager::InitializeDefaultTextures(
    ID3D12Device* device,
    ID3D12GraphicsCommandList* cmdList)
{
    auto loadDefaultTexture =
        [&](TextureType type, const std::wstring& fileName)
        {
            const auto path = AssetPath::Texture(fileName);

            if (!std::filesystem::exists(path))
            {
                LOG("Default texture not found: " << path.string());
                return;
            }

            defaultTextures_[static_cast<size_t>(type)] = LoadTexture(device, cmdList, path);
        };

    loadDefaultTexture(TextureType::BaseColor, L"Default_BaseColor");
    loadDefaultTexture(TextureType::Normal, L"Default_Normal");
}

std::shared_ptr<Shader> AssetManager::LoadShaderByName(
    ID3D12Device* device,
    ID3D12RootSignature* rootSignature,
    const std::string& shaderName)
{
    if (shaderName == "LitShader")
        return LoadLitShader(device, rootSignature, shaderName);
    if (shaderName == "GlassShader")
        return LoadGlassShader(device, rootSignature, shaderName);

    LOG("Unknown shader name: " << shaderName << ". Use LitShader.");
    return LoadLitShader(device, rootSignature, "LitShader");
}

std::shared_ptr<Texture> AssetManager::LoadTexture(
    ID3D12Device* device,
    ID3D12GraphicsCommandList* cmdList,
    const std::filesystem::path& filePath)
{
    const std::string key = AssetPath::Key(filePath);
    std::filesystem::path normalizedPath = std::filesystem::weakly_canonical(filePath);

    if (auto iter = textures_.find(key); iter != textures_.end())
        return iter->second;

    auto texture = std::make_shared<Texture>();
    texture->LoadDDS(device, cmdList, normalizedPath);
    texture->SetKey(key);

    textures_[key] = texture;
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
    shader->SetKey(key);

    shaders_[key] = shader;
    return shader;
}

std::shared_ptr<Shader> AssetManager::LoadGlassShader(
    ID3D12Device* device,
    ID3D12RootSignature* rootSignature,
    const std::string& key)
{
    if (auto iter = shaders_.find(key); iter != shaders_.end())
        return iter->second;

    auto shader = std::make_shared<GlassShader>();
    shader->CreateShader(device, rootSignature);
    shader->SetKey(key);

    shaders_[key] = shader;
    return shader;
}

std::shared_ptr<Material> AssetManager::LoadMaterialFromFile(
    ID3D12Device* device,
    ID3D12GraphicsCommandList* cmdList,
    ID3D12RootSignature* rootSignature,
    const std::filesystem::path& matPath)
{
    const std::string materialKey = AssetPath::Key(matPath);
    const auto normalizedPath = std::filesystem::weakly_canonical(matPath);

    if (auto iter = materials_.find(materialKey); iter != materials_.end())
        return iter->second;

    std::ifstream file(normalizedPath);
    if (!file.is_open())
    {
        LOG("Material file open failed: " << normalizedPath.string());

        const auto defaultMatPath = AssetPath::Material(L"Default");
        const auto defaultMatKey = AssetPath::Key(std::filesystem::weakly_canonical(defaultMatPath));

        if (materialKey == defaultMatKey)
        {
            LOG("Default material load failed");
            return nullptr;
        }

        if (!defaultMaterial_)
        {
            defaultMaterial_ = LoadMaterialFromFile(
                device,
                cmdList,
                rootSignature,
                defaultMatPath
            );
        }

        return defaultMaterial_;
    }

    std::unordered_map<std::string, std::string> values;

    std::string line;
    while (std::getline(file, line))
    {
        const auto pos = line.find('=');
        if (pos == std::string::npos) continue;

        const std::string key = Trim(line.substr(0, pos));
        const std::string value = Trim(line.substr(pos + 1));

        values[key] = value;
    }

    const std::string shaderName = values["Shader"].empty() ? "LitShader" : values["Shader"];
    const std::string renderModeName = values["RenderMode"];

    auto shader = LoadShaderByName(device, rootSignature, shaderName);
    
    std::array<std::shared_ptr<Texture>, static_cast<size_t>(TextureType::End)> textures{};
    const std::filesystem::path textureDir = normalizedPath.parent_path().parent_path() / "Textures";

    auto loadTextureSlot =
        [&](TextureType type, const std::string& textureName)
        {
            if (textureName.empty())
                return;

            const std::filesystem::path texturePath = textureDir / textureName;

            if (!std::filesystem::exists(texturePath))
            {
                LOG("Texture file not found. Material: " << normalizedPath.filename().string() << ", Texture: " << texturePath.string());
                return;
            }

            textures[static_cast<size_t>(type)] = LoadTexture(device, cmdList, texturePath);
        };

    loadTextureSlot(TextureType::BaseColor, values["BaseColorMap"]);
    loadTextureSlot(TextureType::Normal, values["NormalMap"]);

    if (!textures[static_cast<size_t>(TextureType::BaseColor)])
        textures[static_cast<size_t>(TextureType::BaseColor)] = GetDefaultTexture(TextureType::BaseColor);

    if (!textures[static_cast<size_t>(TextureType::Normal)])
        textures[static_cast<size_t>(TextureType::Normal)] = GetDefaultTexture(TextureType::Normal);

    auto material = std::make_shared<Material>();
    material->SetKey(materialKey);
    material->SetShader(shader);

    if (renderModeName == "Transparent")
        material->SetRenderMode(RenderMode::Transparent);
    else
        material->SetRenderMode(RenderMode::Opaque);

    if (values.contains("BaseColorTint"))
    {
        std::stringstream ss(values["BaseColorTint"]);

        float r, g, b, a;
        char comma;

        ss >> r >> comma >> g >> comma >> b >> comma >> a;

        material->SetBaseColorTint(Vector4(r, g, b, a));
    }
    if (values.contains("Tiling"))
    {
        std::stringstream ss(values["Tiling"]);

        float u, v;
        char comma;

        ss >> u >> comma >> v;

        material->SetTiling(Vector2(u, v));
    }
    if (values.contains("Alpha"))
        material->SetAlpha(std::stof(values["Alpha"]));
    if (values.contains("FresnelPower"))
        material->SetFresnelPower(std::stof(values["FresnelPower"]));
    if (values.contains("SpecularStrength"))
        material->SetSpecularStrength(std::stof(values["SpecularStrength"]));
    if (values.contains("ReflectionStrength"))
        material->SetReflectionStrength(std::stof(values["ReflectionStrength"]));
    if (values.contains("RefractionStrength"))
        material->SetRefractionStrength(std::stof(values["RefractionStrength"]));

    for (size_t i = 0; i < static_cast<size_t>(TextureType::End); ++i)
        if (textures[i]) material->SetTexture(static_cast<TextureType>(i), textures[i]);

    materials_[materialKey] = material;

    return material;
}

void AssetManager::Clear()
{
    materials_.clear();
    meshes_.clear();
    shaders_.clear();
    textures_.clear();

    defaultMaterial_.reset();

    for (auto& texture : defaultTextures_)
        texture.reset();
}

void AssetManager::ReleaseUploadResources()
{
    for (auto& [key, texture] : textures_)
        if (texture)
            texture->ReleaseUploadBuffer();

    for (auto& texture : defaultTextures_)
        if (texture)
            texture->ReleaseUploadBuffer();

    for (auto& [key, mesh] : meshes_)
        if (mesh)
            mesh->ReleaseUploadResources();
}

std::shared_ptr<Mesh> AssetManager::LoadLitMesh(
    ID3D12Device* device,
    ID3D12GraphicsCommandList* cmdList,
    const std::string& key,
    const std::vector<LitVertex>& vertices,
    const std::vector<UINT>& indices)
{
    if (auto iter = meshes_.find(key); iter != meshes_.end())
        return iter->second;

    auto mesh = std::make_shared<LoadedMeshLit>(
        device,
        cmdList,
        vertices,
        indices
    );

    mesh->SetKey(key);
    meshes_[key] = mesh;

    return mesh;
}

std::shared_ptr<Mesh> AssetManager::LoadCubeMesh(
    ID3D12Device* device,
    ID3D12GraphicsCommandList* cmdList,
    const std::string& key)
{
    if (auto iter = meshes_.find(key); iter != meshes_.end())
        return iter->second;

    auto mesh = std::make_shared<CubeMesh>(device, cmdList);
    mesh->SetKey(key);

    meshes_[key] = mesh;

    return mesh;
}

std::shared_ptr<Mesh> AssetManager::LoadGlassMesh(
    ID3D12Device* device,
    ID3D12GraphicsCommandList* cmdList,
    float width,
    float height,
    float depth)
{
    const std::string key =
        "Primitive/Glass/" +
        std::to_string(width) + "x" +
        std::to_string(height) + "x" +
        std::to_string(depth);

    if (auto iter = meshes_.find(key); iter != meshes_.end())
        return iter->second;

    auto mesh = std::make_shared<GlassMesh>(
        device,
        cmdList,
        width,
        height,
        depth
    );

    mesh->SetKey(key);
    meshes_[key] = mesh;

    return mesh;
}

std::shared_ptr<Mesh> AssetManager::LoadSphereMesh(
    ID3D12Device* device,
    ID3D12GraphicsCommandList* cmdList,
    const std::string& key,
    UINT sliceCount,
    UINT stackCount)
{
    const std::string sphereKey = key + "/" + std::to_string(sliceCount) + "x" + std::to_string(stackCount);
    
    if (auto iter = meshes_.find(sphereKey); iter != meshes_.end())
        return iter->second;

    auto mesh = std::make_shared<SphereMesh>(
        device,
        cmdList,
        sliceCount,
        stackCount
    );

    mesh->SetKey(sphereKey);
    meshes_[sphereKey] = mesh;

    return mesh;
}