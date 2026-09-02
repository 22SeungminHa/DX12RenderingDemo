#pragma once
#include "pch.h"

struct CubeData
{
    std::string name;

    Vector3 position = Vector3::Zero;
    Vector3 scale = Vector3::One;
};

struct CrystalData
{
    std::string name;
    Vector3 position = Vector3::Zero;
};

class MapObjectLoader
{
public:
    static bool LoadObstacles(
        const std::filesystem::path& filePath,
        std::vector<CubeData>& outObstacles);

    static bool LoadCrystals(
        const std::filesystem::path& filePath,
        std::vector<CrystalData>& outCrystals);

    static bool LoadMapCubes(
        const std::filesystem::path& filePath,
        std::vector<CubeData>& outCubes);
};