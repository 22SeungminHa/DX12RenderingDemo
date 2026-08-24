#include "Material.h"
#include "Shader.h"

bool Material::IsGlassMaterial() const
{
    return shader_ && shader_->IsGlassShader();
}