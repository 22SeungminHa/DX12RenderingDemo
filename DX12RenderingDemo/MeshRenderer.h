#pragma once
#include "pch.h"

class Mesh;
class Material;
class Camera;

class MeshRenderer
{
public:
    MeshRenderer() = default;
    ~MeshRenderer() = default;

    void SetMesh(const std::shared_ptr<Mesh>& mesh) { mesh_ = mesh; }
    void SetMaterial(const std::shared_ptr<Material>& material) { material_ = material; }

    Mesh* GetMesh() const { return mesh_.get(); }
    Material* GetMaterial() const { return material_.get(); }

    void ReleaseUploadBuffers();
    void Render(ID3D12GraphicsCommandList* cmdList, Camera* camera);

private:
    std::shared_ptr<Mesh> mesh_;
    std::shared_ptr<Material> material_;
};