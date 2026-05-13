#pragma once
#include "Asset.h"
#include "EngineTypes.h"

class Shader;
class Texture;

class Material : public Asset
{
public:
    Material() = default;
    virtual ~Material() = default;

    void SetMaterialCBIndex(UINT index) { materialCBIndex_ = index; }
    UINT GetMaterialCBIndex() const { return materialCBIndex_; }

    void SetShader(std::shared_ptr<Shader> shader) { shader_ = std::move(shader); }
    Shader* GetShader() const { return shader_.get(); }

    void SetTexture(TextureType type, std::shared_ptr<Texture> texture) { textures_[static_cast<size_t>(type)] = std::move(texture); }
    Texture* GetTexture(TextureType type) const { return textures_[static_cast<size_t>(type)].get(); }
    Texture* GetBaseColorTexture() const { return GetTexture(TextureType::BaseColor); }
    Texture* GetNormalTexture() const { return GetTexture(TextureType::Normal); }

    void SetRenderMode(RenderMode mode) { renderMode_ = mode; }
    RenderMode GetRenderMode() const { return renderMode_; }

    void SetBaseColor(const Vector4& color) { baseColor_ = color; }
    const Vector4& GetBaseColor() const { return baseColor_; }

    void SetAlpha(float alpha) { alpha_ = alpha; }
    float GetAlpha() const { return alpha_; }

    void SetFresnelPower(float power) { fresnelPower_ = power; }
    float GetFresnelPower() const { return fresnelPower_; }

    void SetSpecularStrength(float strength) { specularStrength_ = strength; }
    float GetSpecularStrength() const { return specularStrength_; }

private:
    UINT materialCBIndex_ = UINT_MAX;

    std::shared_ptr<Shader> shader_;
    std::array<std::shared_ptr<Texture>, static_cast<size_t>(TextureType::End)> textures_;
    
    RenderMode renderMode_ = RenderMode::Opaque;

    Vector4 baseColor_ = Vector4(1.0f, 1.0f, 1.0f, 1.0f);

    float alpha_ = 1.0f;
    float fresnelPower_ = 5.0f;
    float specularStrength_ = 0.5f;
};