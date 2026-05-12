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

    void SetShader(std::shared_ptr<Shader> shader) { shader_ = std::move(shader); }
    Shader* GetShader() const { return shader_.get(); }

    void SetTexture(TextureType type, std::shared_ptr<Texture> texture) { textures_[static_cast<size_t>(type)] = std::move(texture); }
    Texture* GetTexture(TextureType type) const { return textures_[static_cast<size_t>(type)].get(); }
    Texture* GetBaseColorTexture() const { return GetTexture(TextureType::BaseColor); }
    Texture* GetNormalTexture() const { return GetTexture(TextureType::Normal); }

private:
    std::shared_ptr<Shader> shader_;
    std::array<std::shared_ptr<Texture>, static_cast<size_t>(TextureType::END)> textures_;
};