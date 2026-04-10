#include "GameObject.h"
#include "Shader.h"
#include "Camera.h"

CGameObject::CGameObject()
{
	m_xmf4x4World = Matrix::Identity;
} 

CGameObject::~CGameObject()
{
	if (m_pShader) m_pShader->ReleaseShaderVariables();
}

void CGameObject::SetShader(CShader* pShader)
{
	m_pShader.reset(pShader);
}

void CGameObject::SetMesh(CMesh* pMesh)
{
	m_pMesh.reset(pMesh);
}

void CGameObject::ReleaseUploadBuffers()
{
	if (m_pMesh) m_pMesh->ReleaseUploadBuffers();
}

void CGameObject::Animate(float fTimeElapsed)
{
}

void CGameObject::OnPrepareRender()
{
}

void CGameObject::Render(ID3D12GraphicsCommandList* pd3dCommandList, CCamera* pCamera)
{
	OnPrepareRender();

	if (m_pShader)
	{
		//게임 객체의 월드 변환 행렬을 셰이더의 상수 버퍼로 전달(복사)한다.
		m_pShader->UpdateShaderVariable(pd3dCommandList, m_xmf4x4World);
		m_pShader->Render(pd3dCommandList, pCamera);
	}

	if (m_pMesh) m_pMesh->Render(pd3dCommandList);
}

void CGameObject::Rotate(const Vector3& axis, float angle)
{
	Matrix mtxRotate = Matrix::CreateFromAxisAngle(axis, XMConvertToRadians(angle));
	m_xmf4x4World = mtxRotate * m_xmf4x4World;
}

CRotatingObject::CRotatingObject()
{
	m_xmf3RotationAxis = Vector3::Up;
	m_fRotationSpeed = 90.0f;
}

CRotatingObject::~CRotatingObject()
{
}

void CRotatingObject::Animate(float fTimeElapsed)
{
	CGameObject::Rotate(m_xmf3RotationAxis, m_fRotationSpeed * fTimeElapsed);
}
