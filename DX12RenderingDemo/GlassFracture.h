#pragma once
#include "Vertex.h"

struct GlassFragmentData
{
    std::vector<Vector2> polygon;
};

struct GlassFragmentGeometry
{
    std::vector<LitVertex> vertices;
    std::vector<UINT> indices;

    // 원본 유리 로컬 좌표 기준 파편 중심
    Vector3 localPosition = Vector3::Zero;
};

class GlassFracture
{
public:
    static std::vector<GlassFragmentData> GenerateRadialFragments(
        float width,
        float height,
        const Vector2& impactPoint,
        UINT randomRayCount = 8);

    static GlassFragmentGeometry BuildFragmentGeometry(
        const GlassFragmentData& fragment,
        float glassWidth,
        float glassHeight,
        float depth);

private:
    static Vector2 FindBoundaryIntersection(
        float halfWidth,
        float halfHeight,
        const Vector2& origin,
        const Vector2& direction);
};

