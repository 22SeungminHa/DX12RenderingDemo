#pragma once
#include "pch.h"

class Shader;

class Material
{
public:
    Material() = default;
    virtual ~Material() = default;

    void SetShader(std::shared_ptr<Shader> shader) { shader_ = std::move(shader); }
    Shader* GetShader() const { return shader_.get(); }

private:
    std::shared_ptr<Shader> shader_;
};
