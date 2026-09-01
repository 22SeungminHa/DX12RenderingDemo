#include "MapObjectLoader.h"

#include <fstream>
#include <sstream>
#include <cstdlib>

namespace
{
    bool FindMatching(const std::string& text, size_t openPos, char openChar, char closeChar, size_t& outClosePos)
    {
        int depth = 0;

        for (size_t i = openPos; i < text.size(); ++i)
        {
            if (text[i] == openChar)
                ++depth;
            else if (text[i] == closeChar)
            {
                --depth;

                if (depth == 0)
                {
                    outClosePos = i;
                    return true;
                }
            }
        }

        return false;
    }

    bool ExtractObject(const std::string& text, const std::string& key, std::string& outObject)
    {
        const std::string token = "\"" + key + "\"";

        const size_t keyPos = text.find(token);
        if (keyPos == std::string::npos)
            return false;

        const size_t openPos = text.find('{', keyPos);
        if (openPos == std::string::npos)
            return false;

        size_t closePos = std::string::npos;

        if (!FindMatching(text, openPos, '{', '}', closePos))
            return false;

        outObject = text.substr(openPos, closePos - openPos + 1);

        return true;
    }

    bool ExtractString(const std::string& text, const std::string& key, std::string& outValue)
    {
        const std::string token = "\"" + key + "\"";

        const size_t keyPos = text.find(token);
        if (keyPos == std::string::npos)
            return false;

        const size_t colonPos = text.find(':', keyPos);
        if (colonPos == std::string::npos)
            return false;

        const size_t beginQuote = text.find('"', colonPos);
        if (beginQuote == std::string::npos)
            return false;

        const size_t endQuote = text.find('"', beginQuote + 1);
        if (endQuote == std::string::npos)
            return false;

        outValue = text.substr(beginQuote + 1, endQuote - beginQuote - 1);

        return true;
    }

    bool ExtractFloat(const std::string& text, const std::string& key, float& outValue)
    {
        const std::string token = "\"" + key + "\"";

        const size_t keyPos = text.find(token);
        if (keyPos == std::string::npos)
            return false;

        const size_t colonPos = text.find(':', keyPos);
        if (colonPos == std::string::npos)
            return false;

        const char* begin = text.c_str() + colonPos + 1;
        char* end = nullptr;

        outValue = std::strtof(begin, &end);

        return end != begin;
    }

    bool ExtractVector3(const std::string& text, const std::string& key, Vector3& outValue)
    {
        std::string object;

        if (!ExtractObject(text, key, object))
            return false;

        return
            ExtractFloat(object, "x", outValue.x) &&
            ExtractFloat(object, "y", outValue.y) &&
            ExtractFloat(object, "z", outValue.z);
    }

    Vector3 ConvertUnityPosition(Vector3 position)
    {
        position.z = -position.z;
        return position;
    }
}

bool MapObjectLoader::LoadObstacles(
    const std::filesystem::path& filePath,
    std::vector<MapObstacleData>& outObstacles)
{
    outObstacles.clear();

    std::ifstream file(filePath);

    if (!file.is_open())
    {
        LOG("Map object file open failed: " << filePath.string());
        return false;
    }

    std::stringstream buffer;
    buffer << file.rdbuf();

    const std::string json = buffer.str();

    const size_t obstaclesKey = json.find("\"obstacles\"");

    if (obstaclesKey == std::string::npos)
    {
        LOG("Obstacles array not found");
        return false;
    }

    const size_t arrayBegin = json.find('[', obstaclesKey);

    if (arrayBegin == std::string::npos)
        return false;

    size_t arrayEnd = std::string::npos;

    if (!FindMatching(json, arrayBegin, '[', ']', arrayEnd))
        return false;

    size_t position = arrayBegin + 1;

    while (position < arrayEnd)
    {
        const size_t objectBegin = json.find('{', position);

        if (objectBegin == std::string::npos || objectBegin >= arrayEnd)
            break;

        size_t objectEnd = std::string::npos;

        if (!FindMatching(json, objectBegin, '{', '}', objectEnd))
            return false;

        const std::string object = json.substr(objectBegin, objectEnd - objectBegin + 1);

        MapObstacleData obstacle{};

        if (!ExtractString(object, "name", obstacle.name) ||
            !ExtractVector3(object, "position", obstacle.position) ||
            !ExtractVector3(object, "scale", obstacle.scale))
        {
            LOG("Invalid obstacle data");
            return false;
        }

        obstacle.position = ConvertUnityPosition(obstacle.position);

        outObstacles.push_back(obstacle);

        position = objectEnd + 1;
    }

    return true;
}

bool MapObjectLoader::LoadCrystals(const std::filesystem::path& filePath, std::vector<MapCrystalData>& outCrystals)
{
    outCrystals.clear();

    std::ifstream file(filePath);

    if (!file.is_open())
    {
        LOG("Map object file open failed: " << filePath.string());
        return false;
    }

    std::stringstream buffer;
    buffer << file.rdbuf();

    const std::string json = buffer.str();

    const size_t crystalsKey = json.find("\"crystals\"");

    if (crystalsKey == std::string::npos)
    {
        LOG("Crystals array not found");
        return false;
    }

    const size_t arrayBegin = json.find('[', crystalsKey);

    if (arrayBegin == std::string::npos)
        return false;

    size_t arrayEnd = std::string::npos;

    if (!FindMatching(json, arrayBegin, '[', ']', arrayEnd))
        return false;

    size_t position = arrayBegin + 1;

    while (position < arrayEnd)
    {
        const size_t objectBegin = json.find('{', position);

        if (objectBegin == std::string::npos || objectBegin >= arrayEnd)
            break;

        size_t objectEnd = std::string::npos;

        if (!FindMatching(json, objectBegin, '{', '}', objectEnd))
            return false;

        const std::string object = json.substr(objectBegin, objectEnd - objectBegin + 1);

        MapCrystalData crystal{};

        if (!ExtractString(object, "name", crystal.name) ||
            !ExtractVector3(object, "position", crystal.position))
        {
            LOG("Invalid crystal data");
            return false;
        }

        crystal.position = ConvertUnityPosition(crystal.position);

        outCrystals.push_back(crystal);

        position = objectEnd + 1;
    }

    return true;
}