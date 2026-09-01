#pragma once
#include "pch.h"

struct MapObstacleData
{
    std::string name;

    Vector3 position = Vector3::Zero;
    Vector3 scale = Vector3::One;
};

class MapObjectLoader
{
public:
    static bool LoadObstacles(const std::filesystem::path& filePath, std::vector<MapObstacleData>& outObstacles);
};