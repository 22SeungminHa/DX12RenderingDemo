#include "DirectionalLight.h"

void DirectionalLight::SetDirection(const Vector3& direction)
{
    direction_ = direction;
    direction_.Normalize();
}

DirectionalLightData DirectionalLight::GetLightData() const
{
    DirectionalLightData data{};

    data.direction = direction_;
    data.color = Vector4(
        color_.x * intensity_,
        color_.y * intensity_,
        color_.z * intensity_,
        color_.w
    );

    return data;
}