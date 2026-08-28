#include "GlassFracture.h"

#include <random>

namespace
{
    float CalculateSignedAreaTwice(
        const std::vector<Vector2>& polygon)
    {
        float area = 0.0f;

        for (size_t i = 0; i < polygon.size(); ++i)
        {
            const Vector2& a = polygon[i];
            const Vector2& b = polygon[(i + 1) % polygon.size()];

            area += a.x * b.y - b.x * a.y;
        }

        return area;
    }

    Vector2 CalculateCentroid(
        const std::vector<Vector2>& polygon)
    {
        if (polygon.empty())
            return Vector2::Zero;

        const float signedAreaTwice =
            CalculateSignedAreaTwice(polygon);

        constexpr float epsilon = 0.000001f;

        if (fabsf(signedAreaTwice) < epsilon)
        {
            Vector2 center = Vector2::Zero;

            for (const Vector2& point : polygon)
                center += point;

            return center / static_cast<float>(polygon.size());
        }

        float cx = 0.0f;
        float cy = 0.0f;

        for (size_t i = 0; i < polygon.size(); ++i)
        {
            const Vector2& a = polygon[i];
            const Vector2& b = polygon[(i + 1) % polygon.size()];

            const float cross =
                a.x * b.y - b.x * a.y;

            cx += (a.x + b.x) * cross;
            cy += (a.y + b.y) * cross;
        }

        const float factor =
            1.0f / (3.0f * signedAreaTwice);

        return Vector2(
            cx * factor,
            cy * factor
        );
    }
}

Vector2 GlassFracture::FindBoundaryIntersection(
    float halfWidth,
    float halfHeight,
    const Vector2& origin,
    const Vector2& direction)
{
    constexpr float epsilon = 0.000001f;

    float tx = FLT_MAX;
    float ty = FLT_MAX;

    if (fabsf(direction.x) > epsilon)
    {
        const float boundaryX =
            direction.x > 0.0f ? halfWidth : -halfWidth;

        tx = (boundaryX - origin.x) / direction.x;
    }

    if (fabsf(direction.y) > epsilon)
    {
        const float boundaryY =
            direction.y > 0.0f ? halfHeight : -halfHeight;

        ty = (boundaryY - origin.y) / direction.y;
    }

    float t = FLT_MAX;

    if (tx > 0.0f)
    {
        const float y = origin.y + direction.y * tx;

        if (y >= -halfHeight && y <= halfHeight)
            t = std::min(t, tx);
    }

    if (ty > 0.0f)
    {
        const float x = origin.x + direction.x * ty;

        if (x >= -halfWidth && x <= halfWidth)
            t = std::min(t, ty);
    }

    if (t == FLT_MAX)
        return origin;

    return origin + direction * t;
}

std::vector<GlassFragmentData> GlassFracture::GenerateRadialFragments(
    float width,
    float height,
    const Vector2& impactPoint,
    UINT randomRayCount)
{
    std::vector<GlassFragmentData> fragments;

    if (width <= 0.0f || height <= 0.0f)
        return fragments;

    const float halfWidth = width * 0.5f;
    const float halfHeight = height * 0.5f;

    if (impactPoint.x < -halfWidth ||
        impactPoint.x > halfWidth ||
        impactPoint.y < -halfHeight ||
        impactPoint.y > halfHeight)
    {
        return fragments;
    }

    std::vector<float> angles;

    angles.reserve(randomRayCount + 4);

    // 네 모서리 방향은 반드시 포함한다.
    const Vector2 corners[4] =
    {
        { -halfWidth, -halfHeight },
        { -halfWidth,  halfHeight },
        {  halfWidth,  halfHeight },
        {  halfWidth, -halfHeight }
    };

    for (const Vector2& corner : corners)
    {
        const Vector2 direction = corner - impactPoint;

        angles.push_back(
            atan2f(direction.y, direction.x)
        );
    }

    static std::mt19937 randomEngine{ std::random_device{}() };

    std::uniform_real_distribution<float> angleDistribution(
        -XM_PI,
        XM_PI
    );

    for (UINT i = 0; i < randomRayCount; ++i)
    {
        angles.push_back(angleDistribution(randomEngine));
    }

    std::sort(angles.begin(), angles.end());

    constexpr float minAngleDifference = XMConvertToRadians(1.0f);

    angles.erase(
        std::unique(
            angles.begin(),
            angles.end(),
            [minAngleDifference](float a, float b)
            {
                return fabsf(a - b) < minAngleDifference;
            }),
        angles.end()
    );

    std::vector<Vector2> boundaryPoints;
    boundaryPoints.reserve(angles.size());

    for (float angle : angles)
    {
        const Vector2 direction(
            cosf(angle),
            sinf(angle)
        );

        boundaryPoints.push_back(
            FindBoundaryIntersection(
                halfWidth,
                halfHeight,
                impactPoint,
                direction
            )
        );
    }

    fragments.reserve(boundaryPoints.size());

    for (size_t i = 0; i < boundaryPoints.size(); ++i)
    {
        const size_t next =
            (i + 1) % boundaryPoints.size();

        GlassFragmentData fragment;

        fragment.polygon.reserve(3);

        fragment.polygon.push_back(impactPoint);
        fragment.polygon.push_back(boundaryPoints[i]);
        fragment.polygon.push_back(boundaryPoints[next]);

        fragments.push_back(std::move(fragment));
    }

    return fragments;
}

