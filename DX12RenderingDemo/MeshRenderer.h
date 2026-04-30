#pragma once
#include "pch.h"

class Mesh;
class Material;

class MeshRenderer
{
public:
    MeshRenderer() = default;
    ~MeshRenderer() = default;

    void SetMesh(const std::shared_ptr<Mesh>& mesh) { mesh_ = mesh; }
    void SetMaterial(const std::shared_ptr<Material>& material) { material_ = material; }

    Mesh* GetMesh() const { return mesh_.get(); }
    Material* GetMaterial() const { return material_.get(); }

    bool IsRenderable() const { return mesh_ && material_; }

    void ReleaseUploadResources();

private:
    std::shared_ptr<Mesh> mesh_;
    std::shared_ptr<Material> material_;
};