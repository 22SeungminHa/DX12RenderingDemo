#pragma once
#include "EngineTypes.h"

class DirectionalLight
{
public:
    DirectionalLight() = default;
    ~DirectionalLight() = default;

    void SetDirection(const Vector3& direction);
    void SetColor(const Vector4& color) { color_ = color; }
    void SetIntensity(float intensity) { intensity_ = intensity; }
    void SetEnabled(bool enabled) { enabled_ = enabled; }

    const Vector3& GetDirection() const { return direction_; }
    const Vector4& GetColor() const { return color_; }
    float GetIntensity() const { return intensity_; }
    bool IsEnabled() const { return enabled_; }

    DirectionalLightData GetLightData() const;

private:
    Vector3 direction_ = { 0.0f, 0.0f, -1.0f };
    Vector4 color_ = { 1.0f, 1.0f, 1.0f, 1.0f };

    float intensity_ = 1.0f;
    bool enabled_ = true;
};