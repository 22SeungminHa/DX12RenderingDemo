#pragma once

#include "Mesh.h"
#include "Camera.h"

class CShader;

class CGameObject
{
public:
    CGameObject();
    virtual ~CGameObject();

public:
    void Rotate(const Vector3& axis, float angle);

protected:
    Matrix m_xmf4x4World = Matrix::Identity;
    std::unique_ptr<CMesh> m_pMesh = NULL;
    std::unique_ptr<CShader> m_pShader = NULL;

public:
    void ReleaseUploadBuffers();

    virtual void SetMesh(CMesh* pMesh);
    virtual void SetShader(CShader* pShader);

    virtual void Animate(float fTimeElapsed);

    virtual void OnPrepareRender();
    virtual void Render(ID3D12GraphicsCommandList* pd3dCommandList, CCamera* pCamera);
};

class CRotatingObject : public CGameObject
{
public:
    CRotatingObject();
    virtual ~CRotatingObject();

private:
    Vector3 m_xmf3RotationAxis;
    float m_fRotationSpeed;

public:
    void SetRotationSpeed(float fRotationSpeed) { m_fRotationSpeed = fRotationSpeed; }
    void SetRotationAxis(const Vector3& xmf3RotationAxis) { m_xmf3RotationAxis = xmf3RotationAxis; }

    virtual void Animate(float fTimeElapsed);
};