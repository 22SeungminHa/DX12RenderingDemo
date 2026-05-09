#include <iostream>
#include <fstream>
#include <sstream>
#include <string>
#include <unordered_map>
#include <filesystem>

namespace fs = std::filesystem;

std::string Trim(const std::string& str)
{
    const size_t first = str.find_first_not_of(" \t\r\n");
    if (first == std::string::npos)
        return "";

    const size_t last = str.find_last_not_of(" \t\r\n");
    return str.substr(first, last - first + 1);
}

std::string ExtractGuidFromLine(const std::string& line)
{
    const std::string key = "guid:";
    size_t pos = line.find(key);

    if (pos == std::string::npos)
        return "";

    pos += key.length();

    size_t end = line.find(',', pos);
    if (end == std::string::npos)
        end = line.length();

    return Trim(line.substr(pos, end - pos));
}

std::string FindGuidInMetaFile(const fs::path& metaPath)
{
    std::ifstream file(metaPath);
    if (!file.is_open())
        return "";

    std::string line;

    while (std::getline(file, line))
    {
        line = Trim(line);

        if (line.rfind("guid:", 0) == 0)
        {
            return Trim(line.substr(5));
        }
    }

    return "";
}

std::unordered_map<std::string, fs::path> BuildGuidToTexturePathMap(
    const fs::path& textureDir)
{
    std::unordered_map<std::string, fs::path> guidMap;

    for (const auto& entry : fs::recursive_directory_iterator(textureDir))
    {
        if (!entry.is_regular_file())
            continue;

        if (entry.path().extension() != ".meta")
            continue;

        std::string guid = FindGuidInMetaFile(entry.path());

        if (guid.empty())
            continue;

        fs::path texturePath = entry.path();
        texturePath.replace_extension("");

        guidMap[guid] = texturePath;
    }

    return guidMap;
}

std::string ToDdsFileName(const fs::path& texturePath)
{
    fs::path ddsPath = texturePath.filename();
    ddsPath.replace_extension(".dds");
    return ddsPath.string();
}

std::string FindTextureGuidInUnityMat(
    const fs::path& unityMatPath,
    const std::string& propertyName)
{
    std::ifstream file(unityMatPath);
    if (!file.is_open())
        return "";

    std::string line;
    bool foundProperty = false;

    while (std::getline(file, line))
    {
        std::string trimmed = Trim(line);

        if (trimmed == "- " + propertyName + ":")
        {
            foundProperty = true;
            continue;
        }

        if (!foundProperty)
            continue;

        if (trimmed.find("m_Texture:") != std::string::npos)
        {
            return ExtractGuidFromLine(trimmed);
        }

        if (trimmed.rfind("- ", 0) == 0)
            break;
    }

    return "";
}

std::string FindMaterialName(const fs::path& unityMatPath)
{
    std::ifstream file(unityMatPath);
    if (!file.is_open())
        return unityMatPath.stem().string();

    std::string line;

    while (std::getline(file, line))
    {
        std::string trimmed = Trim(line);

        if (trimmed.rfind("m_Name:", 0) == 0)
        {
            return Trim(trimmed.substr(std::string("m_Name:").length()));
        }
    }

    return unityMatPath.stem().string();
}

std::string ResolveTextureName(
    const std::unordered_map<std::string, fs::path>& guidMap,
    const std::string& guid)
{
    if (guid.empty())
        return "";

    auto iter = guidMap.find(guid);
    if (iter == guidMap.end())
        return "";

    return ToDdsFileName(iter->second);
}

bool ConvertUnityMatToEngineMat(
    const fs::path& unityMatPath,
    const fs::path& textureDir,
    const fs::path& outputPath)
{
    if (!fs::exists(unityMatPath))
    {
        std::cout << "Unity mat file not found: " << unityMatPath << '\n';
        return false;
    }

    if (!fs::exists(textureDir))
    {
        std::cout << "Texture directory not found: " << textureDir << '\n';
        return false;
    }

    auto guidMap = BuildGuidToTexturePathMap(textureDir);

    std::string name = FindMaterialName(unityMatPath);

    std::string baseColorGuid = FindTextureGuidInUnityMat(unityMatPath, "_BaseMap");
    if (baseColorGuid.empty())
        baseColorGuid = FindTextureGuidInUnityMat(unityMatPath, "_MainTex");

    std::string normalGuid = FindTextureGuidInUnityMat(unityMatPath, "_BumpMap");
    std::string metallicGuid = FindTextureGuidInUnityMat(unityMatPath, "_MetallicGlossMap");
    std::string emissiveGuid = FindTextureGuidInUnityMat(unityMatPath, "_EmissionMap");

    std::string baseColor = ResolveTextureName(guidMap, baseColorGuid);
    std::string normal = ResolveTextureName(guidMap, normalGuid);
    std::string metallic = ResolveTextureName(guidMap, metallicGuid);
    std::string emissive = ResolveTextureName(guidMap, emissiveGuid);

    std::ofstream out(outputPath);

    if (!out.is_open())
    {
        std::cout << "Output file open failed: " << outputPath << '\n';
        return false;
    }

    out << "Name=" << name << '\n';
    out << "Shader=LitShader" << '\n';
    out << "BaseColor=" << baseColor << '\n';
    out << "Normal=" << normal << '\n';
    out << "MetallicRoughness=" << metallic << '\n';
    out << "Emissive=" << emissive << '\n';

    std::cout << "Converted: " << outputPath << '\n';

    return true;
}

int main()
{
    std::string fileName;

    std::cout << "Unity .mat file name: ";
    std::getline(std::cin, fileName);

    fs::path unityMatPath =
        fs::path("../Unity") / fileName;

    fs::path textureDir =
        "../../Textures";

    fs::path outputPath =
        fs::path("../") / fileName;

    ConvertUnityMatToEngineMat(
        unityMatPath,
        textureDir,
        outputPath
    );

    return 0;
}