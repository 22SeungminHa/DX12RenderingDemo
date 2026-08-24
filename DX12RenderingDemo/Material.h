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
    //Texture* GetMetallicRoughnessTexture() const { return GetTexture(TextureType::MetallicRoughness); }

    void SetRenderMode(RenderMode mode) { renderMode_ = mode; }
    void SetBaseColorTint(const Vector4& color) { baseColorTint_ = color; }
    void SetAlpha(float alpha) { alpha_ = alpha; }
    void SetFresnelPower(float power) { fresnelPower_ = power; }
    void SetSpecularStrength(float strength) { specularStrength_ = strength; }
    void SetReflectionStrength(float strength) { reflectionStrength_ = strength; }
    void SetRefractionStrength(float strength) { refractionStrength_ = strength; }
    
    RenderMode GetRenderMode() const { return renderMode_; }
    const Vector4& GetBaseColorTint() const { return baseColorTint_; }
    float GetAlpha() const { return alpha_; }
    float GetFresnelPower() const { return fresnelPower_; }
    float GetSpecularStrength() const { return specularStrength_; }
    float GetReflectionStrength() const { return reflectionStrength_; }
    float GetRefractionStrength() const { return refractionStrength_; }

    bool UseEnvironmentReflection() const { return reflectionStrength_ > 0.0f; }

    bool IsGlassMaterial() const;

private:
    UINT materialCBIndex_ = UINT_MAX;

    std::shared_ptr<Shader> shader_;
    std::array<std::shared_ptr<Texture>, static_cast<size_t>(TextureType::End)> textures_;
    
    RenderMode renderMode_ = RenderMode::Opaque;

    Vector4 baseColorTint_ = Vector4(1.0f, 1.0f, 1.0f, 1.0f);

    float alpha_ = 1.0f;
    float fresnelPower_ = 5.0f;
    float specularStrength_ = 0.5f;
    float reflectionStrength_ = 0.0f;
    float refractionStrength_ = 0.0f;
};