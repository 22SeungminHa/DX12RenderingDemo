#include "GameObject.h"
#include "Shader.h"
#include "Camera.h"
#include "Material.h"
#include "Mesh.h"

GameObject::GameObject()
{
} 

GameObject::~GameObject()
{
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

	if (material_ && material_->GetShader())
		material_->GetShader()->Render(pd3dCommandList, pCamera);

	if (mesh_)
		mesh_->Render(pd3dCommandList);
}

void GameObject::Rotate(const Vector3& axis, float angle)
{
	transform_.rotation += axis * XMConvertToRadians(angle);
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
