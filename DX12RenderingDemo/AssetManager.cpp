#include "AssetManager.h"
#include "Texture.h"
#include "Shader.h"
#include "Material.h"

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
            const std::filesystem::path& defaultTextureDir = L"../Assets/Textures/";
            const auto path = defaultTextureDir / fileName;

            if (!std::filesystem::exists(path))
            {
                LOG("Default texture not found: " << path.string());
                return;
            }

            defaultTextures_[static_cast<size_t>(type)] = LoadTexture(device, cmdList, path.wstring());
        };

    loadDefaultTexture(TextureType::BaseColor, L"Default_BaseColor.dds");
    loadDefaultTexture(TextureType::Normal, L"Default_Normal.dds");
}

std::shared_ptr<Shader> AssetManager::LoadShaderByName(
    ID3D12Device* device,
    ID3D12RootSignature* rootSignature,
    const std::string& shaderName)
{
    if (shaderName == "LitShader")
        return LoadLitShader(device, rootSignature, shaderName);

    LOG("Unknown shader name: " << shaderName << ". Use LitShader.");
    return LoadLitShader(device, rootSignature, "LitShader");
}

std::shared_ptr<Texture> AssetManager::LoadTexture(
    ID3D12Device* device,
    ID3D12GraphicsCommandList* cmdList,
    const std::wstring& filePath)
{
    std::filesystem::path normalizedPath =
        std::filesystem::weakly_canonical(filePath);

    std::wstring key = normalizedPath.wstring();

    if (auto iter = textures_.find(key); iter != textures_.end())
        return iter->second;

    auto texture = std::make_shared<Texture>();
    texture->LoadDDS(device, cmdList, key);
    texture->SetName(normalizedPath.string());

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
    shader->SetName(key);

    shaders_[key] = shader;
    return shader;
}

std::shared_ptr<Material> AssetManager::LoadMaterialFromFile(
    ID3D12Device* device,
    ID3D12GraphicsCommandList* cmdList,
    ID3D12RootSignature* rootSignature,
    const std::filesystem::path& matPath)
{
    const auto normalizedPath = std::filesystem::weakly_canonical(matPath);
    const std::wstring materialKey = normalizedPath.wstring();

    if (auto iter = materials_.find(materialKey); iter != materials_.end())
        return iter->second;

    std::ifstream file(normalizedPath);
    if (!file.is_open())
    {
        LOG("Material file open failed: " << normalizedPath.string());
        return nullptr;
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

    const std::string materialName = values["Name"].empty() ? normalizedPath.stem().string() : values["Name"];
    const std::string shaderName = values["Shader"].empty() ? "LitShader" : values["Shader"];

    auto shader = LoadShaderByName(device, rootSignature, shaderName);

    std::array<std::shared_ptr<Texture>, static_cast<size_t>(TextureType::end)> textures{};

    const std::filesystem::path textureDir =
        normalizedPath.parent_path().parent_path() / "Textures";

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

            textures[static_cast<size_t>(type)] = LoadTexture(device, cmdList, texturePath.wstring());
        };

    loadTextureSlot(TextureType::BaseColor, values["BaseColor"]);
    loadTextureSlot(TextureType::Normal, values["Normal"]);
    //loadTextureSlot(TextureType::MetallicRoughness, values["MetallicRoughness"]);
    //loadTextureSlot(TextureType::Emissive, values["Emissive"]);

    if (!textures[static_cast<size_t>(TextureType::BaseColor)])
        textures[static_cast<size_t>(TextureType::BaseColor)] = GetDefaultTexture(TextureType::BaseColor);

    if (!textures[static_cast<size_t>(TextureType::Normal)])
        textures[static_cast<size_t>(TextureType::Normal)] = GetDefaultTexture(TextureType::Normal);

    auto material = std::make_shared<Material>();
    material->SetName(materialName);
    material->SetShader(shader);

    for (size_t i = 0; i < static_cast<size_t>(TextureType::end); ++i)
    {
        if (textures[i])
            material->SetTexture(static_cast<TextureType>(i), textures[i]);
    }

    materials_[materialKey] = material;

    LOG("Material loaded from .mat: " << materialName);

    return material;
}

void AssetManager::Clear()
{
    textures_.clear();
    shaders_.clear();
    materials_.clear();
}

void AssetManager::ReleaseUploadResources()
{
    for (auto& [key, texture] : textures_)
    {
        if (texture)
            texture->ReleaseUploadBuffer();
    }

    for (auto& texture : defaultTextures_)
    {
        if (texture)
            texture->ReleaseUploadBuffer();
    }
}