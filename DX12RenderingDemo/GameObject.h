#pragma once

#include "Mesh.h"
#include "Camera.h"

class Shader;

class GameObject
{
public:
    GameObject();
    virtual ~GameObject();

public:
    void Rotate(const Vector3& axis, float angle);

protected:
    Matrix worldMatrix = Matrix::Identity;
    std::unique_ptr<Mesh> mesh_ = NULL;
    std::unique_ptr<Shader> shader_ = NULL;

public:
    void ReleaseUploadBuffers();

    virtual void SetMesh(Mesh* pMesh);
    virtual void SetShader(Shader* pShader);

    virtual void Animate(float fTimeElapsed);

    virtual void OnPrepareRender();
    virtual void Render(ID3D12GraphicsCommandList* pd3dCommandList, Camera* pCamera);
};

class CRotatingObject : public GameObject
{
public:
    CRotatingObject();
    virtual ~CRotatingObject();

private:
    Vector3 rotationAxis_;
    float rotationSpeed_;

public:
    void SetRotationSpeed(float fRotationSpeed) { rotationSpeed_ = fRotationSpeed; }
    void SetRotationAxis(const Vector3& xmf3RotationAxis) { rotationAxis_ = xmf3RotationAxis; }

    virtual void Animate(float fTimeElapsed);
};