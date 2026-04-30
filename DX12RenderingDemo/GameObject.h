#pragma once
#include "pch.h"
#include "Transform.h"
#include "MeshRenderer.h"

class Material;
class Mesh;

class GameObject
{
public:
    GameObject();
    virtual ~GameObject();

public:
    void Rotate(const Vector3& axis, float angle);

    void SetObjectCBIndex(UINT index) { objectCBIndex_ = index; }
    UINT GetObjectCBIndex() const { return objectCBIndex_; }

    const Matrix GetWorldMatrix() const { return transform_.GetWorldMatrix(); }

    MeshRenderer* GetMeshRenderer() { return &meshRenderer_; }
    const MeshRenderer* GetMeshRenderer() const { return &meshRenderer_; }

    void ReleaseUploadBuffers();

    virtual void SetMesh(const std::shared_ptr<Mesh>& mesh) { meshRenderer_.SetMesh(mesh); }
    virtual void SetMaterial(const std::shared_ptr<Material>& material) { meshRenderer_.SetMaterial(material); }

    virtual void Animate(float fTimeElapsed);
    virtual void OnPrepareRender();

protected:
    Transform transform_;
    MeshRenderer meshRenderer_;

    UINT objectCBIndex_ = 0;
};

class RotatingObject : public GameObject
{
public:
    RotatingObject();
    virtual ~RotatingObject();

private:
    Vector3 rotationAxis_;
    float rotationSpeed_;

public:
    void SetRotationSpeed(float fRotationSpeed) { rotationSpeed_ = fRotationSpeed; }
    void SetRotationAxis(const Vector3& xmf3RotationAxis) { rotationAxis_ = xmf3RotationAxis; }

    virtual void Animate(float fTimeElapsed);
};