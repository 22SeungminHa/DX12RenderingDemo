#include "GameObject.h"
#include "Shader.h"
#include "Camera.h"

GameObject::GameObject()
{
	worldMatrix = Matrix::Identity;
} 

GameObject::~GameObject()
{
	if (shader_) shader_->ReleaseShaderVariables();
}

void GameObject::SetShader(Shader* pShader)
{
	shader_.reset(pShader);
}

void GameObject::SetMesh(Mesh* pMesh)
{
	mesh_.reset(pMesh);
}

void GameObject::ReleaseUploadBuffers()
{
	if (mesh_) mesh_->ReleaseUploadBuffers();
}

void GameObject::Animate(float fTimeElapsed)
{
}

void GameObject::OnPrepareRender()
{
}

void GameObject::Render(ID3D12GraphicsCommandList* pd3dCommandList, Camera* pCamera)
{
	OnPrepareRender();

	if (shader_)
	{
		//게임 객체의 월드 변환 행렬을 셰이더의 상수 버퍼로 전달(복사)한다.
		shader_->UpdateShaderVariable(pd3dCommandList, worldMatrix);
		shader_->Render(pd3dCommandList, pCamera);
	}

	if (mesh_) mesh_->Render(pd3dCommandList);
}

void GameObject::Rotate(const Vector3& axis, float angle)
{
	Matrix mtxRotate = Matrix::CreateFromAxisAngle(axis, XMConvertToRadians(angle));
	worldMatrix = mtxRotate * worldMatrix;
}

RotatingObject::RotatingObject()
{
	rotationAxis_ = Vector3::Up;
	rotationSpeed_ = 90.0f;
}

RotatingObject::~RotatingObject()
{
}

void RotatingObject::Animate(float fTimeElapsed)
{
	GameObject::Rotate(rotationAxis_, rotationSpeed_ * fTimeElapsed);
}
