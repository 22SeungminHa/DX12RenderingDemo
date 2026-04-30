#pragma once
#include "pch.h"
#include "Transform.h"

class Material;
class Mesh;
class Camera;

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

    void ReleaseUploadBuffers();

    virtual void SetMesh(const std::shared_ptr<Mesh>& mesh) { mesh_ = mesh; }
    virtual void SetMaterial(const std::shared_ptr<Material>& material) { material_ = material; }

    virtual void Animate(float fTimeElapsed);
    virtual void OnPrepareRender();
    virtual void Render(ID3D12GraphicsCommandList* pd3dCommandList, Camera* pCamera);

protected:
    Transform transform_;

    std::shared_ptr<Mesh> mesh_ = NULL;
    std::shared_ptr<Material> material_ = NULL;

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