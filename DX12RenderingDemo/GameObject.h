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
    void AddChild(std::unique_ptr<GameObject> child);
    void RemoveChild(GameObject* child);

    void ReleaseUploadResources();

    void Rotate(const Vector3& axis, float angle);

    virtual void Animate(float deltaTime);
    virtual void OnPrepareRender();

    void SetObjectCBIndex(UINT index) { objectCBIndex_ = index; }
    virtual void SetMesh(const std::shared_ptr<Mesh>& mesh) { meshRenderer_.SetMesh(mesh); }
    virtual void SetMaterial(const std::shared_ptr<Material>& material) { meshRenderer_.SetMaterial(material); }

    UINT GetObjectCBIndex() const { return objectCBIndex_; }
    Matrix GetWorldMatrix() const { return transform_.GetWorldMatrix(); }
    Transform* GetTransform() { return &transform_; }
    MeshRenderer* GetMeshRenderer() { return &meshRenderer_; }
    const std::vector<std::unique_ptr<GameObject>>& GetChildren() const { return children_; }
    const Transform* GetTransform() const { return &transform_; }
    const MeshRenderer* GetMeshRenderer() const { return &meshRenderer_; }

protected:
    Transform transform_;
    MeshRenderer meshRenderer_;

    UINT objectCBIndex_ = 0;

    std::vector<std::unique_ptr<GameObject>> children_;
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
    void SetRotationSpeed(float rotationSpeed) { rotationSpeed_ = rotationSpeed; }
    void SetRotationAxis(const Vector3& rotationAxis) { rotationAxis_ = rotationAxis; }

    virtual void Animate(float deltaTime);
};