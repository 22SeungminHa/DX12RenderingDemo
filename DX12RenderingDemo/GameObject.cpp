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

void GameObject::ReleaseUploadResources()
{
	meshRenderer_.ReleaseUploadResources();
}

void GameObject::Animate(float fTimeElapsed)
{
}

void GameObject::OnPrepareRender()
{
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