std::vector<GlassFragmentData> GlassFracture::GenerateRingFragments(
    float width,
    float height,
    const Vector2& impactPoint,
    UINT randomRayCount,
    UINT ringCount)
{
    std::vector<GlassFragmentData> fragments;

    if (width <= 0.0f ||
        height <= 0.0f ||
        ringCount == 0)
    {
        return fragments;
    }

    const float halfWidth = width * 0.5f;
    const float halfHeight = height * 0.5f;

    if (impactPoint.x < -halfWidth ||
        impactPoint.x > halfWidth ||
        impactPoint.y < -halfHeight ||
        impactPoint.y > halfHeight)
    {
        return fragments;
    }

    std::vector<float> angles;
    angles.reserve(randomRayCount + 4);

    const Vector2 corners[4] =
    {
        { -halfWidth, -halfHeight },
        { -halfWidth,  halfHeight },
        {  halfWidth,  halfHeight },
        {  halfWidth, -halfHeight }
    };

    for (const Vector2& corner : corners)
    {
        const Vector2 direction =
            corner - impactPoint;

        angles.push_back(
            atan2f(direction.y, direction.x)
        );
    }

    static std::mt19937 randomEngine{
        std::random_device{}()
    };

    std::uniform_real_distribution<float> angleDistribution(
        -XM_PI,
        XM_PI
    );

    for (UINT i = 0; i < randomRayCount; ++i)
    {
        angles.push_back(
            angleDistribution(randomEngine)
        );
    }

    std::sort(
        angles.begin(),
        angles.end()
    );

    constexpr float minAngleDifference =
        XMConvertToRadians(1.0f);

    angles.erase(
        std::unique(
            angles.begin(),
            angles.end(),
            [minAngleDifference](float a, float b)
            {
                return fabsf(a - b)
                    < minAngleDifference;
            }),
        angles.end()
    );

    if (angles.size() < 3)
        return fragments;

    const size_t rayCount = angles.size();

    // 각 ray가 직사각형 외곽과 만나는 점
    std::vector<Vector2> boundaryPoints;
    boundaryPoints.reserve(rayCount);

    for (float angle : angles)
    {
        const Vector2 direction(
            cosf(angle),
            sinf(angle)
        );

        boundaryPoints.push_back(
            FindBoundaryIntersection(
                halfWidth,
                halfHeight,
                impactPoint,
                direction
            )
        );
    }

    // rings[ring][ray]
    std::vector<std::vector<Vector2>> rings(
        ringCount,
        std::vector<Vector2>(rayCount)
    );

    std::uniform_real_distribution<float> radiusJitter(
        -0.025f,
        0.025f
    );

    for (size_t ray = 0; ray < rayCount; ++ray)
    {
        const Vector2 radialVector =
            boundaryPoints[ray] - impactPoint;

        float previousFraction = 0.0f;

        for (UINT ring = 0; ring < ringCount; ++ring)
        {
            float fraction = 1.0f;

            if (ring + 1 < ringCount)
            {
                const float normalizedRing =
                    static_cast<float>(ring + 1) /
                    static_cast<float>(ringCount);

                // 충돌점 주변 Ring 간격을 더 좁게 만든다.
                fraction =
                    powf(normalizedRing, 1.6f);

                fraction +=
                    radiusJitter(randomEngine);

                const float minFraction =
                    previousFraction + 0.03f;

                fraction = std::max(
                    fraction,
                    minFraction
                );

                fraction = std::min(
                    fraction,
                    0.95f
                );
            }

            rings[ring][ray] =
                impactPoint +
                radialVector * fraction;

            previousFraction = fraction;
        }
    }

    fragments.reserve(
        rayCount * ringCount
    );

    // --------------------------------
    // 충돌점 중심의 작은 삼각형
    // --------------------------------

    for (size_t ray = 0; ray < rayCount; ++ray)
    {
        const size_t next =
            (ray + 1) % rayCount;

        GlassFragmentData fragment;

        fragment.polygon.reserve(3);

        fragment.polygon.push_back(
            impactPoint
        );

        fragment.polygon.push_back(
            rings[0][ray]
        );

        fragment.polygon.push_back(
            rings[0][next]
        );

        fragments.push_back(
            std::move(fragment)
        );
    }

    // --------------------------------
    // Ring 사이의 사각형 파편
    // --------------------------------

    for (UINT ring = 0;
        ring + 1 < ringCount;
        ++ring)
    {
        for (size_t ray = 0;
            ray < rayCount;
            ++ray)
        {
            const size_t next =
                (ray + 1) % rayCount;

            GlassFragmentData fragment;

            fragment.polygon.reserve(4);

            fragment.polygon.push_back(
                rings[ring][ray]
            );

            fragment.polygon.push_back(
                rings[ring + 1][ray]
            );

            fragment.polygon.push_back(
                rings[ring + 1][next]
            );

            fragment.polygon.push_back(
                rings[ring][next]
            );

            fragments.push_back(
                std::move(fragment)
            );
        }
    }

    return fragments;
}

