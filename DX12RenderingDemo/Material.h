#pragma once
#include "pch.h"

class Shader;
class Texture;

class Material
{
public:
    Material() = default;
    virtual ~Material() = default;

    void SetName(std::string name) { name_ = std::move(name); }
    const std::string& GetName() const { return name_; }

    void SetShader(std::shared_ptr<Shader> shader) { shader_ = std::move(shader); }
    Shader* GetShader() const { return shader_.get(); }

    void SetTexture(std::shared_ptr<Texture> texture) { texture_ = std::move(texture); }
    Texture* GetTexture() const { return texture_.get(); }

private:
    std::string name_;

    std::shared_ptr<Shader> shader_;
    std::shared_ptr<Texture> texture_;
};