GlassFragmentGeometry GlassFracture::BuildFragmentGeometry(
    const GlassFragmentData& fragment,
    float glassWidth,
    float glassHeight,
    float depth)
{
    GlassFragmentGeometry geometry;

    if (fragment.polygon.size() < 3 ||
        glassWidth <= 0.0f ||
        glassHeight <= 0.0f ||
        depth <= 0.0f)
    {
        return geometry;
    }

    std::vector<Vector2> polygon = fragment.polygon;

    // 이후 triangulation / side 생성이 단순하도록
    // 항상 CCW로 맞춘다.
    if (CalculateSignedAreaTwice(polygon) < 0.0f)
        std::reverse(polygon.begin(), polygon.end());

    const Vector2 centroid =
        CalculateCentroid(polygon);

    geometry.localPosition =
        Vector3(centroid.x, centroid.y, 0.0f);

    const float frontZ = -depth * 0.5f;
    const float backZ = depth * 0.5f;

    const UINT polygonCount =
        static_cast<UINT>(polygon.size());

    auto frontUV =
        [glassWidth, glassHeight](const Vector2& point)
        {
            return Vector2(
                point.x / glassWidth + 0.5f,
                0.5f - point.y / glassHeight
            );
        };

    auto backUV =
        [glassWidth, glassHeight](const Vector2& point)
        {
            return Vector2(
                0.5f - point.x / glassWidth,
                0.5f - point.y / glassHeight
            );
        };

    // -----------------------
    // Front (-Z)
    // -----------------------

    const UINT frontBase =
        static_cast<UINT>(geometry.vertices.size());

    for (const Vector2& point : polygon)
    {
        const Vector3 position(
            point.x - centroid.x,
            point.y - centroid.y,
            frontZ
        );

        geometry.vertices.emplace_back(
            position,
            Vector4::One,
            Vector3(0.0f, 0.0f, -1.0f),
            Vector3(1.0f, 0.0f, 0.0f),
            frontUV(point)
        );
    }

    // -----------------------
    // Back (+Z)
    // -----------------------

    const UINT backBase =
        static_cast<UINT>(geometry.vertices.size());

    for (const Vector2& point : polygon)
    {
        const Vector3 position(
            point.x - centroid.x,
            point.y - centroid.y,
            backZ
        );

        geometry.vertices.emplace_back(
            position,
            Vector4::One,
            Vector3(0.0f, 0.0f, 1.0f),
            Vector3(-1.0f, 0.0f, 0.0f),
            backUV(point)
        );
    }

    // -----------------------
    // Front / Back indices
    // -----------------------

    for (UINT i = 1; i + 1 < polygonCount; ++i)
    {
        // Front (-Z)
        geometry.indices.push_back(frontBase);
        geometry.indices.push_back(frontBase + i + 1);
        geometry.indices.push_back(frontBase + i);

        // Back (+Z)
        geometry.indices.push_back(backBase);
        geometry.indices.push_back(backBase + i);
        geometry.indices.push_back(backBase + i + 1);
    }

    // -----------------------
    // Side faces
    // -----------------------

    for (UINT i = 0; i < polygonCount; ++i)
    {
        const UINT next =
            (i + 1) % polygonCount;

        const Vector2& a = polygon[i];
        const Vector2& b = polygon[next];

        Vector3 tangent(
            b.x - a.x,
            b.y - a.y,
            0.0f
        );

        tangent.Normalize();

        // polygon이 CCW이므로
        // edge의 오른쪽이 바깥 방향
        Vector3 normal(
            tangent.y,
            -tangent.x,
            0.0f
        );

        const UINT sideBase =
            static_cast<UINT>(geometry.vertices.size());

        geometry.vertices.emplace_back(
            Vector3(
                a.x - centroid.x,
                a.y - centroid.y,
                frontZ),
            Vector4::One,
            normal,
            tangent,
            Vector2(0.0f, 1.0f)
        );

        geometry.vertices.emplace_back(
            Vector3(
                b.x - centroid.x,
                b.y - centroid.y,
                frontZ),
            Vector4::One,
            normal,
            tangent,
            Vector2(1.0f, 1.0f)
        );

        geometry.vertices.emplace_back(
            Vector3(
                b.x - centroid.x,
                b.y - centroid.y,
                backZ),
            Vector4::One,
            normal,
            tangent,
            Vector2(1.0f, 0.0f)
        );

        geometry.vertices.emplace_back(
            Vector3(
                a.x - centroid.x,
                a.y - centroid.y,
                backZ),
            Vector4::One,
            normal,
            tangent,
            Vector2(0.0f, 0.0f)
        );

        geometry.indices.push_back(sideBase + 0);
        geometry.indices.push_back(sideBase + 1);
        geometry.indices.push_back(sideBase + 2);

        geometry.indices.push_back(sideBase + 0);
        geometry.indices.push_back(sideBase + 2);
        geometry.indices.push_back(sideBase + 3);
    }

    return geometry;
}